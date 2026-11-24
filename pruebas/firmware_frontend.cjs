// Integración del paquete generado por C++ con ingest() real del HTML.
// No emula LoRa, Web Serial ni dibujo en navegador.
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const assert = require('node:assert/strict');
const html = fs.readFileSync(path.join(__dirname,'..','estacion_terrena.html'),'utf8');
const elements = new Map();
const context = vm.createContext({document:{
  getElementById(id){
    if(!elements.has(id)) elements.set(id,{textContent:'',innerHTML:'',firstChild:null,insertAdjacentHTML(){}});
    return elements.get(id);
  }, querySelectorAll(){return [];}
}, setInterval(){},clearInterval(){},requestAnimationFrame(){}, Date, Math});
vm.runInContext(html.match(/<script>([\s\S]*?)<\/script>/)[1],context);
const lines = fs.readFileSync(0,'utf8').trim().split(/\r?\n/);
for (const line of lines) vm.runInContext(`ingest(${JSON.stringify(line)})`,context);
assert.equal(vm.runInContext('D.length',context),lines.length);
assert.equal(vm.runInContext('bad',context),0);
assert.equal(vm.runInContext('D[0].rssi',context),-75);
assert.equal(vm.runInContext('D[0].snr',context),8.5);
assert.equal(vm.runInContext('D.every(d=>d.st===0&&d.flags===0&&d.sd===-1&&d.sb===-1)',context),true);
if (process.argv.includes('--receptor')) {
  assert.equal(lines.length,2);
  assert.equal(vm.runInContext('D[0].seq',context),42);
  assert.equal(vm.runInContext('D[1].seq',context),43);
  assert.equal(vm.runInContext('D[1].rssi',context),-91);
  assert.equal(vm.runInContext('D[1].snr',context),-2.25);
  assert.equal(vm.runInContext('D.every(d=>d.h===null&&d.vel===null&&d.g===null)',context),true);
  assert.equal(vm.runInContext('off',context),null);
} else if (lines.length === 1) {
  assert.equal(vm.runInContext('D[0].seq',context),42);
  assert.equal(vm.runInContext('D[0].h===null&&D[0].vel===null&&D[0].g===null',context),true);
  assert.equal(vm.runInContext('off',context),null);
} else {
  assert.equal(vm.runInContext('off',context),432);
  assert.equal(vm.runInContext('maxA',context),25);
  assert.equal(vm.runInContext('[...new Set(D.map(d=>d.f))].join()',context),'0,1,2,3');
  assert.equal(vm.runInContext('D.at(-1).f',context),3);
  assert.equal(vm.runInContext('D.at(-1).t-tL>=10',context),true);
  assert.equal(vm.runInContext('lost',context),0);
  // Las medidas originales permanecen sin corregir para exportar el CSV.
  assert.equal(vm.runInContext('D[0].alt',context),432);
}
console.log(`Firmware → frontend v2: ${lines.length} paquetes, formato y derivados OK (datos de prueba)`);
