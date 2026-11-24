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
const line = fs.readFileSync(0,'utf8').trim();
vm.runInContext(`ingest(${JSON.stringify(line)})`,context);
assert.equal(vm.runInContext('D.length',context),1);
assert.equal(vm.runInContext('D[0].seq',context),42);
assert.equal(vm.runInContext('D[0].rssi',context),-75);
assert.equal(vm.runInContext('D[0].snr',context),8.5);
assert.equal(vm.runInContext('D[0].st',context),0);
console.log('Paquete de firmware aceptado por ingest() del HTML: OK');
