#include "../firmware/telemetria/transmisor/transmisor.ino"
#include <assert.h>
#include <iostream>

void arrancar(unsigned fallos = 0) {
  ciclo = CicloTx{}; LoRa = LoRaPrueba{}; Serial = SerialPrueba{};
  relojPrueba = 0; LoRa.fallosInicio = fallos; setup(); loop();
}
size_t errores() {
  size_t n = 0, p = 0;
  while ((p = Serial.salida.find("# ERROR", p)) != std::string::npos) { ++n; ++p; }
  return n;
}
void verificarConfiguracion() {
  assert(Serial.baud == 115200);
  assert(LoRa.parametros["ss"] == 5 && LoRa.parametros["rst"] == 14 && LoRa.parametros["dio"] == 26);
  assert(LoRa.parametros["hz"] == 433000000 && LoRa.parametros["sf"] == 7);
  assert(LoRa.parametros["bw"] == 250000 && LoRa.parametros["cr"] == 5);
  assert(LoRa.parametros["sync"] == 0x12 && LoRa.parametros["preamble"] == 8);
  assert(LoRa.parametros["crc"] == 1 && LoRa.parametros["iq"] == 0 && LoRa.parametros["power"] == 17);
}
int main() {
  arrancar(); verificarConfiguracion();
  loop(); assert(LoRa.envios == 1 && LoRa.asincrono && ciclo.secuencia == 1);
  LoRa.ocupada = false; relojPrueba += 50; loop(); loop(); assert(LoRa.envios == 1);
  relojPrueba += 150; loop(); assert(LoRa.envios == 2);

  arrancar(); loop();
  const std::string fifo = LoRa.escrito;
  relojPrueba += 999; loop(); assert(ciclo.estado == CicloTx::TRANSMITIENDO && errores() == 0);
  assert(LoRa.escrito == fifo && ciclo.secuencia == 1 && LoRa.envios == 1);
  relojPrueba += 1; loop(); assert(ciclo.estado == CicloTx::PENDIENTE && errores() == 1 && LoRa.abortos == 1);
  relojPrueba += 1999; loop(); assert(LoRa.inicios == 1);
  relojPrueba += 1; loop(); verificarConfiguracion();
  assert(LoRa.inicios == 2 && LoRa.configuraciones == 16 && ciclo.secuencia == 1);
  loop(); assert(LoRa.enviados.back().find("$1,4000,") == 0);
  assert(ciclo.secuencia == 2); // Nueva muestra, no retransmisión del paquete 0.

  // Finalización tiene prioridad cuando coincide con el timeout (y después).
  for (uint32_t paso : {999U, 1000U, 1001U}) {
    arrancar(); loop(); relojPrueba += paso; LoRa.ocupada = false; loop();
    assert(ciclo.estado == CicloTx::DISPONIBLE && errores() == 0);
  }
  arrancar(); loop(); relojPrueba += 1001; loop();
  assert(ciclo.estado == CicloTx::PENDIENTE && errores() == 1);

  // Cada fallo de preparación/escritura/inicio TX consume un intento y recupera.
  for (int tipo = 0; tipo < 3; ++tipo) {
    arrancar();
    if (tipo == 0) LoRa.prepara = false;
    if (tipo == 1) LoRa.parcial = true;
    if (tipo == 2) LoRa.aceptaEnvio = false;
    loop();
    assert(ciclo.estado == CicloTx::PENDIENTE && ciclo.secuencia == 1);
    assert(errores() == 1 && LoRa.abortos == 1 && LoRa.enviados.empty());
    assert(LoRa.envios == (tipo == 2 ? 1U : 0U));
    relojPrueba += 1999; loop(); assert(LoRa.inicios == 1);
    LoRa.prepara = true; LoRa.parcial = false; LoRa.aceptaEnvio = true;
    relojPrueba += 1; loop(); assert(LoRa.inicios == 2);
    loop(); assert(ciclo.secuencia == 2 && LoRa.enviados.back().find("$1,3000,") == 0);
  }

  arrancar(2); assert(ciclo.estado == CicloTx::PENDIENTE && ciclo.secuencia == 0);
  relojPrueba += 1999; loop(); assert(LoRa.inicios == 1);
  relojPrueba += 1; loop(); assert(LoRa.inicios == 2);
  relojPrueba += 2000; loop(); verificarConfiguracion();
  assert(LoRa.inicios == 3 && ciclo.secuencia == 0 && LoRa.configuraciones == 8);
  loop(); assert(ciclo.secuencia == 1);

  // Wrap de reloj, secuencia y retrasos sin ráfagas.
  arrancar(); relojPrueba = UINT32_MAX-100; ciclo.ultimoIntento = relojPrueba;
  ciclo.secuencia = UINT32_MAX;
  relojPrueba += 199; loop(); assert(LoRa.envios == 0);
  relojPrueba += 1; loop(); assert(LoRa.envios == 1 && ciclo.secuencia == 0);
  LoRa.ocupada = false; relojPrueba += 10000; loop(); loop();
  assert(LoRa.envios == 2); loop(); assert(LoRa.envios == 2);
  // Timeout y recuperación también cruzan el límite de millis.
  arrancar(); relojPrueba = UINT32_MAX-100; loop();
  relojPrueba += 1000; loop(); assert(ciclo.estado == CicloTx::PENDIENTE);
  relojPrueba += 2000; loop(); assert(ciclo.estado == CicloTx::DISPONIBLE);
  std::cout << "Transmisor real con dobles: temporizacion, fallos, recuperacion y wrap OK\n";
}
