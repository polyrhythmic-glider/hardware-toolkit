const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const page=fs.readFileSync(process.argv[2],'utf8'),header=fs.readFileSync(process.argv[4],'utf8');
const source=page.slice(page.indexOf('function petSample('),page.indexOf('const atlas=new Image()'));
const body=header.match(/Frame160_0\[\] PROGMEM = \{([\s\S]*?)\};/)[1];
const base=Uint8Array.from([...body.matchAll(/0x([0-9a-f]{2})/g)].map(m=>parseInt(m[1],16))),context={frames:[],base};
vm.runInNewContext(source+`
 for(const pose of ['idle','happy','sleepy','alert'])for(let tick=0;tick<100;tick++){
  const p=petSample(pose,tick*150),out=petRaster(base,160,p);let hash=2166136261;
  for(const b of out)hash=Math.imul(hash^b,16777619)>>>0;
  frames.push([p.blink,p.mouth,p.bob,p.tail,p.happy?1:0,hash].join(','));
 }
`,context);
assert.equal(context.frames.join('\n')+'\n',fs.readFileSync(process.argv[3],'utf8'));
console.log('400 coherent raster animations match bit-for-bit between firmware and web.');
