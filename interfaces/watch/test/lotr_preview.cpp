// Draft clips of the Gandalf / Sauron scenes (same .raw format as preview.cpp, so preview.py makes the GIFs):
//   g++ -std=c++17 -O2 -I../firmware -I ~/Arduino/libraries/Adafruit_GFX_Library lotr_preview.cpp -o /tmp/lotr_g
//   g++ ... -DSAURON ... -o /tmp/lotr_s
//   /tmp/lotr_g <out dir> [clip]
// The Gandalf build first runs the bridge game's self-check (a bot that plays it right never cracks the bridge, one
// that never presses breaks it).
// The Sauron build first runs the maze's: every cell reachable at every size, a bot rolls the Ring to the Eye through
// each level's maze (nobody in the way).
#include <cassert>
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
#ifdef SAURON
uint8_t *VIEW() { return view(); }  // the maze game draws finer
int VN() { return viewN(); }
#else
uint8_t *VIEW() { return fb; }
int VN() { return N; }
#endif

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
    if (game.on) {
      char s[24];
      gameLabel(s, sizeof s);
      if (s[0]) bubble(s, t, game.labelY);
    }
    uint8_t pal[256][3] = {};
    memcpy(pal, PAL, sizeof PAL);
    fwrite(pal, 1, sizeof pal, f);
    static uint8_t scr[240 * 240];
    for (int y = 0; y < 240; y++)
      for (int x = 0; x < 240; x++) scr[y * 240 + x] = VIEW()[y * VN() / 240 * VN() + x * VN() / 240];
    fwrite(scr, 1, sizeof scr, f);
  }
  fclose(f);
  printf("%s\n", c.name.c_str());
}

#ifndef SAURON
// The bridge game's bot, at the firmware's input rate: stays in its lane and taps `react` s after the ring closes
// (a block, or a strike if he isn't aiming there), taps the free strike; tilts to the open side while he shields.
void bot(bool play, float react = 0) {
  static bool held = false;
  gameTilt(play ? (duel.phase == GUARD ? duel.open : duel.lane) * 0.3f : 0);
  bool want = duel.phase == GUARD ? duel.lane == duel.open : counter() || (windowOpen() && atk.t - atk.dur >= react);
  held = play && want && !held;  // a tap: down one frame, up the next
  gameInput(held);
}
void selfCheck() {
  for (int play = 1; play >= 0; play--) {
    init();
    gameStart();
    for (float t = 0; t < 90 && !game.dead; t += 0.045f) bot(play), step(0.045f);
    printf("bridge bot %s: score %d level %d cracks %d white %d\n", play ? "plays" : "idle", game.score, duel.level, duel.cracks, white > 0.5f);
    if (play) assert(duel.cracks == 0 && duel.level >= 3 && white > 0.5f);
    else assert(game.dead && game.score == 0);
    gameStop();
  }
  init();  // a masher taps all the time: every blow is a whiff
  gameStart();
  for (float t = 0; t < 90 && !game.dead; t += 0.045f) gameInput(int(t / 0.045f) & 1), step(0.045f);
  printf("bridge masher: score %d level %d dead %d\n", game.score, duel.level, game.dead);
  assert(game.dead);
  gameStop();
  for (float react : {0.15f, 0.25f}) {  // a human reacting to the burst: not asserted, shows where the window bites
    init();
    gameStart();
    float t = 0;
    for (; t < 300 && !game.dead; t += 0.045f) bot(true, react), step(0.045f);
    printf("bridge bot reacting in %.2f s: score %d level %d lasted %.0f s\n", react, game.score, duel.level, t);
    gameStop();
  }
  pet::best() = 0;  // the clips start from no high score
}
#endif

#ifdef SAURON
// The maze bot: knows the way (breadth-first from the Eye), tilts toward the next cell, damped. Doesn't dodge anyone.
int toEye[MAXC];
void plan() {
  static int q[MAXC];
  int cells = ringOff[mz.rings + 1], h = 0, n = 0;
  for (int c = 0; c < cells; c++) toEye[c] = -1;
  toEye[0] = 0;
  for (int c = 1; c <= ringN[1]; c++)
    if (link_[c] & 2) toEye[c] = 1, q[n++] = c;
  while (h < n) {
    int c = q[h++], nb[5], m = ways(c, nb, false);
    for (int k = 0; k < m; k++)
      if (toEye[nb[k]] < 0) toEye[nb[k]] = toEye[c] + 1, q[n++] = nb[k];
  }
}
void mazeBot(bool play) {
  static float lastT = 1e9f;
  if (!mz.zeroed || !play) return gameTilt2(0, 0);
  if (mz.t < lastT) plan();  // a new maze
  lastT = mz.t;
  int c = cellAt(mz.x, mz.y);
  if (c <= 0) return gameTilt2(-mz.x * 0.03f, -mz.y * 0.03f);
  int nb[5], m = ways(c, nb, true), nx = nb[0];
  for (int k = 0; k < m; k++)
    if (toEye[nb[k]] < toEye[nx]) nx = nb[k];
  int i = ringOf(c), j = ringOf(nx);
  float a = atan2f(mz.y, mz.x), r = midR(j), ta = cellA(nx);
  if (j < i) ta = cellA(c);  // in: from the middle of its cell, straight in
  if (j != i && fabsf(wrapPi(a - ta)) * hypotf(mz.x, mz.y) > 1.5f) r = midR(i);  // in or out: along to the door first
  float tx = r * cosf(ta), ty = r * sinf(ta);
  auto g = [](float v) { return clampf(v / SPEED_G + copysignf(DEAD_G, v), -0.3f, 0.3f); };  // past the dead zone
  gameTilt2(g(6 * (tx - mz.x)), g(6 * (ty - mz.y)));
}
void selfCheck() {
  for (int rings = 3; rings <= MAXR; rings++) {  // every cell reachable from the Eye
    buildMaze(rings);
    plan();
    for (int c = 0; c < ringOff[rings + 1]; c++) assert(toEye[c] >= 0);
    assert(ringOff[rings + 1] <= MAXC);
  }
  printf("maze cells at %d rings: %d, outer radius %.0f px\n", MAXR, ringOff[MAXR + 1], outerR());
  init(), gameStart();  // idle on level 1: nobody there, nothing happens
  for (float t = 0; t < 20; t += 0.045f) mazeBot(false), step(0.045f);
  assert(!game.dead && game.score == 0);
  gameStop();
  init(), gameStart();  // with nobody in the way it solves every size
  for (int lvl = 1; lvl <= MAXR; lvl++) {
    mz.level = lvl - 1, startLevel();
    for (auto &w : walkers) w.on = false;
    float t = 0;
    for (; t < 300 && mz.wonT < 0; t += 0.045f) mazeBot(true), step(0.045f);
    printf("maze level %d (%d rings): solved in %.1f s\n", lvl, mz.rings, t);
    fflush(stdout);
    assert(mz.wonT >= 0);
  }
  gameStop();
  for (int k = 0; k < 3; k++) {  // with the Fellowship (the bot doesn't dodge): not asserted
    init(), gameStart();
    float t = 0;
    for (; t < 300 && !game.dead; t += 0.045f) mazeBot(true), step(0.045f);
    printf("maze bot, not dodging: score %d, caught after %.0f s\n", game.score, t);
    gameStop();
  }
  pet::best() = 0;
}
#endif

int main(int argc, char **argv) {
  selfCheck();
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
      {"maze", 13 * 60, CLEAR, 40, [](float a, float) { if (a == 0) gameStart(); mazeBot(true); }},
      {"maze_big", 13 * 60, CLEAR, 30, [](float a, float) {
         if (a == 0) gameStart(), mz.level = 6, startLevel();
         mazeBot(true);
       }},
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
      {"bridge", 22 * 60, CLOUDY, 40, [](float a, float) { if (a == 0) gameStart(); bot(true); }},
      {"bridge_lost", 22 * 60, CLOUDY, 16, [](float a, float) { if (a == 0) gameStart(); bot(a < 4); }},
#endif
  };
  for (const Clip &c : clips)
    if (argc < 3 || c.name == argv[2]) run(c, dir);
}
