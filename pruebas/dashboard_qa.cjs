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
check('Rechaza st fuera de 0–3', run=>{
  feed(run,{st:4}); assert.equal(run('D.length'),0);
});
// Vuelo a 5 Hz con altura barométrica: 2 s en el suelo (430 m), sube a 1.5 m/s hasta 452 m,
// espera 2 s, baja a 10 m/s y queda 10 s en el suelo. El firmware manda st=0 todo el tiempo.
function vuelo(run){
  const alts=[];for(let i=0;i<10;i++)alts.push(430);
  for(let a=430.3;a<452;a+=.3)alts.push(+a.toFixed(2));
  for(let i=0;i<10;i++)alts.push(452);
  for(let a=450;a>430;a-=2)alts.push(a);
  for(let i=0;i<=50;i++)alts.push(430);
  alts.forEach((alt,i)=>feed(run,{seq:i,ms:i*200,alt}));
}
check('Calcula las 4 fases con la altura', run=>{
  vuelo(run);
  assert.equal(run('[...new Set(D.map(d=>d.f))].join()'),'0,1,2,3');
  assert.equal(run('PH[D.at(-1).f]'),'Aterrizado');
  assert.equal(run('Math.round(maxA)'),22);
});
check('Esperar arriba sin moverse sigue siendo subida', run=>{
  vuelo(run);
  assert.equal(run('D.filter(d=>d.alt===452).every(d=>d.f===1)'),true);
});
check('Mide 10 s transmitiendo después de aterrizar', run=>{
  vuelo(run);
  assert.equal(run('Math.round(D.at(-1).t-tL)'),10);
});
check('Toma el cero solo con los 5 primeros paquetes en tierra', run=>{
  for(let i=0;i<5;i++) feed(run,{seq:i,ms:i*200,alt:430+(i%2)*.2});
  feed(run,{seq:5,ms:1000,alt:452.1});
  assert.equal(run('offTipo'),'auto'); assert.equal(run('off.toFixed(2)'),'430.08');
  assert.equal(run('D.at(-1).h.toFixed(2)'),'22.02'); assert.equal(run('D[0].alt'),430);
});
check('Sin barómetro no toma cero ni cambia de fase', run=>{
  for(let i=0;i<8;i++) feed(run,{seq:i,ms:i*200,alt:-1});
  assert.equal(run('off'),null); assert.equal(run('D.at(-1).f'),0);
});
check('Rechaza flags fuera de los dos bits definidos', run=>{
  feed(run,{flags:4}); assert.equal(run('D.length'),0);
});
check('Rechaza números con formato no decimal', run=>{
  feed(run,{temp:'0x1'}); feed(run,{seq:1,ms:200,temp:' 1'});
  assert.equal(run('D.length'),0); assert.equal(run('bad'),2);
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
check('Fijar cero conserva la altura máxima de la captura recalibrada', run=>{
  feed(run,{alt:100}); feed(run,{seq:1,ms:200,alt:20});
  run('cero()'); assert.equal(run('maxA'),80);
});
check('Fijar cero conserva la altura original para exportar', run=>{
  feed(run,{alt:20}); run('cero()');
  // csv() exporta d.alt: si se modifica, se pierde la medida recibida.
  assert.equal(run('D[0].alt'),20);
});
check('Sensores en -1 se muestran como sin dato, no como medida', run=>{
  feed(run,{alt:-1,ax:-1,ay:-1,az:-1}); feed(run,{seq:1,ms:200,alt:-1,ax:-1,ay:-1,az:-1});
  assert.equal(run('D[1].g'),null); assert.equal(run('D[1].h'),null);
  assert.equal(run('D[1].vel'),null); assert.equal(run('maxA'),null);
});
check('Un reinicio del CanSAT no inventa pérdidas', run=>{
  feed(run,{seq:500,ms:100000}); run('tLast-=2000');
  feed(run,{seq:0,ms:1200}); feed(run,{seq:1,ms:1400});
  assert.equal(run('lost'),0); assert.equal(run('D.at(-1).ses'),2);
});
check('Calcula la velocidad de descenso', run=>{
  feed(run,{alt:20}); feed(run,{seq:1,ms:200,alt:18});
  assert.equal(run('D[1].vel'),-10);
});
process.stdout.write(JSON.stringify({tipo:'QA lógica del dashboard; sin navegador ni hardware',
  total:results.length, pasan:results.filter(r=>r.resultado==='PASA').length,
  fallan:results.filter(r=>r.resultado==='FALLA').length, casos:results},null,2)+'\n');
process.exitCode=results.some(r=>r.resultado==='FALLA')?1:0;
