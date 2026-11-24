# CanSAT 2026 · Telemetría v1

Esta especificación solo incluye lo que pide la guía del concurso. No asume sensores, pines ni batería que el equipo no haya confirmado.

## 1. Qué debe cubrir la telemetría

| Requisito de la misión | Dato que lo demuestra |
|---|---|
| Presión, temperatura, orientación, aceleración | `pres`, `temp`, `roll/pitch/yaw`, `ax/ay/az` |
| Velocidad instantánea y altura máxima | `alt` (a bordo); velocidad y máximo se derivan en tierra |
| Autogiro a 200 m, descenso de 8–12 m/s | `st` (fase), `flags` bit 1, `vel` derivada |
| Separación de cargas a 200 m | `st`=3, `flags` bit 0 |
| Transmitir ≥ 10 s tras aterrizar | `st`=5 y contador en tierra |
| Distancia y dirección de la carga secundaria | `sd`, `sb`, `flags` bit 3 |
| Alcance ≥ 250 m | `rssi`, `snr`, paquetes perdidos |

## 2. Formato del paquete

Texto CSV, una línea por paquete. El CanSAT envía los campos 0–14; la estación terrena agrega `rssi,snr` y emite por USB:

```
$seq,ms,st,pres,temp,alt,ax,ay,az,roll,pitch,yaw,flags,sd,sb,rssi,snr
```

| # | Campo | Unidad | Descripción |
|---|---|---|---|
| 0 | `seq` | – | Contador de paquetes (permite detectar pérdidas) |
| 1 | `ms` | ms | Tiempo desde el arranque del CanSAT |
| 2 | `st` | 0–5 | Fase: 0 Preparación, 1 Ascenso, 2 Caída libre, 3 Separación, 4 Autogiro, 5 Aterrizaje |
| 3 | `pres` | hPa | Presión |
| 4 | `temp` | °C | Temperatura |
| 5 | `alt` | m | Altura sobre el suelo, calculada con la presión |
| 6–8 | `ax ay az` | g | Aceleración |
| 9–11 | `roll pitch yaw` | ° | Orientación (requisito de la misión) |
| 12 | `flags` | bitmask | b0 separación hecha, b1 autogiro desplegado, b2 aterrizado, b3 carga secundaria localizada |
| 13 | `sd` | m | Distancia a la carga secundaria (`-1` = sin dato) |
| 14 | `sb` | ° | Rumbo a la carga secundaria (`-1` = sin dato) |
| 15 | `rssi` | dBm | Lo agrega la estación terrena |
| 16 | `snr` | dB | Lo agrega la estación terrena |

Ejemplo: `$1204,60400,1,986.12,26.4,310.52,0.02,0.01,1.00,3.5,-2.1,141,0,-1,-1,-71,8.5`

**Derivados en tierra** (no se transmiten): velocidad vertical (derivada suavizada de `alt`), altura máxima, módulo de aceleración en g, paquetes perdidos y tiempo transmitido tras aterrizar.

## 3. Decisiones de diseño (propuestas)

- **Tasa:** 5 Hz. Con SF7 y ancho de banda de 250 kHz, un paquete de unos 70 bytes ocupa ≈ 65 ms en el aire; con 125 kHz serían ≈ 130 ms. Ambos módulos deben usar los mismos parámetros de radio.
- **Altura a bordo, velocidad en tierra:** si se pierden paquetes, la velocidad se puede seguir calculando.
- **Texto CSV:** fácil de depurar con el monitor serie. Si falta ancho de banda, se puede migrar a binario.
- **Reenviar el último estado** (`flags`, `sd`, `sb`) en cada paquete, para que un paquete perdido no oculte un evento.
- **Campos sin sensor:** si algún dato no se puede medir con el hardware disponible, el campo se envía como `-1` o se elimina de la especificación en el mismo cambio en que se actualice el dashboard.
