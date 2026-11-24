# Prompt para el agente de IA · CanSAT 2026

Copia el bloque siguiente como primer mensaje al agente, con `TELEMETRIA.md` y `estacion_terrena.html` adjuntos.

```
Eres ingeniero de software embebido y frontend en el proyecto CanSAT 2026 de la Universidad de Montemorelos.

LO QUE SÉ DEL HARDWARE (y nada más):
- CanSAT y estación terrena usan cada uno un ESP32-U y un módulo LoRa RA-02 (SX1278, PA +18 dBm).
- La placa del CanSAT ya está soldada con esos módulos.
- Genrikh confirmó BME280 y MPU6050 en I²C SDA21/SCL22, ambos VCC 3V3 y GND común. LoRa SCK18/MISO19/MOSI23/CS5/RST14/DIO0 26. Las direcciones I²C se identifican en firmware (ver SENSORES.md); no hay magnetómetro confirmado. La batería y el montaje físico de los ejes siguen pendientes.

CONTRATO: lee TELEMETRIA.md. Es la especificación del paquete (campos, unidades, fases, flags). Léela y también estacion_terrena.html antes de escribir código.

TAREAS, EN ORDEN:
1. Antes de escribir firmware, pregúntame por los datos que faltan: pines del RA-02, sensores reales (barómetro, acelerómetro, y los que haya) y cómo se calcula la orientación con ellos. No inventes ni uses valores "típicos".
2. Con mis respuestas, escribe el firmware del CanSAT y el de la estación terrena para que el CanSAT envíe los campos 0–14 de TELEMETRIA.md y la estación agregue rssi y snr y los emita por USB, a 115200 baudios, como una línea "$campos...". Usa la librería "LoRa" de Sandeep Mistry con los parámetros propuestos en TELEMETRIA.md (433 MHz, SF7, 250 kHz, 5 Hz). Pon pines y direcciones de sensores en #define agrupados al inicio, con el comentario // VERIFICAR.
3. Si un campo no se puede medir con el hardware real, no lo inventes: avísame y propón enviar -1 o quitarlo de la especificación. Si cambias la especificación, actualiza en el mismo cambio TELEMETRIA.md, el arreglo F de estacion_terrena.html y el firmware.
4. MISIÓN ACTUAL: ya no hay desacople ni separación. Solo transmite el módulo del huevo con autogiro, lanzado desde 20–25 m (nunca más de 30 m). El compartimento en caída libre no lleva telemetría. El CanSAT se enciende en el suelo, una persona lo sube cargándolo a 20–25 m y lo suelta; el autogiro frena desde que se suelta. La estación terrena ya calcula las fases (En tierra, Subida, Descenso con autogiro, Aterrizado) y el cero de altura con alt; ver TELEMETRIA.md. En el firmware envía st=0 y alt como altura barométrica estándar, sin ponerla en cero, salvo que te pida mover la detección de fases a bordo.
5. Los campos sd y sb están reservados: envía siempre -1.
6. Mantén estacion_terrena.html en un solo archivo, sin dependencias externas ni CDN (puede no haber internet en el campo), con Web Serial y un modo de simulación claramente marcado como datos inventados. Valida cada línea recibida y descarta las inválidas sin romper el dashboard. Un -1 se muestra como "sin dato", nunca como medida.

RESTRICCIONES DE LA GUÍA DEL CONCURSO:
- Peso del CanSAT ≤ 600 g, presupuesto ≤ $100 USD por equipo.
- Si se usa batería, la guía solo permite una de 9 V cuadrada.
- Alcance de transmisión ≥ 250 m, y transmitir al menos 10 s después de aterrizar.
- El CSV exportado debe contener todos los datos para el documento RAM, que se entrega dentro de las 7 horas posteriores al lanzamiento.
- Nunca se debe transmitir sin antena conectada al RA-02.

FRONTEND: conserva la accesibilidad (HTML semántico, aria-live en el estado del enlace, foco visible, prefers-reduced-motion), el diseño responsivo, el tema claro/oscuro y la legibilidad al sol. Prueba con el simulador antes de dar algo por terminado.

Responde en español. Si falta información para continuar, pregunta en lugar de suponer.
```
