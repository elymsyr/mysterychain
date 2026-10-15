// T11: Bench variant (schematic_pnp_white): USB power only, Q2 = PNP (BC327 + R4 1 kΩ) instead of the AO3401A.
//   Short press: display power on/off through Q2.  Long press: 3 motor pulses with the display on.
//   Serial Monitor: d = display on/off,  m = one motor pulse,  s = 20 x display off/on stress.
// If the uptime restarts (screen or serial "alive"), the board has reset: a supply dip when Q2 switches the display on.
//
// Measure with the meter (black probe on GND unless noted):
//   Display ON:  D7 < 0.3 V;  base 2.4–2.6 V (3V3 − base = Vbe ~0.7–0.85);  collector = display VCC >= 3.15 V
//                (3V3 − collector = Vce(sat) < 0.15 V; more = not saturated: check R4 and the leg order)
//   Display OFF: D7 ~3.3 V;  base ~3.3 V;  display VCC < 0.3 V;  screen and backlight fully dark
#include <Adafruit_TinyUSB.h>  // for Serial
#include <Adafruit_GC9A01A.h>

const uint8_t BTN = D0, TFT_CS = D1, TFT_DC = D2, TFT_RST = D3, MOTOR = D6, DISP_PWR = D7;
const uint32_t SPI_HZ = 8000000;  // value that worked in T4
const uint32_t DEBOUNCE_MS = 25, LONG_MS = 600;

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);
bool dispOn;
uint32_t toggles;

void motor(uint32_t ms) {
  digitalWrite(MOTOR, HIGH);
  delay(ms);
  digitalWrite(MOTOR, LOW);
}

void drawScreen() {
  tft.fillScreen(GC9A01A_BLACK);
  tft.drawCircle(120, 120, 119, GC9A01A_WHITE);
  tft.fillCircle(85, 100, 14, GC9A01A_WHITE);
  tft.fillCircle(155, 100, 14, GC9A01A_WHITE);
  tft.setTextColor(GC9A01A_WHITE, GC9A01A_BLACK);
}

void display(bool on) {
  if (on == dispOn) return;
  if (on) {
    digitalWrite(DISP_PWR, LOW);  // Q2 on
    delay(50);                    // module LDO up before the reset pulse
    tft.begin(SPI_HZ);            // the display lost its state with VCC: full init every time
    drawScreen();
  } else {
    SPI.end();  // display pins LOW first, or the display keeps running through them (same as T10 powerOff)
    const uint8_t pins[] = {TFT_CS, TFT_DC, TFT_RST, PIN_SPI_SCK, PIN_SPI_MOSI};
    for (uint8_t p : pins) pinMode(p, OUTPUT), digitalWrite(p, LOW);
    digitalWrite(DISP_PWR, HIGH);  // Q2 off
  }
  dispOn = on;
  toggles++;
  Serial.printf("display %s  (toggle %lu)\n", on ? "ON" : "OFF", toggles);
}

void setup() {
  pinMode(MOTOR, OUTPUT);
  digitalWrite(MOTOR, LOW);  // the core preloads outputs HIGH
  pinMode(BTN, INPUT_PULLUP);
  pinMode(DISP_PWR, OUTPUT);
  digitalWrite(DISP_PWR, HIGH);  // start with Q2 off
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
  Serial.printf("BOOT  reset reason = 0x%lX\n", readResetReason());
  display(true);
  toggles = 0;
}

void loop() {
  static bool stable = HIGH, lastRaw = HIGH;
  static uint32_t tChange, tDown;
  bool raw = digitalRead(BTN);
  if (raw != lastRaw) { lastRaw = raw; tChange = millis(); }
  if (raw != stable && millis() - tChange > DEBOUNCE_MS) {
    stable = raw;
    if (stable == LOW) {
      tDown = millis();
    } else if (millis() - tDown >= LONG_MS) {
      display(true);
      for (int i = 0; i < 3; i++) { motor(150); delay(250); }
    } else {
      display(!dispOn);
    }
  }

  switch (Serial.read()) {
    case 'd': display(!dispOn); break;
    case 'm': motor(150); break;
    case 's':
      for (int i = 0; i < 20; i++) { display(false); delay(300); display(true); delay(700); }
      Serial.printf("stress done, uptime %lu ms (no BOOT line above = pass)\n", millis());
      break;
  }

  static uint32_t last;
  if (millis() - last >= 1000) {
    last = millis();
    if (dispOn) {
      tft.setTextSize(2);
      tft.setCursor(66, 150);
      tft.printf("%6lu s", last / 1000);
      tft.setTextSize(1);
      tft.setCursor(84, 180);
      tft.printf("toggles %lu", toggles);
    }
    if (last % 5000 < 1000) Serial.printf("alive %lu ms  display %s\n", last, dispOn ? "ON" : "OFF");
  }
  delay(10);
}
