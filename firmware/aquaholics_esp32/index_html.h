const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>Aquaholics Tank Monitor</title>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Bricolage+Grotesque:wght@600;800&family=DM+Sans:wght@400;500;700&display=swap">
<style>
:root{--bg:#EEF5F6;--surface:#FFFFFF;--ink:#0D2B38;--mute:#587583;--line:#D3E2E6;--ok:#14876B;--warn:#C27B0A;--bad:#C23B2E;--accent:#0B6E8A;
box-sizing:border-box;padding-top:env(safe-area-inset-top,0px);padding-bottom:env(safe-area-inset-bottom,0px)}
@media (prefers-color-scheme:dark){:root:not([data-theme="light"]){--bg:#091C25;--surface:#10303D;--ink:#E4F2F5;--mute:#8FB0BC;--line:#1E4555;--ok:#3FD1A6;--warn:#F0B34A;--bad:#FF7B6B;--accent:#5CC8E6}}
:root[data-theme="dark"]{--bg:#091C25;--surface:#10303D;--ink:#E4F2F5;--mute:#8FB0BC;--line:#1E4555;--ok:#3FD1A6;--warn:#F0B34A;--bad:#FF7B6B;--accent:#5CC8E6}
*{box-sizing:border-box}
html,body{margin:0;background:var(--bg);color:var(--ink);font-family:'DM Sans',system-ui,sans-serif}
html{scroll-padding-top:env(safe-area-inset-top,0px)}
main{max-width:480px;margin:0 auto;padding:16px 16px 88px}
h1,h2,.big{font-family:'Bricolage Grotesque',system-ui,sans-serif;margin:0}
h1{font-size:22px;font-weight:800}
.sub{color:var(--mute);font-size:14px;margin:2px 0 14px}
.card{background:var(--surface);border:1px solid var(--line);border-radius:18px;padding:16px;margin-bottom:14px}
.tank{display:block;width:100%;height:auto}
.row{display:flex;justify-content:space-between;align-items:flex-end;gap:12px}
.big{font-size:52px;font-weight:800;line-height:1}
.unit{font-size:16px;color:var(--mute);font-weight:500;margin-left:4px}
.pill{display:inline-block;padding:6px 14px;border-radius:99px;font-weight:700;font-size:14px;color:#fff}
.msg{margin:12px 0 0;font-size:15px;line-height:1.45}
.grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px;text-align:left}
.grid b{display:block;font-size:20px;font-family:'Bricolage Grotesque',sans-serif}
.grid span{font-size:12px;color:var(--mute)}
h2{font-size:17px;margin-bottom:10px}
.btns{display:flex;gap:8px;flex-wrap:wrap}
button{font:inherit;font-weight:500;border:1px solid var(--line);background:var(--bg);color:var(--ink);padding:10px 14px;border-radius:12px;min-height:44px}
button:focus-visible,input:focus-visible{outline:3px solid var(--accent);outline-offset:2px}
button.on{background:var(--accent);color:#fff;border-color:var(--accent)}
ul{list-style:none;margin:0;padding:0}
li{padding:10px 0;border-top:1px solid var(--line);font-size:14px;display:flex;gap:10px}
li:first-child{border-top:0}
li time{color:var(--mute);min-width:64px}
.axis{display:flex;justify-content:space-between;font-size:12px;color:var(--mute);margin-top:4px}
.chart{width:100%;height:140px;display:block}
input{width:100%;font:inherit;padding:11px;border-radius:12px;border:1px solid var(--line);background:var(--bg);color:var(--ink)}
.note{font-size:13px;color:var(--mute);line-height:1.5;margin:8px 0 0}
nav{position:fixed;left:0;right:0;bottom:0;background:var(--surface);border-top:1px solid var(--line);display:flex;justify-content:center;gap:6px;padding:8px 12px calc(8px + env(safe-area-inset-bottom,0px))}
nav button{flex:1;max-width:150px;border:0;background:transparent;color:var(--mute);font-weight:700}
nav button[aria-selected="true"]{color:var(--accent);background:var(--bg)}
section[hidden]{display:none}
@media (prefers-reduced-motion:no-preference){.water{transition:fill .8s}}
</style>
</head>
<body>
<main>
<h1>Tank monitor</h1>
<p class="sub">Team Aquaholics · Overhead tank 1 · <span id="mode">Demo data</span></p>

<section id="now">
  <div class="card">
    <svg class="tank" viewBox="0 0 320 220" role="img" aria-label="Tank with infrared beam passing through water">
      <rect x="70" y="14" width="180" height="192" rx="14" fill="none" stroke="var(--mute)" stroke-width="3"/>
      <rect id="water" class="water" x="73" y="52" width="174" height="151" rx="11" fill="#7FD6E8" opacity=".85"/>
      <circle id="dots" cx="0" cy="0" r="0"/>
      <g id="specks"></g>
      <rect x="30" y="104" width="36" height="26" rx="5" fill="var(--ink)"/>
      <text x="48" y="150" text-anchor="middle" font-size="11" fill="var(--mute)">IR LED</text>
      <rect x="254" y="104" width="36" height="26" rx="5" fill="var(--ink)"/>
      <text x="272" y="150" text-anchor="middle" font-size="11" fill="var(--mute)">Detector</text>
      <line id="beam" x1="66" y1="117" x2="254" y2="117" stroke="#E0457B" stroke-width="5" stroke-linecap="round"/>
    </svg>
    <div class="row" style="margin-top:8px">
      <div><span class="big" id="ntu">0.0</span><span class="unit" id="unit">NTU</span></div>
      <span class="pill" id="pill">Clear</span>
    </div>
    <p class="msg" id="msg"></p>
    <p class="note">Sensors flag changes in the water. They cannot confirm that it is safe to drink.</p>
  </div>

  <div class="card grid">
    <div><b id="volt">0.00 V</b><span>Sensor signal</span></div>
    <div><b id="light">100%</b><span>Light received</span></div>
    <div><b id="clock">--:--</b><span>Last reading</span></div>
  </div>

  <div class="card grid">
    <div><b id="temp">–</b><span>Temperature</span></div>
    <div><b id="ph">–</b><span>pH</span></div>
    <div><b id="clean">–</b><span>Since cleaning</span></div>
  </div>

  <div class="card">
    <h2>Try the demo</h2>
    <div class="btns">
      <button id="b1">Clean refill</button>
      <button id="b2">Stir sediment</button>
      <button id="b3">Contaminate</button>
    </div>
    <p class="note">Demo mode uses simulated readings, including the Today tab. With the ESP32 connected, every screen shows real values.</p>
  </div>
</section>

<section id="today" hidden>
  <div class="card">
    <h2>Turbidity, last 24 hours</h2>
    <svg class="chart" id="chart" viewBox="0 0 300 140" preserveAspectRatio="none" role="img" aria-label="Turbidity over the day"></svg>
    <div class="axis"><span id="t0"></span><span>Now</span></div>
    <p class="note" id="hnote"></p>
  </div>
  <div class="card">
    <h2>Evening report</h2>
    <div class="grid">
      <div><b id="avg">–</b><span>Average</span></div>
      <div><b id="max">–</b><span>Highest</span></div>
      <div><b id="over">–</b><span>Hours not normal</span></div>
    </div>
    <p class="msg" id="verdict"></p>
  </div>
  <div class="card">
    <h2>Alerts</h2>
    <ul id="alerts"></ul>
    <p class="note">Phone alerts are sent by the ESP32 itself (Telegram), so they arrive even when this app is closed. The Android app also checks the cloud about every 2 minutes and notifies you when the status changes or the device goes offline.</p>
  </div>
</section>

<section id="setup" hidden>
  <div class="card">
    <h2>Connect your ESP32</h2>
    <label for="ip" class="note" style="display:block;margin:0 0 6px">ESP32 address on your Wi-Fi</label>
    <input id="ip" placeholder="http://192.168.1.50/reading" inputmode="url" autocomplete="off">
    <div class="btns" style="margin-top:10px"><button id="save">Save address</button></div>
    <p class="note" id="conn"></p>
    <label for="chan" class="note" style="display:block;margin:12px 0 6px">Cloud (ThingSpeak) channel ID and read key, for remote monitoring</label>
    <input id="chan" placeholder="Channel ID" inputmode="numeric" autocomplete="off">
    <input id="rkey" placeholder="Read API key" style="margin-top:8px" autocomplete="off">
    <div class="btns" style="margin-top:10px"><button id="cleaned">Tank cleaned today</button></div>
    <div class="btns" style="margin-top:6px"><button id="cal">1. Set current water as clear</button></div>
    <label for="known" class="note" style="display:block;margin:12px 0 6px">2. Optional: turbidity of a known sample (NTU), for real NTU values</label>
    <input id="known" type="number" min="1" step="any" inputmode="decimal" placeholder="e.g. 20">
    <div class="btns" style="margin-top:10px"><button id="cal2">Set current water as this value</button><button id="reset">Reset calibration</button></div>
    <p class="note" id="saved"></p>
    <p class="note">If you open this app from the ESP32 itself, leave the address empty. It finds the sensor on its own. Calibration is saved on the ESP32, so alerts use it even when this app is closed. Calibrate with clean water in the tank first. Without step 2 the number is a relative turbidity index, not NTU. Use a calibration standard or a reference meter for the sample.</p>
  </div>
  <div class="card">
    <h2>Status levels (turbidity)</h2>
    <ul>
      <li><time>Under 5</time>Normal. No action needed.</li>
      <li><time>5 to 25</time>Caution. Plan a tank check.</li>
      <li><time>Over 25</time>Inspection. Check the tank.</li>
    </ul>
  </div>
</section>
</main>

<nav role="tablist">
  <button role="tab" aria-selected="true" data-t="now">Now</button>
  <button role="tab" aria-selected="false" data-t="today">Today</button>
  <button role="tab" aria-selected="false" data-t="setup">Setup</button>
</nav>

<script>
const $=id=>document.getElementById(id);
const lerp=(a,b,t)=>Math.round(a+(b-a)*t);
const pad=n=>String(n).padStart(2,'0');
const hm=t=>{const d=new Date(t);return pad(d.getHours())+':'+pad(d.getMinutes())};
const LV=[{k:'Normal',c:'var(--ok)',m:'Readings are normal. No action needed.'},{k:'Caution',c:'var(--warn)',m:'Readings are changing. Plan a tank check soon.'},{k:'Inspection',c:'var(--bad)',m:'Inspection needed. Check the tank before using the water.'}];
const AL=['Water is back to normal.','Caution: water quality is changing.','Inspection alert: check the tank.'];
const lvOf=n=>n<5?0:n<25?1:2;
const get=k=>{try{return localStorage.getItem(k)||''}catch(e){return ''}};
const put=(k,v)=>{try{localStorage.setItem(k,v)}catch(e){}};
let src='demo',cur=null,hist=[],lastHist=0,polling=false,ntu=2.2,target=2.2,base=2.2,last=0;
const t00=Date.now(),demoHist=[];
for(let i=0;i<48;i++){const n=Math.max(0.8,1.8+Math.sin(i/6)*0.8+(i>30&&i<36?9:0)+Math.random()*0.8);demoHist.push({t:t00-(47-i)*1800000,n,l:lvOf(n)})}
const demoAlerts=[{t:'06:10',m:'Tank refilled, readings normal.'},{t:'16:40',m:'Caution after refill. Back to normal in 2 hours.'}];
function series(){
  if(src==='demo')return demoHist;
  const s=hist.slice();
  if(src==='local')s.push({t:cur.t,n:cur.ntu,l:cur.level});
  return s.length?s:[{t:Date.now(),n:cur.ntu,l:cur.level}];
}
function render(){
  const demo=src==='demo',n=demo?ntu:cur.ntu,lv=demo?lvOf(n):cur.level,s=LV[lv]||LV[0],tt=Math.min(n/60,1);
  $('ntu').textContent=n.toFixed(1);
  $('unit').textContent=demo||cur.cal?'NTU':'index';
  const off=!demo&&Date.now()-cur.t>900000;
  $('mode').textContent=demo?'Demo data':src==='local'?'Live':'Cloud'+(off?' · device offline':'');
  $('pill').textContent=s.k;$('pill').style.background=s.c;
  $('msg').textContent=off?'No new reading for over 15 minutes. Check the ESP32 power and Wi-Fi.':s.m;
  const w=$('water');
  w.setAttribute('fill',`rgb(${lerp(127,140,tt)},${lerp(214,100,tt)},${lerp(232,50,tt)})`);
  w.setAttribute('opacity',(0.85+tt*0.12).toFixed(2));
  const light=Math.round(100-tt*85);
  $('beam').setAttribute('opacity',(light/100).toFixed(2));
  $('light').textContent=light+'%';
  $('volt').textContent=(demo?3.3*light/100:cur.volt).toFixed(2)+' V';
  $('clock').textContent=hm(demo?Date.now():cur.t);
  $('temp').textContent=!demo&&cur.temp!=null?cur.temp.toFixed(1)+' °C':'–';
  $('ph').textContent=!demo&&cur.ph!=null?cur.ph.toFixed(1):'–';
  $('clean').textContent=!demo&&cur.clean!=null?cur.clean+' d':'–';
  const sp=$('specks'),k=Math.round(tt*40);
  if(sp.childElementCount!==k){let h='';for(let i=0;i<k;i++){h+=`<circle cx="${80+((i*53)%160)}" cy="${60+((i*37)%135)}" r="${1.5+(i%3)}" fill="#5b4220" opacity=".6"/>`}sp.innerHTML=h}
  if(lv!==last){if(demo)demoAlerts.unshift({t:hm(Date.now()),m:AL[lv]});last=lv}
  if(demo){const p=demoHist[demoHist.length-1];p.n=ntu;p.l=lv}
  drawAlerts();drawToday();
}
function drawAlerts(){
  let a=demoAlerts;
  if(src!=='demo'){a=[];let k=null;series().forEach(p=>{if(k!==null&&p.l!==k)a.unshift({t:hm(p.t),m:AL[p.l]});k=p.l})}
  $('alerts').innerHTML=a.length?a.slice(0,8).map(x=>`<li><time>${x.t}</time><span>${x.m}</span></li>`).join(''):'<li>No status changes in this period.</li>';
}
function drawToday(){
  const s=series(),mx=Math.max(30,...s.map(p=>p.n)),t0=s[0].t,span=Math.max(s[s.length-1].t-t0,1);
  const X=t=>(t-t0)/span*300,Y=v=>136-v/mx*128;
  const pts=s.map(p=>X(p.t).toFixed(1)+','+Y(p.n).toFixed(1)).join(' ');
  $('chart').innerHTML=`<line x1="0" x2="300" y1="${Y(5)}" y2="${Y(5)}" stroke="var(--warn)" stroke-dasharray="4 4" vector-effect="non-scaling-stroke"/><line x1="0" x2="300" y1="${Y(25)}" y2="${Y(25)}" stroke="var(--bad)" stroke-dasharray="4 4" vector-effect="non-scaling-stroke"/><polyline points="${pts}" fill="none" stroke="var(--accent)" stroke-width="2.5" stroke-linejoin="round" vector-effect="non-scaling-stroke"/>`;
  let sum=0,over=0,mxv=0,insp=false;
  s.forEach((p,i)=>{sum+=p.n;mxv=Math.max(mxv,p.n);if(p.l===2)insp=true;if(i&&p.l>0)over+=Math.min(p.t-s[i-1].t,1800000)});
  const hrs=over/3600000;
  $('avg').textContent=(sum/s.length).toFixed(1);$('max').textContent=mxv.toFixed(1);$('over').textContent=hrs.toFixed(1);
  $('t0').textContent=hm(t0);
  $('hnote').textContent=(src==='local'?`Real readings from the ESP32, one every 5 minutes (${s.length} so far). `:src==='cloud'?`Stored in the cloud, one reading every 5 minutes (${s.length} loaded). `:'Demo data. Connect the ESP32 or the cloud for real history. ')+'Dashed lines mark 5 (caution) and 25 (inspection).';
  $('verdict').textContent=insp?'An inspection alert was raised in this period. Check the tank before relying on this water.':hrs>0?'Readings were outside normal for part of this period. Watch for sediment.':'Readings stayed normal.';
}
function espBase(){const v=$('ip').value.trim();return v?v.replace(/\/reading\/?$/,''):(location.protocol==='http:'?'':null)}
const getJSON=async u=>{const c=new AbortController(),to=setTimeout(()=>c.abort(),5000);try{return await(await fetch(u,{cache:'no-store',signal:c.signal})).json()}finally{clearTimeout(to)}};
async function poll(){
  if(polling)return;polling=true;
  try{
    const b=espBase(),ch=$('chan').value.trim(),rk=$('rkey').value.trim();let ok=false,msg='';
    if(b!==null){
      try{
        const j=await getJSON(b+'/reading');
        cur={ntu:j.ntu,volt:j.voltage,level:j.level,temp:j.temp,ph:j.ph,t:Date.now(),clean:j.cleanDays,cal:j.calibrated};src='local';ok=true;msg='Live from the ESP32.';
        if(Date.now()-lastHist>60000){lastHist=Date.now();try{const h=await getJSON(b+'/history'),iv=h.interval*1000,n=h.ntu.length,lt=Date.now()-h.age*1000;hist=h.ntu.map((v,i)=>({t:lt-(n-1-i)*iv,n:v/10,l:lvOf(v/10)}))}catch(e){}}
      }catch(e){}
    }
    if(!ok&&ch&&rk){
      try{
        const f=(await getJSON(`https://api.thingspeak.com/channels/${ch}/feeds.json?results=288&api_key=${rk}`)).feeds||[];
        if(f.length){
          hist=f.map(x=>{const n=+x.field1||0;return{t:Date.parse(x.created_at),n,l:x.field5!=null?+x.field5:lvOf(n)}});
          const x=f[f.length-1];
          cur={ntu:+x.field1||0,volt:+x.field2||0,level:+x.field5||0,temp:x.field3!=null?+x.field3:null,ph:x.field4!=null?+x.field4:null,t:Date.parse(x.created_at),clean:null,cal:+x.field6===1};
          src='cloud';ok=true;msg='Reading from the cloud (works from anywhere).';
        }else msg='Cloud channel has no data yet.';
      }catch(e){msg='Could not read the cloud channel. Check the channel ID and read key.'}
    }
    if(!ok){src='demo';cur=null;msg=(b===null&&!ch)?'No ESP32 address or cloud channel set. Showing demo data.':(msg||'Not connected. Showing demo data.')}
    $('conn').textContent=msg;
  }finally{polling=false}
}
function tick(){
  if(src==='demo'){target+=(base-target)*0.08;ntu+=(target-ntu)*0.3+(Math.random()-0.5)*0.25;ntu=Math.max(0.5,ntu)}
  render();
}
const say=m=>{$('saved').textContent=m};
async function dev(path,ok){
  const b=espBase();
  if(b===null||src!=='local'){say('Connect to the ESP32 first. Calibration and cleaning dates are saved on the device.');return}
  try{const j=await getJSON(b+path);cur.cal=j.calibrated;cur.clean=j.cleanDays;say(ok)}catch(e){say('The ESP32 did not accept that. For a sample, use water murkier than clear.')}
}
$('b1').onclick=()=>{base=2;target=2};
$('b2').onclick=()=>{target=Math.max(target,14)};
$('b3').onclick=()=>{base=38;target=45};
document.querySelectorAll('nav button').forEach(b=>b.onclick=()=>{
  document.querySelectorAll('nav button').forEach(x=>x.setAttribute('aria-selected',x===b));
  ['now','today','setup'].forEach(id=>$(id).hidden=id!==b.dataset.t);window.scrollTo(0,0)});
$('ip').value=get('esp-ip');$('chan').value=get('ts-chan');$('rkey').value=get('ts-key');
const sync=()=>{try{window.Android&&Android.saveCloud($('chan').value,$('rkey').value)}catch(e){}};sync();
$('save').onclick=()=>{put('esp-ip',$('ip').value);put('ts-chan',$('chan').value);put('ts-key',$('rkey').value);say('Saved.');sync();hist=[];lastHist=0;poll()};
$('cal').onclick=()=>dev('/calibrate?clear=1','Clear water saved on the ESP32.');
$('cal2').onclick=()=>{const n=parseFloat($('known').value);if(!(n>0)){say('Enter the known turbidity of your sample in NTU.');return}dev('/calibrate?ntu='+n,'Calibrated to '+n+' NTU and saved on the ESP32.')};
$('reset').onclick=()=>dev('/calibrate?reset=1','Calibration cleared.');
$('cleaned').onclick=()=>dev('/cleaned','Cleaning date saved. A reminder will be sent after 90 days.');
render();poll();setInterval(poll,2000);setInterval(tick,1500);
</script>
</body>
</html>
)rawliteral";
