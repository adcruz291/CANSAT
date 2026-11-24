# Proposal

## Why

Genrikh entregará la placa y los programas a un compañero después de que fallara la conexión de carga por USB. Antes de esa entrega necesitamos reforzar el transporte y preparar una versión reproducible, distinguiendo las comprobaciones en computadora de la carga, la radio y los sensores todavía no verificados físicamente.

## What Changes

- Conservar el contrato v2 del frontend: 15 campos LoRa, 17 por USB a 115200, cinco intentos por segundo, fases y cero de altura calculados en tierra; sin nuevos sensores ni valores inventados.
- Acotar la espera de transmisión y recuperar fallos de inicialización/radio sin bloquear indefinidamente la aplicación ni producir ráfagas de recuperación.
- Convertir lecturas no finitas de sensores en el marcador existente `-1`, conservando la validación estricta de estados, bits y campos reservados.
- Asegurar que el codificador no emita una trama que el receptor rechace, incluyendo límites de longitud y representación numérica.
- Agregar pruebas de fallos mediante dobles de radio/reloj, además de la integración ya existente con el HTML y la compilación de ambos sketches para ESP32.
- Preparar un paquete de entrega con ambos sketches completos, frontend compatible, configuración, huellas de archivos, resultados y una guía de carga/diagnóstico para el compañero.
- Mantener separados los datos artificiales y los registros físicos pendientes; no presentar el fallo de carga como corregido por cambios de firmware.

## Capabilities

### New Capabilities

- `telemetria-lora`: transporte v2 validado, temporización acotada y recuperación de fallos entre transmisor y estación.
- `entrega-firmware`: distribución reproducible y verificada de los programas, con instrucciones y límites de la evidencia.

### Modified Capabilities

Ninguna: no hay capacidades publicadas en `openspec/specs/`. Las nuevas especificaciones formalizan el contrato existente y los comportamientos adicionales de recuperación y entrega; no sustituyen la misión descrita en `TELEMETRIA.md`.

## Impact

- `firmware/telemetria/transmisor/`: ciclo de transmisión, lecturas ausentes y codificación.
- `firmware/telemetria/receptor/`: validación, inicialización y diagnósticos.
- Nuevos auxiliares locales de sketch si hacen falta, sin perder la posibilidad de abrir cada carpeta desde Arduino IDE.
- `pruebas/test_firmware.cpp`, `pruebas/firmware_frontend.cjs` y nuevas pruebas de radio/reloj sin hardware.
- Documentación de entrega, verificación y empaquetado; conservar los sketches provisionales y no sobrescribir el ZIP local de Genrikh.
- LoRa de Sandeep Mistry 0.8.0 y core ESP32 3.3.12 como versiones de referencia de compilación. No cambiar el frontend ni migrar el analizador histórico v1 en este alcance.

Fuera de alcance: resolver físicamente USB/BOOT/alimentación, cargar la placa, verificar alcance, identificar sensores por su apariencia, implementar sus controladores sin modelos confirmados, cambiar a 10 Hz o certificar un vuelo real.
