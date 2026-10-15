// Seven Havens / Pavi: 80x80 scene (pavi.h) scaled 3x and pushed to the 240x240 screen in strips over SPI DMA.
// Shake the board: the fairy flees, the girl chases it and jumps, plays for 6 s. Everything else is the same as ../cat.
// BLE (Nordic UART, name "MysteryChain"): text from the phone shows in a speech bubble, the motor buzzes twice,
// the girl cheers. Button and shake events go to the phone as "button N" / "shake N".
// Button held 3 s: off. Hold ~1 s to turn on again; plugging in USB also turns it on.
// Motion is tied to real time: speed stays the same if fps drops. Serial monitor prints fps once a second.
#include <Adafruit_TinyUSB.h>  // for Serial
#include <Adafruit_GC9A01A.h>
#include <LSM6DS3.h>
#include <bluefruit.h>
#include "pavi.h"

const uint8_t TFT_CS = D1, TFT_DC = D2, TFT_RST = D3, MOTOR = D6, BTN = D0, DISP_PWR = D7;
const uint32_t ON_HOLD_MS = 1000, OFF_HOLD_MS = 3000;  // power button, see powerOff()
const uint32_t SPI_HZ = 32000000;          // if the image glitches: 16000000 (~17 fps)
const int SC = 240 / pavi::N, BAND = 8;  // 3x scale; strip of 8 logical rows = 11.5 KB

const float SHAKE_G = 0.5;           // shake when |a| deviates from 1 g by this much
const uint32_t SHAKE_GAP_MS = 400;   // don't count one shake several times
const uint32_t MOTOR_MASK_MS = 400;  // ignore shakes this long after the motor stops (use the value measured in T8)

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);
LSM6DS3 imu(I2C_MODE, 0x6A);
BLEUart bleuart;
bool imuOk;
uint16_t pal[pavi::NCOL], strip[240 * SC * BAND];

// Route Adafruit_GFX text drawing into the scene buffer (by color index); clip window on x.
struct FbGfx : Adafruit_GFX {
  int16_t clipL = 0, clipR = pavi::N;
  FbGfx() : Adafruit_GFX(pavi::N, pavi::N) {}
  void drawPixel(int16_t x, int16_t y, uint16_t c) override {
    if (x >= clipL && x < clipR) pavi::px(x, y, c);
  }
} gfx;

char msg[64];
uint32_t msgT0, msgDur, buzzT0, buzzN;  // vibration: buzzN pulses, 120 ms on / 120 ms off

void show(uint32_t pulses) {  // show msg in the bubble, buzz, the girl cheers
  msgT0 = buzzT0 = millis();
  msgDur = constrain(2 * (strlen(msg) * 6 + 20) * 40, 4000, 15000);  // scrolling text loops at least twice
  buzzN = pulses;
  pavi::cheer();
  Serial.printf("screen: %s\n", msg);
}

void tell(const char *s) {  // to the serial monitor and, if connected, the phone
  Serial.print(s);
  if (Bluefruit.connected()) bleuart.write(s, strlen(s));
}

// UTF-8 -> ASCII: the font has no ş ğ ı; fold Turkish letters to ASCII, anything else (emoji etc.) becomes '?'.
void toAscii(const uint8_t *s, int n, char *out, int cap) {
  static const uint16_t CP[] = {0xE7, 0xC7, 0x11F, 0x11E, 0x131, 0x130, 0xF6, 0xD6, 0x15F, 0x15E, 0xFC, 0xDC};
  static const char AS[] = "cCgGiIoOsSuU";
  int o = 0;
  for (int i = 0; i < n && o < cap - 1; i++) {
    uint8_t b = s[i];
    if (b < 0x80) {
      if (b >= 0x20) out[o++] = b;
      continue;
    }
    int len = b >= 0xF0 ? 4 : b >= 0xE0 ? 3 : 2;
    char c = '?';
    if (len == 2 && i + 1 < n)
      for (int k = 0; k < 12; k++)
        if (CP[k] == (((b & 0x1F) << 6) | (s[i + 1] & 0x3F))) c = AS[k];
    out[o++] = c;
    i += len - 1;
  }
  out[o] = 0;
}

void drawBubble(uint32_t now) {
  if (!msg[0] || now - msgT0 > msgDur) return;
  const int L = 12, R = 68, Y = 11, H = 13;  // fits the top band of the round screen
  gfx.fillRoundRect(L, Y, R - L, H, 4, pavi::INK);
  gfx.fillRoundRect(L + 1, Y + 1, R - L - 2, H - 2, 3, pavi::CREAM);
  int w = strlen(msg) * 6, inner = R - L - 6;
  gfx.setTextColor(pavi::INK);
  gfx.setTextWrap(false);
  gfx.clipL = L + 2, gfx.clipR = R - 2;
  if (w <= inner) {
    gfx.setCursor((L + R - w) / 2 + 1, Y + 3);
    gfx.print(msg);
  } else {  // long message: scrolling text, 25 px/s
    int x = L + 3 - (int)((now - msgT0) / 40 % (w + 20));
    for (int k = 0; k < 2; k++, x += w + 20) gfx.setCursor(x, Y + 3), gfx.print(msg);
  }
  gfx.clipL = 0, gfx.clipR = pavi::N;
}

// Power: daily on/off is the button (the battery switch on the back stays on). Hold the button OFF_HOLD_MS to turn off, about ON_HOLD_MS to turn on
// (electronics/tests/t10_power). Off = display cut by Q2 + System OFF, a few uA; waking is a reset.
bool dispOn;

void flashSleep() {  // on-board QSPI flash into deep power-down (0xB9): ~25 uA less while off
  const uint8_t pins[] = {PIN_QSPI_CS, PIN_QSPI_SCK, PIN_QSPI_IO0, PIN_QSPI_IO2, PIN_QSPI_IO3};
  for (uint8_t p : pins) pinMode(p, OUTPUT);  // the core preloads outputs HIGH: CS, WP#, HOLD# idle
  digitalWrite(PIN_QSPI_SCK, LOW);
  digitalWrite(PIN_QSPI_CS, LOW);
  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_QSPI_IO0, (0xB9 >> i) & 1);
    digitalWrite(PIN_QSPI_SCK, HIGH);
    digitalWrite(PIN_QSPI_SCK, LOW);
  }
  digitalWrite(PIN_QSPI_CS, HIGH);
}

void powerOff() {
  digitalWrite(MOTOR, LOW);
  if (dispOn) {  // display pins LOW first so they don't feed the display, then cut its power
    tft.fillScreen(0);  // goes dark even without Q2
    SPI.end();
    const uint8_t pins[] = {TFT_CS, TFT_DC, TFT_RST, PIN_SPI_SCK, PIN_SPI_MOSI};
    for (uint8_t p : pins) pinMode(p, OUTPUT), digitalWrite(p, LOW);
  }
  pinMode(DISP_PWR, OUTPUT);
  digitalWrite(DISP_PWR, HIGH);  // Q2 off
  const uint8_t loads[] = {PIN_LSM6DS3TR_C_POWER, PIN_PDM_PWR};  // IMU and mic supplies
  for (uint8_t p : loads) pinMode(p, OUTPUT), digitalWrite(p, LOW);
  flashSleep();
  while (digitalRead(BTN) == LOW) delay(10);  // System OFF with the button held = instant wake
  delay(50);
  systemOff(BTN, LOW);
}

void setup() {
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);  // the core drives it HIGH at boot; keep P0.31 safe once a battery is attached
  pinMode(MOTOR, OUTPUT);
  digitalWrite(MOTOR, LOW);  // outputs start HIGH in this core: the motor would run through setup
  pinMode(BTN, INPUT_PULLUP);
  if (readResetReason() & POWER_RESETREAS_OFF_Msk) {  // woken by the button: on only if held, a tap goes back off
    while (digitalRead(BTN) == LOW && millis() < ON_HOLD_MS) delay(10);
    if (millis() < ON_HOLD_MS) powerOff();
  }
  Serial.begin(115200);
  pinMode(DISP_PWR, OUTPUT);
  digitalWrite(DISP_PWR, LOW);  // display power on through Q2
  delay(50);
  tft.begin(SPI_HZ);
  dispOn = true;
  for (int i = 0; i < pavi::NCOL; i++) {
    uint16_t c = tft.color565(pavi::PAL[i][0], pavi::PAL[i][1], pavi::PAL[i][2]);
    pal[i] = c << 8 | c >> 8;  // display wants big endian: swap once here, no swap while sending
  }
  pavi::init();
  imuOk = imu.begin() == 0;
  if (!imuOk) Serial.println("NO IMU: shake won't work");

  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);  // large MTU: long messages arrive in one write
  Bluefruit.begin();
  Bluefruit.setName("MysteryChain");
  bleuart.begin();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();           // name in the main packet so every scanner sees it (20/31 bytes)
  Bluefruit.ScanResponse.addService(bleuart);  // 128-bit UART UUID doesn't fit the main packet
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.start(0);
  while (digitalRead(BTN) == LOW) delay(10);  // the power-on hold is not a press
}

void loop() {
  static uint32_t last = millis(), n = 0, t0 = last;
  uint32_t now = millis();
  pavi::step(min(now - last, 50UL) / 1000.0f);
  last = now;
  char line[32];

  // BLE: a message is complete when a write ends (no new byte this loop, or a line break)
  static uint8_t rx[200];
  static int rxN = 0;
  bool got = false;
  while (bleuart.available()) {
    int c = bleuart.read();
    got = true;
    if (c != '\n' && c != '\r' && rxN < (int)sizeof rx) rx[rxN++] = c;
    else if (rxN) toAscii(rx, rxN, msg, sizeof msg), rxN = 0, show(2);
  }
  if (!got && rxN) toAscii(rx, rxN, msg, sizeof msg), rxN = 0, show(2);

  static bool wasConn = false;
  if (bool(Bluefruit.connected()) != wasConn) {
    wasConn = !wasConn;
    strcpy(msg, wasConn ? "connected" : "disconnected");
    show(1);
  }

  static uint32_t presses = 0;
  static bool btnPrev = HIGH;
  bool btn = digitalRead(BTN);  // loop is ~33 ms: debounces itself
  if (btn != btnPrev && (btnPrev = btn) == LOW) {
    snprintf(line, sizeof line, "button %lu\n", ++presses);
    tell(line);
    buzzT0 = now, buzzN = 1;
    pavi::cheer();
  }

  static uint32_t btnDown = 0;  // held OFF_HOLD_MS: power off
  if (btn == HIGH) btnDown = 0;
  else if (!btnDown) btnDown = now;
  else if (now - btnDown >= OFF_HOLD_MS) powerOff();

  uint32_t e = now - buzzT0;
  digitalWrite(MOTOR, e < buzzN * 240 && e % 240 < 120);

  static uint32_t shakes = 0, lastShake = 0;
  if (imuOk && e >= buzzN * 240 + MOTOR_MASK_MS) {  // motor vibration must not count as a shake
    float ax = imu.readFloatAccelX(), ay = imu.readFloatAccelY(), az = imu.readFloatAccelZ();
    float dev = fabsf(sqrtf(ax * ax + ay * ay + az * az) - 1.0f);
    if (dev > SHAKE_G && now - lastShake > SHAKE_GAP_MS) {
      lastShake = now;
      pavi::shake();
      snprintf(line, sizeof line, "shake %lu (%.2f g)\n", ++shakes, dev);
      tell(line);
    }
  }

  drawBubble(now);
  tft.startWrite();
  tft.setAddrWindow(0, 0, 240, 240);
  for (int y0 = 0; y0 < pavi::N; y0 += BAND) {
    uint16_t *p = strip;
    for (int y = y0; y < y0 + BAND; y++) {
      uint16_t *row = p;
      for (const uint8_t *s = pavi::fb + y * pavi::N, *end = s + pavi::N; s < end; s++)
        for (int k = 0; k < SC; k++) *p++ = pal[*s];
      for (int k = 1; k < SC; k++, p += 240) memcpy(p, row, 240 * 2);
    }
    tft.writePixels(strip, 240 * SC * BAND, true, true);
  }
  tft.endWrite();

  n++;
  if (now - t0 >= 1000) {
    Serial.printf("%lu fps\n", n);
    n = 0;
    t0 += 1000;
  }
}
