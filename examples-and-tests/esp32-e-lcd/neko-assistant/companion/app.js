const SERVICE='01004f4b-454e-a19e-6f4d-5280106e2b7a';
const uuid=n=>`${n.toString(16).padStart(2,'0')}004f4b-454e-a19e-6f4d-5280106e2b7a`;
const ids={protocol:uuid(0x10),time:uuid(0x12),alarm:uuid(0x14),timer:uuid(0x15),notification:uuid(0x16),catName:uuid(0x17),wifi:uuid(0x18),control:uuid(0x1a),diagnostics:uuid(0x1b),result:uuid(0x1c)};
let device,custom,batteryService,environmentService,resultCharacteristic,pending,refreshing=false,work=Promise.resolve();
const enc=new TextEncoder(),dec=new TextDecoder(),$=id=>document.getElementById(id);
const gatt=fn=>{const next=work.then(fn);work=next.catch(()=>{});return next};
function notice(text){$('feedback').textContent=text}
const action=fn=>async e=>{const b=e?.currentTarget;if(b)b.disabled=true;try{await fn();notice('Fatto. Neko ha ricevuto.')}catch(e){notice(e.message||String(e))}finally{if(b)b.disabled=false}};
function utf8(value,max){const b=enc.encode(value.trim());if(!b.length||b.length>max)throw Error(`Usa da 1 a ${max} byte di testo`);return b}
async function connect(){
 device=await navigator.bluetooth.requestDevice({filters:[{services:[SERVICE]}],optionalServices:['battery_service','environmental_sensing']});
 device.addEventListener('gattserverdisconnected',()=>{custom=null;resultCharacteristic=null;if(pending){pending.reject(Error('Collegamento interrotto'));pending=null}$('connection').textContent='Disconnesso';$('connect').textContent='Ricollega'});
 const server=await device.gatt.connect();custom=await server.getPrimaryService(SERVICE);batteryService=await server.getPrimaryService('battery_service');environmentService=await server.getPrimaryService('environmental_sensing');
 const protocol=await(await custom.getCharacteristic(ids.protocol)).readValue();if(protocol.getUint8(0)!==1)throw Error('Versione protocollo non supportata');
 try{resultCharacteristic=await custom.getCharacteristic(ids.result);resultCharacteristic.addEventListener('characteristicvaluechanged',e=>{const value=dec.decode(e.target.value);if(pending){const p=pending;pending=null;value.startsWith('OK')?p.resolve():p.reject(Error(value.replace(/^ERR /,'')))}});await resultCharacteristic.startNotifications()}catch{resultCharacteristic=null}
 $('connection').textContent=`Connesso a ${device.name}`;$('connect').textContent='Ricollega';await refresh();
}
async function refresh(){
 if(!custom||refreshing)return;refreshing=true;
 try{await gatt(async()=>{const bat=await(await batteryService.getCharacteristic('battery_level')).readValue(),temp=await(await environmentService.getCharacteristic('temperature')).readValue(),humidity=await(await environmentService.getCharacteristic('humidity')).readValue(),name=await(await custom.getCharacteristic(ids.catName)).readValue();
 $('battery').textContent=bat.getUint8(0)+'%';const t=temp.getInt16(0,true),h=humidity.getUint16(0,true);$('temperature').textContent=t===-32768?'-- °C':(t/100).toFixed(1)+' °C';$('humidity').textContent=h===65535?'--%':(h/100).toFixed(0)+'%';if(document.activeElement!==$('catName'))$('catName').value=dec.decode(name);
 const diagnostics=await(await custom.getCharacteristic(ids.diagnostics)).readValue();let data;try{data=JSON.parse(dec.decode(diagnostics))}catch{return}
 if(!Array.isArray(data.timers))return;
 if(document.activeElement!==$('profile'))$('profile').value=data.profile;
 $('timers').replaceChildren();for(const t of data.timers){const row=document.createElement('div');row.className='timer-row';const text=document.createElement('span');text.textContent=`#${t.id} · ${Math.floor(t.seconds/60)}:${String(t.seconds%60).padStart(2,'0')} ${t.paused?'in pausa':''}`;row.append(text);for(const[label,op]of[[t.paused?'Riprendi':'Pausa',t.paused?0x11:0x10],['Annulla',0x12]]){const button=document.createElement('button');button.textContent=label;button.onclick=action(async()=>{const b=new Uint8Array(5);b[0]=op;new DataView(b.buffer).setUint32(1,t.id,true);await write(ids.control,b);await refresh()});row.append(button)}$('timers').append(row)}
 })}finally{refreshing=false}
}
async function write(id,data){return gatt(async()=>{
 if(!custom)throw Error('Connetti prima Neko');const c=await custom.getCharacteristic(id);
 let timer,ack;if(resultCharacteristic){ack=new Promise((resolve,reject)=>{pending={resolve,reject};timer=setTimeout(()=>{pending=null;reject(Error('Neko non ha confermato. Controlla il display.'))},6000)});ack.catch(()=>{})}
 try{await c.writeValueWithResponse(data);if(ack)await ack}finally{clearTimeout(timer);pending=null}
})}
$('connect').onclick=action(connect);
$('saveName').onclick=action(()=>write(ids.catName,utf8($('catName').value,23)));
$('setTime').onclick=action(()=>{const b=new ArrayBuffer(8);new DataView(b).setBigUint64(0,BigInt(Math.floor(Date.now()/1000)),true);return write(ids.time,b)});
$('sendNotification').onclick=action(()=>write(ids.notification,utf8($('notification').value,95)));
$('saveAlarm').onclick=action(()=>{const[h,m]=$('alarmTime').value.split(':').map(Number);if(!Number.isFinite(h)||!Number.isFinite(m))throw Error('Scegli un orario');const label=utf8($('alarmLabel').value,23),b=new Uint8Array(29);b.set([h,m,Number($('alarmDays').value),1]);b.set(label,4);b[28]=Number($('alarmIndex').value);return write(ids.alarm,b)});
$('startTimer').onclick=action(async()=>{const seconds=Number($('timerMinutes').value)*60;if(!Number.isInteger(seconds)||seconds<1||seconds>604800)throw Error('Durata non valida');const label=utf8($('timerLabel').value,23),b=new Uint8Array(4+label.length);new DataView(b.buffer).setUint32(0,seconds,true);b.set(label,4);await write(ids.timer,b);await refresh()});
$('saveProfile').onclick=action(()=>write(ids.control,new Uint8Array([0x13,Number($('profile').value)])));
$('saveReminder').onclick=action(()=>{const minutes=Number($('reminderMinutes').value);if(!Number.isInteger(minutes)||minutes<1||minutes>10080)throw Error('Durata non valida');const text=utf8($('reminderText').value,63),b=new Uint8Array(9+text.length);b[0]=0x14;new DataView(b.buffer).setBigUint64(1,BigInt(Math.floor(Date.now()/1000)+minutes*60),true);b.set(text,9);return write(ids.control,b)});
$('saveWifi').onclick=action(async()=>{const s=utf8($('ssid').value,32),p=enc.encode($('wifiPassword').value);if(p.length>64)throw Error('Password troppo lunga');const b=new Uint8Array(2+s.length+p.length);b.set([s.length,p.length]);b.set(s,2);b.set(p,2+s.length);try{await write(ids.wifi,b)}finally{$('wifiPassword').value=''}});
if(!navigator.bluetooth){$('connect').disabled=true;$('connection').textContent='Web Bluetooth non disponibile. Usa la dashboard Wi-Fi di Neko.'}
setInterval(()=>{if(!document.hidden)refresh().catch(e=>notice(e.message))},2500);
