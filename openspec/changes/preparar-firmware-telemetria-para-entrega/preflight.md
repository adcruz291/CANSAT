# Spec Preflight: preparar-firmware-telemetria-para-entrega

## Status

READY

## Intent

Reforzar y entregar ambos firmwares v2 a un compañero sin depender de una conexión funcional con la placa: preservar el frontend actual, acotar errores de radio, verificar el software y distribuir fuentes completas con evidencia identificable. Este preflight revisa el árbol de trabajo actual, incluidos los cambios locales. Después de la autorización de Genrikh se corrigieron las dos especificaciones, el diseño y las tareas, y se repitió la revisión de coherencia y trazabilidad. La propuesta mantiene su objetivo original. Esta revisión no implementa firmware ni marca tareas como terminadas.

## Evidence inspected

- `proposal.md`, ambas `specs/`, `design.md` y `tasks.md` del cambio — alcance, contratos y 15 tareas; hay planificación completa, con 0 tareas implementadas.
- `TELEMETRIA.md`, secciones 0–3 — contrato v2, altura estándar, campos reservados, fases calculadas en tierra y 5 Hz. La propuesta conserva esos requisitos.
- `firmware/telemetria/transmisor/transmisor.ino`, `leerMedidas`, `setup`, `loop` — campos ausentes, configuración, secuencia por intento, retorno ante `beginPacket` fallido y escritura parcial, finalización bloqueante.
- `firmware/telemetria/transmisor/paquete.h`, `crearPaquete` — rechazo de no finitos, estados, reservados y límites de buffer; justifica el saneamiento y control de longitud propuestos.
- `firmware/telemetria/receptor/contrato.h`, `paqueteValido` — validación de 15 campos, token máximo de 31 caracteres, uint32 y rangos v2.
- `firmware/telemetria/receptor/receptor.ino`, `setup` y `loop` — inicialización sin reintento, lectura del paquete, descarte, RSSI/SNR y emisión USB como operaciones separadas de la validación.
- `estacion_terrena.html`, `F`, `ingest`, `faseDe`, `fijarCero` y `conectar` — 17 campos, cuatro fases, cero automático, datos ausentes, USB 115200. Las fases dependen de altura; sensores en `-1` no permiten demostrar el vuelo.
- `pruebas/test_firmware.cpp` — pruebas directas de codificador/validador y emisor de vuelo artificial. El emisor añade literalmente `,-75,8.50`: no ejecuta el receptor.
- `pruebas/firmware_frontend.cjs` — acepta las líneas anteriores y ejecuta la lógica real del HTML con un DOM mínimo; no prueba LoRa, Serial ni el `loop` del receptor.
- `firmware/telemetria/README.md` — versiones de referencia, comandos y limitaciones de pruebas físicas. Sus comandos nativos dependen de archivos bajo `pruebas/`.
- LoRa 0.8.0 local, `src/LoRa.cpp`, `beginPacket`, `endPacket`, `isTransmitting` — confirma espera indefinida de la llamada bloqueante y alternativa asíncrona disponible. Es dependencia temporal inspeccionada, no un archivo distribuido por el proyecto.
- `firmware/telemetria/transmisor.zip` — existe un archivo local sin seguimiento. No se inspeccionó su contenido ni se presume que esté vigente; conservarlo.

**Known:** los contratos y comportamientos descritos arriba se localizaron en sus archivos. La compilación anterior está documentada, pero no se repitió una compilación ESP32 en este preflight.

**Inference:** el paquete debe permitir reproducir en otra carpeta las comprobaciones que lo respaldan; lo sustentan los requisitos de entrega reproducible y la tarea 4.1. Confianza alta. La lista explícita y el escenario de verificación desde la extracción ahora resuelven esa condición.

**Unknown:** modelos y conexiones de sensores, causa física del fallo de carga, estado del enlace y alcance. Ya están explícitamente fuera de esta implementación; no se convierten en supuestos ni requieren bloquear esta preparación sin hardware.

Comprobaciones ejecutadas durante esta revisión:

- Compilación C++11 de `test_firmware.cpp` con `-Wall -Wextra -Werror -pedantic`: correcta.
- Ejecución de la prueba nativa de contrato: correcta.
- Vuelo artificial de 186 paquetes contra el HTML actual: correcto.
- `openspec validate preparar-firmware-telemetria-para-entrega --strict`: correcto. Este resultado valida estructura, no cubre los defectos de requisitos identificados abajo.

## Requirement defects

No quedan defectos materiales abiertos. Se conserva la historia de los tres hallazgos y su resolución comprobada en los documentos actuales:

| ID | Type | Requirement/area | Evidence | Impact | Resolution |
|---|---|---|---|---|---|
| O-001 | OMISSION | Recuperación antes de TX | `specs/telemetria-lora/spec.md`: escenarios de preparación fallida y escritura parcial; `design.md` decisiones 1–2; tareas 2.2 y 2.5 | Evitar que distintas implementaciones elijan transiciones incompatibles tras el mismo fallo | RESUELTO en planificación: aborto sin TX parcial, un diagnóstico, recuperación a 2000 ms, secuencia consumida y nueva muestra; sin retransmitir el paquete fallido. También queda definida la prioridad de finalización observada sobre timeout. |
| O-002 | OMISSION | Reproducción desde el paquete | `specs/entrega-firmware/spec.md`: inventario y escenario de verificación extraída; `design.md` decisión 5; tareas 4.1 y 4.3 | Evitar una entrega que dependa de fuentes o pruebas ausentes | RESUELTO en planificación: incluir verificador y fuentes/fixtures con rutas relativas, comprobar huellas y ejecutar desde la extracción sin acceso al árbol original; un fallo obligatorio impide etiquetar la entrega como verificada. |
| O-003 | OMISSION | Contrato USB del receptor completo | `specs/telemetria-lora/spec.md`: métricas y delimitación por paquete; `design.md` decisión 4; tareas 1.3 y 3.2 | Evitar falsos positivos cuando las pruebas agregan métricas sin ejecutar el receptor | RESUELTO en planificación: ejecutar ciclo/adaptador real con radio y Serial falsos, invalidar tramas corruptas, variar métricas por paquete, verificar 17 campos y salto de línea y pasar la salida capturada al HTML. |

Los comportamientos pendientes de implementar no se presentan como verificados. La revisión cubre la definición de requisitos, aceptación y pruebas; los sensores desconocidos y la falta de evidencia física permanecen fuera de alcance.

## Traceability

| Req | Requirement | Evidence | Acceptance criterion | Verification | Task |
|---|---|---|---|---|---|
| T1 | Contrato v2 compatible | `TELEMETRIA.md`; `F`; receptor `loop` | Una trama LoRa válida produce una línea USB de 17 campos, medidas intactas y métricas de ese paquete | Integración del HTML + captura del receptor completo con dobles, requerida por O-003 resuelto | 1.3, 3.2 |
| T2 | Medidas ausentes explícitas | `Medidas`; `leerMedidas`; HTML `has` | Solo NaN/inf o ausencia se marcan `-1`; permanecen los demás datos; fase/flags base 0 y reservados -1 | Casos nuevos de saneamiento y caso sin sensores del HTML | 1.1, 3.2 |
| T3 | Rechazo de tramas inválidas | `paqueteValido`; `crearPaquete` | No se emite/transmite parcial; un paquete válido posterior funciona; toda salida aceptada por emisor satisface receptor | Corpus de límites y recepción con salida Serial capturada | 1.2, 1.3 |
| T4 | Espera TX acotada | LoRa `endPacket`; diseño 1 | Radio ocupada hasta deadline termina el intento a 1000 ms y entra en recuperación | Reloj/radio falsos, fronteras anterior/exacta/posterior a deadline | 2.1, 2.2 |
| T5 | Recuperación de radio | `setup` de ambos sketches; diseño 1 | Fallo de inicio reintenta a 2000 ms y aplica la misma configuración sin reset de contadores | Dos fallos y éxito con llamadas registradas; preparación fallida y escritura parcial con recuperación definida | 2.2, 2.3, 2.5 |
| T6 | Intentos de 200 ms sin ráfagas | Transmisor `loop`; diseño 2 | Máximo un intento por turno elegible, secuencia consumida por preparación y continuidad ante wrap | Dobles de reloj, demora, recuperación y fallos locales | 2.4 |
| E1 | Paquete completo e identificable | README; diseño 5 | Ambos sketches autocontenidos y huellas coincidentes al extraer | Inventario completo de fuentes/pruebas/verificador + SHA-256 | 4.1, 4.3 |
| E2 | Verificación reproducible | Pruebas y comandos del README | Ambos sketches compilan y las pruebas pueden repetirse desde la entrega con dependencias disponibles | Compilación ESP32, corpus, radio/reloj e integración desde la carpeta extraída | 3.1–3.3, 4.1–4.3 |
| E3 | Guía de carga/diagnóstico | Pines en ambos sketches; error USB del usuario | Identifica roles, archivos y pasos sin prometer que cambian un problema físico | Revisión contra sketches y checklist de guía | 3.4 |
| E4 | Evidencia física pendiente | `leerMedidas`; limitaciones del README | Ningún informe convierte compilación o simulación en prueba RF/USB/sensores real | Inspección de informe y etiquetas de datos | 3.3, 3.4, 4.2 |
| E5 | Fuente vigente y originales conservados | ZIP local existente; árbol modificado | ZIP nuevo con huellas de fuentes verificadas; ZIP antiguo sin modificaciones | Hash antes/después, extracción y comparación con fuentes compiladas | 4.1, 4.3 |

## Edge cases

- NaN e infinitos: plan cubierto; mantener la distinción entre saneamiento de sensores y metadatos inválidos.
- Token demasiado largo aunque el paquete completo quepa: plan cubierto por 1.2; no hace falta cambiar a binario ni el frontend.
- `beginPacket` ocupado y escritura parcial: tienen transición y pruebas explícitas en la especificación y tareas 2.2/2.5.
- TX que finaliza justo en el deadline: prioridad de finalización sobre timeout definida en escenario, diseño y prueba 2.2.
- Reloj que cruza uint32 y vuelta desde recuperación: plan cubierto por 2.4; no reiniciar `seq/ms` al recuperar radio.
- Paquete corrupto seguido de válido: el plan exige probar el receptor completo, además de su función validadora, en 1.3 y 3.2.
- Duplicados y orden: el frontend ya los trata; no añadir al receptor una deduplicación nueva que elimine evidencia sin requisito explícito.
- Dependencias ausentes o compilación fallida: el informe debe conservar el fallo/no ejecutado; el paquete no puede etiquetarse como verificado si las comprobaciones obligatorias no pasaron.
- Extracción lejos del repositorio original: inventario de pruebas y verificador incluido, con ejecución extraída obligatoria; no se requieren rutas absolutas al equipo de Genrikh.
- Sensores y RF no verificables ahora: pendientes explícitos y fuera del criterio de finalización de este cambio; no inventar modelos ni reclamar que el error de carga está resuelto.

## Required spec changes

Ninguno pendiente. Se releyeron las dos especificaciones, `design.md`, `tasks.md` y `proposal.md` después de los ajustes, y se comprobó que cada hallazgo tiene escenarios de aceptación, decisiones y tareas concordantes. Las 15 tareas siguen sin marcar: READY describe la preparación del plan, no implementación completada.

La validación estricta de OpenSpec volvió a pasar después de editar los artefactos. No se repitieron compilaciones ESP32 ni pruebas de radio inexistentes: las comprobaciones de código enumeradas arriba son las ejecutadas en la revisión inicial, sobre un firmware que no cambió en este refinamiento.

## Apply gate

PROCEED

El cambio está listo para el flujo normal `openspec-apply-change`. Esta revisión no lo inicia.

Orden del handoff:

1. **Preservar requisitos:** 15/17 campos CSV v2, USB 115200, GPIO y parámetros de radio actuales, 200 ms sin ráfagas, fases/cero en tierra, sensores ausentes en -1 y sin datos inventados.
2. **Verificación a ejecutar al implementar:** corpus de contrato y saneamiento, dobles de radio/reloj/Serial, límites de timeout y wrap, integración desde la salida del receptor y vuelo artificial, QA del HTML, compilación de ambos sketches y repetición desde la extracción del ZIP.
3. **Tareas:** seguir los grupos 1–4 de `tasks.md`, incluyendo recuperación antes de TX, nuevos auxiliares autocontenidos, guía e informe, manifiesto y empaquetado reproducible. No reemplazar el ZIP previo.
4. **Riesgos de regresión:** secuencia reiniciada al recuperar, ráfagas tras demoras, confundir finalización local con recepción, métricas de otro paquete, rutas a fuentes externas e informes que presenten simulación como evidencia física.

Continuar con el flujo normal de apply cuando Genrikh solicite implementar. Sensores, carga, USB físico, enlace RF, alcance y vuelo siguen pendientes de evidencia real y no se certifican por este resultado.

## Hallazgo durante implementación: C-001

La compilación completa con LoRa 0.8.0 rechazó `LoRa.isTransmitting()` porque el método es privado (`src/LoRa.h`, línea 102). La decisión 1 de `design.md` asumía que podía consultarse directamente; el doble de pruebas exponía el método y no detectó esa restricción de acceso. Por ello se reabre la tarea 2.2 y se pausa antes de continuar. Este hallazgo sustituye el resultado READY anterior hasta corregirlo y repetir el preflight.

Ajuste propuesto: consultar la disponibilidad mediante `LoRa.beginPacket()`, API pública que internamente comprueba si sigue transmitiendo y retorna 0 mientras está ocupada. Al retornar 1 deja la radio preparada y reinicia la FIFO; la siguiente muestra vuelve a prepararla antes de escribir. No iniciar una nueva transmisión en la consulta ni consumir secuencia por consultar. Mantener timeout, prioridad de finalización y recuperación; registrar los efectos de preparación en el diseño y hacer que el doble refleje esta API pública, eliminando la exposición de `isTransmitting()`. Repetir pruebas y compilación para ambas placas.

RESUELTO tras autorización de Genrikh: el diseño y el sketch utilizan la API pública `beginPacket()`, el doble ya no expone `isTransmitting()` y las pruebas comprueban que consultar radio ocupada no modifica FIFO ni consume secuencia. Volvieron a pasar las pruebas nativas y la integración del frontend, y ambos sketches compilaron con el core y la biblioteca reales. Este hallazgo ya no bloquea continuar apply; las tareas restantes de entrega siguen su seguimiento en tasks.md. Carga y recepción física permanecen pendientes.
