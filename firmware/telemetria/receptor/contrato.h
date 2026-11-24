#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Valida una trama LoRa v2 de 15 campos, sin salto de línea.
// Se acota cada token para evitar memoria dinámica y lecturas fuera del buffer.
inline bool paqueteValido(const char* line, size_t length) {
  if (!line || length < 2 || length > 255 || line[0] != '$') return false;
  size_t pos = 1;
  for (unsigned field = 0; field < 15; ++field) {
    const size_t start = pos;
    while (pos < length && line[pos] != ',') {
      const char c = line[pos++];
      if (!((c >= '0' && c <= '9') || c == '+' || c == '-' ||
            c == '.' || c == 'e' || c == 'E')) return false;
    }
    const size_t count = pos - start;
    if (count == 0 || count >= 32) return false;
    char token[32];
    memcpy(token, line + start, count);
    token[count] = '\0';
    char* end = nullptr;
    const double value = strtod(token, &end);
    if (end == token || *end != '\0' || !isfinite(value)) return false;
    if (field == 0 || field == 1 || field == 2 || field == 12) {
      const double maximum = (field == 2 || field == 12) ? 3 : 4294967295.0;
      if (value < 0 || value > maximum || floor(value) != value) return false;
    }
    // Misión v2: sd/sb ya no son localización; ambos están reservados en -1.
    if ((field == 13 || field == 14) && value != -1) return false;
    if (field < 14) {
      if (pos >= length || line[pos] != ',') return false;
      ++pos;
    } else if (pos != length) return false;
  }
  return true;
}
