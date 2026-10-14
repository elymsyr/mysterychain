// Nausicaa + Teto: 80x80 scene (nausicaa.h) scaled 3x and pushed to the 240x240 screen in strips over SPI DMA.
// Shake the board: Teto leaps off her shoulder and Nausicaa tries to catch him for 6 s; then she kneels and holds out
// her hand, and Teto climbs back to her shoulder. Everything else is the same as ../cat.
// BLE (Nordic UART, name "MysteryChain"): text from the phone shows in a speech bubble, the motor buzzes twice,
// Nausicaa cheers. Button and shake events go to the phone as "button N" / "shake N".
// Motion is tied to real time: speed stays the same if fps drops. Serial monitor prints fps once a second.
#include <Adafruit_TinyUSB.h>  // for Serial
#include <Adafruit_GC9A01A.h>
#include <LSM6DS3.h>
#include <bluefruit.h>
#include "nausicaa.h"

const uint8_t TFT_CS = D1, TFT_DC = D2, TFT_RST = D3, MOTOR = D6, BTN = D0;
const uint32_t SPI_HZ = 32000000;          // if the image glitches: 16000000 (~17 fps)
const int SC = 240 / nausicaa::N, BAND = 8;  // 3x scale; strip of 8 logical rows = 11.5 KB

const float SHAKE_G = 0.5;           // shake when |a| deviates from 1 g by this much
const uint32_t SHAKE_GAP_MS = 400;   // don't count one shake several times
const uint32_t MOTOR_MASK_MS = 400;  // ignore shakes this long after the motor stops (use the value measured in T8)

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);
LSM6DS3 imu(I2C_MODE, 0x6A);
BLEUart bleuart;
bool imuOk;
uint16_t pal[nausicaa::NCOL], strip[240 * SC * BAND];

// Route Adafruit_GFX text drawing into the scene buffer (by color index); clip window on x.
struct FbGfx : Adafruit_GFX {
  int16_t clipL = 0, clipR = nausicaa::N;
  FbGfx() : Adafruit_GFX(nausicaa::N, nausicaa::N) {}
  void drawPixel(int16_t x, int16_t y, uint16_t c) override {
    if (x >= clipL && x < clipR) nausicaa::px(x, y, c);
  }
} gfx;

char msg[64];
uint32_t msgT0, msgDur, buzzT0, buzzN;  // vibration: buzzN pulses, 120 ms on / 120 ms off

void show(uint32_t pulses) {  // show msg in the bubble, buzz, Nausicaa cheers
  msgT0 = buzzT0 = millis();
  msgDur = constrain(2 * (strlen(msg) * 6 + 20) * 40, 4000, 15000);  // scrolling text loops at least twice
  buzzN = pulses;
  nausicaa::cheer();
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
  gfx.fillRoundRect(L, Y, R - L, H, 4, nausicaa::INK);
  gfx.fillRoundRect(L + 1, Y + 1, R - L - 2, H - 2, 3, nausicaa::CREAM);
  int w = strlen(msg) * 6, inner = R - L - 6;
  gfx.setTextColor(nausicaa::INK);
  gfx.setTextWrap(false);
  gfx.clipL = L + 2, gfx.clipR = R - 2;
  if (w <= inner) {
    gfx.setCursor((L + R - w) / 2 + 1, Y + 3);
    gfx.print(msg);
  } else {  // long message: scrolling text, 25 px/s
    int x = L + 3 - (int)((now - msgT0) / 40 % (w + 20));
    for (int k = 0; k < 2; k++, x += w + 20) gfx.setCursor(x, Y + 3), gfx.print(msg);
  }
  gfx.clipL = 0, gfx.clipR = nausicaa::N;
}

void setup() {
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);  // the core drives it HIGH at boot; keep P0.31 safe once a battery is attached
  pinMode(MOTOR, OUTPUT);
  pinMode(BTN, INPUT_PULLUP);
  Serial.begin(115200);
  tft.begin(SPI_HZ);
  for (int i = 0; i < nausicaa::NCOL; i++) {
    uint16_t c = tft.color565(nausicaa::PAL[i][0], nausicaa::PAL[i][1], nausicaa::PAL[i][2]);
    pal[i] = c << 8 | c >> 8;  // display wants big endian: swap once here, no swap while sending
  }
  nausicaa::init();
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
}

void loop() {
  static uint32_t last = millis(), n = 0, t0 = last;
  uint32_t now = millis();
  nausicaa::step(min(now - last, 50UL) / 1000.0f);
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
    nausicaa::cheer();
  }

  uint32_t e = now - buzzT0;
  digitalWrite(MOTOR, e < buzzN * 240 && e % 240 < 120);

  static uint32_t shakes = 0, lastShake = 0;
  if (imuOk && e >= buzzN * 240 + MOTOR_MASK_MS) {  // motor vibration must not count as a shake
    float ax = imu.readFloatAccelX(), ay = imu.readFloatAccelY(), az = imu.readFloatAccelZ();
    float dev = fabsf(sqrtf(ax * ax + ay * ay + az * az) - 1.0f);
    if (dev > SHAKE_G && now - lastShake > SHAKE_GAP_MS) {
      lastShake = now;
      nausicaa::shake();
      snprintf(line, sizeof line, "shake %lu (%.2f g)\n", ++shakes, dev);
      tell(line);
    }
  }

  drawBubble(now);
  tft.startWrite();
  tft.setAddrWindow(0, 0, 240, 240);
  for (int y0 = 0; y0 < nausicaa::N; y0 += BAND) {
    uint16_t *p = strip;
    for (int y = y0; y < y0 + BAND; y++) {
      uint16_t *row = p;
      for (const uint8_t *s = nausicaa::fb + y * nausicaa::N, *end = s + nausicaa::N; s < end; s++)
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
