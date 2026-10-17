// AI Buddy - motion-reactive eyes for ESP32 + SSD1306 + MPU6050.
//
// Data flow:  Imu -> MotionAnalyzer -> (MotionState, Events) -> Features (Behavior, ...)
//                                         -> EyeIntent -> EyePhysics -> EyeFrame -> EyeRenderer -> OLED
// Every layer only talks to its neighbours through the structs in Types.h.
//
// Libraries: Adafruit SSD1306, Adafruit GFX. (RoboEyes is no longer needed.)

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "Config.h"
#include "Types.h"
#include "Imu.h"
#include "MotionAnalyzer.h"
#include "EyePhysics.h"
#include "EyeRenderer.h"
#include "Feature.h"
#include "Behavior.h"

Adafruit_SSD1306 display(cfg::SCREEN_W, cfg::SCREEN_H, &Wire, -1);

Imu             imu;
MotionAnalyzer  analyzer;
EyePhysics      eyes;
EyeRenderer     renderer;
EventQueue      events;
EyeIntent       intent;
FeatureRegistry features;
Behavior        behavior;
// Future features: declare here, then features.add(&yourFeature) in setup().

static uint32_t lastTickMs = 0, lastRenderMs = 0, lastDebugMs = 0;

static Context makeContext(uint32_t now, float dt) {
  Context c;
  c.nowMs = now; c.dt = dt;
  c.motion = &analyzer.state();
  c.intent = &intent;
  c.frame  = &eyes.frame();
  return c;
}

static void showMessage(const char* line1, const char* line2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(cfg::C_ON);
  display.setCursor(4, 22); display.print(line1);
  display.setCursor(4, 34); display.print(line2);
  display.display();
}

static void fatal(const char* msg) {
  Serial.println(msg);
  showMessage("ERROR", msg);
  while (true) delay(1000);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Wire.begin(cfg::PIN_SDA, cfg::PIN_SCL);
  Wire.setClock(cfg::I2C_HZ);

  if (!display.begin(SSD1306_SWITCHCAPVCC, cfg::OLED_ADDR)) fatal("OLED not found");
  if (!imu.begin(Wire))                                      fatal("MPU6050 not found");

  showMessage("Calibrating...", "keep me still");
  imu.calibrate();                       // rejects itself if moved; falls back to 1.000 g
  analyzer.configure(imu.baselineG());

  const uint32_t now = millis();
  eyes.begin(now);
  features.add(&behavior);
  Context ctx = makeContext(now, 0.f);
  features.beginAll(ctx);

  lastTickMs = lastRenderMs = now;
  BUDDY_LOG("[buddy] ready\n");
}

static void simulate(uint32_t now, float dt) {
  ImuSample s;
  if (imu.read(s)) analyzer.update(s, now, events);   // on a failed read, the last state is reused

  Context ctx = makeContext(now, dt);
  Event e;
  while (events.pop(e)) features.dispatch(ctx, e);
  features.updateAll(ctx);
  eyes.step(dt, now, analyzer.state(), intent);

#if BUDDY_DEBUG
  if (now - lastDebugMs >= 200) {
    lastDebugMs = now;
    const MotionState& m = analyzer.state();
    BUDDY_LOG("mag=%.2f gx=%.2f gy=%.2f shake=%.2f spin=%.0f mode=%s\n",
              m.magnitude, m.gravity.x, m.gravity.y, m.shakeLevel, m.spinDps, behavior.modeName());
  }
#endif
}

static void render(uint32_t now) {
  display.clearDisplay();
  renderer.draw(display, eyes.frame());
  Context ctx = makeContext(now, 0.f);
  features.drawAll(ctx, display);        // overlays (clock, icons...) go on top of the eyes
  display.display();                     // ~23 ms of I2C traffic
}

void loop() {
  const uint32_t now = millis();

  if (now - lastTickMs >= cfg::SIM_PERIOD_MS) {
    float dt = (now - lastTickMs) * 0.001f;
    if (dt > cfg::MAX_DT) dt = cfg::MAX_DT;
    lastTickMs = now;
    simulate(now, dt);
  }
  if (now - lastRenderMs >= cfg::RENDER_PERIOD_MS) {
    lastRenderMs = now;
    render(now);
  }
}
