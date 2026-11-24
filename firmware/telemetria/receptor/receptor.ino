#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "contrato.h"
#include "ciclo.h"

#define LORA_SCK 18   // VERIFICAR: confirmado por Genrikh para estación terrena
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

CicloRx ciclo;

bool iniciarRadio() {
  if (!LoRa.begin(RADIO_HZ)) {
    Serial.println("# ERROR: no se pudo iniciar LoRa; reintento en 2000 ms.");
    return false;
  }
  LoRa.setSpreadingFactor(RADIO_SF);
  LoRa.setSignalBandwidth(RADIO_BW);
  LoRa.setCodingRate4(RADIO_CR);
  LoRa.setSyncWord(RADIO_SYNC);
  LoRa.setPreambleLength(RADIO_PREAMBLE);
  LoRa.enableCrc();
  LoRa.disableInvertIQ();
  Serial.println("# Receptor v2 listo: 433 MHz, SF7, BW250, CRC; USB 115200.");
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
}

void loop() {
  if (ciclo.tocaInicializar(millis())) {
    const bool ok = iniciarRadio();
    ciclo.resultado(ok, millis());
    return;
  }
  if (!ciclo.lista) return;
  const int expected = LoRa.parsePacket();
  if (expected <= 0) return;
  char paquete[256];
  size_t n = 0;
  bool overflow = false;
  while (LoRa.available()) {
    const int c = LoRa.read();
    if (c < 0) break;
    if (n < sizeof(paquete) - 1) paquete[n++] = (char)c;
    else overflow = true;
  }
  paquete[n] = '\0';
  if (overflow || n != (size_t)expected || !paqueteValido(paquete, n)) {
    Serial.println("# Paquete descartado: formato distinto de telemetria v2.");
    return;
  }
  const float snr = LoRa.packetSnr();
  if (!isfinite(snr)) return;
  // Única línea de datos: exactamente los 17 campos del arreglo F del HTML.
  Serial.print(paquete);
  Serial.print(',');
  Serial.print(LoRa.packetRssi());
  Serial.print(',');
  Serial.println(snr, 2);
}
