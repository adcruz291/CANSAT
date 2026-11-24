#pragma once
#include <stdint.h>

struct CicloRx {
  static const uint32_t REINTENTO_MS = 2000;
  bool lista = false, primerInicio = true;
  uint32_t fallo = 0;
  bool tocaInicializar(uint32_t now) const {
    return !lista && (primerInicio || uint32_t(now-fallo) >= REINTENTO_MS);
  }
  void resultado(bool ok, uint32_t now) { lista = ok; primerInicio = false; fallo = now; }
};
