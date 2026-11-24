# Design

## Context

Ver `proposal.md` para la motivación. La revisión es del árbol de trabajo actual en `main`, que contiene los ajustes v2 aún sin commit; no asumir que `HEAD` representa esos archivos.

Hallazgos observados:

- `transmisor.ino` usa `LoRa.endPacket()` bloqueante. El código local de LoRa 0.8.0 espera `IRQ_TX_DONE` en un bucle sin límite; añadir un temporizador alrededor de esa llamada no permite interrumpirla.
- Ambos `setup()` se quedan en `while (true)` si la inicialización falla. No hay recuperación posterior.
- `leerMedidas()` devuelve datos ausentes; no hay controladores de sensores confirmados. `crearPaquete()` rechaza cualquier no finito y deja de emitir ese intento completo.
- El receptor valida 15 campos y añade RSSI/SNR. Codificador y validador son independientes: el primero limita la trama a 255 bytes, pero el segundo también limita cada token a 31 caracteres. Un número finito extremo puede superar ese segundo límite.
- El HTML conserva 17 campos, interpreta `-1` como ausente y calcula cuatro fases y el cero con altura barométrica. No cambiar sus umbrales en esta entrega.
- Hay pruebas nativas de contrato e integración de un vuelo artificial, y evidencia anterior de compilación completa. No prueban el ciclo de radio con fallos.
- Existe `firmware/telemetria/transmisor.zip` sin seguimiento; no asumir que refleja el código actual ni sobrescribirlo.
- No hay especificaciones principales OpenSpec publicadas. `TELEMETRIA.md` v2 es el contrato de misión; la herramienta Python y sus capturas v1 son históricas.

## Goals / Non-Goals

**Goals:** aislar la temporización para probar fallos sin hardware, conservar las carpetas de sketch autocontenidas para Arduino IDE y producir una entrega reconstruible desde archivos identificados.

**Non-Goals:** añadir controladores de sensores, cambiar el protocolo o frontend, usar interrupciones nuevas, instalar herramientas silenciosamente en la máquina del compañero o corregir por software la falta de respuesta del cargador ROM.

## Decisions

### 1. Estado de radio con reloj inyectable

Reemplazar la espera bloqueante por `endPacket(true)` y consultar disponibilidad con la API pública `beginPacket()` desde el ciclo. En LoRa 0.8.0 esta devuelve 0 mientras TX continúa; al devolver 1 pone la radio en reposo y vacía/prepara la FIFO sin iniciar otro envío. La siguiente muestra vuelve a prepararla antes de escribir. La consulta no consume secuencia ni genera una muestra. `isTransmitting()` es privado y no se llama desde el sketch. Usar estados mínimos: radio pendiente, disponible y transmitiendo; el receptor necesita los dos primeros. El adaptador Arduino conserva las llamadas SPI/LoRa, mientras un auxiliar pequeño permite probar decisiones temporales con reloj y radio falsos. No introducir un framework ni un sistema de plugins.

Al iniciar TX registrar su tiempo; si sigue ocupada a los 1000 ms, registrar `# ERROR`, abortar a reposo y pasar a recuperación. Si falla inicialización, reintentar a los 2000 ms; reaplicar SF, BW, CR, sync word, preámbulo, CRC e IQ, además de potencia en el transmisor. Mantener secuencia y reloj de arranque, sin reiniciar el ESP32. Cada diagnóstico corresponde a un evento o reintento, no a cada iteración.

El límite TX se mide desde el inicio asíncrono, no desde la adquisición de sensores. En cada consulta, comprobar primero si terminó la transmisión; solo si sigue ocupada evaluar el timeout. Por tanto, una finalización ya observable en el turno del deadline no se marca como timeout. Si no se pudo iniciar el paquete o la escritura fue parcial, abortar a reposo, emitir un solo diagnóstico por ese intento y pasar también a recuperación; no comenzar a transmitir bytes parciales. Registrar la entrada a recuperación y esperar 2000 ms antes de reintentar inicialización.

Alternativa descartada: conservar `endPacket()` y comprobar tiempo después, porque el bucle interno impediría recuperar el control. No hacen falta callbacks ni asumir que DIO0 funciona para esta estrategia de consulta.

### 2. Temporización y contabilidad explícitas

Conservar `PERIODO_MS=200`, diferencias unsigned y un solo intento elegible tras un retraso. Un intento preparado consume secuencia incluso si falla `beginPacket`, la escritura o la finalización; un turno sin radio disponible no prepara intento. No incrementar secuencia por cada espera/reintento de inicialización. Los huecos no son exclusivamente pérdidas de radio.

No guardar el paquete fallido para retransmitirlo tras recuperar radio. La siguiente oportunidad elegible toma una muestra nueva con tiempo y secuencia nuevos; la secuencia consumida por el fallo permanece consumida. Los turnos de espera no generan ráfagas ni muestras ficticias.

`ms` describe el instante de adquisición/preparación, no la recepción USB. Mantener emisión continua mientras haya alimentación; la detección de aterrizaje sigue siendo del frontend. No subir a 10 Hz sin medición física del tiempo de aire.

### 3. Saneamiento en la frontera de lecturas

Antes de serializar, reemplazar solo NaN e infinitos de campos de sensores por `-1`. La codificación sigue rechazando metadatos inválidos y tramas no representables; no convertir arbitrariamente cualquier valor finito extraño en una lectura plausible. `st=flags=0` y `sd=sb=-1` son la configuración de esta entrega.

El emisor debe comprobar longitud total, truncamiento y límites de tokens antes de transmitir, para que toda salida aceptada por el codificador sea aceptada por el receptor. Mantener validación de estados 0–3 y bits 0–3. Preservar los campos y unidades del HTML. Los auxiliares necesarios permanecerán dentro de cada carpeta de sketch; no depender de rutas externas o del otro sketch al distribuirlo.

Alternativa descartada: inventar lecturas o activar un simulador por defecto para llenar las gráficas. La simulación queda exclusivamente en pruebas identificadas.

### 4. Pruebas del comportamiento y de la entrega

Extender el corpus C++ con buffers cortos, uint32 máximos, números finitos extremos, campos vacíos/adicionales, NUL y paquetes posteriores a errores. Probar que toda trama generada válida cumple el receptor. Añadir pruebas del auxiliar de ciclo con TX ocupado para siempre, finalización normal, escritura parcial, radio ausente y recuperación, reloj desbordado y ausencia de ráfagas.

Reutilizar `firmware_frontend.cjs` para el paquete sin sensores y el vuelo artificial: cero 432 m, máximo relativo 25 m, fases 0–3 y postaterrizaje. Ejecutar QA del HTML sin modificarlo. Compilar ambos sketches completos con ESP32 Dev Module, core 3.3.12 y LoRa 0.8.0. Las pruebas con dobles no validan las señales SPI ni la recepción RF.

Añadir una prueba que ejecute el ciclo o adaptador de recepción realmente utilizado por el sketch, con radio y Serial falsos. No basta probar `paqueteValido` ni concatenar métricas desde el emisor de pruebas. Inyectar un paquete inválido seguido de dos válidos con RSSI/SNR diferentes; capturar la salida y comprobar cero líneas `$` por inválido, una línea terminada en salto por válido, 17 campos y métricas asociadas al paquete correcto. Pasar esa salida capturada al HTML para verificar el recorrido de recepción hasta el consumidor; mantener separados los fixtures de vuelo artificial.

Crear un comando de verificación/empaquetado reproducible, con rutas de CLI/biblioteca configurables, archivos de salida dentro del destino solicitado y errores explícitos si faltan dependencias. No cargar placas ni descargar/instalar dependencias sin una acción explícita separada.

### 5. Paquete de entrega desde lista explícita

Producir un ZIP nuevo de ambos roles y conservar el ZIP anterior. Incluir sketches y sus `.h`, HTML, `TELEMETRIA.md`, guía para el compañero, informe, verificador y todas las fuentes/fixtures de las pruebas exigidas. Preservar las rutas relativas entre firmware, pruebas y HTML, incluidas las cabeceras de dobles que se añadan; enumerar cada dependencia local en la lista explícita del empaquetado. Las herramientas externas y la biblioteca LoRa se configuran por opciones documentadas, no mediante rutas fijadas al equipo de Genrikh. Generar manifiesto con SHA-256 por archivo, referencia Git como contexto, indicador de árbol modificado y versiones efectivamente usadas; no identificar solo con `HEAD` una entrega con cambios locales.

Tras extraer, ejecutar compilaciones y pruebas desde esa carpeta sin acceder al árbol original. Esta segunda ejecución puede usar herramientas externas instaladas y sus cachés, pero las fuentes probadas y compiladas deben ser las extraídas. Mantener informe y registros de esta comprobación fuera de los archivos ya manifestados para no invalidar sus huellas; si cambia una fuente distribuida, regenerar manifiesto/paquete y verificar de nuevo. No etiquetar como verificado un paquete con comprobaciones obligatorias fallidas o no ejecutadas; devolver código no satisfactorio e indicar el motivo.

Verificar extracción, archivos obligatorios y huellas antes de declarar el paquete listo. La guía debe explicar cómo abrir/agregar pestañas, instalar LoRa, seleccionar placa/puerto, mantener antena, cargar ambos equipos y liberar el puerto antes del dashboard. Registrar el error USB de Genrikh como pendiente; incluir pasos BOOT/EN y recogida de log sin prometer que solucionan todos los fallos.

## Risks / Trade-offs

- [Sin prueba física] → etiquetar la entrega como verificada en computadora, con carga, cableado, sensores, recepción y alcance pendientes.
- [El watchdog de TX no interrumpe un bloqueo interno de SPI] → limitar la promesa a esperas controladas por la aplicación; documentar ese límite y probar el auxiliar con dobles.
- [La recuperación interrumpe paquetes y reduce la tasa] → diagnóstico de eventos y medición posterior de tasa efectiva; no declarar 5 Hz sostenidos durante una avería.
- [`-1` puede ser una medición real] → conservar la ambigüedad documentada del contrato actual; no migrar a otro formato sin actualizar todo el sistema en otro cambio.
- [Copias manuales incompletas] → carpetas autocontenidas, lista obligatoria y verificación tras extraer el ZIP.
- [Sensores visibles en la foto sin identificación suficiente] → mantener `leerMedidas()` como punto de integración pendiente, sin seleccionar modelos ni direcciones por apariencia.

## Migration Plan

1. Conservar el trabajo local y los originales provisionales; implementar sobre el contrato v2 actual.
2. Ejecutar pruebas de computadora, compilar ambos sketches y generar el paquete con manifiesto e informe.
3. Entregar al compañero el paquete y la placa; la implementación de este cambio termina con las comprobaciones físicas expresamente pendientes, no requiere fingir ni completar un vuelo.
4. El compañero probará carga y recepción y devolverá registros. Si necesita volver al programa provisional, usar su copia conservada y documentar que su formato no sirve para el frontend v2.
