# Resultados de pruebas de Genrikh

Fecha: 24/11/2026. Estado: software probado; hardware pendiente.
Archivos base: telemetría y dashboard del commit `2e1fd9e`. Herramienta Python preexistente en la carpeta local. No se modificó el frontend ni se cargó firmware. Después se prepararon ambos firmwares base en `firmware/telemetria/`, conservando los sketches provisionales proporcionados por Genrikh.

## Validación actual: firmware para el frontend v2

Base actualizada a `main` remoto, commit `91ac812` (misión v2). Se ajustaron los firmwares para validar `st` 0–3, `flags` 0–3 y `sd=sb=-1`, manteniendo el formato de 17 campos y los pines previamente confirmados. El frontend no fue editado en esta actualización.

| Prueba | Resultado actual |
|---|---|
| Compilación completa de transmisor y receptor | Correcta para ESP32 Dev Module, core ESP32 3.3.12, Arduino CLI 1.6.0 y LoRa 0.8.0; sin cargar a las placas |
| Contrato C++ v2 | Generación y validación correctas; rechaza estados, bits y campos reservados de v1 |
| Firmware → frontend, sin sensores | Acepta paquete; altura, velocidad y aceleración derivadas permanecen sin dato |
| Firmware → frontend, vuelo artificial | 186 paquetes; cero barométrico 432 m, máximo relativo 25 m, cuatro fases y ≥10 s posteriores al aterrizaje, con `st=flags=0` enviados |
| QA del frontend v2 | 20 de 20 casos aprobados |
| Regresión de herramienta histórica v1 | 11 de 11 casos aprobados; no certifica los derivados de v2 |

La compilación utilizó la biblioteca LoRa 0.8.0 descargada en `/tmp`, sin instalarla en la configuración del IDE. Ver comandos y memoria utilizada en `firmware/telemetria/README.md`. No se midió el enlace físico ni se integraron sensores: las lecturas del transmisor siguen en `-1`. Los datos del vuelo de integración son artificiales.

## Evidencia inicial v1 (histórica)

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

**Actualización 24/11/2026:** D01–D08 corregidas en `estacion_terrena.html`. `dashboard_qa.cjs` se actualizó a la misión v2 (fases calculadas con la altura, cero automático en el suelo) y ahora tiene 20 casos, todos aprobados (`dashboard_resultados.json`). Los archivos `SIMULADO_*` y el simulador de `herramientas/telemetria.py` siguen siendo del vuelo v1 (450 m) y no se regeneraron.

Los cuatro casos aprobados verifican aceptación de paquete correcto, rechazo de truncado, conteo de pérdidas conocidas e intervalo de 10 s entre dos muestras de aterrizaje. El último no garantiza recepción continua durante esos 10 s.

## Decisiones que Alex debe confirmar

- El marcador general `-1` de la especificación es ambiguo en aceleración, orientación y temperatura, donde puede ser una medida real. La herramienta actual interpreta cualquier eje con `-1` como ausente al calcular `g`; evitar conclusiones sobre aceleración hasta acordar una representación inequívoca. La altura relativa también puede valer -1 m. No se cambió el contrato unilateralmente.
- La herramienta identifica reinicios cuando retroceden ambos contadores cerca del arranque. Una muestra atrasada de esos primeros instantes puede confundirse con un reinicio. Para casos ambiguos, separar capturas y registrar el reinicio físico; no confiar en una clasificación automática perfecta.
- Las pérdidas corresponden a huecos de secuencia observados. El porcentaje y los 12 s de simulación no certifican transmisión sostenida ni fiabilidad a 250 m. Registrar huecos máximos y tiempos de recepción reales en las pruebas físicas.
- Los umbrales de sensores, tolerancias y lógica de fases no están definidos. El ensayo físico debe acordarlos antes de marcar aprobación.

## Mensaje listo para Alex

> Ya preparé y corrí la validación de telemetría: 11 pruebas de la herramienta aprobadas, simulación completa y otra con pérdidas conocidas. Encontré 8 incidencias reproducibles en el dashboard para revisar con Geisler; están documentadas con sus pruebas. Tengo los componentes, pero siguen pendientes confirmar pines, modelos de sensores y firmware para medir el enlace real, el alcance y los 10 s después de aterrizar. Los resultados guardados hasta ahora son simulados. También te paso mi usuario de GitHub para que me agregues: [completar usuario].

## Corrección de compatibilidad LoRa y compilación actual

La llamada a `isTransmitting()` del primer intento de recuperación era incompatible con LoRa 0.8.0 porque ese método es privado. Se sustituyó por consulta de disponibilidad con `beginPacket()`, API pública, sin interrupciones nuevas ni cambios al contrato. Se ajustó el doble a esa API para impedir que las pruebas oculten el problema.

Ambos sketches completos volvieron a compilar con ESP32 Dev Module, core 3.3.12 y LoRa 0.8.0: transmisor 280428 bytes de flash y 22300 de RAM; receptor 286832 de flash y 22284 de RAM. Pasaron pruebas nativas de contrato, saneamiento, temporización y fallos ejecutando los sketches con dobles, y las integraciones del vuelo artificial y salida capturada del receptor con el HTML. El empaquetado reproducible guarda informes actuales de todos sus comandos en sus directorios de salida. No se cargó ninguna placa ni se validó recepción física.

## Integración BME280 y MPU6050 (cableado confirmado por Genrikh)

El transmisor usa I²C SDA21/SCL22 y comprueba chip ID/direcciones alternativas. Añade lecturas reales de presión, temperatura, altitud estándar y aceleración; roll/pitch estiman inclinación cerca del reposo, yaw permanece ausente. Se comprueban transacciones completas y se reintenta cada sensor de forma independiente. El receptor conserva su formato y no requiere cambios de pines. Ver `firmware/telemetria/SENSORES.md`.

Pruebas con I²C simulado: compensación numérica (25.08248 °C / 1006.53267 hPa), unidades, signos/inclinación, direcciones alternativas, identificación incorrecta, calibración inválida, saturación, caída libre, giro fuerte, lecturas cortas y recuperación. Pasó además el recorrido I²C de prueba → transmisor real → receptor real con dobles → lógica HTML. Los datos de esas pruebas son artificiales; las fuentes de placa no contienen fixtures.

El transmisor completo compiló para ESP32 Dev Module/core 3.3.12/LoRa 0.8.0 con 313752 bytes de flash y 23756 de RAM. No necesita una biblioteca de sensores externa, solo Wire del core. El paquete nuevo de sensores se genera en `entregas/telemetria-v2-sensores/`; su verificador repite controles y compilación desde la extracción. Los ZIP anteriores se conservan como entregas históricas. La lectura efectiva/precisión en el circuito, la carga y el enlace físico siguen pendientes.
