// Draft clips of the Gandalf / Sauron scenes (same .raw format as preview.cpp, so preview.py makes the GIFs):
//   g++ -std=c++17 -O2 -I../firmware -I ~/Arduino/libraries/Adafruit_GFX_Library lotr_preview.cpp -o /tmp/lotr_g
//   g++ ... -DSAURON ... -o /tmp/lotr_s
//   /tmp/lotr_g <out dir> [clip]
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#ifdef SAURON
#include "sauron.h"
using namespace sauron;
#else
#include "gandalf.h"
using namespace gandalf;
#endif
#include "glcdfont.c"

const float FRAME_S = 1 / 25.f;

// ---- firmware.ino's speech bubble (as in preview.cpp) ----
int clipL = 0, clipR = 80;
void text(int x, int y, const char *s, uint8_t c) {
  for (; *s; s++, x += 6)
    for (int i = 0; i < 5; i++)
      for (int j = 0, line = font[(uint8_t)*s * 5 + i]; j < 8; j++, line >>= 1)
        if ((line & 1) && x + i >= clipL && x + i < clipR) px(x + i, y + j, c);
}
void roundRect(int x0, int y0, int w, int h, int r, uint8_t c) {
  for (int y = y0; y < y0 + h; y++)
    for (int x = x0; x < x0 + w; x++) {
      int dx = x < x0 + r ? x0 + r - x : x >= x0 + w - r ? x - (x0 + w - r - 1) : 0;
      int dy = y < y0 + r ? y0 + r - y : y >= y0 + h - r ? y - (y0 + h - r - 1) : 0;
      if (dx * dx + dy * dy <= r * r) px(x, y, c);
    }
}
void bubble(const char *s, float t, int Y = 11) {
  const int L = 12, R = 68, H = 13;
  roundRect(L, Y, R - L, H, 4, INK);
  roundRect(L + 1, Y + 1, R - L - 2, H - 2, 3, CREAM);
  int w = strlen(s) * 6, inner = R - L - 6;
  clipL = L + 2, clipR = R - 2;
  if (w <= inner) text((L + R - w) / 2 + 1, Y + 3, s, INK);
  else
    for (int k = 0, x = L + 3 - (int)(t * 1000 / 40) % (w + 20); k < 2; k++, x += w + 20) text(x, Y + 3, s, INK);
  clipL = 0, clipR = 80;
}

struct Clip {
  std::string name;
  int minute;
  uint8_t wx;
  float secs;
  std::function<void(float, float)> act;
};
char msg[64];
float msgT0 = -1, msgDur = 0;
void say(const char *s, float t, bool react = true) {
  strcpy(msg, s), msgT0 = t, msgDur = fminf(fmaxf(2 * (strlen(s) * 6 + 20) * 0.04f, 3), 8);
  if (react) cheer();
}
bool at(float t0, float t1, float when) { return t0 < when && when <= t1; }

void run(const Clip &c, const char *dir) {
  msg[0] = 0, msgT0 = -1;
  pet::wx = c.wx, snowCover = c.wx == pet::SNOW ? 0.8f : 0, boltT = 1.5f;
  init();
  tint(c.minute < -1 ? 0 : c.minute);
  for (int i = 0; i < 10; i++) step(FRAME_S);
  FILE *f = fopen((std::string(dir) + "/" + c.name + ".raw").c_str(), "wb");
  for (float t = 0; t < c.secs; t += FRAME_S) {
    if (c.minute < -1) tint(int(1440 * t / c.secs + 360) % 1440);
    c.act(t, t + FRAME_S);
    step(FRAME_S);
    if (msg[0] && t - msgT0 <= msgDur) bubble(msg, t - msgT0);
    uint8_t pal[256][3] = {};
    memcpy(pal, PAL, sizeof PAL);
    fwrite(pal, 1, sizeof pal, f);
    static uint8_t scr[240 * 240];
    for (int y = 0; y < 240; y++)
      for (int x = 0; x < 240; x++) scr[y * 240 + x] = fb[y * N / 240 * N + x * N / 240];
    fwrite(scr, 1, sizeof scr, f);
  }
  fclose(f);
  printf("%s\n", c.name.c_str());
}

int main(int argc, char **argv) {
  const char *dir = argc > 1 ? argv[1] : ".";
  auto idle = [](float, float) {};
  using namespace pet;
  std::vector<Clip> clips = {
#ifdef SAURON
      {"mordor_day", 13 * 60, CLEAR, 8, idle},
      {"mordor_night", 23 * 60, CLEAR, 8, idle},
      {"cheer", 13 * 60, CLEAR, 4, [](float a, float b) { if (at(a, b, 0.4f)) say("Aksam yemege gel!", b); }},
      {"slam", 13 * 60, CLEAR, 4, [](float a, float b) { if (at(a, b, 0.4f)) shake(); }},
      {"ring", 21 * 60, CLEAR, 4.5f, [](float a, float b) { if (at(a, b, 0.3f)) ringUp(); }},
      {"gaze", 13 * 60, CLEAR, 6, [](float a, float b) { if (at(a, b, 0.3f)) gaze(); }},
      {"eruption", 19 * 60 + 15, CLEAR, 6, [](float a, float b) { if (at(a, b, 0.3f)) erupt(); }},
      {"nazgul", 16 * 60, CLOUDY, 7, [](float a, float b) { if (at(a, b, 0.2f)) nazgul(); }},
      {"storm", 22 * 60, STORM, 8, idle},
      {"ash_snow", 13 * 60, SNOW, 6, idle},
      {"daycycle", -2, CLEAR, 12, idle},
#else
      {"shire", 11 * 60, CLOUDY, 6, idle},
      {"shire_smoke", 16 * 60, CLOUDY, 5, [](float a, float b) { if (at(a, b, 0.3f)) smoke(); }},
      {"shire_firework", 20 * 60, WIND, 4, [](float a, float b) { if (at(a, b, 0.3f)) firework(); }},
      {"gondor_white", 13 * 60, CLEAR, 5, [](float a, float b) { if (at(a, b, 1.5f)) say("Aksam yemege gel!", b); }},
      {"gondor_night", 22 * 60, CLEAR, 5, idle},
      {"rivendell_rain", 14 * 60, RAIN, 6, idle},
      {"rivendell_snow", 9 * 60, SNOW, 6, idle},
      {"battle_storm", 17 * 60, STORM, 7, [](float a, float b) {
         if (at(a, b, 2)) shake(), say("YOU SHALL NOT PASS!", b, false);
       }},
      {"slam", 13 * 60, CLOUDY, 3, [](float a, float b) { if (at(a, b, 0.3f)) shake(); }},
      {"grey_to_white", 12 * 60, CLOUDY, 5, [](float a, float b) { if (at(a, b, 0.6f)) pet::wx = CLEAR; }},
      {"white_to_grey", 12 * 60, CLEAR, 5, [](float a, float b) { if (at(a, b, 0.6f)) pet::wx = RAIN; }},
      {"daycycle", -2, CLOUDY, 12, idle},
#endif
  };
  for (const Clip &c : clips)
    if (argc < 3 || c.name == argv[2]) run(c, dir);
}
