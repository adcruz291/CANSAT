# Pruebas de Genrikh · CanSAT 2026

Responsable: Genrikh. Fecha de preparación: 24/11/2026.
Base: `TELEMETRIA.md`, `PROMPT_AGENTE.md` y el HTML actual.

## Entrega y estado

La parte de Genrikh cubre validación de paquetes, mediciones derivadas y pruebas de recepción, sensores, alcance y transmisión posterior al aterrizaje. La interfaz la trabaja Geisler. Esta asignación se interpreta de los mensajes de Alex; no sustituye acuerdos del equipo.

Está preparada y ejecutada la verificación de software descrita en `RESULTADOS.md`. Las pruebas físicas están **pendientes**. Tener los componentes disponibles no demuestra que el circuito, el firmware o el enlace funcionen. Genrikh confirmó el cableado LoRa de la estación terrena indicado abajo. Se prepararon ambos firmwares base en `firmware/telemetria/`, compatibles con el HTML; falta compilar para ESP32, cargar y probar. Sus lecturas de sensores permanecen en `-1` hasta identificar el hardware; faltan también los criterios de fases y eventos.

## 1. Identificación antes de energizar

Cableado LoRa de la **estación terrena**, confirmado por Genrikh el 24/11/2026 (números GPIO del ESP32):

| Señal LoRa | GPIO ESP32 |
|---|---|
| SCK | 18 |
| MISO | 19 |
| MOSI | 23 |
| NSS / CS | 5 |
| RST / RESET | 14 |
| DIO0 | 26 |

Es información proporcionada por Genrikh; todavía no se ha verificado con una prueba física. Los programas provisionales de transmisor y receptor que compartió usan exactamente estos seis pines. Genrikh indica que entiende que esos son los pines actualmente usados. Los firmwares nuevos conservan esa asignación, marcada para verificar en las placas.

Genrikh proporcionó los programas provisionales de ambos roles; están guardados en `firmware/provisional/transmisor/transmisor.ino` y `firmware/provisional/receptor/receptor.ino`, sin cambios de comportamiento. Ambos usan serie a 115200 baudios y radio a 433 MHz. El transmisor configura potencia 17 y espera 1000 ms después de cada envío; no implementa la tasa de 5 Hz. Los programas no configuran explícitamente SF, ancho de banda ni los demás parámetros de radio; falta registrar versión de la librería y configuración efectiva. El receptor imprime `Mensaje recibido: CANSAT,PRUEBA,N | RSSI: ... dBm`, sin SNR. No implementan el contrato de 17 campos ni se ha validado la recepción con una captura. El sketch del transmisor declara los mismos pines, pero falta verificar su correspondencia con el circuito físico.

Completar con etiquetas, inspección del circuito y continuidad con la placa apagada. No probar pines al azar. No energizar transmisores LoRa sin antena conectada, según el prompt del proyecto.

| Dato | CanSAT | Estación terrena |
|---|---|---|
| Modelo exacto de placa ESP32 | Pendiente | Pendiente |
| Módulo LoRa y antena conectada | Pendiente | Pendiente |
| SPI SCK / MISO / MOSI | Pendiente | 18 / 19 / 23 (confirmado por Genrikh) |
| LoRa NSS-CS / RESET / DIO0 | Pendiente | 5 / 14 / 26 (confirmado por Genrikh) |
| Alimentación y tierra, verificadas por electrónica | Pendiente | Pendiente |
| Puerto USB y firmware/versión cargada | Sketch transmisor recibido; carga por confirmar | Sketch receptor recibido; firmware provisional cargado según Genrikh, serie 115200. CP2102 detectado por Linux como ttyUSB0, nodo no visible en esta sesión. |
| Barómetro: modelo, conexión, dirección | Pendiente | Si aplica |
| Acelerómetro y orientación: modelos y método | Pendiente | Si aplica |

Alex debe confirmar cómo detectar desacople, separación, autogiro y aterrizaje, y cómo obtener `sd/sb`. Mientras no haya localización, `sd=sb=-1`. No inventar lecturas ni eventos. La alimentación, masa y presupuesto se revisan con electrónica contra la guía original; esta carpeta solo contiene el resumen de Alex.

### Primera prueba con los programas provisionales

1. Mantener las antenas conectadas y encender ambos equipos con su alimentación verificada. Usar el firmware ya cargado; no es necesario reprogramar para observar la salida.
2. Abrir el monitor serie del receptor a **115200 baudios**, sin otro programa usando el puerto. En Linux el adaptador CP2102 fue registrado como `ttyUSB0`; verificar el puerto actual al conectar.
3. Observar al menos 60 s y guardar el texto original. Se esperan mensajes `Mensaje recibido: CANSAT,PRUEBA,N | RSSI: ... dBm` con contadores crecientes; el intervalo supera ligeramente 1 s porque incluye envío y espera.
4. Registrar primer y último contador, cantidad recibida y huecos. Si no hay duplicados ni reinicios, los huecos entre primero y último son `(último - primero + 1) - recibidos`. No usar esta fórmula a través de un reinicio.
5. Si solo aparece `Receptor LoRa listo`, aún no hay evidencia de recepción. Si aparece `ERROR: No se pudo iniciar LoRa`, falla la inicialización local. Guardar la salida antes de cambiar configuración.

En el transmisor, `Paquete enviado correctamente` no confirma recepción remota: estos sketches no implementan acuse de recibo. Estas líneas tampoco son compatibles con el dashboard ni con `herramientas/telemetria.py`; conservarlas como evidencia de enlace provisional. El paso siguiente será acordar los parámetros explícitos de ambas radios y adaptar la telemetría cuando se identifique el hardware del CanSAT.

## 2. Repetir las pruebas de software

Desde la raíz del proyecto, con Python 3 y Node.js; sin instalar paquetes:

```bash
python3 -m unittest discover -s pruebas -v
python3 herramientas/telemetria.py simular --salida pruebas/SIMULADO_vuelo.txt
python3 herramientas/telemetria.py verificar pruebas/SIMULADO_vuelo.txt --csv pruebas/SIMULADO_vuelo.csv --informe pruebas/SIMULADO_informe.json
python3 herramientas/telemetria.py simular --perder-cada 10 --salida pruebas/SIMULADO_perdidas.txt
python3 herramientas/telemetria.py verificar pruebas/SIMULADO_perdidas.txt --informe pruebas/SIMULADO_perdidas_informe.json
node pruebas/dashboard_qa.cjs > pruebas/dashboard_resultados.json
```

La última orden devuelve código 1 mientras haya hallazgos: es el resultado de QA, no un fallo al ejecutar Node. La prueba carga el JavaScript real del HTML en un entorno mínimo; no dibuja gráficas ni valida Web Serial, aspecto, accesibilidad o comportamiento del navegador.

La simulación de Python es determinista, recorre las seis fases y termina 12 s después del aterrizaje. Sus perfiles son datos artificiales para verificar cálculos. No son un modelo físico ni firmware. El simulador del HTML es independiente y usa azar.

## 3. Pruebas físicas por realizar

Guardar cada captura cruda de la estación en un archivo de texto, una línea por paquete, incluidos mensajes de diagnóstico. Solo una aplicación debe abrir el puerto a la vez. Usar 115200 baudios cuando el firmware esté confirmado. No confundir el CSV exportado del dashboard con una captura cruda: el validador espera líneas que comienzan con `$`.

Antes de cada prueba registrar operador, fecha/hora, circuito, firmware, sensores, alimentación, entorno, distancia medida, antenas y parámetros de ambas radios. Los parámetros de radio propuestos en el prompt son 433 MHz, SF7, ancho 250 kHz y 5 Hz; confirmar también los demás parámetros coincidentes de ambos firmwares. Estos documentos no prueban que ya estén configurados.

| ID | Procedimiento | Criterio / evidencia |
|---|---|---|
| H01 USB y contrato | Conectar el receptor, observar al menos 30 s sin simulación | Líneas con 17 campos, `$`, unidades acordadas, contadores crecientes; guardar captura y errores. Si el firmware no existe, detener esta prueba. |
| H02 Sensores en reposo | Mantener la placa quieta 60 s; comparar presión y temperatura con referencias disponibles | Registrar valores y referencias; acordar tolerancias con Alex según sensores antes de dar aprobado. No afirmar precisión solo porque hay números. |
| H03 Orientación | Inclinar cada eje de forma controlada | Confirmar eje, signo y unidades. Anotar cómo se calcula yaw y sus limitaciones; no asumir giroscopio o magnetómetro. |
| H04 Altura y velocidad | Mover a dos alturas conocidas, de forma segura y sin lanzamiento | Comparar altura relativa; ascenso positivo, descenso negativo. Registrar referencia y error. Acordar tolerancia antes de aprobar. |
| H05 Enlace de mesa | Capturar al menos 60 s con las dos antenas instaladas | Cerca de 5 paquetes/s, registrar recibidos, pérdidas, RSSI, SNR e intervalo máximo. El proyecto no fija pérdida máxima: reportarla, no inventar un umbral de aprobación. |
| H06 Interrupción y reinicio | Interrumpir la recepción sin quitar antenas y luego reiniciar el emisor | Pérdidas y sesiones explicables; no velocidades artificiales. Guardar capturas separadas antes/después si el reinicio es ambiguo. |
| H07 Alcance ≥250 m | Medir distancia real entre equipos y guardar captura en el punto ≥250 m | Registrar obstáculos, orientación y duración; sugerido 60 s por punto. Mostrar recepción real, pérdidas y señal a esa distancia. Una prueba de mesa no acredita alcance. |
| H08 Fases y flags | Con electrónica, activar los eventos con el método de banco acordado | `st` 0–5, flags persistentes; contrastar eventos físicos con tiempos de captura. El software de prueba no acciona mecanismos. |
| H09 Después de aterrizar | Con aterrizaje detectado por el método confirmado, mantener captura al menos 12 s | Evidencia de paquetes `st=5` que cubran ≥10 s; revisar también huecos y tasa. Dos paquetes separados 10 s solo prueban el intervalo, no transmisión continua. |
| H10 Autogiro y separación | Ensayo de vuelo autorizado por el equipo, con referencia de altura y eventos | Evaluar separación/autogiro cerca de 200 m y descenso 8–12 m/s con tolerancias acordadas. No ejecutar un lanzamiento solo para validar software. |
| H11 Localización secundaria | Cuando esté definido el método, comparar con distancia y rumbo medidos | Evaluar `sd/sb` y bit 3; por ahora marcar pendiente, no sustituir con valores simulados. |

Analizar una captura real sin modificar el original:

```bash
python3 herramientas/telemetria.py verificar pruebas/REAL_H05.txt --csv pruebas/REAL_H05.csv --informe pruebas/REAL_H05_informe.json
```

El código de salida del validador solo indica validez de formato / existencia de muestras; no certifica la misión. `validos` incluye duplicados y paquetes atrasados; `recibidos` cuenta los aceptados por sesión. No estima pérdidas antes del primer paquete ni después del último.

## 4. Comprobación manual del dashboard con Geisler

Abrir `estacion_terrena.html`, pulsar **Simular vuelo** y observar fases, gráficas y aterrizaje. El vuelo tarda aproximadamente dos minutos en llegar a tierra; esperar otros 12 s. Descargar CSV y revisar encabezados y número de muestras. La simulación no comprueba el puerto USB.

Después repetir sin simulación con una captura real en el navegador que soporte Web Serial. Comprobar pérdida de señal, reconexión, reinicio de pantalla, exportación, uso sin internet, pantalla estrecha, teclado, tema claro/oscuro y movimiento reducido. Registrar navegador y versión. Evitar iniciar el simulador durante una captura real: falta una prueba de aislamiento de ambas fuentes.

## 5. Registro para entregar a Alex

Copiar esta tabla para cada prueba; conservar captura, informe y CSV con el mismo identificador.

| Campo | Registro |
|---|---|
| ID / operador / fecha-hora | Pendiente |
| Firmware y circuito usados | Pendiente |
| Sensores y configuración de radio | Pendiente |
| Distancia / entorno / referencia | Pendiente |
| Duración / recibidos / pérdidas / intervalo máximo | Pendiente |
| Resultado y criterio aplicado | Pendiente |
| Captura / CSV / informe / observaciones | Pendiente |

La entrega incluye resultados de software, incidencias para Geisler y registros físicos cuando se realicen. Enviar a Alex tu usuario de GitHub para que pueda agregarte al repositorio. El RAM requiere datos reales; no utilizar archivos `SIMULADO_*` como evidencia de vuelo.
