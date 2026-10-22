// PC check for battery.h: the mV -> % curve, and the gauge drawn over a scene-colored strip.
//   g++ -std=c++17 -I../firmware battery_test.cpp -o /tmp/battery_test && /tmp/battery_test [out.ppm]
// With a path it also writes the four looks (normal, low, charging, plugged in and full) side by side, 4x, to look at.
#include <cassert>
#include <cstdio>
#include "battery.h"

int main(int argc, char **argv) {
  assert(bat::percent(3000) == 0 && bat::percent(3400) == 0);
  assert(bat::percent(3750) == 34);  // halfway between 22 and 46
  assert(bat::percent(4200) == 100 && bat::percent(4400) == 100);
  for (int mv = 3400; mv < 4200; mv += 10) assert(bat::percent(mv) <= bat::percent(mv + 10));

  const bat::Look looks[] = {{true, 0.63f, bat::GREEN, false}, {true, 0.1f, bat::RED, false},
                             {true, 0.55f, bat::YELLOW, true}, {true, 1, bat::GREEN, true}};
  const int W = 51, H = 30, S = 3, X0 = bat::X0 * bat::SC - 3, Y0 = bat::Y0 * bat::SC - 3;
  static uint16_t strip[240 * 240], blank[240 * 240];
  for (int i = 0; i < 240 * 240; i++) {
    uint16_t c = i / 240 & 4 ? 0x0A5D : 0xA6EF;  // grass and sky stripes, big endian like the display
    strip[i] = blank[i] = c;
  }
  bat::draw({false}, strip, 0, 240);
  for (int i = 0; i < 240 * 240; i++) assert(strip[i] == blank[i]);  // hidden: untouched
  if (argc < 2) return puts("battery ok"), 0;
  FILE *f = fopen(argv[1], "wb");
  fprintf(f, "P6 %d %d 255\n", W * 4 * S, H * S);
  static uint8_t img[H * S][W * 4 * S][3];
  for (int l = 0; l < 4; l++) {
    for (int i = 0; i < 240 * 240; i++) strip[i] = blank[i];
    for (int y0 = 0; y0 < 240; y0 += 24) bat::draw(looks[l], strip + y0 * 240, y0, 24);  // in strips, like the sketch
    for (int y = 0; y < H * S; y++)
      for (int x = 0; x < W * S; x++) {
        uint16_t p = strip[(Y0 + y / S) * 240 + X0 + x / S], c = p << 8 | p >> 8;
        uint8_t *o = img[y][l * W * S + x];
        o[0] = (c >> 11) << 3, o[1] = (c >> 5 & 63) << 2, o[2] = (c & 31) << 3;
      }
  }
  fwrite(img, sizeof img, 1, f);
  fclose(f);
}
