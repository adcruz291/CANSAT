#pragma once
#include <Wire.h>
#include <math.h>
#include "paquete.h"

// BME280: compensación de fábrica según Bosch BST-BME280-DS001, sección 8.
// MPU6050: registros según InvenSense RM-MPU-6000A-00, revisión 4.
// Referencias, unidades y límites en SENSORES.md. No usa datos simulados.
class SensoresCanSat {
  TwoWire& bus;
  uint8_t bmeCandidatas[2], mpuCandidatas[2], bme = 0, mpu = 0;
  uint16_t t1 = 0, p1 = 0;
  int16_t t2 = 0, t3 = 0, p[8] = {};
  bool busLista = false, intentoBus = false, intentoBme = false, intentoMpu = false;
  uint32_t ultimoBus = 0, ultimoBme = 0, ultimoMpu = 0, inicioBme = 0, inicioMpu = 0;
  int sda = -1, scl = -1;
  static uint16_t u16(const uint8_t* v) { return uint16_t(v[0]) | uint16_t(v[1]) << 8; }
  static int16_t be16(const uint8_t* v) { return int16_t(uint16_t(v[0]) << 8 | v[1]); }
  static bool toca(bool intento, uint32_t ultimo, uint32_t now) {
    return !intento || uint32_t(now-ultimo) >= 2000;
  }
  bool leerReg(uint8_t addr, uint8_t reg, uint8_t* datos, size_t n) {
    bus.beginTransmission(addr);
    if (bus.write(reg) != 1 || bus.endTransmission(false) != 0) return false;
    if (bus.requestFrom(addr, n) != n) return false;
    for (size_t i = 0; i < n; ++i) {
      const int value = bus.read();
      if (value < 0) return false;
      datos[i] = uint8_t(value);
    }
    return true;
  }
  bool escribirReg(uint8_t addr, uint8_t reg, uint8_t value) {
    bus.beginTransmission(addr);
    if (bus.write(reg) != 1 || bus.write(value) != 1) return false;
    return bus.endTransmission() == 0;
  }
  bool iniciarBme(uint8_t addr) {
    uint8_t id, estado, coef[24];
    if (!leerReg(addr, 0xD0, &id, 1) || id != 0x60 ||
        !leerReg(addr, 0xF3, &estado, 1) || (estado & 1) ||
        !leerReg(addr, 0x88, coef, sizeof(coef))) return false;
    t1 = u16(coef); t2 = int16_t(u16(coef+2)); t3 = int16_t(u16(coef+4));
    p1 = u16(coef+6);
    for (unsigned i = 0; i < 8; ++i) p[i] = int16_t(u16(coef+8+2*i));
    if (t1 == 0 || t1 == 0xFFFF || p1 == 0 || p1 == 0xFFFF) return false;
    // Primero sleep para que config se aplique aunque hubiera un programa anterior.
    // Humedad desactivada (no está en el contrato), filtro apagado para menor retardo.
    // Temperatura x2, presión x4, modo normal: nueva conversión cada ~16 ms.
    return escribirReg(addr, 0xF4, 0) && escribirReg(addr, 0xF2, 0) &&
           escribirReg(addr, 0xF5, 0) && escribirReg(addr, 0xF4, 0x4F);
  }
  bool iniciarMpu(uint8_t addr) {
    uint8_t id;
    if (!leerReg(addr, 0x75, &id, 1) || id != 0x68) return false;
    // Sin reset bloqueante: despertar todos los ejes, PLL X, FIFO/interrupciones off.
    // ±16 g (2048 LSB/g), ±2000 °/s (16.4 LSB/(°/s)), DLPF ~21 Hz, 50 Hz.
    return escribirReg(addr, 0x6B, 1) && escribirReg(addr, 0x6C, 0) &&
           escribirReg(addr, 0x23, 0) && escribirReg(addr, 0x6A, 0) &&
           escribirReg(addr, 0x38, 0) && escribirReg(addr, 0x1A, 4) &&
           escribirReg(addr, 0x19, 19) && escribirReg(addr, 0x1B, 0x18) &&
           escribirReg(addr, 0x1C, 0x18);
  }
  void leerBme(Medidas& d, uint32_t now) {
    if (uint32_t(now-inicioBme) < 40) return;
    uint8_t raw[6];
    if (!leerReg(bme, 0xF7, raw, sizeof(raw))) { falloBme(now); return; }
    const uint32_t ap = uint32_t(raw[0]) << 12 | uint32_t(raw[1]) << 4 | raw[2] >> 4;
    const uint32_t at = uint32_t(raw[3]) << 12 | uint32_t(raw[4]) << 4 | raw[5] >> 4;
    if (ap == 0x80000 || at == 0x80000) { falloBme(now); return; }
    const double a = (double(at)/16384.0-double(t1)/1024.0)*t2;
    const double delta = double(at)/131072.0-double(t1)/8192.0;
    const double fine = a+delta*delta*t3;
    const double temperatura = fine/5120.0;
    double x = fine/2.0-64000.0;
    double y = x*x*p[4]/32768.0 + x*p[3]*2.0;
    y = y/4.0 + double(p[2])*65536.0;
    x = (p[1]*x*x/524288.0 + p[0]*x)/524288.0;
    x = (1.0+x/32768.0)*p1;
    if (!isfinite(x) || x == 0) { falloBme(now); return; }
    double pa = (1048576.0-ap-y/4096.0)*6250.0/x;
    pa += (p[7]*pa*pa/2147483648.0 + pa*p[6]/32768.0 + p[5])/16.0;
    const double hpa = pa/100.0;
    if (!isfinite(temperatura) || temperatura < -40 || temperatura > 85 ||
        !isfinite(hpa) || hpa < 300 || hpa > 1100) { falloBme(now); return; }
    d.temp = float(temperatura); d.pres = float(hpa);
    // Altitud estándar, no altura sobre el suelo: el frontend calcula su cero.
    d.alt = float(44330.0*(1.0-pow(hpa/1013.25, 1.0/5.255)));
  }
  void leerMpu(Medidas& d, uint32_t now) {
    if (uint32_t(now-inicioMpu) < 100) return;
    uint8_t estado, raw[14];
    if (!leerReg(mpu, 0x3A, &estado, 1) || !(estado & 1) ||
        !leerReg(mpu, 0x3B, raw, sizeof(raw))) { falloMpu(now); return; }
    const int16_t ax = be16(raw), ay = be16(raw+2), az = be16(raw+4);
    if (ax == INT16_MIN || ax == INT16_MAX || ay == INT16_MIN || ay == INT16_MAX ||
        az == INT16_MIN || az == INT16_MAX) return; // Saturación: no inventar el impacto real.
    d.ax = ax/2048.0f; d.ay = ay/2048.0f; d.az = az/2048.0f;
    const double g = sqrt(double(d.ax)*d.ax + double(d.ay)*d.ay + double(d.az)*d.az);
    const double gx = be16(raw+8)/16.4, gy = be16(raw+10)/16.4, gz = be16(raw+12)/16.4;
    // Inclinación por gravedad, no actitud dinámica. Evitar caída libre/giro fuerte.
    if (g >= 0.85 && g <= 1.15 && fabs(gx) <= 20 && fabs(gy) <= 20 && fabs(gz) <= 20) {
      const double lateral = hypot(d.ay, d.az);
      if (lateral > 0.05) d.roll = float(atan2(d.ay, d.az)*180.0/3.141592653589793);
      d.pitch = float(atan2(-d.ax, lateral)*180.0/3.141592653589793);
    }
    // Sin magnetómetro: yaw absoluto no es observable, permanece en -1.
  }
  void falloBme(uint32_t now) {
    bme = 0; ultimoBme = now;
    Serial.println("# ERROR: lectura BME280 invalida; reintento en 2000 ms.");
  }
  void falloMpu(uint32_t now) {
    mpu = 0; ultimoMpu = now;
    Serial.println("# ERROR: lectura MPU6050 invalida; reintento en 2000 ms.");
  }
public:
  SensoresCanSat(TwoWire& wire, uint8_t ba, uint8_t bb, uint8_t ma, uint8_t mb)
      : bus(wire), bmeCandidatas{ba, bb}, mpuCandidatas{ma, mb} {}
  void iniciar(int pinSda, int pinScl) {
    sda = pinSda; scl = pinScl;
    busLista = intentoBus = intentoBme = intentoMpu = false;
    bme = mpu = 0;
  }
  Medidas leer(uint32_t now) {
    Medidas d; // Una muestra nueva: nunca reutilizar lecturas anteriores tras un fallo.
    if (!busLista && toca(intentoBus, ultimoBus, now)) {
      intentoBus = true; ultimoBus = now;
      busLista = bus.begin(sda, scl, 100000);
      bus.setTimeOut(20);
      if (!busLista) Serial.println("# ERROR: no se pudo iniciar I2C; reintento en 2000 ms.");
    }
    if (!busLista) return d;
    if (!bme && toca(intentoBme, ultimoBme, now)) {
      intentoBme = true; ultimoBme = now;
      for (uint8_t addr : bmeCandidatas) if (iniciarBme(addr)) { bme = addr; inicioBme = now; break; }
      Serial.println(bme ? "# BME280 identificado y configurado." : "# ERROR: BME280 no detectado (0x76/0x77).");
    }
    if (!mpu && toca(intentoMpu, ultimoMpu, now)) {
      intentoMpu = true; ultimoMpu = now;
      for (uint8_t addr : mpuCandidatas) if (iniciarMpu(addr)) { mpu = addr; inicioMpu = now; break; }
      Serial.println(mpu ? "# MPU6050 identificado y configurado." : "# ERROR: MPU6050 no detectado (0x68/0x69).");
    }
    if (bme) leerBme(d, now);
    if (mpu) leerMpu(d, now);
    return d;
  }
};
