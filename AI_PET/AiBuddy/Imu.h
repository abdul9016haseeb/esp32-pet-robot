// Imu.h - MPU6050 driver. Hardware access only; no interpretation of motion here.
#pragma once
#include <Wire.h>
#include "Types.h"

class Imu {
 public:
  bool  begin(TwoWire& wire);          // false on I2C failure
  bool  calibrate();                   // true if the board was still; false = nominal values used
  bool  read(ImuSample& out);          // false on I2C failure
  float baselineG() const { return baseline_; }

 private:
  bool writeReg(uint8_t reg, uint8_t val);
  bool readRegs(uint8_t reg, uint8_t* buf, uint8_t n);

  TwoWire* wire_     = nullptr;
  float    baseline_ = 1.0f;           // measured |a| at rest
  float    gyroBias_[3] = {0, 0, 0};
  uint32_t lastUs_   = 0;
};
