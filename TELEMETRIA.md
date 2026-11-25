# CanSAT 2026 · Telemetría v2

Esta especificación solo incluye lo que pide la misión. No asume sensores, pines ni batería que el equipo no haya confirmado.

## 0. Misión (cambio del 24/11/2026)

El CanSAT ya no se separa ni se desacopla en el aire. Ahora son dos partes que se lanzan por separado:

| Parte | Qué hace | ¿Transmite? |
|---|---|---|
| **Módulo del huevo con autogiro** | Desciende frenado por el autogiro | **Sí**: es la única que lleva telemetría |
| **Compartimento en caída libre** | Se deja caer sin frenado, para ver si sobrevive | No |

- **Altura de lanzamiento prevista:** 20–25 m. **Límite:** no pasar de 30 m.
- **Secuencia:** el CanSAT se enciende **en el suelo** (ahí empiezan los datos y el cero de altura), una persona sube cargándolo a 20–25 m, lo suelta y el autogiro frena el descenso desde ese momento. Cuando vuelve a 0 m, terminó.
- Ya no hay separación a 200 m, liberación a 450 m ni carga secundaria que localizar.

### Fases (las calcula la estación con la altura)

| Fase | Cuándo empieza | Umbral actual (por confirmar en una prueba real) |
|---|---|---|
| En tierra | Al encender | – |
| Subida | Empieza a subir | Más de 1 m sobre el suelo y subiendo a más de 0.3 m/s. Esperar arriba sigue siendo subida |
| Descenso con autogiro | Lo sueltan | Baja a más de 2 m/s y ya está 1.5 m por debajo de la altura máxima |
| Aterrizado | Vuelve a 0 m | Menos de 1 m sobre el suelo |

Las fases solo avanzan. El cero de altura se toma solo con el promedio de los 5 primeros paquetes, así que hay que encender el CanSAT en el suelo; el botón "Fijar altura 0" permite volver a tomarlo.

## 1. Qué debe cubrir la telemetría

| Requisito de la misión | Dato que lo demuestra |
|---|---|
| Presión, temperatura, orientación, aceleración | `pres`, `temp`, `roll/pitch/yaw`, `ax/ay/az` |
| Velocidad instantánea y altura máxima (≤ 30 m) | `alt` (a bordo); velocidad y máximo se derivan en tierra |
| Subida y descenso con autogiro | Fase y `vel` calculadas en tierra con `alt` |
| Transmitir ≥ 10 s tras aterrizar | Fase "Aterrizado" y contador en tierra |
| Alcance ≥ 250 m | `rssi`, `snr`, paquetes perdidos |

El descenso de 8–12 m/s y los 10 s después de aterrizar vienen de la guía original y se conservan mientras no se indique otra cosa.

## 2. Formato del paquete

Texto CSV, una línea por paquete. El formato es el mismo que en v1 (17 campos), para no cambiar el firmware. El CanSAT envía los campos 0–14; la estación terrena agrega `rssi,snr` y emite por USB:

```
$seq,ms,st,pres,temp,alt,ax,ay,az,roll,pitch,yaw,flags,sd,sb,rssi,snr
```

| # | Campo | Unidad | Descripción |
|---|---|---|---|
| 0 | `seq` | – | Contador de paquetes (permite detectar pérdidas) |
| 1 | `ms` | ms | Tiempo desde el arranque del CanSAT |
| 2 | `st` | 0–3 | Fase según el CanSAT, opcional (0 En tierra, 1 Subida, 2 Descenso con autogiro, 3 Aterrizado). Mientras el firmware no la calcule, enviar `0`: la estación calcula la suya con la altura y la guarda en el CSV como `fase_calc` |
| 3 | `pres` | hPa | Presión |
| 4 | `temp` | °C | Temperatura |
| 5 | `alt` | m | Altura barométrica estándar (referencia 1013.25 hPa). La estación resta el cero tomado en el suelo. **No ponerla en cero en el firmware:** el ruido del barómetro cerca del suelo puede dar exactamente `-1.00`, que significa "sin dato" |
| 6–8 | `ax ay az` | g | Aceleración |
| 9–11 | `roll pitch yaw` | ° | Orientación |
| 12 | `flags` | bitmask 0–3 | Opcional: b0 autogiro desplegado, b1 aterrizado. La estación no los usa; solo se guardan en el CSV. Los bits 2 y 3 deben ir en 0 |
| 13 | `sd` | – | **Reservado.** Antes era la distancia a la carga secundaria. Enviar siempre `-1` |
| 14 | `sb` | – | **Reservado.** Antes era el rumbo a la carga secundaria. Enviar siempre `-1` |
| 15 | `rssi` | dBm | Lo agrega la estación terrena |
| 16 | `snr` | dB | Lo agrega la estación terrena |

Ejemplo: `$120,24400,0,962.30,26.4,450.52,0.02,0.01,1.00,3.5,-2.1,141,0,-1,-1,-71,8.5`

**Dato ausente:** un sensor que todavía no existe o no se pudo leer se envía como `-1`. La estación muestra "sin dato" y no calcula derivados con él.

**Derivados en tierra** (no se transmiten): fase, velocidad vertical (derivada suavizada de `alt`), altura máxima, módulo de aceleración en g, paquetes perdidos y tiempo transmitido tras aterrizar.

### CSV de la estación terrena

Columnas: `origen,sesion,fecha,hora_recepcion,hora_medicion,t_s`, los 17 campos del paquete y `cero_m,alt_rel_m,fase_calc,vel_calc_ms,g_calc`.

- **Hora real:** el CanSAT no tiene reloj, así que la pone la computadora (hora local; conviene que esté sincronizada por internet). `hora_recepcion` es cuándo llegó el paquete e incluye el retraso de radio y USB. `hora_medicion` es la hora estimada en que el CanSAT tomó la muestra: hora de la PC que corresponde a `t_s = 0` más el tiempo del CanSAT, sin el titubeo de la radio. Formato `HH:MM:SS.mmm`.
- **Guardar en vivo** (Chrome/Edge): cada segundo se agregan al archivo elegido los paquetes nuevos, para no perder el vuelo si se cierra el navegador. Excel bloquea el archivo si se abre durante la captura; la estación reintenta sin perder filas. Las filas escritas antes de tomar el cero tienen `cero_m` y `alt_rel_m` vacíos.
- **Descargar CSV** genera la versión final, con la altura recalculada con el cero definitivo.
- `alt_rel_m` queda vacío mientras no haya cero, para no confundir la altura barométrica con la altura sobre el suelo.

## 3. Decisiones de diseño (propuestas)

- **Tasa:** 5 Hz. Con SF7 y ancho de banda de 250 kHz, un paquete de unos 70 bytes ocupa ≈ 65 ms en el aire. **Ojo con la misión nueva:** desde 20–25 m, a 8–12 m/s, el descenso dura solo 2–3 s, o sea 10–15 paquetes. Conviene subir a 10 Hz (`PERIODO_MS 100`) si el enlace lo aguanta.
- **Altura a bordo, velocidad en tierra:** si se pierden paquetes, la velocidad se puede seguir calculando.
- **Texto CSV:** fácil de depurar con el monitor serie. Si falta ancho de banda, se puede migrar a binario.
- **Reenviar el último estado** (`st`, `flags`) en cada paquete, para que un paquete perdido no oculte un evento.
- **Validación v2:** los firmwares de `firmware/telemetria/` validan `st` 0–3, `flags` 0–3 y `sd=sb=-1`. La estación terrena aplica los rangos de v2. `herramientas/telemetria.py` conserva el análisis histórico de v1 (incluidas sus fases y simulación de 450 m): no usarlo para certificar fases, altura relativa ni aterrizaje de la misión v2; estos derivados y el CSV v2 los genera el frontend.

## 4. Hardware de sensores confirmado por Genrikh

BME280 y MPU6050 comparten I²C: SDA GPIO21, SCL GPIO22, VCC a 3V3 y GND común. El transmisor los identifica en 0x76/0x77 y 0x68/0x69 respectivamente, comprobando chip ID; las direcciones son alternativas, no valores asumidos del circuito. `pres/temp/alt` proceden del BME280 y `ax/ay/az` del MPU6050, en g incluyendo gravedad. No se transmite humedad.

`roll/pitch` son inclinaciones aproximadas por gravedad en los ejes del módulo, solo cerca del reposo; no se afirma actitud dinámica durante caída/autogiro. `yaw=-1` porque no existe referencia absoluta de rumbo. Ver `firmware/telemetria/SENSORES.md` para configuración, validez y fallos. El formato y las fases calculadas en tierra no cambian. Las lecturas que fallen permanecen en -1, con reintento independiente. La lectura/precisión efectiva sigue pendiente de comprobar en el circuito.
