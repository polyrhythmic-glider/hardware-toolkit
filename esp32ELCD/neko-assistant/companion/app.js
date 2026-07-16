const SERVICE='01004f4b-454e-a19e-6f4d-5280106e2b7a';
const uuid=n=>`${n.toString(16).padStart(2,'0')}004f4b-454e-a19e-6f4d-5280106e2b7a`;
const ids={protocol:uuid(0x10),status:uuid(0x11),time:uuid(0x12),timezone:uuid(0x13),alarm:uuid(0x14),timer:uuid(0x15),notification:uuid(0x16),catName:uuid(0x17),wifi:uuid(0x18),volume:uuid(0x19),control:uuid(0x1a),diagnostics:uuid(0x1b)};
let device,server,custom,batteryService,environmentService;
const enc=new TextEncoder(),dec=new TextDecoder();
const $=id=>document.getElementById(id);

async function connect(){
  device=await navigator.bluetooth.requestDevice({filters:[{services:[SERVICE]}],optionalServices:['battery_service','environmental_sensing']});
  device.addEventListener('gattserverdisconnected',()=>{$('connection').textContent='Disconnesso';});
  server=await device.gatt.connect();
  custom=await server.getPrimaryService(SERVICE);
  batteryService=await server.getPrimaryService('battery_service');
  environmentService=await server.getPrimaryService('environmental_sensing');
  const protocol=await (await custom.getCharacteristic(ids.protocol)).readValue();
  if(protocol.getUint8(0)!==1) throw new Error('Versione protocollo non supportata');
  $('connection').textContent=`Connesso a ${device.name}`;
  await refresh();
}

async function refresh(){
  const bat=await (await batteryService.getCharacteristic('battery_level')).readValue();
  const temp=await (await environmentService.getCharacteristic('temperature')).readValue();
  const humidity=await (await environmentService.getCharacteristic('humidity')).readValue();
  const name=await (await custom.getCharacteristic(ids.catName)).readValue();
  $('battery').textContent=`${bat.getUint8(0)}%`;
  $('temperature').textContent=`${(temp.getInt16(0,true)/100).toFixed(1)} °C`;
  $('humidity').textContent=`${(humidity.getUint16(0,true)/100).toFixed(0)}%`;
  $('catName').value=dec.decode(name);
}

async function write(id,data){if(!custom)throw new Error('Connetti prima Neko');const c=await custom.getCharacteristic(id);await c.writeValueWithResponse(data);}
const action=fn=>async()=>{try{await fn();}catch(e){alert(e.message||String(e));}};

$('connect').onclick=action(connect);
$('saveName').onclick=action(async()=>write(ids.catName,enc.encode($('catName').value.trim())));
$('setTime').onclick=action(async()=>{const b=new ArrayBuffer(8);new DataView(b).setBigUint64(0,BigInt(Math.floor(Date.now()/1000)),true);await write(ids.time,b);});
$('sendNotification').onclick=action(async()=>write(ids.notification,enc.encode($('notification').value.trim())));
$('saveAlarm').onclick=action(async()=>{const [h,m]=$('alarmTime').value.split(':').map(Number),label=enc.encode($('alarmLabel').value.trim()),b=new Uint8Array(4+label.length);b.set([h,m,Number($('alarmDays').value),1]);b.set(label,4);await write(ids.alarm,b);});
$('saveWifi').onclick=action(async()=>{const s=enc.encode($('ssid').value),p=enc.encode($('wifiPassword').value);if(!s.length||s.length>32||p.length>64)throw new Error('Lunghezza SSID/password non valida');const b=new Uint8Array(2+s.length+p.length);b.set([s.length,p.length]);b.set(s,2);b.set(p,2+s.length);await write(ids.wifi,b);$('wifiPassword').value='';});

if(!navigator.bluetooth){$('connect').disabled=true;$('connection').textContent='Web Bluetooth non disponibile';}
