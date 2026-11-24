#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>
#include <deque>
#include <map>

struct TramaPrueba {
  std::string bytes;
  int rssi;
  float snr;
  int declarados;
};
struct LoRaPrueba {
  std::deque<TramaPrueba> cola;
  TramaPrueba actual{"", 0, 0, 0};
  size_t pos = 0;
  std::map<std::string, long> parametros;
  unsigned inicios = 0, configuraciones = 0, preparados = 0, envios = 0, abortos = 0;
  unsigned fallosInicio = 0;
  bool prepara = true, parcial = false, aceptaEnvio = true, ocupada = false, asincrono = false;
  std::string escrito;
  std::vector<std::string> enviados;
  void setPins(int ss, int rst, int dio) {
    parametros["ss"] = ss; parametros["rst"] = rst; parametros["dio"] = dio;
  }
  int begin(long hz) {
    ++inicios; parametros["hz"] = hz;
    if (fallosInicio) { --fallosInicio; return 0; }
    ocupada = false;
    return 1;
  }
  void setSpreadingFactor(int v) { parametros["sf"] = v; ++configuraciones; }
  void setSignalBandwidth(long v) { parametros["bw"] = v; ++configuraciones; }
  void setCodingRate4(int v) { parametros["cr"] = v; ++configuraciones; }
  void setSyncWord(int v) { parametros["sync"] = v; ++configuraciones; }
  void setPreambleLength(long v) { parametros["preamble"] = v; ++configuraciones; }
  void enableCrc() { parametros["crc"] = 1; ++configuraciones; }
  void disableInvertIQ() { parametros["iq"] = 0; ++configuraciones; }
  void setTxPower(int v) { parametros["power"] = v; ++configuraciones; }
  int beginPacket() {
    ++preparados;
    // Igual que la API pública real: no tocar FIFO cuando sigue transmitiendo.
    if (ocupada || !prepara) return 0;
    escrito.clear();
    return 1;
  }
  size_t write(const uint8_t* data, size_t n) {
    const size_t acepta = parcial && n ? n - 1 : n;
    escrito.assign(reinterpret_cast<const char*>(data), acepta);
    return acepta;
  }
  int endPacket(bool async = false) {
    ++envios; asincrono = async;
    if (!aceptaEnvio) return 0;
    enviados.push_back(escrito); ocupada = true; return 1;
  }
  void idle() { ++abortos; ocupada = false; }
  void push(std::string bytes, int rssi = -75, float snr = 8.5f, int declarados = -1) {
    if (declarados < 0) declarados = static_cast<int>(bytes.size());
    cola.push_back({bytes, rssi, snr, declarados});
  }
  int parsePacket() {
    if (cola.empty()) return 0;
    actual = cola.front(); cola.pop_front(); pos = 0;
    return actual.declarados;
  }
  int available() { return pos < actual.bytes.size(); }
  int read() { return available() ? static_cast<unsigned char>(actual.bytes[pos++]) : -1; }
  int packetRssi() { return actual.rssi; }
  float packetSnr() { return actual.snr; }
};
static LoRaPrueba LoRa;
