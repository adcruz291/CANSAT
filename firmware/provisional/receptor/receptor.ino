#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// Programa provisional proporcionado por Genrikh. No es telemetría de misión.
// Pines del modulo LoRa RA-02 (SX1278)
#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS     5
#define LORA_RST   14
#define LORA_DIO0  26

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Iniciando receptor LoRa...");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("ERROR: No se pudo iniciar LoRa");
    while (true) {
      delay(1000);
    }
  }
  Serial.println("Receptor LoRa listo en 433 MHz");
}

void loop() {
  int paquete = LoRa.parsePacket();
  if (paquete) {
    Serial.print("Mensaje recibido: ");
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }
    Serial.print(" | RSSI: ");
    Serial.print(LoRa.packetRssi());
    Serial.println(" dBm");
  }
}
