// Ejecuta la lógica real del HTML con un DOM mínimo. No sustituye un navegador.
// Salida JSON; exit 1 significa que hay requisitos incumplidos para Geisler.
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const assert = require('node:assert/strict');
const html = fs.readFileSync(path.join(__dirname, '..', 'estacion_terrena.html'), 'utf8');
const script = html.match(/<script>([\s\S]*?)<\/script>/)[1];
function station() {
  const elements = new Map();
  const context = vm.createContext({
    document: {
      getElementById(id) {
        if (!elements.has(id)) elements.set(id, {textContent:'', innerHTML:'',
          firstChild:null, insertAdjacentHTML(){}});
        return elements.get(id);
      },
      querySelectorAll(){ return []; }
    },
    setInterval(){return 1;}, clearInterval(){}, requestAnimationFrame(){},
    Date, Math, Number, console
  });
  vm.runInContext(script, context);
  return code => vm.runInContext(code, context);
}
function line(changes={}) {
  const d = {seq:0,ms:0,st:0,pres:1013.25,temp:25,alt:0,ax:0,ay:0,az:1,
    roll:0,pitch:0,yaw:0,flags:0,sd:-1,sb:-1,rssi:-75,snr:8.5,...changes};
  return '$'+Object.values(d).join(',');
}
const results=[];
function check(name, run) {
  try { run(station()); results.push({caso:name, resultado:'PASA'}); }
  catch(e) {results.push({caso:name, resultado:'FALLA', detalle:e.message});}
}
const feed=(run, changes)=>run(`ingest(${JSON.stringify(line(changes))})`);
check('Acepta el paquete de 17 campos', run=>{
  feed(run); assert.equal(run('D.length'),1);
});
check('Rechaza un paquete truncado', run=>{
  run('ingest("$0,0,0")'); assert.equal(run('D.length'),0);
});
check('Rechaza campos vacíos', run=>{
  feed(run,{temp:''}); assert.equal(run('D.length'),0);
});
check('Rechaza infinito', run=>{
  feed(run,{temp:'Infinity'}); assert.equal(run('D.length'),0);
});
check('Rechaza fase fuera de 0–5', run=>{
  feed(run,{st:6}); assert.equal(run('D.length'),0);
});
check('Rechaza flags fuera de los cuatro bits definidos', run=>{
  feed(run,{flags:16}); assert.equal(run('D.length'),0);
});
check('No cuenta dos veces un paquete duplicado', run=>{
  feed(run); feed(run); assert.equal(run('D.length'),1);
});
check('Descarta paquetes atrasados para calcular pérdidas', run=>{
  feed(run,{seq:10,ms:2000}); feed(run,{seq:9,ms:1800});
  feed(run,{seq:11,ms:2200}); assert.equal(run('lost'),0);
});
check('Calcula las pérdidas conocidas', run=>{
  feed(run); feed(run,{seq:3,ms:600}); assert.equal(run('lost'),2);
});
check('Mide el intervalo entre dos muestras de aterrizaje', run=>{
  feed(run,{st:5,flags:4}); feed(run,{seq:50,ms:10000,st:5,flags:4});
  assert.equal(run('(D.at(-1).ms-tL)/1000'),10);
});
check('Fijar cero conserva la altura máxima de la captura recalibrada', run=>{
  feed(run,{alt:100}); feed(run,{seq:1,ms:200,alt:20});
  run('cero()'); assert.equal(run('maxA'),80);
});
check('Fijar cero conserva la altura original para exportar', run=>{
  feed(run,{alt:20}); run('cero()');
  // csv() exporta d.alt: si se modifica, se pierde la medida recibida.
  assert.equal(run('D[0].alt'),20);
});
process.stdout.write(JSON.stringify({tipo:'QA lógica del dashboard; sin navegador ni hardware',
  total:results.length, pasan:results.filter(r=>r.resultado==='PASA').length,
  fallan:results.filter(r=>r.resultado==='FALLA').length, casos:results},null,2)+'\n');
process.exitCode=results.some(r=>r.resultado==='FALLA')?1:0;
