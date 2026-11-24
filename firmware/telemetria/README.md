# Firmware de telemetría compatible con el HTML actual

Estos sketches sustituyen el mensaje provisional `CANSAT,PRUEBA,N` por el contrato de `TELEMETRIA.md`. Mantienen los nombres, orden y unidades del arreglo `F` de `estacion_terrena.html`, que no se modificó.

## Qué cargar en cada equipo

| Equipo | Abrir en Arduino IDE | Archivos que deben permanecer juntos |
|---|---|---|
| ESP32 del CanSAT, transmisor | `transmisor/transmisor.ino` | `transmisor.ino` y `paquete.h` |
| ESP32 de estación terrena, receptor | `receptor/receptor.ino` | `receptor.ino` y `contrato.h` |

Usar soporte de placas ESP32 y la biblioteca **LoRa de Sandeep Mistry**, como en los programas provisionales. Seleccionar el modelo exacto de placa que utiliza el equipo; no se ha confirmado en esta conversación. Los originales están conservados en `firmware/provisional/`.

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

El receptor solo acepta paquetes con esos 15 campos y números finitos. Valida enteros, rangos de `st/flags`, distancia y rumbo; descarta paquetes provisionales, truncados, vacíos o inválidos. Agrega métricas del paquete recibido y escribe una línea por USB a **115200 baudios**:

```text
$seq,ms,st,pres,temp,alt,ax,ay,az,roll,pitch,yaw,flags,sd,sb,rssi,snr
```

No agrega textos como `Mensaje recibido:` a las líneas de datos. Los diagnósticos comienzan con `#`, por lo que el HTML los ignora como mediciones. El contador avanza por cada intento de emisión: un hueco observado puede incluir un fallo local de envío, además de una pérdida del enlace. No hay acuse de recibo.

## Pendiente para medir la misión real

El transmisor tiene un punto de integración llamado **`leerMedidas()`**. Actualmente devuelve lecturas `-1`, fase `st=0` y `flags=0`, de acuerdo con la opción de datos ausentes del contrato. `seq` y `ms` sí se generan en la placa; RSSI/SNR se obtienen en el receptor. No hay lecturas de sensores simuladas.

Faltan modelos, conexión y calibración de sensores para integrar presión, temperatura, altura, aceleración y orientación. También faltan criterios de fases y eventos; no se implementó detección automática ni accionamiento de mecanismos. La distancia y rumbo de la carga secundaria permanecen en `-1`. El transmisor continúa enviando indefinidamente mientras esté alimentado; eso no prueba detección de aterrizaje ni el requisito de 10 s posterior a este.

**El HTML actual no trata todos los `-1` como datos ausentes.** Mostrará esas cifras e incluso calculará `g≈1.73` con tres ejes en `-1`. Con esta base solo validar enlace, secuencias, RSSI/SNR y formato. No interpretar las gráficas de sensores como medidas reales. El problema del marcador ambiguo está explicado en `pruebas/RESULTADOS.md`; acordar cualquier cambio de contrato con el equipo.

## Primera conexión al frontend

1. Guardar los programas provisionales antes de cargar estos sketches. Compilar cada carpeta para la placa correspondiente y cargar el transmisor en el CanSAT y el receptor en la estación.
2. Con ambos equipos encendidos y antenas conectadas, abrir el monitor serie del **receptor** a 115200 y confirmar líneas `$...` de 17 campos. Los diagnósticos `# TX` del transmisor son solo locales y no son las líneas que necesita el dashboard.
3. Cerrar el monitor serie para liberar el puerto. Abrir `estacion_terrena.html` en el navegador con Web Serial, pulsar **Conectar USB** y elegir el receptor. No iniciar simultáneamente **Simular vuelo**.
4. Comprobar paquetes recibidos y RSSI/SNR; descargar CSV. Los campos de sensores permanecen sin medición hasta integrar `leerMedidas()`.

## Verificación realizada en computadora

```bash
g++ -std=c++11 -Wall -Wextra -Werror -pedantic pruebas/test_firmware.cpp -o /tmp/cansat_test_firmware
/tmp/cansat_test_firmware
/tmp/cansat_test_firmware --emit | node pruebas/firmware_frontend.cjs
python3 -m unittest discover -s pruebas -v
```

Pasaron las pruebas nativas de generación/validación de paquetes, la aceptación de una línea de 17 campos por el `ingest()` real del HTML y los 11 casos Python existentes. En el caso de integración, RSSI/SNR son valores de prueba añadidos en computadora, no lecturas físicas. No se ejecutó el dibujo del navegador ni Web Serial.

No hay `arduino-cli` ni PlatformIO disponibles en esta sesión: los sketches completos aún no se compilaron con el core ESP32 ni se cargaron. Las pruebas C++ verifican los encabezados de formato y validación, no los controladores Arduino/LoRa. La recepción en radio y el funcionamiento físico siguen pendientes.
