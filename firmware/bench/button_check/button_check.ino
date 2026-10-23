// PIN SCAN: finds which GPIO a button is really connected to.
// Serial Monitor 115200. Press one button at a time; the pin that goes LOW is printed.
// Skipped: GPIO 1/3 (USB serial), 6-11 (flash), 34-39 (input only, no internal pull-up).

const uint8_t PINS[] = {0, 2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33};
const uint8_t N = sizeof(PINS) / sizeof(PINS[0]);
bool last[N];

void setup() {
  Serial.begin(115200);
  delay(300);
  for (uint8_t i = 0; i < N; i++) pinMode(PINS[i], INPUT_PULLUP);
  delay(20);
  Serial.println("Pin scan ready. Press one button at a time.");
  for (uint8_t i = 0; i < N; i++) {
    last[i] = digitalRead(PINS[i]);
    if (!last[i]) Serial.printf("NOTE: GPIO %u already LOW at start (held, shorted to GND, or BOOT/strap)\n", PINS[i]);
  }
}

void loop() {
  for (uint8_t i = 0; i < N; i++) {
    const bool v = digitalRead(PINS[i]);
    if (v == last[i]) continue;
    delay(20);                                   // simple debounce
    if (digitalRead(PINS[i]) != v) continue;
    last[i] = v;
    Serial.printf("GPIO %2u -> %s\n", PINS[i], v ? "HIGH (released)" : "LOW  (PRESSED)");
  }
}
