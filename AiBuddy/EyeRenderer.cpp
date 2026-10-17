#include "EyeRenderer.h"
#include "Config.h"
#include <math.h>

void EyeRenderer::draw(Adafruit_GFX& g, const EyeFrame& f) const {
  for (int side = 0; side < 2; side++) {
    const float cx = cfg::SCREEN_W * 0.5f + (side ? 0.5f : -0.5f) * cfg::EYE_SPACING + f.pos[side].x;
    const float cy = cfg::EYE_CENTER_Y + f.pos[side].y;
    if (f.dizzy > 0.5f) drawSpiral(g, cx, cy, f, side);
    else                drawEye(g, cx, cy, f, side);
  }
}

void EyeRenderer::drawEye(Adafruit_GFX& g, float cx, float cy, const EyeFrame& f, int side) const {
  const int x0 = (int)lroundf(cx - f.w * 0.5f);
  const int y0 = (int)lroundf(cy - f.h * 0.5f);
  const int w  = (int)lroundf(f.w);
  const int h  = (int)lroundf(f.h);
  g.fillRoundRect(x0, y0, w, h, (int)lroundf(f.radius), cfg::C_ON);

  if (f.anger > 0.02f) {                 // black triangle slices the inner-top corner = angry brow
    const int drop  = (int)lroundf(f.anger * f.h * 0.55f);
    const int outer = side == 0 ? x0 - 1 : x0 + w;
    const int inner = side == 0 ? x0 + w : x0 - 1;
    g.fillTriangle(outer, y0 - 1, inner, y0 - 1, inner, y0 + drop, cfg::C_OFF);
  }
}

void EyeRenderer::drawSpiral(Adafruit_GFX& g, float cx, float cy, const EyeFrame& f, int side) const {
  const float R   = 15.f * f.scale;
  const int   N   = 28;
  const float dir = side == 0 ? 1.f : -1.f;      // eyes spin in opposite directions
  g.drawCircle((int)lroundf(cx), (int)lroundf(cy), (int)lroundf(R), cfg::C_ON);
  float px = cx, py = cy;
  for (int i = 1; i < N; i++) {
    const float t = (float)i / (N - 1);
    const float a = dir * f.dizzyPhase + dir * t * 13.8f;  // ~2.2 turns
    const float x = cx + cosf(a) * R * t;
    const float y = cy + sinf(a) * R * t;
    g.drawLine((int)lroundf(px), (int)lroundf(py), (int)lroundf(x), (int)lroundf(y), cfg::C_ON);
    px = x; py = y;
  }
}
