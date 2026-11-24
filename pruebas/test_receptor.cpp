// Compila y ejecuta el sketch real, sustituyendo únicamente Arduino/SPI/LoRa.
#include "../firmware/telemetria/receptor/receptor.ino"
#include "../firmware/telemetria/transmisor/paquete.h"
#include <assert.h>
#include <iostream>
#include <limits>

std::string muestra(uint32_t seq, uint32_t ms) {
  char packet[256];
  int n = crearPaquete(packet, sizeof(packet), seq, ms, Medidas{});
  assert(n > 0);
  return packet;
}
int main(int argc, char**) {
  LoRa.fallosInicio = 2;
  setup();
  assert(Serial.baud == 115200);
  loop(); assert(LoRa.inicios == 1 && !ciclo.lista);
  relojPrueba += 1999; loop(); assert(LoRa.inicios == 1);
  relojPrueba += 1; loop(); assert(LoRa.inicios == 2 && !ciclo.lista);
  relojPrueba += 1999; loop(); assert(LoRa.inicios == 2);
  relojPrueba += 1; loop(); assert(LoRa.inicios == 3 && ciclo.lista);
  assert(LoRa.configuraciones == 7);
  assert(LoRa.parametros["hz"] == 433000000 && LoRa.parametros["sf"] == 7);
  assert(LoRa.parametros["bw"] == 250000 && LoRa.parametros["cr"] == 5);
  assert(LoRa.parametros["sync"] == 0x12 && LoRa.parametros["preamble"] == 8);
  assert(LoRa.parametros["crc"] == 1 && LoRa.parametros["iq"] == 0);
  Serial.salida.clear();
  const std::string a = muestra(42, 8400), b = muestra(43, 8600);
  std::vector<std::string> invalidas = {"CANSAT,PRUEBA,0", "$1,2,3", a + ",0",
    "$0,0,4,1,1,1,1,1,1,1,1,1,0,-1,-1",
    "$0,0,0,1,,1,1,1,1,1,1,1,0,-1,-1",
    "$0,0,0,1,1,1,1,1,1,1,1,1,4,-1,-1", a.substr(0, a.rfind(','))};
  std::string nul = a; nul[5] = '\0'; invalidas.push_back(nul);
  invalidas.push_back(std::string(256, '9'));
  for (const auto& value : invalidas) { LoRa.push(value); loop(); }
  LoRa.push(a, -75, std::numeric_limits<float>::infinity()); loop();
  LoRa.push(a, -75, 8.5, static_cast<int>(a.size())+1); loop();
  assert(Serial.salida.find('$') == std::string::npos);
  Serial.salida.clear();
  LoRa.push(a, -75, 8.5); loop();
  LoRa.push(b, -91, -2.25); loop();
  const std::string expected = a + ",-75,8.50\n" + b + ",-91,-2.25\n";
  assert(Serial.salida == expected);
  const std::string capturada = Serial.salida;
  // Reintento a través del desbordamiento de millis.
  ciclo = CicloRx{}; LoRa.fallosInicio = 1; relojPrueba = UINT32_MAX-100;
  loop(); const unsigned inicios = LoRa.inicios;
  relojPrueba += 1999; loop(); assert(LoRa.inicios == inicios);
  relojPrueba += 1; loop(); assert(LoRa.inicios == inicios+1 && ciclo.lista);
  if (argc > 1) std::cout << capturada;
  else std::cout << "Receptor real con dobles: rechazo, recuperacion y salida USB exacta OK\n";
}
