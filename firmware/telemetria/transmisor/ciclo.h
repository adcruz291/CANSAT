#pragma once
#include <stdint.h>

// Estado independiente de Arduino; millis() se pasa explícitamente para probar wrap.
struct CicloTx {
  enum Estado { PENDIENTE, DISPONIBLE, TRANSMITIENDO } estado = PENDIENTE;
  static const uint32_t PERIODO_MS = 200, TIMEOUT_MS = 1000, REINTENTO_MS = 2000;
  uint32_t secuencia = 0, ultimoIntento = 0, inicioTx = 0, recuperacion = 0;
  bool primerInicio = true;
  bool tocaInicializar(uint32_t now) const {
    return estado == PENDIENTE && (primerInicio || uint32_t(now-recuperacion) >= REINTENTO_MS);
  }
  void recuperar(uint32_t now) {
    estado = PENDIENTE; recuperacion = now; primerInicio = false;
  }
  void disponible(uint32_t now) { estado = DISPONIBLE; ultimoIntento = now-PERIODO_MS; }
  bool tocaIntento(uint32_t now) const {
    return estado == DISPONIBLE && uint32_t(now-ultimoIntento) >= PERIODO_MS;
  }
  uint32_t preparar(uint32_t now) { ultimoIntento = now; return secuencia++; }
  void transmitiendo(uint32_t now) { estado = TRANSMITIENDO; inicioTx = now; }
  bool vencido(uint32_t now) const { return uint32_t(now-inicioTx) >= TIMEOUT_MS; }
};
