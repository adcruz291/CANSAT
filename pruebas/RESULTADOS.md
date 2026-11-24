# Resultados de pruebas de Genrikh

Fecha: 24/11/2026. Estado: software probado; hardware pendiente.
Archivos base: telemetría y dashboard del commit `2e1fd9e`. Herramienta Python preexistente en la carpeta local. No se modificó el frontend ni se cargó firmware. Después se prepararon ambos firmwares base en `firmware/telemetria/`, conservando los sketches provisionales proporcionados por Genrikh.

## Evidencia ejecutada

| Prueba | Resultado |
|---|---|
| Suite Python (`test_telemetria.py`) | 11 casos aprobados: formato, vuelo, pérdidas, duplicados, orden, reinicio observable, wrap uint32 y ausencia de altura |
| `SIMULADO_vuelo.txt` → informe y CSV | 386 paquetes válidos, 0 inválidos, 0 pérdidas, altura máxima 450 m, intervalo máximo 0.2 s, intervalo posterior al aterrizaje 12 s |
| `SIMULADO_perdidas.txt` → informe | 348 válidos, 38 pérdidas estimadas; 9.845% de los paquetes entre primero y último recibidos |
| `dashboard_qa.cjs` sobre el HTML real | 12 casos: 4 aprobados y 8 fallidos; detalle en `dashboard_resultados.json` |
| Detección USB en esta sesión | No aparecieron `/dev/serial/by-id`, `ttyUSB0/1` ni `ttyACM0/1`. Esto no descarta otros puertos ni sustituye inspección local. |
| Navegador / Web Serial / sensores / LoRa / alcance / vuelo | No ejecutados |
| Formato de firmware (`test_firmware.cpp`) | Encabezados compilados con g++ C++11 y advertencias como errores; pasan generación, validación, límites y rechazo de entradas inválidas |
| Firmware → lógica del frontend (`firmware_frontend.cjs`) | Línea de 17 campos generada con el codificador C++ y RSSI/SNR de prueba aceptada por `ingest()` real del HTML |
| Compilación ESP32 / carga de nuevos sketches | Pendiente: no se encontraron arduino-cli ni PlatformIO; las pruebas nativas no sustituyen compilación Arduino |

Los archivos `SIMULADO_*` son artificiales. Sus resultados verifican la herramienta, no la electrónica, la precisión de sensores o la misión.

## Incidencias reproducibles para Geisler

Ejecutar `node pruebas/dashboard_qa.cjs`. Cada caso inicia una estación limpia. El comando devuelve 1 mientras existan fallos.

| ID | Función | Entrada o acción | Observado | Esperado |
|---|---|---|---|---|
| D01 | `ingest` | Campo `temp` vacío | Se convierte a 0 y acepta | Descartar línea |
| D02 | `ingest` | `temp=Infinity` | Acepta valor no finito | Descartar línea |
| D03 | `ingest` | `st=6` | Acepta fase inexistente | Entero 0–5 |
| D04 | `ingest` | `flags=16` | Acepta bit sin definir | Entero 0–15 para v1 |
| D05 | `ingest` | Mismo paquete dos veces | Cuenta 2 muestras | Contar duplicado aparte |
| D06 | `ingest` | Secuencias 10, 9, 11 | Inventa una pérdida | Descartar atrasado del cálculo |
| D07 | `cero` | Alturas 100, 20; fijar cero | Máximo 0 | Máximo recalibrado 80 m |
| D08 | `cero` / `csv` | Altura 20; fijar cero; exportar | Medida original pasa a 0 | Conservar altura recibida y exportar corrección/offset en columna separada |

Los cuatro casos aprobados verifican aceptación de paquete correcto, rechazo de truncado, conteo de pérdidas conocidas e intervalo de 10 s entre dos muestras de aterrizaje. El último no garantiza recepción continua durante esos 10 s.

## Decisiones que Alex debe confirmar

- El marcador general `-1` de la especificación es ambiguo en aceleración, orientación y temperatura, donde puede ser una medida real. La herramienta actual interpreta cualquier eje con `-1` como ausente al calcular `g`; evitar conclusiones sobre aceleración hasta acordar una representación inequívoca. La altura relativa también puede valer -1 m. No se cambió el contrato unilateralmente.
- La herramienta identifica reinicios cuando retroceden ambos contadores cerca del arranque. Una muestra atrasada de esos primeros instantes puede confundirse con un reinicio. Para casos ambiguos, separar capturas y registrar el reinicio físico; no confiar en una clasificación automática perfecta.
- Las pérdidas corresponden a huecos de secuencia observados. El porcentaje y los 12 s de simulación no certifican transmisión sostenida ni fiabilidad a 250 m. Registrar huecos máximos y tiempos de recepción reales en las pruebas físicas.
- Los umbrales de sensores, tolerancias y lógica de fases no están definidos. El ensayo físico debe acordarlos antes de marcar aprobación.

## Mensaje listo para Alex

> Ya preparé y corrí la validación de telemetría: 11 pruebas de la herramienta aprobadas, simulación completa y otra con pérdidas conocidas. Encontré 8 incidencias reproducibles en el dashboard para revisar con Geisler; están documentadas con sus pruebas. Tengo los componentes, pero siguen pendientes confirmar pines, modelos de sensores y firmware para medir el enlace real, el alcance y los 10 s después de aterrizar. Los resultados guardados hasta ahora son simulados. También te paso mi usuario de GitHub para que me agregues: [completar usuario].
