# BME280 y MPU6050: lecturas reales

Genrikh confirmó ambos modelos y su cableado: VCC a 3V3, GND común, **SDA GPIO21 y SCL GPIO22**. Se usa `Wire` del core ESP32 a 100 kHz con timeout de 20 ms por operación. Los pines y las direcciones candidatas están agrupados al principio de `transmisor.ino` y marcados para verificar. El LoRa conserva sus GPIO 18/19/23/5/14/26.

No hay que instalar bibliotecas nuevas: el código usa registros de los sensores con `Wire`, además de LoRa 0.8.0. Conservar **`sensores.h`, `paquete.h` y `ciclo.h`** junto a `transmisor.ino`. El receptor conserva `contrato.h` y su propio `ciclo.h`.

## Datos y límites

| Campos | Origen / significado |
|---|---|
| `pres` | BME280, presión compensada con coeficientes de fábrica, en hPa |
| `temp` | BME280, temperatura compensada en °C; puede diferir del ambiente por calentamiento del montaje |
| `alt` | Altitud estándar calculada con presión y referencia 1013.25 hPa; el frontend resta el cero en el suelo |
| `ax/ay/az` | MPU6050, g en los ejes del módulo, incluyendo gravedad; no aceleración lineal con gravedad eliminada |
| `roll/pitch` | Inclinación aproximada a partir de gravedad: roll=`atan2(ay,az)`, pitch=`atan2(-ax,hypot(ay,az))`, en grados |
| `yaw` | `-1`: no hay magnetómetro ni referencia de rumbo absoluto; no se integra un giro con deriva como si fuera rumbo |
| `st/flags` | 0: las fases siguen calculándose en tierra |
| `sd/sb` | -1: reservados en la misión v2 |

El BME280 se identifica por chip ID **0x60** en 0x76 o 0x77, sin confundirlo con un BMP280. Se leen 24 bytes de calibración de temperatura/presión. Se configura temperatura x2, presión x4, modo normal, filtro sin retardo adicional y humedad desactivada (no hay campo de humedad en el contrato). Después de configurar se esperan al menos 40 ms sin bloquear el programa antes de publicar lecturas. Se rechazan coeficientes inválidos, conversiones desactivadas y lecturas fuera de los rangos operativos -40…85 °C / 300…1100 hPa.

El MPU6050 se identifica con WHO_AM_I **0x68** tanto en dirección 0x68 como en 0x69. Se habilitan ejes y reloj PLL X, acelerómetro ±16 g, giróscopo ±2000 °/s, filtro ~21 Hz y muestreo interno de 50 Hz. La telemetría continúa a 5 Hz. Tras configurar se esperan 100 ms; cada muestra comprueba data-ready y lee el bloque de 14 bytes en una transacción completa. El acelerómetro usa 2048 LSB/g; un eje saturado invalida ese grupo en lugar de inventar la aceleración del impacto.

**Roll/pitch no son una solución de actitud en vuelo.** Solo se publican cuando la aceleración total está entre 0.85 y 1.15 g y las tres velocidades angulares están dentro de ±20 °/s. Fuera de esas condiciones quedan en `-1`. Cerca del eje X vertical, roll también es indeterminado y queda ausente. Esos controles reducen errores evidentes, pero una aceleración externa todavía puede parecer gravedad. En autogiro/caída, puede haber aceleración válida sin inclinación válida. No se afirma fusión de sensores, calibración de sesgo ni precisión dinámica.

Los ejes corresponden a la orientación física del **módulo**, no a una orientación del CanSAT confirmada. Revisar signos/ejes con el circuito quieto e inclinaciones conocidas antes de interpretar el vuelo. La aceleración incluye la calibración de fábrica; falta contrastar sesgo, montaje y valores con referencias reales. El marcador -1 conserva la ambigüedad del contrato existente: una medida real que redondea a -1.00 se verá como ausente en el HTML; no se cambió el protocolo unilateralmente.

## Fallos y recuperación

Cada intento prepara una muestra nueva, sin reutilizar lecturas anteriores. Un sensor ausente, ID incorrecto, respuesta I²C incompleta o lectura inválida deja solo sus campos en -1 y emite un diagnóstico `# ERROR`; la otra fuente y la radio pueden seguir funcionando. Se reintenta identificar/configurar el sensor a los 2000 ms. Un fallo al iniciar el bus también reintenta a los 2000 ms. Durante las primeras conversiones los campos pueden aparecer temporalmente como sin dato.

Esto permite detectar ausencia y errores de transporte, pero no garantiza distinguir todos los fallos internos de un sensor que responda con números plausibles. Los timeouts acotan las operaciones de `Wire`; no verifican conexiones físicas ni reparan un problema de alimentación. Los umbrales de fases del frontend siguen pendientes de probar con las lecturas reales.

## Comprobación de mesa pendiente

1. Cargar la nueva carpeta completa del transmisor y la del receptor, manteniendo las antenas conectadas.
2. Encender el CanSAT **en el suelo**, quieto durante al menos los primeros segundos. Buscar los diagnósticos de sensores identificados en el monitor del transmisor a 115200.
3. En el receptor deben llegar líneas de 17 campos con presión, temperatura y aceleración numéricas. Con el módulo quieto y Z hacia arriba, esperar aproximadamente ax≈0, ay≈0, az≈+1 g, según montaje y sesgo; no son tolerancias de aprobación.
4. Cerrar el monitor del receptor y conectarlo al frontend. La altura relativa debe tomar cero tras cinco alturas válidas; la altitud transmitida no tiene por qué ser cero.
5. Inclinar la placa con calma, confirmar signos de roll/pitch y anotar diferencias. Yaw permanece sin dato. Guardar captura cruda y CSV.

## Evidencia de software y fuentes

`pruebas/test_sensores.cpp` comprueba un vector numérico conocido de compensación (25.08248 °C, 1006.53267 hPa), conversión de aceleración y signos, inclinación de 30°/45°, direcciones alternativas, IDs incorrectos, saturación, caída libre, giro fuerte, ausencia, lecturas cortas y recuperación independiente. También ejecuta el sketch completo con dispositivos I²C falsos y exige un paquete con las medidas compensadas. Los fixtures están únicamente en `pruebas/`; no se compilan dentro del firmware de las placas.

- [Bosch BME280, hoja de datos](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf): registros, calibración y ecuaciones de compensación de temperatura/presión.
- [InvenSense MPU-6000/MPU-6050 Register Map, revisión 4](https://cdn.sparkfun.com/datasheets/Sensors/Accelerometers/RM-MPU-6000A.pdf): registros 0x19–0x1C, 0x3A–0x48, 0x6B/0x6C y WHO_AM_I.
- [Wire del core ESP32 3.3.12](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/Wire/src/Wire.h): transacciones I²C y timeout.

Estas pruebas validan cálculos y manejo de fallos en computadora. La lectura efectiva en el circuito, la precisión, la carga USB, el enlace, el alcance y el vuelo aún requieren evidencia física.
