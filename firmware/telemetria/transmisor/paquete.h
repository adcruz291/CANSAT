#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>

// Telemetría v2: orden idéntico a los primeros 15 campos de F del frontend.
struct Medidas {
  // La estación calcula las cuatro fases con alt; por defecto enviar st=0.
  uint8_t st = 0;
  float pres = -1, temp = -1, alt = -1;
  float ax = -1, ay = -1, az = -1;
  float roll = -1, pitch = -1, yaw = -1;
  uint8_t flags = 0;
  float sd = -1, sb = -1; // Reservados en v2: siempre -1.
};

inline Medidas sanearMedidas(Medidas d) {
  float* campos[] = {&d.pres,&d.temp,&d.alt,&d.ax,&d.ay,&d.az,&d.roll,&d.pitch,&d.yaw};
  for (float* campo : campos) if (!isfinite(*campo)) *campo = -1;
  return d; // Metadatos y reservados siguen sujetos a validación estricta.
}

inline int crearPaquete(char* out, size_t capacity, uint32_t seq,
                        uint32_t ms, const Medidas& d) {
  if (!out || capacity == 0) return -1;
  out[0] = '\0';
  const float values[] = {d.pres,d.temp,d.alt,d.ax,d.ay,d.az,d.roll,d.pitch,d.yaw,d.sd,d.sb};
  for (float value : values) if (!isfinite(value)) return -1;
  if (d.st > 3 || d.flags > 3 || d.sd != -1 || d.sb != -1) return -1;
  const int n = snprintf(out, capacity,
      "$%lu,%lu,%u,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%u,%.2f,%.2f",
      (unsigned long)seq, (unsigned long)ms, (unsigned)d.st,
      d.pres,d.temp,d.alt,d.ax,d.ay,d.az,d.roll,d.pitch,d.yaw,
      (unsigned)d.flags,d.sd,d.sb);
  // Mismos límites que contrato.h: máximo 255 bytes y 31 por token.
  bool valido = n > 0 && (size_t)n < capacity && n <= 255;
  size_t token = 0;
  if (valido) for (int i = 1; i < n; ++i) {
    if (out[i] == ',') token = 0;
    else if (++token > 31) { valido = false; break; }
  }
  if (!valido) { out[0] = '\0'; return -1; }
  return n;
}
