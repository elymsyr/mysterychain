// Battery gauge: a pixel-art battery at the bottom of the screen, on the scenes' 3x pixel grid, drawn over the scaled
// scene in its own colors (any palette, find's inverted one too). Plain C++: test/battery_test.cpp checks it on the PC.
// The sketch reads the voltage and decides when it shows (firmware.ino, drawBattery()).
#pragma once
#include <cmath>
#include <cstdint>

namespace bat {

// LiPo at a light load: mV -> %, every 100 mV from 3400 (the firmware turns off there) to 4200.
// ponytail: a generic 1-cell curve; if the gauge sits full too long or drops fast at the end, log a discharge and refit.
const uint8_t CURVE[] = {0, 3, 8, 22, 46, 63, 78, 90, 100};
inline int percent(float mv) {
  float i = (mv - 3400) / 100;
  if (i <= 0) return 0;
  if (i >= 8) return 100;
  int k = int(i);
  return int(CURVE[k] + (CURVE[k + 1] - CURVE[k]) * (i - k) + 0.5f);
}

struct Look {
  bool on;        // drawn at all
  float fill;     // 0..1 of the inside
  uint32_t rgb;   // fill color
  bool bolt;      // USB in: a lightning bolt over the fill
};

const int SC = 3, X0 = 33, Y0 = 69;  // scene px: the art's top left (scaled 3x like the scene)
const uint32_t EDGE = 0x5A4C66, BODY = 0xFFF7EA, BOLT = 0x5A4C66;
const uint32_t GREEN = 0x6FD68A, YELLOW = 0xFFCB4D, RED = 0xFF6F6A;

// E edge, B body, F fill (10 columns), '.' the scene shows through. Rounded corners, the nub on the right.
const char *const ART[] = {".EEEEEEEEEEEE..", "EBBBBBBBBBBBBE.", "EBFFFFFFFFFFBEE", "EBFFFFFFFFFFBEE",
                           "EBFFFFFFFFFFBEE", "EBFFFFFFFFFFBEE", "EBBBBBBBBBBBBE.", ".EEEEEEEEEEEE.."};
const int AW = 15, AH = 8, FX = 2, FY = 2, FW = 10;
const char *const BOLT_ART[] = {"..##", ".##.", "####", ".##.", "##.."};  // over the fill's middle
const int BX = FX + 3, BY = FY - 1;  // one row into the body: 5 rows tall

inline uint16_t be565(uint32_t rgb) {  // 0xRRGGBB -> big-endian RGB565, what the display gets
  uint16_t c = (rgb >> 8 & 0xF800) | (rgb >> 5 & 0x7E0) | (rgb >> 3 & 0x1F);
  return c << 8 | c >> 8;
}
inline uint32_t lighter(uint32_t rgb) {  // halfway to white: the shine on the fill's top row
  return (rgb >> 1 & 0x7F7F7F) + 0x808080;
}

// Paints the gauge into a strip of 240-px rows that starts at screen row y0.
inline void draw(const Look &k, uint16_t *strip, int y0, int rows) {
  if (!k.on) return;
  int n = int(fminf(fmaxf(k.fill, 0), 1) * FW + 0.5f);  // filled columns
  for (int y = y0; y < y0 + rows; y++) {
    int ay = y / SC - Y0;
    if (ay < 0 || ay >= AH) continue;
    for (int ax = 0; ax < AW; ax++) {
      char c = ART[ay][ax];
      if (c == '.') continue;
      uint32_t rgb = c == 'E' ? EDGE : c == 'B' || ax - FX >= n ? BODY : ay == FY ? lighter(k.rgb) : k.rgb;
      int bx = ax - BX, by = ay - BY;
      if (k.bolt && bx >= 0 && bx < 4 && by >= 0 && by < 5 && BOLT_ART[by][bx] == '#') rgb = BOLT;
      uint16_t p = be565(rgb), *o = strip + (y - y0) * 240 + (X0 + ax) * SC;
      for (int i = 0; i < SC; i++) o[i] = p;
    }
  }
}

}  // namespace bat
