#include "../firmware/telemetria/transmisor/paquete.h"
#include "../firmware/telemetria/receptor/contrato.h"
#include <assert.h>
#include <iostream>
#include <limits>
#include <string>

// Vuelo artificial para probar el frontend v2, no firmware de sensores.
void emitirVuelo() {
  Medidas d;
  for (uint32_t seq = 0; seq <= 185; ++seq) {
    const double t = seq / 5.0;
    double h = 0;
    if (t >= 2 && t < 14.5) h = (t - 2) * 2;
    else if (t >= 14.5 && t < 15.5) h = 25;
    else if (t >= 15.5 && t < 18) h = 25 - (t - 15.5) * 10;
    d.alt = (float)(432 + h); // Altura estándar: el cero lo resta el frontend.
    d.pres = (float)(1013.25 * pow(1 - d.alt / 44330.0, 5.255));
    char buffer[256];
    const int n = crearPaquete(buffer, sizeof(buffer), seq, seq * 200, d);
    assert(n > 0 && paqueteValido(buffer, n));
    std::cout << buffer << ",-75,8.50\n";
  }
}

int main(int argc, char** argv) {
  if (argc > 1 && std::string(argv[1]) == "--emit-flight") {
    emitirVuelo();
    return 0;
  }
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
  d.st = 3; d.flags = 3; d.pres = 990; d.temp = -10;
  d.alt = 100; d.ax = 0; d.ay = -1; d.az = 0;
  d.roll = -30; d.pitch = 15; d.yaw = 359;
  n = crearPaquete(buffer, sizeof(buffer), UINT32_MAX, UINT32_MAX, d);
  assert(n > 0 && paqueteValido(buffer, n));
  assert(crearPaquete(buffer, 8, 0, 0, d) == -1);
  d.st = 4;
  assert(crearPaquete(buffer, sizeof(buffer), 0, 0, d) == -1);
  d = Medidas{};
  d.temp = std::numeric_limits<float>::quiet_NaN();
  d.ax = std::numeric_limits<float>::infinity();
  d.yaw = -std::numeric_limits<float>::infinity();
  d.pres = 990; d.alt = 432; d.az = 1;
  const Medidas limpia = sanearMedidas(d);
  assert(limpia.temp == -1 && limpia.ax == -1 && limpia.yaw == -1);
  assert(limpia.pres == 990 && limpia.alt == 432 && limpia.az == 1);
  assert(limpia.st == 0 && limpia.flags == 0 && limpia.sd == -1 && limpia.sb == -1);
  n = crearPaquete(buffer, sizeof(buffer), 0, 0, limpia);
  assert(n > 0 && paqueteValido(buffer, n));
  assert(crearPaquete(nullptr, 0, 0, 0, limpia) == -1);
  assert(crearPaquete(buffer, 0, 0, 0, limpia) == -1);
  assert(crearPaquete(buffer, 1, 0, 0, limpia) == -1 && buffer[0] == '\0');
  const float limites[] = {0, -1, -0.001f, 1013.25f, 1e20f, 1e30f,
    std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest(),
    std::numeric_limits<float>::min(), std::numeric_limits<float>::denorm_min()};
  for (float value : limites) {
    Medidas extremo;
    extremo.pres = value;
    n = crearPaquete(buffer, sizeof(buffer), UINT32_MAX, UINT32_MAX, extremo);
    if (n > 0) assert(paqueteValido(buffer, n));
    else assert(buffer[0] == '\0');
  }
  d.st = 0; d.flags = 4;
  assert(crearPaquete(buffer, sizeof(buffer), 0, 0, d) == -1);
  d.flags = 0; d.sd = 0;
  assert(crearPaquete(buffer, sizeof(buffer), 0, 0, d) == -1);
  d.sd = -1; d.sb = 180;
  assert(crearPaquete(buffer, sizeof(buffer), 0, 0, d) == -1);
  d.sb = -1;
  d.temp = std::numeric_limits<float>::infinity();
  assert(crearPaquete(buffer, sizeof(buffer), 0, 0, d) == -1);
  const char* bad[] = {
    "CANSAT,PRUEBA,0", "$1,2,3", "", "$",
    "$0,0,0,1,,1,1,1,1,1,1,1,0,-1,-1",
    "$0,0,4,1,1,1,1,1,1,1,1,1,0,-1,-1",
    "$0,0,0,1,1,1,1,1,1,1,1,1,4,-1,-1",
    "$0,0,0,nan,1,1,1,1,1,1,1,1,0,-1,-1",
    "$0,0,0,1e999,1,1,1,1,1,1,1,1,0,-1,-1",
    "$0,0,0, 1,1,1,1,1,1,1,1,1,0,-1,-1",
    "$0,0,0,0x1,1,1,1,1,1,1,1,1,0,-1,-1",
    "$0.5,0,0,1,1,1,1,1,1,1,1,1,0,-1,-1",
    "$4294967296,0,0,1,1,1,1,1,1,1,1,1,0,-1,-1",
    "$0,0,0,1,1,1,1,1,1,1,1,1,0,0,-1",
    "$0,0,0,1,1,1,1,1,1,1,1,1,0,-1,180"
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
