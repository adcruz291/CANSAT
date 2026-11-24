#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "paquete.h"
#include "ciclo.h"
#include "sensores.h"

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
#define SENSOR_SDA 21       // VERIFICAR: confirmado por Genrikh
#define SENSOR_SCL 22       // VERIFICAR: confirmado por Genrikh
#define BME_ADDR_1 0x76     // VERIFICAR: se identifica el chip en ambas direcciones
#define BME_ADDR_2 0x77     // VERIFICAR
#define MPU_ADDR_1 0x68     // VERIFICAR
#define MPU_ADDR_2 0x69     // VERIFICAR
CicloTx ciclo;
SensoresCanSat sensores(Wire, BME_ADDR_1, BME_ADDR_2, MPU_ADDR_1, MPU_ADDR_2);

Medidas leerMedidas() {
  return sensores.leer(millis());
}

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
  LoRa.setTxPower(RADIO_TX_DBM);
  Serial.println("# Radio TX lista: 433 MHz, SF7, BW250, CRC; BME280 y MPU6050.");
  return true;
}

void recuperarRadio(uint32_t ahora, const char* motivo) {
  LoRa.idle();
  Serial.println(motivo);
  ciclo.recuperar(ahora);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("# CanSAT v2: BME280 + MPU6050; fase y cero calculados en tierra.");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  sensores.iniciar(SENSOR_SDA, SENSOR_SCL);
}

void loop() {
  const uint32_t ahora = millis();
  if (ciclo.tocaInicializar(ahora)) {
    if (iniciarRadio()) ciclo.disponible(millis());
    else ciclo.recuperar(millis());
    return;
  }
  if (ciclo.estado == CicloTx::TRANSMITIENDO) {
    // API pública de LoRa 0.8.0: retorna 0 mientras TX sigue ocupada.
    // Al retornar 1 deja la FIFO vacía/preparada, sin iniciar otro envío.
    // La próxima muestra vuelve a llamar beginPacket() antes de escribir.
    // Prioridad de disponibilidad observada sobre timeout en el mismo turno.
    if (LoRa.beginPacket()) {
      ciclo.estado = CicloTx::DISPONIBLE;
      Serial.println("# TX finalizada localmente; sin acuse de recepcion.");
    } else if (ciclo.vencido(ahora)) {
      recuperarRadio(ahora, "# ERROR: timeout TX (1000 ms); recuperando radio.");
    }
    return;
  }
  if (!ciclo.tocaIntento(ahora)) return;
  // Sin ráfagas ni retransmisión de paquetes fallidos.
  const Medidas d = sanearMedidas(leerMedidas());
  char paquete[256];
  // Cada intento consume secuencia, incluso si falla localmente.
  const int n = crearPaquete(paquete, sizeof(paquete), ciclo.preparar(ahora), ahora, d);
  if (n < 0) {
    Serial.println("# ERROR: medidas invalidas o paquete demasiado largo.");
    return;
  }
  if (!LoRa.beginPacket()) {
    recuperarRadio(millis(), "# ERROR: fallo al preparar TX; recuperando radio.");
    return;
  }
  const size_t escritos = LoRa.write((const uint8_t*)paquete, (size_t)n);
  if (escritos != (size_t)n) {
    recuperarRadio(millis(), "# ERROR: escritura incompleta; recuperando radio sin transmitir parcial.");
    return;
  }
  if (LoRa.endPacket(true) != 1) {
    recuperarRadio(millis(), "# ERROR: fallo al iniciar TX asincrona; recuperando radio.");
    return;
  }
  ciclo.transmitiendo(millis());
  // Diagnóstico local de 15 campos; el dashboard debe conectarse al RECEPTOR.
  Serial.print("# TX ");
  Serial.println(paquete);
}
