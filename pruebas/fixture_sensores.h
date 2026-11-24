#pragma once
#include <Wire.h>

inline void poner16le(std::vector<uint8_t>& m, unsigned reg, int value) {
  m[reg] = uint16_t(value) & 255; m[reg+1] = uint16_t(value) >> 8;
}
inline void poner16be(std::vector<uint8_t>& m, unsigned reg, int value) {
  m[reg] = uint16_t(value) >> 8; m[reg+1] = uint16_t(value) & 255;
}
inline void prepararSensores(uint8_t bmeAddr = 0x76, uint8_t mpuAddr = 0x68) {
  Wire = TwoWire{};
  auto& b = Wire.memoria[bmeAddr]; b.resize(256);
  b[0xD0] = 0x60;
  const int coef[] = {27504,26435,-1000,36477,-10685,3024,2855,140,-7,15500,-14600,6000};
  for (unsigned i = 0; i < 12; ++i) poner16le(b, 0x88+2*i, coef[i]);
  const unsigned ap = 415148, at = 519888;
  b[0xF7] = ap >> 12; b[0xF8] = (ap >> 4) & 255; b[0xF9] = (ap & 15) << 4;
  b[0xFA] = at >> 12; b[0xFB] = (at >> 4) & 255; b[0xFC] = (at & 15) << 4;
  auto& m = Wire.memoria[mpuAddr]; m.resize(256); m[0x75] = 0x68; m[0x3A] = 1;
  poner16be(m, 0x3F, 2048); // Z=+1g, no implica que estos valores existan en el circuito.
}
