#pragma once
#include <stdint.h>
#include <stddef.h>
#include <map>
#include <vector>

struct TwoWire {
  std::map<uint8_t, std::vector<uint8_t>> memoria;
  std::vector<uint8_t> tx, rx;
  uint8_t direccion = 0, registro = 0;
  size_t pos = 0;
  int sda = -1, scl = -1, timeout = 0, cortaReg = -1;
  bool inicia = true, fallaEscritura = false;
  unsigned long frecuencia = 0;
  unsigned consultas = 0;
  bool begin(int a, int b, uint32_t hz) { sda = a; scl = b; frecuencia = hz; return inicia; }
  void setTimeOut(uint16_t value) { timeout = value; }
  void beginTransmission(uint8_t addr) { direccion = addr; tx.clear(); }
  size_t write(uint8_t value) { tx.push_back(value); return 1; }
  uint8_t endTransmission(bool = true) {
    if (!memoria.count(direccion) || fallaEscritura || tx.empty()) return 2;
    registro = tx[0];
    for (size_t i = 1; i < tx.size(); ++i) memoria[direccion][uint8_t(registro+i-1)] = tx[i];
    return 0;
  }
  size_t requestFrom(uint8_t addr, size_t n) {
    ++consultas; rx.clear(); pos = 0;
    if (!memoria.count(addr)) return 0;
    const size_t count = registro == cortaReg && n ? n-1 : n;
    for (size_t i = 0; i < count; ++i) rx.push_back(memoria[addr][uint8_t(registro+i)]);
    return count;
  }
  int read() { return pos < rx.size() ? rx[pos++] : -1; }
};
static TwoWire Wire;
