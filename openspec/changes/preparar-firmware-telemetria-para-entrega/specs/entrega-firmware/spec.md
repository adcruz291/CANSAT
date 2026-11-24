# Spec Delta

## Purpose

Permitir que otro integrante reciba y compile una versión identificable de ambos programas ESP32, con instrucciones suficientes y evidencia honesta de su estado de verificación.

## ADDED Requirements

### Requirement: Paquete completo e identificable
La entrega SHALL incluir ambos sketches y encabezados, frontend compatible, contrato v2, guía, verificador y fuentes de todas las pruebas exigidas. Un manifiesto SHALL enumerar huellas SHA-256 y versiones de compilación. SHALL excluir archivos temporales, credenciales y capturas presentadas como reales sin evidencia.

#### Scenario: El compañero extrae la entrega
- **WHEN** se extrae el paquete en una carpeta nueva
- **THEN** las carpetas de ambos sketches se pueden abrir en Arduino IDE sin copiar archivos desde ubicaciones externas y las huellas coinciden con el manifiesto

### Requirement: Verificación reproducible sin placa
La entrega SHALL registrar la compilación de ambos sketches para ESP32 Dev Module y las pruebas de contrato, recuperación e integración con el frontend. SHALL incluir comandos, versiones, resultados y limitaciones. Si falta una dependencia o falla una verificación, SHALL indicarse sin marcarla aprobada.

#### Scenario: Se verifican los programas en computadora
- **WHEN** las herramientas y bibliotecas de las versiones documentadas están disponibles
- **THEN** se compilan ambos sketches sin cargar una placa y se ejecutan pruebas automatizadas de entradas inválidas, temporización, fallos de radio y vuelo artificial

#### Scenario: Verificación desde la entrega extraída
- **WHEN** el paquete se extrae en una carpeta nueva con las dependencias documentadas disponibles y sin acceso a la carpeta original del proyecto
- **THEN** su verificador puede repetir compilaciones y pruebas utilizando los archivos extraídos, sin rutas absolutas al equipo de Genrikh; un error o dependencia ausente devuelve un resultado no satisfactorio y no se declara la entrega verificada

### Requirement: Guía de instalación y diagnóstico
La guía SHALL identificar transmisor y receptor, sus GPIO, placa, biblioteca, archivos necesarios, parámetros de radio y velocidad serie. SHALL explicar BOOT/EN y cómo recopilar el error de carga; SHALL separar la conexión al cargador del ESP32 de la inicialización del LoRa.

#### Scenario: La carga falla sin recibir datos serie
- **WHEN** el compañero encuentra `No serial data received`
- **THEN** la guía ofrece comprobaciones de puerto, cable de datos y modo BOOT/EN, y solicita el registro completo sin afirmar que el firmware resuelve el problema físico

### Requirement: Evidencia física pendiente explícita
La entrega SHALL declarar que sensores, cableado físico, carga, enlace LoRa, alcance y vuelo están pendientes hasta disponer de evidencia. SHALL identificar cualquier dato artificial y explicar que las lecturas actuales de sensores son `-1`.

#### Scenario: Hay compilación correcta pero no se pudo cargar
- **WHEN** el informe se prepara sin acceso funcional a la placa
- **THEN** registra compilación y pruebas de computadora como tales, y conserva carga y recepción real como pendientes

### Requirement: Distribución desde una fuente vigente
El paquete SHALL generarse desde una lista explícita de archivos actuales y superar una comprobación de integridad tras extraerlo. SHALL conservar intactos los sketches provisionales y cualquier ZIP local preexistente.

#### Scenario: Existe un ZIP antiguo del transmisor
- **WHEN** se prepara la nueva entrega con ambos roles
- **THEN** se genera un paquete separado con sus huellas actuales y no se reutiliza ni sobrescribe silenciosamente el ZIP antiguo
