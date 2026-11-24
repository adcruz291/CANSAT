# Spec Delta

## Purpose

Mantener la comunicación de telemetría v2 compatible con la estación terrena, con errores observables y esperas acotadas verificables sin disponer del circuito.

## ADDED Requirements

### Requirement: Contrato v2 compatible
El sistema SHALL enviar 15 campos CSV por radio y emitir 17 por USB a 115200 baudios, en el orden de TELEMETRIA.md. El receptor SHALL agregar RSSI y SNR del paquete recibido. Los datos SHALL comenzar con `$` y los diagnósticos con `#`.

#### Scenario: Recepción de un paquete correcto
- **WHEN** llega una trama válida de 15 campos
- **THEN** se emite una sola línea de 17 campos aceptable por el frontend actual, conservando las medidas y contadores originales

#### Scenario: Métricas de radio y delimitación por paquete
- **WHEN** el ciclo de recepción procesa dos paquetes válidos con pares RSSI/SNR distintos
- **THEN** emite exactamente dos líneas de 17 campos, cada una terminada en salto de línea y con RSSI y SNR de su propio paquete en ese orden; los diagnósticos no se mezclan en las líneas de datos

### Requirement: Medidas ausentes explícitas
El transmisor SHALL representar sensores no integrados o lecturas no finitas mediante `-1`, sin inventar medidas. En esta entrega SHALL enviar `st=0`, `flags=0` y `sd=sb=-1`; SHALL conservar la referencia barométrica estándar cuando exista altura válida.

#### Scenario: Una lectura falla
- **WHEN** una lectura de temperatura es NaN y las demás lecturas son válidas
- **THEN** el paquete transmite temperatura `-1` y conserva los otros datos válidos

#### Scenario: No hay sensores integrados
- **WHEN** se ejecuta la versión base entregada
- **THEN** los campos de sensores son `-1` y el frontend no calcula altura ni fases de vuelo con datos inventados

### Requirement: Rechazo de tramas inválidas
El receptor SHALL descartar tramas truncadas, vacías, con campos adicionales, bytes nulos, números no finitos o estados fuera de v2. SHALL exigir `seq/ms` uint32, `st/flags` 0–3 y `sd=sb=-1`. Todo paquete generado correctamente por el transmisor SHALL satisfacer esa validación y el límite de 255 bytes.

#### Scenario: Entrada corrupta seguida de entrada válida
- **WHEN** llega una trama con un campo vacío seguida de una trama válida
- **THEN** no se emite ninguna línea `$` para la primera y la segunda se entrega normalmente

#### Scenario: Formato no representable
- **WHEN** los valores o el buffer disponible impiden representar una trama admitida por el receptor
- **THEN** no se transmite una trama parcial y se registra un diagnóstico

### Requirement: Espera de transmisión acotada
El transmisor SHALL abandonar un envío sin finalización tras 1000 ms, registrar el error e iniciar recuperación de radio. SHALL seguir atendiendo su ciclo sin bucles de espera indefinidos controlados por la aplicación.

#### Scenario: La radio nunca termina el envío
- **WHEN** la radio permanece ocupada durante 1000 ms desde el inicio asíncrono de la transmisión
- **THEN** el intento se declara fallido y el sistema progresa al estado de recuperación, sin afirmar recepción remota

#### Scenario: Finalización observada en el límite
- **WHEN** en el primer turno que alcanza el límite de 1000 ms ya se observa que la transmisión terminó
- **THEN** se registra finalización local sin timeout, sin afirmar recepción remota

### Requirement: Recuperación sin reiniciar la placa
Ambos equipos SHALL reintentar la inicialización fallida de la radio cada 2000 ms, sin espera activa. SHALL reaplicar la configuración completa al recuperarse y conservar los contadores del arranque actual.

#### Scenario: Radio ausente al arrancar y disponible después
- **WHEN** la inicialización falla dos veces y luego tiene éxito
- **THEN** se observan reintentos espaciados, la configuración se restablece y la comunicación continúa sin reiniciar la aplicación

#### Scenario: Falla la preparación del envío
- **WHEN** un intento preparado no puede iniciar el paquete de radio
- **THEN** se aborta, se emite un diagnóstico y se pasa a recuperación; el siguiente intento de inicialización ocurre tras 2000 ms, sin reutilizar el paquete ni la secuencia consumida

#### Scenario: Escritura parcial del paquete
- **WHEN** la radio acepta menos bytes que los del paquete preparado
- **THEN** se aborta sin iniciar la transmisión parcial, se emite un diagnóstico y se pasa a recuperación con reintento de inicialización tras 2000 ms; no se retransmite el paquete fallido y la próxima muestra elegible usa una secuencia nueva

### Requirement: Temporización estable de intentos
Con la radio disponible el transmisor SHALL programar intentos cada 200 ms y consumir una secuencia por intento preparado, incluso si falla. SHALL evitar ráfagas para recuperar retrasos y mantener ese comportamiento cuando el reloj uint32 desborde.

#### Scenario: Desbordamiento del reloj o demora de recuperación
- **WHEN** el reloj cruza su límite o la radio vuelve después de varios intervalos
- **THEN** se prepara como máximo un nuevo intento por turno elegible, sin emitir todos los intervalos atrasados
