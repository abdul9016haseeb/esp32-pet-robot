// AI Buddy - motion-reactive eyes for ESP32 + SSD1306 + MPU6050, with an apps menu.
//
// Data flow:  Imu -> MotionAnalyzer -> (MotionState, Events) -> Features (Behavior, ...)
//                                         -> EyeIntent -> EyePhysics -> EyeFrame -> EyeRenderer -> OLED
//             Buttons -> Events -> Shell (Eyes / Menu / App screens) -> OLED
// Every layer only talks to its neighbours through the structs in Types.h.
//
// Libraries: Adafruit SSD1306, Adafruit GFX.

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
#include "Buttons.h"
#include "Shell.h"
#include "ClockService.h"
#include "ClockApp.h"
#include "Buzzer.h"

Adafruit_SSD1306 display(cfg::SCREEN_W, cfg::SCREEN_H, &Wire, -1);

Imu             imu;
MotionAnalyzer  analyzer;
EyePhysics      eyes;
EyeRenderer     renderer;
EventQueue      events;
EyeIntent       intent;
FeatureRegistry features;
Behavior        behavior;
Buttons         buttons;
Shell           shell;
ClockService    clockService;   // wall clock + countdown timer + stopwatch; ticks in every screen state
// Future features: declare here, then features.add(&yourFeature) in setup().

Buzzer          buzzer;         // optional active buzzer on the spare pin (no-op until cfg::BUZZER_ENABLED)

// ---- Apps. Replace a StubApp with the real app object when it exists. ----
ClockApp clockApp(clockService);   // shares the service, so a timer set here still fires from any screen
StubApp weatherApp("WEATHER");
StubApp gamesApp("GAMES");

// One line per menu row. app == nullptr is the EXIT row (back to the eyes).
const MenuEntry MENU[] = {
  {"CLOCK",   icons::Id::Clock,   &clockApp},
  {"WEATHER", icons::Id::Weather, &weatherApp},
  {"GAMES",   icons::Id::Games,   &gamesApp},
  {"EXIT",    icons::Id::Exit,    nullptr},
};
const uint8_t MENU_COUNT = sizeof(MENU) / sizeof(MENU[0]);

static uint32_t lastTickMs = 0, lastRenderMs = 0, lastDebugMs = 0;

static Context makeContext(uint32_t now, float dt) {
  Context c;
  c.nowMs = now; c.dt = dt;
  c.motion = &analyzer.state();
  c.intent = &intent;
  c.frame  = &eyes.frame();
  c.events = &events;
  c.screen = shell.screen();
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

  buttons.begin();
  buzzer.begin();                        // no-op until a buzzer is wired (cfg::BUZZER_ENABLED)
  shell.attachBuzzer(&buzzer);
  shell.begin(MENU, MENU_COUNT);

  const uint32_t now = millis();
  eyes.begin(now);
  features.add(&behavior);
  features.add(&clockService);
  Context ctx = makeContext(now, 0.f);
  features.beginAll(ctx);

  lastTickMs = lastRenderMs = now;
  BUDDY_LOG("[buddy] ready\n");
}

static void simulate(uint32_t now, float dt) {
  buttons.poll(now, events);
  ImuSample s;
  if (imu.read(s)) analyzer.update(s, now, events);   // on a failed read, the last state is reused

  Context ctx = makeContext(now, dt);
  Event e;
  while (events.pop(e)) {
    if (!shell.onEvent(ctx, e)) features.dispatch(ctx, e);   // the Shell may swallow button events
  }
  shell.update(ctx);
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
  Context ctx = makeContext(now, 0.f);
  if (ctx.screen == Screen::Eyes) renderer.draw(display, eyes.frame());
  features.drawAll(ctx, display);        // overlays (clock, icons...) go on top of the eyes
  shell.draw(ctx, display);              // menu / running app (draws nothing on the Eyes screen)
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
