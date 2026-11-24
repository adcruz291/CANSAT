#include "../firmware/telemetria/transmisor/transmisor.ino"
#include "fixture_sensores.h"
#include <assert.h>
#include <iostream>

void inicio(uint8_t b = 0x76, uint8_t m = 0x68) {
  prepararSensores(b, m); sensores.iniciar(21,22);
  Serial.salida.clear();
  const auto vacia = sensores.leer(0);
  assert(vacia.pres == -1 && vacia.ax == -1); // Esperar la primera conversión, sin bloquear.
}
void cerca(double a, double b, double error = 0.001) { assert(fabs(a-b) < error); }

int main(int argc, char**) {
  inicio();
  auto d = sensores.leer(200);
  cerca(d.temp,25.08248); cerca(d.pres,1006.53267);
  cerca(d.alt,56.076,0.1); // Referencia 1013.25; no se pone en cero a bordo.
  cerca(d.ax,0); cerca(d.ay,0); cerca(d.az,1);
  cerca(d.roll,0); cerca(d.pitch,0);
  assert(d.yaw == -1 && d.st == 0 && d.flags == 0 && d.sd == -1 && d.sb == -1);
  assert(Wire.sda == 21 && Wire.scl == 22 && Wire.frecuencia == 100000 && Wire.timeout == 20);
  assert(Wire.memoria[0x76][0xF4] == 0x4F);
  assert(Wire.memoria[0x68][0x1C] == 0x18 && Wire.memoria[0x68][0x1B] == 0x18);

  inicio(0x77,0x69); d = sensores.leer(200);
  assert(d.pres > 0 && d.az == 1); // Direcciones alternas e ID independiente de AD0.
  Wire.cortaReg = 0xF7; d = sensores.leer(400);
  assert(d.pres == -1 && d.temp == -1 && d.alt == -1 && d.az == 1);
  const unsigned antes = Wire.consultas;
  Wire.cortaReg = -1; d = sensores.leer(600);
  assert(d.pres == -1 && Wire.consultas-antes == 2); // Solo lee el MPU, sin sondeo prematuro BME.
  sensores.leer(2400); d = sensores.leer(2600); assert(d.pres > 0);

  // Lectura corta de IMU no devuelve valores anteriores ni afecta al barómetro.
  Wire.cortaReg = 0x3B; d = sensores.leer(2800);
  assert(d.ax == -1 && d.ay == -1 && d.az == -1 && d.roll == -1 && d.pres > 0);
  Wire.cortaReg = -1; sensores.leer(4800); d = sensores.leer(5000); assert(d.az == 1);

  inicio(); auto& m = Wire.memoria[0x68];
  poner16be(m,0x3D,1448); poner16be(m,0x3F,1448); d = sensores.leer(200);
  cerca(d.roll,45,0.1); cerca(d.pitch,0);
  poner16be(m,0x3B,-1024); poner16be(m,0x3D,0); poner16be(m,0x3F,1774);
  d = sensores.leer(400); cerca(d.ax,-0.5); cerca(d.pitch,30,0.1);
  poner16be(m,0x3B,0); poner16be(m,0x3F,0); d = sensores.leer(600);
  assert(d.az == 0 && d.roll == -1 && d.pitch == -1); // Caída libre no es actitud válida.
  poner16be(m,0x3F,2048); poner16be(m,0x43,1640); d = sensores.leer(800);
  assert(d.roll == -1 && d.pitch == -1 && d.az == 1); // Giro fuerte, orientación no fiable.
  poner16be(m,0x3B,32767); d = sensores.leer(1000); assert(d.ax == -1 && d.az == -1);

  inicio(); Wire.memoria[0x76][0xD0] = 0x58; // BMP280 no se anuncia como BME280.
  sensores.iniciar(21,22); sensores.leer(0); d = sensores.leer(200);
  assert(d.pres == -1 && d.az == 1);
  inicio(); poner16le(Wire.memoria[0x76],0x8E,0); sensores.iniciar(21,22);
  sensores.leer(0); d = sensores.leer(200); assert(d.pres == -1 && d.az == 1);

  Wire = TwoWire{}; sensores.iniciar(21,22); sensores.leer(0); d = sensores.leer(200);
  assert(d.pres == -1 && d.az == -1); // Ausentes: no bloquear la radio.
  prepararSensores(); sensores.leer(2000); d = sensores.leer(2200); assert(d.pres > 0 && d.az == 1);
  Wire = TwoWire{}; Wire.inicia = false; sensores.iniciar(21,22);
  d = sensores.leer(0); assert(d.pres == -1);
  prepararSensores(); d = sensores.leer(1999); assert(d.pres == -1);
  sensores.leer(2000); d = sensores.leer(2200); assert(d.pres > 0);

  // El sketch completo emite la lectura real decodificable, no solo el auxiliar.
  prepararSensores(); ciclo = CicloTx{}; LoRa = LoRaPrueba{}; relojPrueba = 0;
  setup(); loop(); loop(); LoRa.ocupada = false; relojPrueba += 200; loop(); loop();
  assert(LoRa.enviados.back().find(",1006.53,25.08,") != std::string::npos);
  if (argc > 1) std::cout << LoRa.enviados.back() << '\n';
  else std::cout << "BME280/MPU6050: compensacion, unidades, inclinacion, fallos y recuperacion OK\n";
}
