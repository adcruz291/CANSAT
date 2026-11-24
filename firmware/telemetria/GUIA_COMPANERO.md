# Entrega a compañero: ESP32 CanSAT y estación terrena

Esta versión es una **base de comunicación v2**, con verificaciones de software. La carga USB, sensores, recepción LoRa, alcance y vuelo todavía requieren pruebas físicas. Genrikh encontró `Failed to connect to ESP32: No serial data received` al intentar cargar; no se ha demostrado que ese problema esté resuelto.

## Archivos que cargar

| Placa | Abrir | Conservar en la misma carpeta |
|---|---|---|
| CanSAT: módulo del huevo con autogiro | `firmware/telemetria/transmisor/transmisor.ino` | `paquete.h` y `ciclo.h` del transmisor |
| Estación terrena: recibe y conecta a computadora | `firmware/telemetria/receptor/receptor.ino` | `contrato.h` y `ciclo.h` del receptor |

Abrir el `.ino` directamente en Arduino IDE. Si se copia/pega, crear una pestaña para **cada `.h` de esa carpeta**, con el nombre exacto. Los dos archivos `ciclo.h` tienen funciones distintas; no intercambiarlos. Compilar cada sketch de forma independiente. No usar el ZIP anterior que contenía solo el transmisor.

Seleccionar **ESP32 Dev Module** para el ESP32-WROOM-32 indicado por el equipo. Versiones de referencia: core **esp32 de Espressif Systems 3.3.12**, biblioteca **LoRa de Sandeep Mistry 0.8.0**, Arduino CLI 1.6.0. Instalar biblioteca/core desde los administradores del IDE si no están disponibles. No se requieren bibliotecas de sensores en esta base.

## Conexión y radio

Los dos sketches conservan estos GPIO de los programas proporcionados por Genrikh; comprobar continuidad física si el circuito no responde:

| Señal LoRa | GPIO ESP32 |
|---|---|
| SCK | 18 |
| MISO | 19 |
| MOSI | 23 |
| NSS/CS | 5 |
| RESET | 14 |
| DIO0 | 26 |

Parámetros en ambos: **433 MHz, SF7, BW 250 kHz, CR 4/5, sync word 0x12, preámbulo 8, CRC activado, IQ sin invertir, cabecera explícita**. TX usa 17 dBm e intervalos de 200 ms cuando la radio está disponible. Mantener la antena conectada antes de energizar el LoRa. La alimentación debe ser la verificada por electrónica; no cambiar reguladores ni conexiones de alimentación por estas instrucciones.

## Cargar y observar

1. Conectar un ESP32 por USB con cable de datos, seleccionar su puerto y abrir el sketch que corresponde a su función. El puerto detectado anteriormente fue `ttyUSB0`; puede cambiar al reconectar. Si hay dos placas, identificar cada puerto al conectarlas una por una.
2. Cerrar monitor/plotter serie y desconectar el dashboard de ese puerto. Compilar con **Verificar** y después pulsar **Subir**. No hace falta cambiar a otro protocolo de radio para cargar.
3. Si queda en `Connecting...`, mantener **BOOT** presionado y pulsar/soltar **EN**. Mantener BOOT hasta que empiece `Writing...` o los porcentajes de escritura; el resumen de memoria todavía no indica que haya comenzado la carga.
4. Si falla de nuevo, registrar el log completo desde el puerto, placa seleccionada y cable usado; probar otro cable de datos/puerto USB. Con el monitor a 115200 y BOOT sin presionar, pulsar EN y guardar el mensaje de arranque o indicar que no aparece nada. Consultar las [indicaciones de Espressif](https://docs.espressif.com/projects/esptool/en/latest/esp32/troubleshooting.html). No confundir detección del adaptador CP2102 con comunicación confirmada con el chip.
5. Una carga correcta debe terminar sin error. Reiniciar con EN si hace falta. Un error `# ERROR: no se pudo iniciar LoRa` posterior es del módulo radio/cableado/alimentación; es una etapa diferente de `No serial data received` durante la carga.
6. Repetir con el otro equipo. Encender ambos, observar el **receptor** a **115200 baudios** y guardar las líneas crudas. Se esperan líneas `$` de 17 campos; los diagnósticos comienzan con `#`. El aviso TX local no confirma recepción.
7. Cerrar monitor serie, abrir `estacion_terrena.html` en Chrome/Edge con Web Serial, pulsar **Conectar receptor USB** y seleccionar el receptor. No iniciar simulación durante una captura real. Descargar CSV antes de borrar datos.

## Qué se puede medir ahora

`seq/ms` proceden del transmisor y `rssi/snr` del receptor. Los modelos de sensores no están confirmados ni sus controladores integrados: presión, temperatura, altura, aceleración y orientación se envían como **`-1` (sin dato)**. `st=flags=0`, `sd=sb=-1`. La interfaz no puede inferir un vuelo sin altura válida; una gráfica vacía con esos campos no demuestra fallo de radio.

El frontend v2 calcula fases y cero de altura. Al integrar un barómetro, enviar altura estándar referida a 1013.25 hPa, sin restar el suelo a bordo; encender en el suelo para el cero automático. La misión prevista es de 20–25 m, límite 30 m. Estas instrucciones no autorizan una prueba de lanzamiento ni afirman precisión de sensores.

El transmisor usa TX asíncrona con timeout de 1000 ms y recuperación a 2000 ms. No llama a `isTransmitting()`, que es privado en LoRa 0.8.0: consulta la disponibilidad mediante `beginPacket()` sin iniciar otro envío ni consumir secuencia. No retransmite muestras fallidas ni genera ráfagas para recuperar retrasos. El receptor reintenta inicialización cada 2000 ms; el silencio de radio no permite distinguir emisor apagado de una avería física.

## Repetir la verificación en computadora

El ZIP incluye fuentes de pruebas y verificador. Requiere Python ≥3.9, g++ con C++11, Node.js, Arduino CLI, core ESP32 3.3.12 y carpeta de LoRa 0.8.0. No descarga dependencias, no usa red para instalar y no carga placas. Desde la raíz extraída:

```bash
python3 herramientas/preparar_entrega.py integridad
python3 herramientas/preparar_entrega.py verificar --arduino-cli /ruta/a/arduino-cli --lora /ruta/a/arduino-LoRa --salida /tmp/cansat-verificacion-companero
```

Las rutas de herramientas se proporcionan para cada computadora; los logs/builds se guardan **fuera de las fuentes extraídas**, para conservar las huellas del manifiesto. Si falta una dependencia o falla un control, el proceso devuelve código no cero y `verificado_en_computadora=false` en `informe.json`; no se debe declarar aprobado.

Para reconstruir un paquete completo desde estas fuentes o el repositorio:

```bash
python3 herramientas/preparar_entrega.py empaquetar --arduino-cli /ruta/a/arduino-cli --lora /ruta/a/arduino-LoRa --salida /tmp/cansat-nueva-entrega
```

Usar una carpeta nueva si ya hay un ZIP. El proceso verifica las fuentes, arma una lista explícita, incluye `INFORME_VERIFICACION.json`, genera huellas en `MANIFIESTO.json`, extrae el ZIP y repite pruebas/compilaciones desde esa extracción antes de publicar el archivo final. Los registros de esa segunda ejecución quedan fuera del ZIP, en `verificacion_extraida/`; no modifican el manifiesto. El propio manifiesto no se incluye en su lista de hashes, para evitar una referencia a sí mismo.

Los datos de vuelo y radio usados por las pruebas son artificiales; los dobles ejecutan la lógica del sketch en computadora, pero no validan SPI, señales, RF ni dibujo/Web Serial del navegador. Los archivos históricos `SIMULADO_*` v1 no forman parte de esta entrega.

## Registro que debe devolver el compañero

- Modelo de placa, versiones de core/biblioteca, sketch cargado y resultado/log de carga de cada equipo.
- Mensaje de arranque a 115200, diagnóstico de radio y captura USB cruda del receptor.
- Estado de antenas, alimentación y circuito probado; distancia y duración medidas si se prueba enlace.
- Identificación exacta de sensores y conexiones antes de integrar lecturas.

Compilación correcta no certifica carga, sensores, recepción, alcance ≥250 m, vuelo ni transmisión real durante 10 s después de aterrizar. Mantenerlos pendientes hasta obtener registros físicos.
