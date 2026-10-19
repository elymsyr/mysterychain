// Plays a pet scene on a PC and dumps clips of every animation, for a look before flashing (preview.py makes GIFs + a page):
//   g++ -std=c++17 -O2 -I../firmware -I ~/Arduino/libraries/Adafruit_GFX_Library preview.cpp -o /tmp/preview_cat
//   g++ ... -DPAVI ... -o /tmp/preview_pavi   (-DNAUSICAA: Nausicaa and Teto)
//   /tmp/preview_cat <out dir>   (one <clip>.raw per clip: per frame a 256-entry RGB palette, then the 240x240 screen indices)
// The speech bubble and the music marquee are redrawn here the way firmware.ino draws them (keep the two in step).
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#if defined(PAVI)
#include "pavi.h"
#elif defined(NAUSICAA)
#include "nausicaa.h"
#else
#include "cat.h"
#endif
#include "glcdfont.c"

using namespace scene;
const float FRAME_S = 1 / 25.f;  // clip frame rate
const int HEART = NCOL;     // firmware's extra palette entries: heart, then the weather icon's sun, cloud grey, rain blue
int icon = -1;              // clock bubble: weather icon (pet::WX_ICON) left of the time, -1 = none

// ---- firmware.ino's overlay, by color index ----
int clipL = 0, clipR = N;
void text(int x, int y, const char *s, uint8_t c) {  // Adafruit_GFX classic 5x7 font, 6 px a character
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
void bubble(const char *s, uint8_t color, float t, int Y = 11) {
  const int L = 12, R = 68, H = 13;
  roundRect(L, Y, R - L, H, 4, INK);
  roundRect(L + 1, Y + 1, R - L - 2, H - 2, 3, CREAM);
  int w = strlen(s) * 6, inner = R - L - 6;
  clipL = L + 2, clipR = R - 2;
  if (icon >= 0) {  // icon, 2 px gap, the time: centered together
    static const char K[] = "KGBY";
    int x0 = (L + R - (w + 11)) / 2 + 1;
    for (int r = 0; r < pet::WX_H; r++)
      for (int c = 0; c < 9; c++)
        if (const char *k = strchr(K, pet::WX_ICON[icon][r][c]); k && *k) px(x0 + c, Y + 2 + r, k == K ? INK : HEART + (k - K));
    text(x0 + 11, Y + 3, s, color);
  } else if (w <= inner) text((L + R - w) / 2 + 1, Y + 3, s, color);
  else
    for (int k = 0, x = L + 3 - (int)(t * 1000 / 40) % (w + 20); k < 2; k++, x += w + 20) text(x, Y + 3, s, color);
  clipL = 0, clipR = N;
}
void marquee(const char *s, float t) {  // music screen: no bubble, always scrolling, in from the right, outlined
  const int L = 8, R = 72, Y = 14, GAP = 24;
  int w = strlen(s) * 6, x = R - (int)(t * 1000 / 40);
  while (x + w + GAP <= L) x += w + GAP;
  clipL = L, clipR = R;
  for (; x < R; x += w + GAP) {
    for (int k = 0; k < 4; k++) text(x + (k == 0) - (k == 1), Y + (k == 2) - (k == 3), s, INK);
    text(x, Y, s, CREAM);
  }
  clipL = 0, clipR = N;
}

// ---- one clip ----
struct Clip {
  std::string name;
  int minute;                             // clock, -1 = unknown; < -1: time-lapse over the whole clip
  float secs;
  std::function<void()> setup;            // stats and pose at the start
  std::function<void(float, float)> act;  // (t before, t after) each frame: events
};
char msg[64];
float msgT0 = -1, msgDur = 0;
bool msgMusic;
uint8_t msgColor;

void say(const char *s, float t, uint8_t color = INK) {  // firmware show(): plain message, buzzes, cheers
  strcpy(msg, s), msgT0 = t, msgColor = color, msgMusic = false;
  msgDur = fminf(fmaxf(2 * (strlen(s) * 6 + 20) * 0.04f, 4), 15);
  cheer();
  pet::message();
}
void song(const char *title, float t) {  // firmware #music
  snprintf(msg, sizeof msg, "\x0E %s", title), msgT0 = t, msgColor = INK, msgMusic = true;
  msgDur = fmaxf(fminf(fmaxf(2 * (strlen(msg) * 6 + 20) * 0.04f, 4), 15), 12);
}
bool at(float t0, float t1, float when) { return t0 < when && when <= t1; }

void run(const Clip &c, const char *dir) {
  pet::s = pet::Saved();
  msg[0] = 0, msgT0 = -1, msgMusic = false, icon = -1;
  pet::wx = pet::CLEAR;
#if !defined(PAVI) && !defined(NAUSICAA)
  snowCover = 0, boltT = 3;
#endif
  game.on = false;
  music(false);
  pose(IDLE, 0.5f);
  vis.on = false;
  awake = 0;
#ifdef NAUSICAA
  tgo(SHOULDER, 0);
#endif
  int minute = c.minute;
  c.setup();  // before tint: it may set the weather
  tint(minute < -1 ? 0 : minute);
  for (int i = 0; i < 25; i++) setMood(pet::mood(minute < 0 ? -1 : minute / 60)), step(FRAME_S);  // settle into the pose
  std::string path = std::string(dir) + "/" + c.name + ".raw";
  FILE *f = fopen(path.c_str(), "wb");
  for (float t = 0; t < c.secs; t += FRAME_S) {
    if (c.minute < -1) tint(minute = int(1440 * t / c.secs + 360) % 1440);  // time-lapse from 06:00
    setMood(pet::mood(minute < 0 ? -1 : minute / 60));
    c.act(t, t + FRAME_S);
    bool on = msg[0] && t - msgT0 <= msgDur;
    music(on && msgMusic);
    step(FRAME_S);
    if (on && msgMusic && !game.on) marquee(msg, t - msgT0);
    else if (on) bubble(msg, msgColor, t - msgT0, game.on ? game.labelY : 11);
    else if (game.on) {
      char g[24];
      gameLabel(g, sizeof g);
      bubble(g, INK, t, game.labelY);
    }
    uint8_t pal[256][3] = {};
    memcpy(pal, PAL, sizeof PAL);
    pal[HEART][0] = 255, pal[HEART][1] = 120, pal[HEART][2] = 150;
    const uint8_t ICON_RGB[3][3] = {{150, 158, 172}, {60, 120, 230}, {250, 180, 30}};  // G cloud, B rain, Y sun
    memcpy(pal[HEART + 1], ICON_RGB, sizeof ICON_RGB);
    fwrite(pal, 1, sizeof pal, f);
#if !defined(PAVI) && !defined(NAUSICAA)
    const uint8_t *v = view;  // the cat's game: the wider view at 2x, like firmware.ino
    int vn = VN;
#else
    const uint8_t *v = fb;
    int vn = N;
#endif
    static uint8_t scr[240 * 240];
    for (int y = 0; y < 240; y++)
      for (int x = 0; x < 240; x++) scr[y * 240 + x] = v[y * vn / 240 * vn + x * vn / 240];
    fwrite(scr, 1, sizeof scr, f);
  }
  fclose(f);
  printf("%s\n", c.name.c_str());
}

#ifdef PAVI
// Game autopilot: tilts to a lane with nothing close ahead; after `until` s it steers into the nearest one (a crash).
void autopilot(float t, float until) {
  auto ahead = [](int l) {  // distance to the next obstacle in lane l
    float d = 1e9f;
    for (const Thing &o : obs)
      if (fabsf(o.wx - l * LANE) < 0.1f && o.w > dist + ZP - 0.3f) d = fminf(d, o.w - dist - ZP);
    return d;
  };
  static int want = 0;
  if (t >= until) {
    for (int l = -1; l <= 1; l++)
      if (ahead(l) < ahead(want)) want = l;
  } else if (ahead(want) < 5)
    for (int l : {0, -1, 1})
      if (ahead(l) > ahead(want)) want = l;
  gameTilt(want * 0.6f);
}
#elif defined(NAUSICAA)
// Game autopilot: flies under the lowest plane and fires, lets the gun cool before it jams, sidesteps bullets close
// above; after `until` s it stops firing and flies into the nearest bullet or plane (a crash).
void autopilot(float t, float until) {
  float to = 40, best = -1e9f;
  for (const Plane &p : planes)
    if (p.on && p.y < PY - 12 && p.y > best && (t >= until || p.y + 8 > SHOT_TOP)) best = p.y, to = p.x;
  static bool cooling = false;
  if (heat > 0.85f) cooling = true;
  if (heat < 0.3f) cooling = false;
  bool fire = !cooling && best > -1e9f;
  for (const Dot &b : bullets)
    if (b.on && b.y > PY - 25 && b.y < PY + 2 && fabsf(b.x - mx) < 5) {
      if (t >= until) to = b.x;
      else to = mx + (b.x < mx ? 10 : -10);
    }
  if (t >= until) fire = false;
  gameTilt((to - 40) / REACH_X * TILT_FULL);
  gameInput(fire);
}
#else
// Runner autopilot: hops pots (also under birds), holds over fences; stops trying after `until` s.
float pressT = -1;
void autopilot(float t, float until) {
  bool held = false;
  for (const Ob &o : game.ob) {
    float d = o.x - hero.x;
    if (o.passed || d < 0) continue;
    if (pressT < 0 && t < until && d - 4 < game.speed * (o.kind == FENCE ? 0.35f : 0.25f)) pressT = t;  // mid-window
    held = pressT >= 0 && t - pressT < (o.kind == FENCE ? 0.4f : 0.1f);
  }
  if (hero.y >= GROUND && pressT >= 0 && t - pressT > 0.2f) pressT = -1;
  gameInput(held);
}
#endif

int main(int argc, char **argv) {
  const char *dir = argc > 1 ? argv[1] : ".";
  init();
  auto none = [] {};
  auto idle = [](float, float) {};
  auto stats = [](float full, float happy, float energy) { pet::s.full = full, pet::s.happy = happy, pet::s.energy = energy; };
  std::vector<Clip> clips = {
      {"morning", 7 * 60 + 15, 4, none, [](float a, float b) { if (at(a, b, 1.5f)) cheer(); }},
      {"noon", 13 * 60, 6, none, [](float a, float b) { if (at(a, b, 0.6f)) shake(); }},
      {"evening", 19 * 60 + 10, 4, none, [](float a, float b) { if (at(a, b, 1.5f)) cheer(); }},
      {"night", 21 * 60 + 30, 4, none, idle},
      {"daycycle", -2, 12, none, idle},
      {"music", 13 * 60, 12, none, [](float a, float b) {
         if (at(a, b, 0.4f)) song("Duman - Bu Aksam", b);
         if (at(a, b, 7)) shake();
       }},
      {"travel_morning", 7 * 60 + 15, 8, none, [](float a, float b) { if (at(a, b, 0.2f)) travel(30); }},
      {"travel_noon", 13 * 60, 10, none, [](float a, float b) {
         if (at(a, b, 0.2f)) travel(30);
         if (at(a, b, 6)) shake();
       }},
      {"travel_evening", 19 * 60 + 10, 8, none, [](float a, float b) { if (at(a, b, 0.2f)) travel(30); }},
      {"travel_night", 21 * 60 + 30, 8, none, [](float a, float b) { if (at(a, b, 0.2f)) travel(30); }},
      {"travel_daycycle", -2, 14, none, [](float a, float b) { if (at(a, b, 0.05f)) travel(30); }},
      {"travel_end", 16 * 60, 8, none, [](float a, float b) { if (at(a, b, 0.2f)) travel(4); }},
      {"music_night", 21 * 60 + 30, 7, none, [](float a, float b) { if (at(a, b, 0.4f)) song("Sezen Aksu - Firuze", b); }},
      {"message", 13 * 60, 5, none, [](float a, float b) { if (at(a, b, 0.4f)) say("Aksam yemege gel!", b); }},
      {"sleep", 2 * 60, 9, none, [](float a, float b) {
         if (at(a, b, 2)) say("iyi geceler", b);
         if (at(a, b, 5)) shake();
       }},
      {"eat", 13 * 60, 4.5f, [&] { stats(50, 80, 80); }, [](float a, float b) { if (at(a, b, 0.3f)) pet::feed(), eat(); }},
      {"refuse", 13 * 60, 2.5f, [&] { stats(95, 80, 80); }, [](float a, float b) { if (at(a, b, 0.3f)) refuse(); }},
      {"petting", 13 * 60, 3, none, [](float a, float b) { if (at(a, b, 0.3f)) purr(); }},
      {"tired", 15 * 60, 4, [&] { stats(80, 80, 20); }, idle},
      {"sad", 15 * 60, 5, [&] { stats(80, 15, 80); }, idle},
      {"hungry", 15 * 60, 6, [&] { stats(15, 80, 80); }, idle},
      {"angry", 15 * 60, 3, [&] { stats(5, 80, 80); }, [](float a, float b) { if (at(a, b, 0.5f)) shake(); }},
      {"visitor_day", 11 * 60, 9, none, [](float a, float b) { if (at(a, b, 0.2f)) visit(); }},
      {"visitor_night", 21 * 60 + 30, 9, none, [](float a, float b) { if (at(a, b, 0.2f)) visit(); }},
#if defined(NAUSICAA)
      {"wave", 13 * 60, 3, none, [](float a, float b) { if (at(a, b, 0.3f)) pose(WAVE, 2.4f); }},
      {"sit", 16 * 60, 8, none, [](float a, float b) { if (at(a, b, 0.3f)) pose(SIT, 7.5f); }},
      {"breeze", 10 * 60, 4, none, [](float a, float b) { if (at(a, b, 0.3f)) pose(BREEZE, 3.5f); }},
      {"twirl", 13 * 60, 3, none, [](float a, float b) { if (at(a, b, 0.3f)) pose(TWIRL, 1.6f); }},
      {"teto_play", 14 * 60, 12, none, [](float a, float b) { if (at(a, b, 0.5f)) shake(); }},
#elif defined(PAVI)
      {"wave", 13 * 60, 3, none, [](float a, float b) { if (at(a, b, 0.3f)) pose(WAVE, 2.4f); }},
      {"sit", 16 * 60, 8, none, [](float a, float b) { if (at(a, b, 0.3f)) pose(SIT, 7.5f); }},
      {"twirl", 13 * 60, 3, none, [](float a, float b) { if (at(a, b, 0.3f)) pose(TWIRL, 1.6f); }},
#else
      {"groom", 13 * 60, 4, none, [](float a, float b) { if (at(a, b, 0.3f)) pose(GROOM, 3.2f); }},
      {"tail_chase", 13 * 60, 3, none, [](float a, float b) { if (at(a, b, 0.3f)) pose(SPIN, 2.2f); }},
#endif
#if !defined(PAVI) && !defined(NAUSICAA)
      // Weather (A5): each kind once, by day and some by night; clock_icons taps the clock through all of them.
      {"wx_clear", 11 * 60, 8, [] { pet::wx = pet::CLEAR; }, [](float a, float b) { if (at(a, b, 0.5f)) pose(SUNBATHE, 6.5f); }},
      {"wx_cloudy", 13 * 60, 6, [] { pet::wx = pet::CLOUDY; }, [](float a, float b) { if (at(a, b, 3)) cheer(); }},
      {"wx_wind", 13 * 60, 9, [] { pet::wx = pet::WIND; }, [](float a, float b) { if (at(a, b, 0.3f)) visit(); }},
      {"wx_rain", 14 * 60, 10, [] { pet::wx = pet::RAIN; }, [](float a, float b) { if (at(a, b, 6)) shake(); }},
      {"wx_rain_night", 23 * 60 + 30, 7, [] { pet::wx = pet::RAIN; }, idle},
      {"wx_snow", 10 * 60, 10, [] { pet::wx = pet::SNOW, snowCover = 0.8f; }, [](float a, float b) {
         if (at(a, b, 0.3f) || at(a, b, 5)) pose(SNOWPLAY, 2.4f);
       }},
      {"wx_snow_night", 21 * 60 + 30, 6, [] { pet::wx = pet::SNOW, snowCover = 1; }, idle},
      {"wx_storm", 16 * 60, 10, [] { pet::wx = pet::STORM, boltT = 1.2f; }, idle},
      {"wx_storm_night", 22 * 60, 8, [] { pet::wx = pet::STORM, boltT = 1.5f; }, idle},
      {"wx_rain_travel", 13 * 60, 7, [] { pet::wx = pet::RAIN; }, [](float a, float b) { if (at(a, b, 0.2f)) travel(30); }},
      {"wx_snow_travel", 13 * 60, 7, [] { pet::wx = pet::SNOW, snowCover = 1; }, [](float a, float b) { if (at(a, b, 0.2f)) travel(30); }},
      {"clock_icons", 13 * 60, 12, none, [](float a, float b) {  // a tap every 2 s, the weather one further each time
         for (int k = 0; k < 6; k++)
           if (at(a, b, 2.f * k + 0.1f)) pet::wx = k, tint(13 * 60), icon = k, strcpy(msg, "13:00"), msgT0 = b, msgDur = 1.9f, msgMusic = false, msgColor = INK;
       }},
#endif
      {"dance", 13 * 60, 5, none, [](float a, float b) { if (at(a, b, 0.3f)) pose(DANCE, 4.5f); }},
#ifdef NAUSICAA
      {"game_night", 21 * 60 + 30, 8, [] { for (auto &b : pet::s.best) b = 40; }, [](float a, float b) {
         if (at(a, b, 0.3f)) gameStart(), game.score = 20;  // the late planes: corvettes and a Bumblecrow
         if (game.on) autopilot(b, 99);
       }},
#endif
      {"game", 17 * 60, 14, none, [](float a, float b) {
         if (at(a, b, 0.3f)) gameStart();
         if (game.on) autopilot(b, 11);
       }},
  };
  for (const Clip &c : clips)
    if (argc < 3 || c.name == argv[2]) run(c, dir);
}
