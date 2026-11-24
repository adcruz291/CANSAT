#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>

// Orden idéntico a los primeros 15 campos de F en estacion_terrena.html.
struct Medidas {
  uint8_t st = 0;
  float pres = -1, temp = -1, alt = -1;
  float ax = -1, ay = -1, az = -1;
  float roll = -1, pitch = -1, yaw = -1;
  uint8_t flags = 0;
  float sd = -1, sb = -1;
};

inline int crearPaquete(char* out, size_t capacity, uint32_t seq,
                        uint32_t ms, const Medidas& d) {
  const float values[] = {d.pres,d.temp,d.alt,d.ax,d.ay,d.az,d.roll,d.pitch,d.yaw,d.sd,d.sb};
  for (float value : values) if (!isfinite(value)) return -1;
  if (d.st > 5 || d.flags > 15 || (d.sd != -1 && d.sd < 0) ||
      (d.sb != -1 && (d.sb < 0 || d.sb >= 360))) return -1;
  // El redondeo de 359.999° no debe producir el rumbo inválido 360.00°.
  const double rumbo = d.sb == -1 ? -1 : fmod(round((double)d.sb * 100) / 100, 360);
  const int n = snprintf(out, capacity,
      "$%lu,%lu,%u,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%u,%.2f,%.2f",
      (unsigned long)seq, (unsigned long)ms, (unsigned)d.st,
      d.pres,d.temp,d.alt,d.ax,d.ay,d.az,d.roll,d.pitch,d.yaw,
      (unsigned)d.flags,d.sd,rumbo);
  // El RA-02 admite como máximo 255 bytes por paquete.
  return n > 0 && (size_t)n < capacity && n <= 255 ? n : -1;
}
