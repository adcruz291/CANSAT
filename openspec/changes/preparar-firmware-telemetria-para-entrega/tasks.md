# Tasks

## 1. Integridad del contrato y lecturas ausentes

- [x] 1.1 Añadir saneamiento de lecturas no finitas antes de codificar en el transmisor; verificar con pruebas que una temperatura NaN pasa a `-1` sin perder las demás medidas y que no se alteran `st=flags=0` ni los reservados.
- [x] 1.2 Ajustar `paquete.h` y la validación correspondiente para que todo paquete generado cumpla los límites totales y por token del receptor; verificar buffers cortos, uint32 máximos y números finitos extremos en `test_firmware.cpp`, sin transmitir una trama parcial.
- [x] 1.3 Ampliar pruebas de rechazo y recuperación del receptor para truncamientos, campos vacíos/adicionales, NUL y rangos v1; ejecutar el ciclo/adaptador real de recepción con radio y Serial falsos y verificar cero líneas `$` por inválido y exactamente una línea terminada en salto de 17 campos por válido posterior, con RSSI/SNR de cada paquete en orden; documentar los límites en el README.

## 2. Ciclo de radio recuperable

- [x] 2.1 Introducir un auxiliar pequeño con reloj/radio sustituibles y estados pendientes/disponible/transmitiendo, manteniendo cada sketch autocontenido; verificar una prueba nativa del ciclo normal sin Arduino ni placa.
- [x] 2.2 Sustituir TX bloqueante por inicio asíncrono y consulta con límite de 1000 ms desde el inicio TX, comprobando finalización antes de timeout; probar las fronteras anterior/exacta/posterior al límite, radio ocupada indefinidamente, fallo de preparación y escritura parcial. En estos fallos verificar aborto sin TX parcial, un diagnóstico, recuperación con reintento a 2000 ms, secuencia consumida y muestra nueva tras recuperar, sin retransmitir el paquete anterior.
- [x] 2.3 Reemplazar la detención permanente tras fallo de inicialización en ambos equipos por reintentos cada 2000 ms; verificar dos fallos y recuperación, reaplicación de toda configuración y conservación de secuencia/tiempo mediante dobles.
- [x] 2.4 Mantener los intentos de 200 ms con aritmética uint32 y sin ráfagas; verificar desbordamiento de reloj, retraso largo y retorno desde recuperación, incluyendo consumo de una secuencia por intento preparado.
- [x] 2.5 Documentar timeout, prioridad de finalización en el deadline, recuperación ante fallo de preparación/escritura parcial, reintento a 2000 ms, muestra nueva sin retransmisión, diagnósticos `#`, ausencia de acuse y límites de recuperación; verificar el README contra las constantes y resultados de las pruebas anteriores.

## 3. Verificación reproducible y guía para el compañero

- [x] 3.1 Añadir un comando de verificación sin carga, con rutas configurables para Arduino CLI y biblioteca LoRa, salidas controladas y fallos explícitos por dependencia ausente; verificar el camino de dependencia faltante y el registro de los comandos ejecutados.
- [x] 3.2 Alimentar al HTML la salida Serial capturada del receptor real con dobles de radio, con paquetes inválidos, métricas distintas por paquete, recuperación y lecturas ausentes; verificar aceptación de las líneas válidas y asociación correcta de métricas sin concatenarlas desde el test. Conservar aparte el vuelo artificial de 186 paquetes y verificar cero 432 m, máximo relativo 25 m, cuatro fases y ≥10 s posteriores al aterrizaje sin editar el HTML.
- [x] 3.3 Compilar ambos sketches completos con ESP32 Dev Module, core 3.3.12 y LoRa 0.8.0; registrar versiones, comandos, resultado y memoria usada en el informe, y señalar cualquier verificación que no se haya podido ejecutar.
- [x] 3.4 Crear una guía de entrega con archivos/pestañas, GPIO, placa, biblioteca, radio, monitor a 115200 y conexión al dashboard; verificarla contra ambos sketches y añadir diagnóstico USB/BOOT/EN con carga física todavía pendiente.

## 4. Paquete e integración final

- [x] 4.1 Añadir empaquetado desde una lista explícita con ambos sketches, todos sus encabezados, HTML, contrato, guía, informe, verificador y fuentes/fixtures de todas las pruebas exigidas, preservando rutas relativas; generar manifiesto SHA-256 y versiones y conservar el ZIP previo. Verificar inventario/huellas tras extraer y ejecutar allí compilaciones y pruebas sin acceso al árbol original, con dependencias externas configurables; mantener registros de esta ejecución fuera de los archivos manifestados.
- [x] 4.2 Ejecutar en conjunto pruebas nativas, dobles de radio/reloj, integración del frontend, los 20 casos de QA existentes y compilación ESP32; verificar que el informe final distingue aprobados de pruebas no ejecutadas y que no afirma carga, sensores ni recepción física.
- [x] 4.3 Generar el paquete final con el comando documentado y verificar que sus fuentes coinciden con las compiladas/probadas desde la extracción y con el manifiesto. No declararlo verificado si falla o falta una comprobación obligatoria; devolver resultado no satisfactorio. Dejar en el informe los datos que debe devolver el compañero: log de carga, salida serie y captura real de recepción.

## Workflow follow-up

- El compañero intentará cargar ambas placas y registrar el resultado; esas comprobaciones físicas no son casillas necesarias para completar esta preparación sin hardware.
- Identificar modelos/pines/direcciones de sensores antes de abrir otro cambio que integre mediciones reales.
- Archivar este cambio después de implementar y revisar sus entregables; no marcar como aprobadas pruebas de alcance o vuelo por el hecho de archivarlo.
