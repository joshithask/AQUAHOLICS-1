# Aquaholics Tank Monitor

Overhead water-tank monitor: turbidity (plus optional temperature and pH) on an ESP32, three status levels
(**Normal / Caution / Inspection**), phone alerts, cloud storage and an Android dashboard.
Sensors flag changes in the water. They cannot confirm that water is safe to drink.

- `firmware/aquaholics_esp32/` ESP32 sketch (Arduino IDE). Open the `.ino` file.
- `android/` Android app: WebView dashboard, plus background alerts every 2 minutes
- `web/` the same dashboard as a standalone page (demo mode without hardware)

## 1. Cloud and alerts (set up once, online)
- **ThingSpeak:** create a channel with 6 fields in this order: Turbidity, Voltage, Temperature, pH, Status, Calibrated.
  Note the channel ID, the Write API key and the Read API key.
- **Telegram:** create a bot with @BotFather, then get the bot token and your chat ID.

## 2. Flash the ESP32
1. In `firmware/aquaholics_esp32/` copy `secrets.example.h` to `secrets.h` and fill in your Wi-Fi, Telegram token/chat ID and ThingSpeak write key.
   **Never commit `secrets.h`** (it is in `.gitignore`; this repository is public).
2. Optional sensors: set `HAS_TEMP` / `HAS_PH` to 1 in the `.ino` once wired (temperature needs the OneWire and DallasTemperature libraries).
3. Upload from the Arduino IDE (ESP32 board, 115200 baud Serial Monitor). The Serial Monitor prints the address and every cloud/alert result.
4. Wiring: GPIO34 tolerates 3.3 V max. Power the sensor module from 3V3. Clear water should read about 2.5 to 3.0 V.

## 3. Android app
Get the APK from the **Actions** tab: "Build Android APK" -> latest run -> Artifacts -> `TankMonitor-apk`.
Locally: open `android/` in Android Studio, or run `gradle assembleDebug` there (Gradle 8.7, Java 17 or 21).

In the app: Setup tab -> ESP32 address (`http://<esp32-ip>`), ThingSpeak channel ID and **read** key -> Save.
Allow notifications and set the app's battery use to Unrestricted. Calibrate with clear water ("Set current water as clear").

## Notes
- Calibration and the last-cleaned date are saved on the ESP32. Cleaning reminder after 90 days.
- Alerts: the ESP32 sends Telegram messages itself (fastest). The app also checks the cloud every 2 minutes while it is allowed to run.
- After a phone restart, open the app once to restart the 2-minute check.
- Do not put the ThingSpeak write key in the app. Only the ESP32 needs it.
