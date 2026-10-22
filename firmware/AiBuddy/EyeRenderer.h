// EyeRenderer.h - draws an EyeFrame onto any Adafruit_GFX surface. No simulation, no state.
#pragma once
#include <Adafruit_GFX.h>
#include "Types.h"

class EyeRenderer {
 public:
  void draw(Adafruit_GFX& g, const EyeFrame& f) const;
 private:
  void drawEye(Adafruit_GFX& g, float cx, float cy, const EyeFrame& f, int side) const;
  void drawSpiral(Adafruit_GFX& g, float cx, float cy, const EyeFrame& f, int side) const;
};
