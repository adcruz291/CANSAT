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

unsigned long contador = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Iniciando transmisor LoRa...");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("ERROR: no se pudo iniciar el RA-02. Revisa conexiones y alimentacion.");
    while (true) {
      delay(1000);
    }
  }
  LoRa.setTxPower(17);
  Serial.println("Transmisor LoRa listo en 433 MHz.");
}

void loop() {
  String mensaje = "CANSAT,PRUEBA," + String(contador);
  Serial.print("Enviando: ");
  Serial.println(mensaje);
  LoRa.beginPacket();
  LoRa.print(mensaje);
  int resultado = LoRa.endPacket();
  if (resultado == 1) {
    Serial.println("Paquete enviado correctamente.\n");
  } else {
    Serial.println("No se pudo completar el envio.\n");
  }
  contador++;
  delay(1000);
}
