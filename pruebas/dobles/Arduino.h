#pragma once
// Dobles exclusivos de pruebas nativas. Nunca se distribuyen dentro del sketch.
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <sstream>
#include <iomanip>

static uint32_t relojPrueba = 0;
inline uint32_t millis() { return relojPrueba; }
inline void delay(uint32_t ms) { relojPrueba += ms; }
struct SerialPrueba {
  std::string salida;
  unsigned baud = 0;
  void begin(unsigned value) { baud = value; }
  template<typename T> void print(const T& value) {
    std::ostringstream s; s << value; salida += s.str();
  }
  template<typename T> void println(const T& value) { print(value); salida += '\n'; }
  void println(float value, int digits) {
    std::ostringstream s; s << std::fixed << std::setprecision(digits) << value;
    salida += s.str() + '\n';
  }
};
static SerialPrueba Serial;
