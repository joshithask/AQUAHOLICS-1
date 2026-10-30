// Aquaholics tank monitor v2 - ESP32
// Sense -> Process -> Indicate -> Act (from the Design Thinking deck):
//  * turbidity (+ optional DS18B20 temperature and pH) -> Normal / Caution / Inspection
//  * Telegram alerts sent by the ESP32 itself, so they arrive even when the app is closed
//  * readings stored in the cloud (ThingSpeak) every 5 min and on every status change
//  * the "clear water" baseline and last-cleaned date are saved on the device
// Wiring: GPIO34 tolerates 3.3 V max. Clear water should read about 2.5-3.0 V.
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <time.h>
#include "index_html.h"

#define HAS_TEMP 0   // 1 = DS18B20 data pin on GPIO4 (install libraries OneWire + DallasTemperature)
#define HAS_PH   0   // 1 = pH module AO on GPIO35 (calibrate PH_V7 / PH_SLOPE below)
#if HAS_TEMP
#include <OneWire.h>
#include <DallasTemperature.h>
OneWire oneWire(4);
DallasTemperature tempSensor(&oneWire);
#endif

#include "secrets.h"   // copy secrets.example.h to secrets.h and fill it in (secrets.h is never committed)

const int   SENSOR_PIN = 34, PH_PIN = 35;
const unsigned long SAMPLE_MS = 300000UL;   // cloud/history reading every 5 min
const unsigned long CHECK_MS  = 30000UL;    // alert check every 30 s
const int   HISTORY_LEN = 288;              // 24 h of history in RAM
const int   DEBOUNCE = 3;                   // checks in a row before a status change counts
const int   CLEAN_DAYS = 90;                // cleaning reminder (users clean every 3-4 months)
const float CAUTION_NTU = 5, INSPECT_NTU = 25;
const float PH_LO = 6.5, PH_HI = 8.5, TEMP_MAX = 35;
const float PH_V7 = 2.5, PH_SLOPE = -0.18;  // volts at pH 7, volts per pH unit (calibrate!)
const char* NOTE = "\\nNote: sensors flag changes but cannot confirm water is safe to drink.";

Preferences pf;
WebServer server(80);
float clearV = 3.3, sampV = 0, sampN = 0;
uint32_t cleanedAt = 0;
float volt = 0, ntu = 0, tempC = NAN, ph = NAN;
int level = 0;                               // 0 Normal, 1 Caution, 2 Inspection
uint16_t histNtu10[HISTORY_LEN];
int histCount = 0, histHead = 0;
unsigned long lastSampleMs = 0, lastCheckMs = 0, lastRepeat = 0, lastCleanMsg = 0;

float readPin(int pin) {
  long sum = 0;
  for (int i = 0; i < 20; i++) { sum += analogReadMilliVolts(pin); delay(5); }
  return (sum / 20.0) / 1000.0;
}

void readAll() {
  volt = readPin(SENSOR_PIN);
  float drop = 1 - min(volt / clearV, 1.0f), k = 60 / 0.85;
  if (sampN > 0 && sampV > 0) { float d2 = 1 - min(sampV / clearV, 1.0f); if (d2 > 0.01) k = sampN / d2; }
  ntu = max(0.0f, drop * k);
#if HAS_TEMP
  tempSensor.requestTemperatures();
  float t = tempSensor.getTempCByIndex(0);
  tempC = (t > -100) ? t : NAN;
#endif
#if HAS_PH
  ph = 7 + (readPin(PH_PIN) - PH_V7) / PH_SLOPE;
#endif
}

int calcLevel() {
  int l = ntu >= INSPECT_NTU ? 2 : ntu >= CAUTION_NTU ? 1 : 0;
  if (!isnan(ph)) { if (ph < 6.0 || ph > 9.0) l = 2; else if (ph < PH_LO || ph > PH_HI) l = max(l, 1); }
  if (!isnan(tempC) && tempC > TEMP_MAX) l = max(l, 1);
  return l;
}

uint32_t nowEpoch() { time_t t = time(nullptr); return t > 1700000000 ? (uint32_t)t : 0; }

String info() {
  String s = "Turbidity " + String(ntu, 1) + " NTU";
  if (!isnan(tempC)) s += ", " + String(tempC, 1) + " C";
  if (!isnan(ph)) s += ", pH " + String(ph, 1);
  return s;
}

bool tg(String msg) {   // phone alert via Telegram
  if (!*TG_TOKEN || WiFi.status() != WL_CONNECTED) return false;
  WiFiClientSecure c; c.setInsecure();   // prototype: skips certificate check
  HTTPClient h;
  if (!h.begin(c, String("https://api.telegram.org/bot") + TG_TOKEN + "/sendMessage")) return false;
  h.addHeader("Content-Type", "application/json");
  int code = h.POST("{\"chat_id\":\"" + String(TG_CHAT) + "\",\"text\":\"" + msg + "\"}");
  h.end();
  Serial.println(code == 200 ? String("Telegram alert sent") : String("Telegram failed, HTTP ") + String(code));
  return code == 200;
}

void uploadCloud() {    // ThingSpeak: f1 turbidity, f2 volts, f3 temp, f4 pH, f5 status, f6 calibrated
  if (!*TS_KEY || WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure c; c.setInsecure();
  HTTPClient h;
  String u = String("https://api.thingspeak.com/update?api_key=") + TS_KEY + "&field1=" + String(ntu, 2) +
             "&field2=" + String(volt, 3) + "&field5=" + String(level) + "&field6=" + String(sampN > 0 ? 1 : 0);
  if (!isnan(tempC)) u += "&field3=" + String(tempC, 1);
  if (!isnan(ph)) u += "&field4=" + String(ph, 2);
  if (h.begin(c, u)) {
    int code = h.GET();
    String body = h.getString();
    h.end();
    Serial.println("ThingSpeak upload: HTTP " + String(code) + ", entry " + body);   // entry 0 = rejected
  }
}

void checkAlerts() {
  static int pend = 0, cnt = 0;
  readAll();
  int l = calcLevel();
  if (l == level) cnt = 0;
  else {
    cnt = (l == pend) ? cnt + 1 : 1; pend = l;
    if (cnt >= DEBOUNCE) {
      level = l; cnt = 0; lastRepeat = millis();
      String m = l == 0 ? "Aquaholics: water readings are back to Normal. "
               : l == 1 ? "Aquaholics CAUTION: water quality is changing. Plan a tank check. "
                        : "Aquaholics INSPECTION needed: please check the tank. ";
      tg(m + info() + NOTE);
      uploadCloud();
    }
  }
  if (level == 2 && millis() - lastRepeat > 21600000UL) {   // repeat every 6 h until resolved
    tg("Aquaholics reminder: the tank still needs inspection. " + info());
    lastRepeat = millis();
  }
  uint32_t now = nowEpoch();
  if (now && cleanedAt && (now - cleanedAt) / 86400 >= CLEAN_DAYS && (lastCleanMsg == 0 || millis() - lastCleanMsg > 604800000UL)) {
    tg("Aquaholics: tank cleaning is due. " + String((now - cleanedAt) / 86400) + " days since the last cleaning.");
    lastCleanMsg = millis();
  }
}

void addSample() {
  readAll();
  histNtu10[histHead] = (uint16_t)min(ntu * 10 + 0.5f, 65535.0f);
  histHead = (histHead + 1) % HISTORY_LEN;
  if (histCount < HISTORY_LEN) histCount++;
  lastSampleMs = millis();
  uploadCloud();
}

String num(float v, int d) { return isnan(v) ? String("null") : String(v, d); }
void sendJson(const String& s) { server.sendHeader("Access-Control-Allow-Origin", "*"); server.send(200, "application/json", s); }

void handleReading() {
  readAll();
  uint32_t now = nowEpoch();
  String days = (now && cleanedAt) ? String((now - cleanedAt) / 86400) : String("null");
  sendJson("{\"voltage\":" + String(volt, 3) + ",\"ntu\":" + String(ntu, 1) + ",\"level\":" + String(level) +
           ",\"calibrated\":" + (sampN > 0 ? "true" : "false") + ",\"temp\":" + num(tempC, 1) + ",\"ph\":" + num(ph, 2) +
           ",\"cleanDays\":" + days + ",\"cleanDue\":" + String(CLEAN_DAYS) + "}");
}

void handleHistory() {
  String out; out.reserve(histCount * 5 + 80);
  out += "{\"interval\":"; out += (unsigned long)(SAMPLE_MS / 1000);
  out += ",\"age\":"; out += (unsigned long)((millis() - lastSampleMs) / 1000);
  out += ",\"ntu\":[";
  int start = (histHead - histCount + HISTORY_LEN) % HISTORY_LEN;
  for (int i = 0; i < histCount; i++) { if (i) out += ','; out += (unsigned int)histNtu10[(start + i) % HISTORY_LEN]; }
  out += "]}";
  sendJson(out);
}

void handleCal() {      // /calibrate?clear=1 | ?ntu=20 | ?reset=1  (saved in flash)
  readAll();
  if (server.hasArg("reset")) { clearV = 3.3; sampV = sampN = 0; }
  else if (server.hasArg("clear")) { clearV = volt; sampV = sampN = 0; }
  else if (server.hasArg("ntu")) {
    float n = server.arg("ntu").toFloat();
    if (n <= 0 || volt >= clearV * 0.99) { server.send(400, "text/plain", "bad"); return; }
    sampV = volt; sampN = n;
  }
  pf.putFloat("clearV", clearV); pf.putFloat("sampV", sampV); pf.putFloat("sampN", sampN);
  handleReading();
}

void handleCleaned() {  // "Tank cleaned today" button
  cleanedAt = nowEpoch();
  if (!cleanedAt) { server.send(503, "text/plain", "clock not synced yet"); return; }
  pf.putUInt("cleaned", cleanedAt); lastCleanMsg = 0;
  handleReading();
}

void handleRoot() { server.send_P(200, "text/html", INDEX_HTML); }

void keepWifi() {
  static bool wasUp = false, announced = false;
  static unsigned long lastTry = 0;
  bool up = (WiFi.status() == WL_CONNECTED);
  if (up && !wasUp) {
    Serial.print("\nWi-Fi connected. Open on your phone: http://"); Serial.println(WiFi.localIP());
    MDNS.end();
    if (MDNS.begin("aquaholics")) { MDNS.addService("http", "tcp", 80); Serial.println("Also at http://aquaholics.local/"); }
    configTime(19800, 0, "pool.ntp.org", "time.google.com");   // IST; needed for the cleaning reminder
    if (!announced) { announced = true; tg("Aquaholics monitor is online. " + info()); }
  }
  if (!up && wasUp) Serial.println("\nWi-Fi lost. Retrying...");
  wasUp = up;
  if (!up && millis() - lastTry > 10000) { lastTry = millis(); WiFi.disconnect(); WiFi.begin(WIFI_SSID, WIFI_PASS); }
}

void setup() {
  Serial.begin(115200);
  pf.begin("aqua", false);
  clearV = pf.getFloat("clearV", 3.3); sampV = pf.getFloat("sampV", 0); sampN = pf.getFloat("sampN", 0);
  cleanedAt = pf.getUInt("cleaned", 0);
  analogSetPinAttenuation(SENSOR_PIN, ADC_11db);
  analogSetPinAttenuation(PH_PIN, ADC_11db);
#if HAS_TEMP
  tempSensor.begin();
#endif
  WiFi.mode(WIFI_STA); WiFi.persistent(false); WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) { delay(500); Serial.print("."); }
  if (WiFi.status() != WL_CONNECTED) Serial.println("\nNo Wi-Fi yet (check name/password). Will keep retrying.");
  server.on("/", handleRoot);
  server.on("/reading", handleReading);
  server.on("/history", handleHistory);
  server.on("/calibrate", handleCal);
  server.on("/cleaned", handleCleaned);
  server.begin();
  addSample();
}

void loop() {
  server.handleClient();
  keepWifi();
  if (millis() - lastCheckMs >= CHECK_MS) { lastCheckMs = millis(); checkAlerts(); }
  if (millis() - lastSampleMs >= SAMPLE_MS) addSample();
}
