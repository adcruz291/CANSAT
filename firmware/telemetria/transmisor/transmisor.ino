#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "paquete.h"

// VERIFICAR: mismos GPIO que el transmisor provisional proporcionado por Genrikh.
#define LORA_SCK 18   // VERIFICAR
#define LORA_MISO 19  // VERIFICAR
#define LORA_MOSI 23  // VERIFICAR
#define LORA_SS 5     // VERIFICAR
#define LORA_RST 14   // VERIFICAR
#define LORA_DIO0 26  // VERIFICAR
#define RADIO_HZ 433000000L
#define RADIO_SF 7
#define RADIO_BW 250000L
#define RADIO_CR 5
#define RADIO_SYNC 0x12
#define RADIO_PREAMBLE 8
#define RADIO_TX_DBM 17
#define PERIODO_MS 200U

uint32_t secuencia = 0;
uint32_t ultimoEnvio = 0;

Medidas leerMedidas() {
  // PENDIENTE: integrar únicamente los sensores reales y sus direcciones/pines.
  // Sin sensores: -1 significa sin dato, no una medición.
  // st=0 y flags=0: no inferir separación ni aterrizaje sin criterios confirmados.
  // sd y sb siguen en -1 hasta definir la localización de la carga secundaria.
  return Medidas{};
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("# CanSAT: telemetria base SIN sensores; conectar antena antes de encender.");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(RADIO_HZ)) {
    Serial.println("# ERROR: no se pudo iniciar LoRa. Revisar cableado y alimentacion.");
    while (true) delay(1000);
  }
  LoRa.setSpreadingFactor(RADIO_SF);
  LoRa.setSignalBandwidth(RADIO_BW);
  LoRa.setCodingRate4(RADIO_CR);
  LoRa.setSyncWord(RADIO_SYNC);
  LoRa.setPreambleLength(RADIO_PREAMBLE);
  LoRa.enableCrc();
  LoRa.disableInvertIQ();
  LoRa.setTxPower(RADIO_TX_DBM);
  ultimoEnvio = millis() - PERIODO_MS;
}

void loop() {
  const uint32_t ahora = millis();
  if ((uint32_t)(ahora - ultimoEnvio) < PERIODO_MS) return;
  // Sin ráfagas para recuperar intervalos atrasados. Sigue funcionando al wrap de millis.
  ultimoEnvio = ahora;
  const Medidas d = leerMedidas();
  char paquete[256];
  // Cada intento consume secuencia, incluso si falla localmente.
  const int n = crearPaquete(paquete, sizeof(paquete), secuencia++, ahora, d);
  if (n < 0) {
    Serial.println("# ERROR: medidas invalidas o paquete demasiado largo.");
    return;
  }
  if (!LoRa.beginPacket()) {
    Serial.println("# ERROR: radio no disponible para transmitir.");
    return;
  }
  const size_t escritos = LoRa.write((const uint8_t*)paquete, (size_t)n);
  if (escritos != (size_t)n) {
    LoRa.idle();
    Serial.println("# ERROR: escritura incompleta; paquete descartado.");
    return;
  }
  if (LoRa.endPacket() != 1) {
    Serial.println("# ERROR: no se completo el envio.");
    return;
  }
  // Diagnóstico local de 15 campos; el dashboard debe conectarse al RECEPTOR.
  Serial.print("# TX ");
  Serial.println(paquete);
}
