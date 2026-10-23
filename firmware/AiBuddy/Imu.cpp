#include "Imu.h"
#include "Config.h"

namespace {
constexpr uint8_t REG_SMPLRT = 0x19, REG_CONFIG = 0x1A, REG_GYRO_CFG = 0x1B,
                  REG_ACCEL_CFG = 0x1C, REG_ACCEL_OUT = 0x3B, REG_PWR_MGMT = 0x6B, REG_WHO_AM_I = 0x75;
inline int16_t be16(const uint8_t* p) { return (int16_t)((p[0] << 8) | p[1]); }
}

bool Imu::writeReg(uint8_t reg, uint8_t val) {
  wire_->beginTransmission(cfg::MPU_ADDR);
  wire_->write(reg);
  wire_->write(val);
  return wire_->endTransmission() == 0;
}

bool Imu::readRegs(uint8_t reg, uint8_t* buf, uint8_t n) {
  wire_->beginTransmission(cfg::MPU_ADDR);
  wire_->write(reg);
  if (wire_->endTransmission(false) != 0) return false;
  if (wire_->requestFrom((uint8_t)cfg::MPU_ADDR, n) != n) return false;
  for (uint8_t i = 0; i < n; i++) buf[i] = wire_->read();
  return true;
}

bool Imu::begin(TwoWire& wire) {
  wire_ = &wire;
  uint8_t id = 0;
  if (!readRegs(REG_WHO_AM_I, &id, 1)) return false;
  if (id != 0x68) BUDDY_LOG("[imu] WHO_AM_I = 0x%02X (expected 0x68), continuing\n", id);
  bool ok = writeReg(REG_PWR_MGMT, 0x01);   // wake, PLL clock
  delay(10);
  ok &= writeReg(REG_SMPLRT, 0x00);
  ok &= writeReg(REG_CONFIG, 0x03);         // low-pass ~44 Hz (same as your bench test)
  ok &= writeReg(REG_GYRO_CFG, 0x08);       // +/-500 dps
  ok &= writeReg(REG_ACCEL_CFG, 0x08);      // +/-4 g (headroom for landing impacts)
  delay(50);
  return ok;
}

bool Imu::read(ImuSample& s) {
  uint8_t b[14];
  if (!readRegs(REG_ACCEL_OUT, b, 14)) return false;
  s.ax = be16(&b[0])  / cfg::ACC_LSB_PER_G;
  s.ay = be16(&b[2])  / cfg::ACC_LSB_PER_G;
  s.az = be16(&b[4])  / cfg::ACC_LSB_PER_G;
  // b[6..7] = temperature, skipped
  s.gx = be16(&b[8])  / cfg::GYRO_LSB_PER_DPS - gyroBias_[0];
  s.gy = be16(&b[10]) / cfg::GYRO_LSB_PER_DPS - gyroBias_[1];
  s.gz = be16(&b[12]) / cfg::GYRO_LSB_PER_DPS - gyroBias_[2];

  const uint32_t nowUs = micros();
  float dt = lastUs_ ? (nowUs - lastUs_) * 1e-6f : 0.01f;
  lastUs_ = nowUs;
  s.dt = dt < 0.001f ? 0.001f : (dt > cfg::MAX_DT ? cfg::MAX_DT : dt);
  return true;
}

// Averages 1 s of data. If the board moved during that time the result is discarded
// and nominal values (1.000 g, zero gyro bias) are used, so a bad boot never poisons detection.
bool Imu::calibrate() {
  gyroBias_[0] = gyroBias_[1] = gyroBias_[2] = 0;
  float sumMag = 0, minMag = 1e9f, maxMag = -1e9f, sg[3] = {0, 0, 0};
  int n = 0;
  for (int i = 0; i < cfg::CAL_SAMPLES; i++) {
    ImuSample s;
    if (read(s)) {
      const float m = sqrtf(s.ax * s.ax + s.ay * s.ay + s.az * s.az);
      sumMag += m;
      if (m < minMag) minMag = m;
      if (m > maxMag) maxMag = m;
      sg[0] += s.gx; sg[1] += s.gy; sg[2] += s.gz;
      n++;
    }
    delay(10);
  }
  const bool still = n > cfg::CAL_SAMPLES / 2 && (maxMag - minMag) < cfg::CAL_MAX_RANGE_G;
  if (still) {
    baseline_ = sumMag / n;
    for (int i = 0; i < 3; i++) gyroBias_[i] = sg[i] / n;
  } else {
    baseline_ = 1.0f;
  }
  BUDDY_LOG("[imu] calibration %s, baseline %.3f g\n", still ? "OK" : "REJECTED (moved)", baseline_);
  return still;
}
