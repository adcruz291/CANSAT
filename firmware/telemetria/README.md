# Firmware de telemetría v2 compatible con el HTML actual

Estos sketches sustituyen el mensaje provisional `CANSAT,PRUEBA,N` por el contrato de `TELEMETRIA.md`. Mantienen los nombres, orden y unidades del arreglo `F` de `estacion_terrena.html`, que no se modificó.

## Qué cargar en cada equipo

| Equipo | Abrir en Arduino IDE | Archivos que deben permanecer juntos |
|---|---|---|
| ESP32 del CanSAT, transmisor | `transmisor/transmisor.ino` | `transmisor.ino`, `paquete.h`, `ciclo.h` y `sensores.h` |
| ESP32 de estación terrena, receptor | `receptor/receptor.ino` | `receptor.ino`, `contrato.h` y `ciclo.h` |

Usar soporte de placas ESP32 y la biblioteca **LoRa de Sandeep Mistry**, como en los programas provisionales. Para el ESP32-WROOM-32 indicado por Genrikh, se utiliza **ESP32 Dev Module** (`esp32:esp32:esp32`). Los originales están conservados en `firmware/provisional/`.

En Arduino IDE se puede abrir directamente el `.ino` de cada carpeta. Si se prefiere copiar y pegar, crear un sketch con ese nombre, pegar el `.ino` en la pestaña principal y agregar una pestaña con el nombre exacto del `.h` correspondiente. Los dos archivos se compilan y cargan juntos. No mezclar el receptor nuevo con una copia antigua de `contrato.h`.

Las funciones de configuración y recepción se verificaron contra la [API oficial de la biblioteca](https://github.com/sandeepmistry/arduino-LoRa/blob/master/API.md). Registrar la versión que se use al compilar para poder reproducir las pruebas.

## Pines y configuración

Los pines declarados en **ambos programas proporcionados por Genrikh** coinciden con los comunicados previamente:

| Señal | GPIO ESP32 |
|---|---|
| SCK | 18 |
| MISO | 19 |
| MOSI | 23 |
| NSS / CS | 5 |
| RESET | 14 |
| DIO0 | 26 |

Se comprobó la coincidencia textual de los programas; no se midió continuidad física. Los pines están agrupados al inicio y marcados `VERIFICAR`.

Ambos sketches configuran 433 MHz, SF7, ancho de banda 250 kHz, coding rate 4/5, preámbulo de 8 símbolos, sync word `0x12`, CRC activado, IQ sin invertir y cabecera explícita. El transmisor conserva los 17 dBm del programa provisional y programa un envío cada 200 ms. Si una operación tarda más, no genera una ráfaga de recuperación; medir la tasa real en la captura.

**Cargar los dos nuevos sketches** para probar este protocolo: no se garantiza comunicación con el firmware provisional, que no configura esos parámetros explícitamente y envía otro formato. Mantener antena conectada al LoRa antes de energizar.

## Qué envía cada uno

El transmisor emite por radio un paquete sin salto de línea:

```text
$seq,ms,st,pres,temp,alt,ax,ay,az,roll,pitch,yaw,flags,sd,sb
```

El receptor solo acepta paquetes con esos 15 campos y números finitos. Valida enteros, `st` entre 0 y 3, `flags` entre 0 y 3 y los reservados `sd=sb=-1`; descarta estados de v1, paquetes provisionales, truncados, vacíos o inválidos. Agrega métricas del paquete recibido y escribe una línea por USB a **115200 baudios**:

```text
$seq,ms,st,pres,temp,alt,ax,ay,az,roll,pitch,yaw,flags,sd,sb,rssi,snr
```

No agrega textos como `Mensaje recibido:` a las líneas de datos. Los diagnósticos comienzan con `#`, por lo que el HTML los ignora como mediciones. El contador avanza por cada intento de emisión: un hueco observado puede incluir un fallo local de envío, además de una pérdida del enlace. No hay acuse de recibo.

Cada trama se limita a 255 bytes y cada campo a 31 caracteres. Un paquete que no quepa se descarta completo. Las lecturas NaN/infinito se convierten en `-1` antes de codificar; los metadatos inválidos no se corrigen silenciosamente. La prueba `pruebas/test_receptor.cpp` ejecuta el sketch real con dobles de Arduino/LoRa/Serial: comprueba rechazo de tramas corruptas, recepción válida posterior y las líneas USB exactas con métricas distintas por paquete. Los dobles están fuera de las carpetas de sketch y no se cargan al ESP32.

## Recuperación y temporización

El transmisor inicia TX de forma asíncrona y sigue atendiendo el ciclo. Consulta disponibilidad mediante `beginPacket()`, API pública de LoRa 0.8.0; no usa su método privado `isTransmitting()`. Una consulta que devuelve 1 prepara/vacía la FIFO sin transmitir: la próxima muestra la prepara de nuevo antes de escribir. Consultar no consume secuencia. Si la radio continúa transmitiendo a los 1000 ms desde el inicio TX, aborta y pasa a recuperación. En cada turno comprueba primero la finalización: si ya terminó al observar el límite, no marca timeout. Una finalización local no confirma recepción remota.

Los fallos de preparación de paquete, escritura parcial o inicio TX también abortan el intento sin transmitir un parcial. Se emite un diagnóstico `# ERROR` por fallo y se reintenta inicializar a los 2000 ms. Ambos equipos reintentan una inicialización fallida cada 2000 ms; al recuperarse reaplican todos los parámetros. No reinician `seq/ms` ni guardan el paquete fallido: el siguiente intento elegible genera una muestra nueva. Un turno esperando radio no consume secuencia. Se conservan los intentos de 200 ms y no se recuperan intervalos atrasados en ráfaga.

Estas garantías acotan la espera controlada por la aplicación, no pueden interrumpir un bloqueo interno de SPI ni verificar por sí solas el estado físico del módulo. En el receptor, silencio de paquetes no distingue radio averiada de un transmisor apagado: los reintentos cubren fallos detectados al inicializar, sin inventar un diagnóstico de avería por falta de señal.

## Pendiente para medir la misión real

El transmisor ahora integra **BME280 y MPU6050** con el cableado confirmado por Genrikh: SDA 21, SCL 22, alimentación 3V3 y GND común. `leerMedidas()` devuelve presión, temperatura y altitud estándar del BME280, aceleración del MPU6050 y estimaciones de inclinación cerca del reposo. Si un sensor falla, sus campos quedan en `-1` y se reintenta cada 2000 ms, sin inventar lecturas ni reutilizar muestras anteriores. Yaw permanece en `-1`: no hay referencia de rumbo absoluto.

Ver **`SENSORES.md`** para direcciones I²C identificadas automáticamente, unidades, rangos y limitaciones. Los ejes son los del módulo; roll/pitch estimados por gravedad no son actitud dinámica durante caída/autogiro. No se necesitan bibliotecas adicionales: el nuevo `sensores.h` usa `Wire` del core ESP32. Conservar los cuatro archivos de la carpeta del transmisor.

El frontend muestra `-1` como sin dato. La radio y la lectura efectiva del circuito todavía no se han probado físicamente. La calibración/precisión y el montaje de ejes requieren pruebas de mesa, aunque el código y los cálculos se verifiquen en computadora.

**Misión v2 (ver `TELEMETRIA.md`):** solo transmite el módulo del huevo con autogiro; se enciende en el suelo, se sube a 20–25 m (límite 30 m) y se suelta. La estación calcula las cuatro fases y el cero con las cinco primeras alturas válidas. El transmisor deja `st=0` y `flags=0`; `alt` debe ser la altura barométrica estándar referida a 1013.25 hPa, sin ponerla en cero a bordo. `paquete.h` y `contrato.h` ya validan los rangos v2. Se mantienen 5 Hz; los 10 Hz son una propuesta de la especificación y requieren medir el tiempo de transmisión y la tasa efectiva antes de cambiarlos.

## Primera conexión al frontend

1. Guardar los programas provisionales antes de cargar estos sketches. Compilar cada carpeta para la placa correspondiente y cargar el transmisor en el CanSAT y el receptor en la estación.
2. Con ambos equipos encendidos y antenas conectadas, abrir el monitor serie del **receptor** a 115200 y confirmar líneas `$...` de 17 campos. Los diagnósticos `# TX` del transmisor son solo locales y no son las líneas que necesita el dashboard.
3. Cerrar el monitor serie para liberar el puerto. Abrir `estacion_terrena.html` en el navegador con Web Serial, pulsar **Conectar USB** y elegir el receptor. No iniciar simultáneamente **Simular vuelo**.
4. Comprobar paquetes recibidos y RSSI/SNR; descargar CSV. Los campos de sensores deben llegar con medidas si el hardware responde; los ausentes o fallidos se muestran como sin dato.

## Verificación realizada en computadora

```bash
g++ -std=c++11 -Wall -Wextra -Werror -pedantic pruebas/test_firmware.cpp -o /tmp/cansat_test_firmware
/tmp/cansat_test_firmware
/tmp/cansat_test_firmware --emit | node pruebas/firmware_frontend.cjs
/tmp/cansat_test_firmware --emit-flight | node pruebas/firmware_frontend.cjs
node pruebas/dashboard_qa.cjs
python3 -m unittest discover -s pruebas -v
```

Pasaron las pruebas nativas de generación/validación de paquetes, la integración de una línea sin sensores y un vuelo artificial de 186 paquetes con el `ingest()` real del HTML v2. El vuelo comprueba cero automático a 432 m barométricos, máximo relativo de 25 m, cuatro fases y ≥10 s posteriores al aterrizaje, con `st=flags=0` en todos los paquetes. También pasaron los 20 casos de QA del frontend y los 11 casos históricos Python. RSSI/SNR y el vuelo de integración son datos de prueba, no lecturas físicas. No se ejecutó el dibujo del navegador ni Web Serial.

`herramientas/telemetria.py` y los archivos `SIMULADO_*` existentes pertenecen a v1: no usar sus fases, máximo absoluto ni postaterrizaje para certificar la misión v2. Para los derivados v2 se utiliza el CSV del frontend.

**Compilación ESP32 realizada (transmisor con sensores actualizado):** se encontró el Arduino CLI incluido en el IDE instalado y se compilaron los dos sketches completos, sin cargarlos a las placas. Configuración: Arduino CLI 1.6.0, core `esp32:esp32` 3.3.12, placa `esp32:esp32:esp32` (ESP32 Dev Module) y LoRa 0.8.0. La biblioteca se descargó en `/tmp` para la verificación; esto no la instala en el administrador de bibliotecas del IDE.

| Sketch | Flash utilizada | RAM estática utilizada | Resultado |
|---|---|---|---|
| Transmisor | 313752 bytes | 23756 bytes | Compilación correcta |
| Receptor | 286832 bytes | 22284 bytes | Compilación correcta |

Con `arduino-cli` disponible y la biblioteca LoRa instalada, se puede repetir:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/telemetria/transmisor
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/telemetria/receptor
```

Si la biblioteca está en otra carpeta, agregar `--library /ruta/a/arduino-LoRa`. Para esta verificación se usaron carpetas de compilación separadas en `/tmp`. La recepción en radio, la lectura efectiva de sensores y el funcionamiento físico siguen pendientes; compilar correctamente no acredita esas pruebas.

## Entrega reproducible

La guía para cargar/copiar archivos, diagnosticar USB y devolver evidencias está en `GUIA_COMPANERO.md`. Cada carpeta de sketch requiere su propio `ciclo.h`, además de `paquete.h` o `contrato.h`.

El verificador `herramientas/preparar_entrega.py` ejecuta las pruebas nativas de contrato, los sketches reales con dobles de radio/Serial/reloj e I²C y las pruebas BME280/MPU6050, el recorrido receptor→HTML, el vuelo artificial, los 20 casos de QA, los controles de integridad/dependencia ausente y la compilación completa de ambos ESP32. Los controles de radio cubren finalización asíncrona normal, timeout, límites temporales, preparación fallida, escritura parcial, reintentos, configuración reaplicada, secuencia conservada y wrap del reloj. El doble no expone la función privada `isTransmitting()`.

```bash
python3 herramientas/preparar_entrega.py verificar --arduino-cli /ruta/a/arduino-cli --lora /ruta/a/arduino-LoRa --salida /tmp/cansat-verificacion
python3 herramientas/preparar_entrega.py empaquetar --arduino-cli /ruta/a/arduino-cli --lora /ruta/a/arduino-LoRa --salida /tmp/cansat-entrega
```

El segundo comando genera un ZIP nuevo únicamente después de verificar fuentes, inventario, huellas y compilaciones/pruebas desde otra extracción. Incluye el verificador y las fuentes de pruebas; los registros se guardan fuera de los archivos manifestados. No sobrescribe un ZIP final previo. `integridad` permite revisar las huellas desde la raíz de una entrega extraída, sin necesidad de Arduino CLI. Ninguna acción instala herramientas o carga placas.
