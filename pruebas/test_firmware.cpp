#include "../firmware/telemetria/transmisor/paquete.h"
#include "../firmware/telemetria/receptor/contrato.h"
#include <assert.h>
#include <iostream>
#include <limits>
#include <string>

int main(int argc, char**) {
  Medidas d;
  char buffer[256];
  int n = crearPaquete(buffer, sizeof(buffer), 42, 8400, d);
  assert(n > 0 && paqueteValido(buffer, (size_t)n));
  const std::string missing(buffer);
  assert(missing == "$42,8400,0,-1.00,-1.00,-1.00,-1.00,-1.00,-1.00,-1.00,-1.00,-1.00,0,-1.00,-1.00");
  if (argc > 1) {
    // Emula exclusivamente los campos de radio para comprobar formato en tierra.
    std::cout << buffer << ",-75,8.50\n";
    return 0;
  }
  d.st = 4; d.flags = 3; d.pres = 990; d.temp = -10;
  d.alt = 100; d.ax = 0; d.ay = -1; d.az = 0;
  d.roll = -30; d.pitch = 15; d.yaw = 359; d.sd = 12; d.sb = 359.999f;
  n = crearPaquete(buffer, sizeof(buffer), UINT32_MAX, UINT32_MAX, d);
  assert(n > 0 && paqueteValido(buffer, n));
  assert(crearPaquete(buffer, 8, 0, 0, d) == -1);
  d.temp = std::numeric_limits<float>::infinity();
  assert(crearPaquete(buffer, sizeof(buffer), 0, 0, d) == -1);
  const char* bad[] = {
    "CANSAT,PRUEBA,0", "$1,2,3", "", "$",
    "$0,0,0,1,,1,1,1,1,1,1,1,0,-1,-1",
    "$0,0,6,1,1,1,1,1,1,1,1,1,0,-1,-1",
    "$0,0,0,1,1,1,1,1,1,1,1,1,16,-1,-1",
    "$0,0,0,nan,1,1,1,1,1,1,1,1,0,-1,-1",
    "$0,0,0,1e999,1,1,1,1,1,1,1,1,0,-1,-1",
    "$0,0,0, 1,1,1,1,1,1,1,1,1,0,-1,-1",
    "$0,0,0,0x1,1,1,1,1,1,1,1,1,0,-1,-1",
    "$0.5,0,0,1,1,1,1,1,1,1,1,1,0,-1,-1",
    "$4294967296,0,0,1,1,1,1,1,1,1,1,1,0,-1,-1",
    "$0,0,0,1,1,1,1,1,1,1,1,1,0,-2,-1",
    "$0,0,0,1,1,1,1,1,1,1,1,1,0,-1,360"
  };
  for (const char* value : bad) assert(!paqueteValido(value, strlen(value)));
  assert(!paqueteValido((missing + ",0").c_str(), missing.size() + 2));
  assert(!paqueteValido((missing + "\n").c_str(), missing.size() + 1));
  std::string embedded(missing); embedded[5] = '\0';
  assert(!paqueteValido(embedded.data(), embedded.size()));
  assert(!paqueteValido(nullptr, 5));
  assert(!paqueteValido(buffer, 256));
  std::cout << "Contrato firmware: casos validos, limites, datos ausentes y rechazos OK\n";
}
