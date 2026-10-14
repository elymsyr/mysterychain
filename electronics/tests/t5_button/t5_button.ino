// T5: Button (D0 <-> GND, internal pull-up). Shows short/long presses and contact bounce.
#include <Adafruit_TinyUSB.h>  // for Serial

const uint8_t BTN = D0;
const uint32_t DEBOUNCE_MS = 25, LONG_MS = 600;

bool stable = HIGH, lastRaw = HIGH;
uint32_t tChange, tDown, presses, edges;

void setup() {
  pinMode(BTN, INPUT_PULLUP);
  Serial.begin(115200);
}

void loop() {
  bool raw = digitalRead(BTN);
  if (raw != lastRaw) { lastRaw = raw; tChange = millis(); edges++; }
  if (raw != stable && millis() - tChange > DEBOUNCE_MS) {
    stable = raw;
    if (stable == LOW) {
      tDown = millis();
    } else {
      uint32_t held = millis() - tDown;
      presses++;
      Serial.printf("#%lu %s (%lu ms)  raw edges: %lu\n", presses, held >= LONG_MS ? "LONG" : "SHORT", held, edges);
      edges = 0;
    }
  }
}
