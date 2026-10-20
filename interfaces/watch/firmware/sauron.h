// Sauron: a pet like the others (pet.h, pets.h look 4). 80x80 color-indexed buffer drawn 3x like the other scenes, no
// Arduino dependency (test/lotr_preview.cpp dumps clips).
// Always Mordor: Barad-dur with the Eye on the left, Mount Doom smoking on the right, lava cracks in the plain of
// Gorgoroth. The clock tints it (day = murky red, night = black with the lava glowing); the weather greys it, snow
// falls as ash, a storm sends lightning.
// Animations: idle (the visor glows, the Eye roams), cheer (mace up, the Eye flares), shake (mace slam, a ring of fire
// and cracks), the Ring (he raises it, it shines), the Eye's gaze (a beam sweeps the plain), Mount Doom erupts, a
// Nazgul on a fell beast flies by.
// Pet: his mood picks what he does (asleep, the Eye shut and the visor down to embers; holding up an empty goblet,
// tapping his mace; brooding, the Eye dim); fed, he drinks a goblet of lava; full, he waves it away; petted, he laughs.
// The music screen: he flies a fell beast over Gorgoroth (see "Flight").
#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "pet.h"

namespace sauron {

const int N = 80, GROUND = 68;
const float G = 90;  // lava and rock bits fall, px/s²

enum : uint8_t {
  SKY0, SKY1, SKY2, SKY3, CLOUD, CLOUD2, STAR, SUN, MOON,                                // sky: tinted by the clock
  FAR, FAR2, TOWER, TOWER2, MTN, MTN2, ROCK, ROCK2, ROCKL, CRUST, SHADOW,                // land: tinted by the clock
  LAVA, LAVA2, EYER, EYEO, EYEY, BEAM,
  INK, ARMORL, ARMOR, ARMOR2, ARMOR3, CLOAK, CLOAK2, LINING, LINING2, SLIT, SLIT2, GOLD, GOLD2,
  SMOKE, SMOKE2, CREAM, DROP, FLAKE, FLASH,
  FLOOR, FLOOR2, WALL, WALLD, ROBE, BEARD, SKIN, CLOAKG, LEATHER, HAIR, PALE, PALE2,  // the maze game: not tinted
  NCOL
};
uint8_t PAL[NCOL][3] = {};
const uint8_t FIXED[NCOL - LAVA][3] = {
    {255, 112, 20}, {255, 214, 90}, {196, 36, 10}, {255, 124, 20}, {255, 232, 130}, {255, 196, 96},  // lava, the Eye, its gaze
    {20, 16, 22},   {172, 170, 186}, {112, 110, 124}, {64, 62, 72}, {40, 38, 46},   // outline; armor: edge, light, mid, dark
    {52, 44, 54},   {34, 28, 36},    {140, 30, 24},   {92, 20, 18},                  // cloak, its folds, the red lining x2
    {255, 150, 50}, {200, 70, 30},   {240, 196, 70},  {196, 136, 30},               // visor glow x2, the Ring x2
    {84, 74, 74},   {54, 46, 48},    {250, 240, 220},                               // smoke x2, cream (bubble)
    {150, 120, 120}, {150, 144, 140}, {255, 220, 210},                              // rain, ash, lightning flash
    {62, 50, 46},   {52, 42, 40},    {150, 124, 104}, {96, 76, 66},                 // maze floor x2, wall, its shade
    {150, 150, 162}, {236, 236, 240}, {228, 182, 142}, {56, 112, 48},               // Gandalf's robe, beard; skin; Frodo's cloak
    {120, 78, 40},  {58, 38, 24},    {176, 186, 150}, {120, 200, 230},              // Aragorn's leather, hair; Gollum, his eyes
};
const uint8_t SKYLOOK[4][MOON + 1][3] = {  // pet::NIGHT, DAWN, DAY, DUSK: sky top to horizon; ash cloud, its shade; ember, sun, moon
    {{12, 6, 8}, {26, 10, 12}, {52, 16, 14}, {96, 30, 18}, {38, 26, 26}, {24, 16, 16}, {255, 120, 60}, {200, 80, 40}, {170, 150, 140}},
    {{46, 22, 28}, {90, 40, 38}, {150, 66, 44}, {196, 100, 50}, {86, 64, 60}, {58, 42, 42}, {255, 150, 80}, {240, 140, 60}, {200, 170, 150}},
    {{64, 38, 36}, {102, 58, 44}, {150, 88, 56}, {192, 122, 70}, {96, 80, 74}, {66, 54, 52}, {255, 170, 90}, {250, 190, 110}, {210, 190, 170}},
    {{36, 12, 18}, {84, 28, 28}, {150, 50, 34}, {214, 88, 38}, {70, 44, 42}, {44, 28, 30}, {255, 140, 70}, {255, 120, 50}, {200, 160, 140}},
};
const int NL = SHADOW - FAR + 1;
const uint8_t LANDC[NL][3] = {  // day colors, FAR..SHADOW: far range x2, tower x2, mountain x2, plain x3, crust, shade
    {70, 44, 42}, {44, 30, 32}, {30, 26, 32}, {54, 48, 56}, {76, 52, 44}, {50, 36, 34},
    {82, 66, 58}, {60, 48, 44}, {112, 92, 78}, {130, 34, 22}, {38, 30, 28},
};
const float FILT[4][4] = {{8, 2, 4, 0.55f}, {80, 30, 30, 0.15f}, {0, 0, 0, 0}, {60, 10, 10, 0.25f}};

int clockMin = -1;
float nightW = 1;
void tint(int minuteOfDay) {
  clockMin = minuteOfDay;
  int a, b;
  float f = pet::look(clockMin, a, b);
  nightW = (a == pet::NIGHT ? 1 - f : 0) + (b == pet::NIGHT ? f : 0);
  for (int k = 0; k <= MOON; k++)
    for (int c = 0; c < 3; c++) PAL[k][c] = lroundf(SKYLOOK[a][k][c] * (1 - f) + SKYLOOK[b][k][c] * f);
  for (int k = 0; k < NL; k++)
    for (int c = 0; c < 3; c++) {
      float v = LANDC[k][c];
      float va = v * (1 - FILT[a][3]) + FILT[a][c] * FILT[a][3], vb = v * (1 - FILT[b][3]) + FILT[b][c] * FILT[b][3];
      PAL[FAR + k][c] = lroundf(va * (1 - f) + vb * f);
    }
  memcpy(PAL[LAVA], FIXED, sizeof FIXED);
  float ov = pet::OVERCAST[pet::wx] * 0.6f, dark = pet::gloom();  // Mordor is grey already: half the overcast
  for (int k = SKY0; k <= CLOUD2; k++) pet::dim(PAL[k], ov, dark);
  for (int k = FAR; k <= SHADOW; k++) pet::dim(PAL[k], ov * 0.3f, dark);
}

// ---- Helpers ----
uint8_t fb[N * N], bg[N * N];
float T = 0;
uint32_t rs = 2463534242u;
float rnd() {
  rs ^= rs << 13, rs ^= rs >> 17, rs ^= rs << 5;
  return (rs >> 8) / 16777216.0f;
}
float clampf(float v, float lo, float hi) { return fminf(fmaxf(v, lo), hi); }
float h01(int i) {
  uint32_t h = i * 2654435761u;
  h ^= h >> 15, h *= 2246822519u, h ^= h >> 13;
  return (h & 0xffff) / 65536.0f;
}
// The maze game draws at twice the resolution (HN x HN, 1.5 screen px a step instead of 3): px() then paints a 2 x 2
// block, shifted hx, hy half px, so the Ring, the Fellowship and the scrolling maze move in half px.
const int HN = 2 * N;
uint8_t hi[HN * HN];
bool hiOn = false;
int hx = 0, hy = 0;
inline void px(int x, int y, uint8_t c) {
  if (hiOn) {
    x = 2 * x + hx, y = 2 * y + hy;
    for (int k = 0; k < 4; k++)
      if ((unsigned)(x + (k & 1)) < HN && (unsigned)(y + (k >> 1)) < HN) hi[(y + (k >> 1)) * HN + x + (k & 1)] = c;
  } else if ((unsigned)x < N && (unsigned)y < N) fb[y * N + x] = c;
}
uint8_t *view() { return hiOn ? hi : fb; }  // what goes on screen
int viewN() { return hiOn ? HN : N; }
// Tapered capsule: radius r0 at (x0, y0) to r1 at (x1, y1).
void cone(float x0, float y0, float r0, float x1, float y1, float r1, uint8_t c) {
  float dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy + 1e-6f, r = fmaxf(r0, r1);
  for (int y = floorf(fminf(y0, y1) - r); y <= ceilf(fmaxf(y0, y1) + r); y++)
    for (int x = floorf(fminf(x0, x1) - r); x <= ceilf(fmaxf(x0, x1) + r); x++) {
      float qx = x + 0.5f - x0, qy = y + 0.5f - y0, t = clampf((qx * dx + qy * dy) / l2, 0, 1), rr = r0 + (r1 - r0) * t;
      qx -= t * dx, qy -= t * dy;
      if (qx * qx + qy * qy <= rr * rr) px(x, y, c);
    }
}
void capsule(float x0, float y0, float x1, float y1, float r, uint8_t c) { cone(x0, y0, r, x1, y1, r, c); }
void disc(float x, float y, float r, uint8_t c) { cone(x, y, r, x, y, r, c); }
void ellipse(float cx, float cy, float rx, float ry, uint8_t c) {
  for (int y = floorf(cy - ry); y <= cy + ry; y++)
    for (int x = floorf(cx - rx); x <= cx + rx; x++) {
      float u = (x + 0.5f - cx) / rx, v = (y + 0.5f - cy) / ry;
      if (u * u + v * v < 1) px(x, y, c);
    }
}
void ring(float cx, float cy, float rx, float ry, uint8_t c, float gap = 0) {
  int n = 8 + 6 * rx;
  for (int i = 0; i < n; i++) {
    if (gap > 0 && h01(i * 7 + int(rx)) < gap) continue;
    float a = 6.2832f * i / n;
    px(lroundf(cx + rx * cosf(a)), lroundf(cy + ry * sinf(a)), c);
  }
}
const float BAYER[4] = {0.125f, 0.625f, 0.875f, 0.375f};
float bayer(int x, int y) { return BAYER[(y & 1) * 2 + (x & 1)]; }
void fillIf(int x0, int y0, int x1, int y1, uint8_t c) {
  for (int y = y0; y <= y1; y++)
    for (int x = x0; x <= x1; x++) px(x, y, c);
}

uint8_t code(char ch) {
  switch (ch) {
    case 'L': return ARMORL; case 'A': return ARMOR; case 'a': return ARMOR2; case 'd': return ARMOR3;
    case 'C': return CLOAK;  case 'c': return CLOAK2; case 'X': return LINING; case 'x': return LINING2;
    case 'O': return SLIT;   case 'o': return SLIT2;  case 'Y': return GOLD;   case 'y': return GOLD2;
    case 'V': return LAVA;   case 'v': return LAVA2;
  }
  return INK;
}
// Rows above headR move by (hdx, hdy) (a nod, a head shake; hdy >= 0, or a gap opens): drawn bottom-up so the lowered
// helm goes over the pauldrons.
void blit(const char *const *rows, int h, int ax, int x, int y0, int hdx = 0, int hdy = 0, int headR = 0) {
  for (int r = h - 1; r >= 0; r--)
    for (int c = 0; rows[r][c]; c++)
      if (rows[r][c] != '.') px(x + c - ax + (r < headR ? hdx : 0), y0 + r + (r < headR ? hdy : 0), code(rows[r][c]));
}
void art(const char *const *rows, int h, int x, int y, int flip = 1) {  // weather.h's blown leaves: here ash flakes
  int w = strlen(rows[0]);
  for (int r = 0; r < h; r++)
    for (int c = 0; c < w; c++)
      if (rows[r][flip > 0 ? c : w - 1 - c] != '.') px(x + c, y + r, rows[r][flip > 0 ? c : w - 1 - c] == 'K' ? INK : SMOKE);
}

// ---- Weather (weather.h): ash drifts right; snow falls as ash (FLAKE is grey) ----
const int WIND_DIR = 1;
bool rove = false;  // flying (the music screen, or a flight of his own)
pet::Game &game = pet::runner;  // score: mazes solved (shared with the other scenes)
float canopy(float) { return 1e9f; }
float landRow() { return rove ? 63 + 16 * rnd() : GROUND - 10 + 18 * rnd(); }
float drift() { return rove ? -50 : 0; }  // flying into the rain
bool airy(uint8_t c) { return c <= MOON; }
void thunder();
#include "weather.h"

// ---- Mordor: drawn once into bg ----
const int HORIZON = 48, EX = 13, EY = 17;  // the Eye between the horns of Barad-dur
const float DX = 70, DY = 29;              // Mount Doom's crater
uint8_t skyAt(int x, int y) {
  float v = clampf(y, 0, HORIZON) * 3.0f / HORIZON;
  int b = v;
  return b + (v - b > bayer(x, y));
}
struct Crack { int8_t x0, y0, x1, y1; } cracks[10];
float towerW(int y) { return y < 26 ? 3 : 3 + (y - 26) * 0.13f + (y > 50 ? (y - 50) * 0.5f : 0); }  // Barad-dur's half width

void buildBg() {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      uint8_t c = skyAt(x, y);
      float top = 46 - 4 * fabsf(sinf(x * 0.31f)) - 2.5f * fabsf(sinf(x * 0.73f + 1)) - fabsf(sinf(x * 1.9f));  // the Ephel Duath
      if (y >= top) c = y < top + 1 ? FAR : FAR2;
      fb[y * N + x] = c;
    }
  for (int y = DY; y < HORIZON + 6; y++) {  // Mount Doom: a broken cone, lit on its left, lava down its flank
    float half = (y - DY) * 0.85f + 2.5f;
    for (int x = floorf(DX - half); x <= DX + half; x++) {
      float u = (x + 0.5f - DX) / half;
      px(x, y, u > 0.15f + 0.2f * sinf(y * 0.4f) ? MTN2 : (x + y) % 7 == 0 ? MTN2 : MTN);
    }
  }
  for (int y = DY - 1; y < DY + 2; y++) fillIf(DX - 2, y, DX + 2, y, MTN2);  // the crater's lip
  for (int k = 0; k < 2; k++)  // two lava runs down the lit flank
    for (int y = DY + 2; y < HORIZON + 4; y++) px(DX - 2 - k * 3 - (y - DY) * (0.35f + 0.2f * k) + sinf(y * 0.5f + k), y, y % 4 ? CRUST : MTN2);
  for (int y = 6; y < GROUND; y++) {  // Barad-dur: a black tower widening to its buttresses, ribs and spikes
    float w = towerW(y);
    for (int x = lroundf(EX - w); x <= EX + w; x++)
      px(x, y, x == lroundf(EX - w) || ((x + 1) % 3 == 0 && y > 28) ? TOWER2 : TOWER);
    if (y > 28 && y % 5 == 0) px(EX - w - 1, y, TOWER2), px(EX + w + 1, y, TOWER2);
  }
  for (int k = -1; k <= 1; k += 2)  // the two horns that hold the Eye, curving up and in
    for (int y = 7; y < 27; y++) {
      float x = EX + k * (4.5f + 2.2f * sinf((27 - y) / 20.f * 3.1416f) - (y < 12 ? (12 - y) * 0.5f : 0));
      px(x, y, TOWER), px(x + k, y, y > 15 ? TOWER : TOWER2);
    }
  for (int i = 0; i < 6; i++) px(EX - 2 + 2 * h01(i * 7) * 2, 32 + i * 5, CRUST);  // a few lit windows
  for (int x = 0; x < N; x++)  // the plain of Gorgoroth
    for (int y = HORIZON + 2; y < N; y++) {
      float top = HORIZON + 2 + 1.2f * sinf(x * 0.2f);
      if (y < top || (fabsf(x - EX) <= towerW(y) && y < GROUND)) continue;
      px(x, y, y < top + 1 ? ROCKL : bayer(x, y) < 0.3f && y > 58 ? ROCK2 : y > 72 && bayer(x, y) < 0.6f ? ROCK2 : ROCK);
    }
  for (int i = 0; i < 10; i++) {  // lava cracks, from small far ones to wide near ones
    float cx = 4 + 72 * h01(i * 5), cy = HORIZON + 5 + (N - HORIZON - 7) * h01(i * 5 + 1), len = 3 + (cy - HORIZON) * 0.25f;
    if (fabsf(cx - 44) < 14 && cy < GROUND + 3) cx += cx < 44 ? -14 : 14;  // not under him
    float a = (h01(i * 5 + 2) - 0.5f) * 0.8f;
    cracks[i] = {(int8_t)cx, (int8_t)cy, (int8_t)(cx + len * cosf(a)), (int8_t)(cy + len * sinf(a) * 0.4f)};
    capsule(cracks[i].x0, cracks[i].y0, cracks[i].x1, cracks[i].y1, 0.6f, CRUST);
  }
  for (int i = 0; i < 12; i++) {  // rocks
    int rx = 2 + 76 * h01(i * 3 + 50), ry = 54 + 24 * h01(i * 3 + 51);
    if (fabsf(rx - 44) < 16 && ry < GROUND + 2) continue;
    px(rx, ry, ROCKL), px(rx + 1, ry, ROCK2), px(rx, ry + 1, ROCK2), px(rx + 1, ry + 1, SHADOW);
  }
  memcpy(bg, fb, sizeof bg);
}

// ---- Sauron: 43 wide, 52 tall, faces us. The body sprite (cloak, legs, armor, pauldrons, helm) is drawn as is; at rest
// the ARMS sprite adds both arms on the mace held across him, in any other pose arms and mace are drawn to the pose. ----
const int FIG_H = 52, AX = 21;  // sprite column over s.x
const char *const BODY[FIG_H] = {  // cloak, legs, armor, pauldrons, helm (no arms, no mace)
    ".....................K", "....................KAK", "....................KAK", ".................K..KAK..K",
    "................KaK.KAK.KAK", "................KaK.KAK.KAK", "................KaK.KAK.KAK",
    "...............KKaK.KAK.KAKK", "...............KaKK.KAK.KKAK", "................KaaaaAAAAAK",
    "................KaaaadAAAAK", "................KaOOadaOOAK", ".......K........KaoaadaaoAK........K",
    ".......aK..K.....KaaadAAAK.....K..Ka", ".......KaK.aK..K..KaadaAK..K..Ka.KaK",
    "........KaKKaK.aK..KdddK..Ka.KaKKaK", "........KAAAAAaKK.KdddddK.KKaAAAAAK",
    ".......KLAAAAAaaKaaaadAAAAKaaAAAAAAK", ".......KaAAAAaadKaaaadAAAAKdaaAAAAaK",
    "........KaaaadddKaaaadAAAAKdddaaaaK", ".........KKKKKKKddddddddddAKKKKKKK", "............KCKaaaaaadAAAAALKK",
    "............KCKaaaaaadAAAAALKK", "............KCKaddddddddddALKCK", "...........KCCKaaaaaadAAAAALKCK",
    "...........KCCCKaaaaadAAAALKCCK", "..........KCCCCKaddddddddALKCCK", "..........KCCCCcKaaaadAAALKCCCcK",
    "..........KCCCcCKaaaadAAALKCCcCK", ".........KCCCCcCKdddddddddKCCcCK", ".........KCCCCKdAAdAAdAAdAAdKcCCK",
    "........KxxXCCKdaadaadaadaadKcCCK", "........KxxXCKdaadaadaadaadaaKCCK", ".......KxxXCCKdaadaadaadaadaaKCCCK",
    ".......KxxXCKdaadaadaadaadaadaKCCK", "......KxxXCCKdaadaadaadaadaadaKCCK",
    "......KxxXXKdaadaadaadaadaadaadKcCK", ".....KxxXXCKdaadaadaadaadaadaadKcCK",
    ".....KxxXXKdaadaadaadaadaadaadaaKCK", "....KxxXXCK.K..KKKKKKKKKKKKK..K.KCCK",
    "...KxxXXCCCcCCCCKdddKcKdddKCCCCcCCCK", "...KxxXXCCCcCCCKaAaKCcCKaaAKCCCcCCCCK",
    "..KxxXXCCCCcCCCKaAaKCcCKaaAKCCCcCCCCK", "..KxxXKCCCCKCCCKaAaKCKCKaaAKCCCKCCCC.K",
    ".KxxKX.CCKc.CCKKdddKc.CKdddKCKc.CCKc.K", ".KKx.X.KC.c.KC.KaAaKc.KKaaAKC.c.KC.c..K",
    "K..x.K..C.K..CKaAaK.K..CKaaAK.K..C.K..K", "...K....K....KKaAaK....KKaaAK....K",
    "..............KaAAaK....KaAAaK", "..............KaAAaK....KaAAaK", "..............KaAAaK....KaAAaK",
    ".............KKKKKKK....KKKKKKK",
};
const char *const ARMS[FIG_H] = {  // at rest: both arms down to the haft held across him, the mace head on his right
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ".....................................K.K",
    "....................................KAKAK", "...................................KAaAaAK",
    "..................................KAKaLaKAK", ".............aAK...........KAaK....KaLAaK",
    ".............aAAK..........KAAaK.KKAKaAaKAK", ".............aAAK.........KK.KaKKAAKAaAaAK",
    ".............KaAK........KaAaK.AAAKKKAKAK", ".............KaAK........KaaaKLAKK...K.K",
    "..............KaAK.......KK.KA.K", "...............K.KK....KKAAAKK", "..............KaA.K..KK....K",
    "..............KaaYKKKAAAKK", "...............KKKK.LAKK", "...............KKAAAKK", "..............KLAAKK",
    "...........KKAAAKK", "..........KAAAKK", ".........KLA.K", "..........K", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "",
};
const int SHL = 12, SHR = 30, SHY = 21, RINGC = 17, RINGR = 29;  // shoulders, the Ring at rest (sprite coords)
const int HEAD_R = 12;  // sprite rows that nod with his head: the helm down to the visor
// A goblet of lava (Y gold, V lava), column 3 over his hand; empty, the lava is dark.
const char *const GOBLET[6] = {"KYYYYYK", "KVvVvVK", ".KyYyK.", "..KYK..", "..KyK..", ".KYYYK."};

// Pet states (SLEEP, HUNGRY, SAD: dur 0 = while the mood lasts); WAKE: shaken awake, he dozes off again.
enum { IDLE, RAISE, SLAM, RING, GAZE, SLEEP, DRINK, REFUSE, GLOAT, YAWN, HUNGRY, SAD, WAKE };
struct {
  float x = 44, t = 0, dur = 0, peek = 0;  // peek: s the Eye stays half open (a message while asleep)
  int st = IDLE;
} s;
void pose(int st, float dur) { s.st = st, s.t = 0, s.dur = dur; }
float trickT = 9, yawnT = 0, tripT = 240, sighAge = -1;  // s to the next trick / tired yawn / flight; a sigh's smoke
pet::Mood mood = pet::HAPPY;
void setMood(pet::Mood m) { mood = m; }
bool tipped() { return s.st == HUNGRY && fmodf(s.t, 3) > 1.6f && fmodf(s.t, 3) < 2.4f; }  // shaking the empty goblet

float eyeLook = 0, eyeFlare = 0, eyeLid = 0;  // pupil offset -1..1; flare 0..1 (bigger, brighter); lid 0 open .. 1 shut
float beamX = -1, beamY = 0;      // where the gaze lands, -1 = no beam
struct Bit { float x, y, vx, vy, age; uint8_t c; bool on; } bits[32];  // lava, sparks, rock chips
float waveAge = -1, waveX = 0, eruptT = -1, nazT = -1, plume = 0;

void spray(float x, float y, int n, float v, float up, uint8_t c) {
  for (int i = 0, k = 0; i < 32 && k < n; i++)
    if (!bits[i].on) {
      float a = -1.5708f + (rnd() - 0.5f) * 2.4f;
      bits[i] = {x, y, v * cosf(a) * (0.5f + rnd()), v * sinf(a) * (0.6f + 0.6f * rnd()) - up, 0, uint8_t(rnd() < 0.3f ? LAVA2 : c), true}, k++;
    }
}

void arm(float sx, float sy, float hx, float hy) {  // plated arm, the gauntlet at the hand
  cone(sx, sy, 2.6f, hx, hy, 2.2f, INK), cone(sx, sy, 1.7f, hx, hy, 1.4f, ARMOR2);
  capsule(sx + 0.6f, sy - 0.6f, hx + 0.6f, hy - 0.6f, 0.5f, ARMOR);
  disc(hx, hy, 2.3f, INK), disc(hx, hy, 1.5f, ARMOR), px(hx + 0.5f, hy - 1, ARMORL);
}
void mace(float hx, float hy, float mx, float my) {  // haft from a little behind the hand to the flanged head
  float bx = hx - (mx - hx) * 0.3f, by = hy - (my - hy) * 0.3f;
  capsule(bx, by, mx, my, 1.3f, INK), capsule(bx, by, mx, my, 0.5f, ARMOR);
  for (int i = 0; i < 8; i++) {
    float a = i * 0.785f + 0.4f;
    capsule(mx, my, mx + 4.4f * cosf(a), my + 4.4f * sinf(a), 0.8f, INK);
  }
  disc(mx, my, 3.2f, INK), disc(mx, my, 2.4f, ARMOR), disc(mx + 0.7f, my - 0.7f, 1, ARMORL);
}

void slit(float pulse, float dim) {  // the visor's glow, dim 1 = full. A palette entry: the firmware rebuilds its colors on a change
  uint8_t c[3] = {uint8_t(lroundf((200 + 55 * pulse) * dim)), uint8_t(lroundf((70 + 110 * pulse) * dim * dim)),
                  uint8_t(lroundf((30 + 60 * pulse * pulse) * dim * dim))};
  if (memcmp(PAL[SLIT], c, 3)) memcpy(PAL[SLIT], c, 3), pet::palDirty = true;
}
void zzz(float x, float y) {  // z z z rising like smoke
  static const char *const Z[4] = {"####", "..#.", ".#..", "####"};
  for (int k = 0; k < 3; k++) {
    float p = fmodf(T * 0.3f + k / 3.f, 1);
    int zx = lroundf(x + 7 * p + 1.5f * sinf(T * 2 + k)), zy = lroundf(y - 13 * p);
    if (p < 0.85f)
      for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
          if (Z[r][c] == '#') px(zx + c, zy + r, ARMORL);
  }
}
void goblet(int x, int y, bool flip, bool empty) {  // column 3 at x, top at y
  for (int r = 0; r < 6; r++)
    for (int c = 0; GOBLET[r][c]; c++) {
      char ch = GOBLET[flip ? 5 - r : r][c];
      if (ch != '.') px(x + c - 3, y + r, empty && (ch == 'V' || ch == 'v') ? INK : code(ch));
    }
}

float ringX, ringY;
void drawSauron() {
  int x0 = lroundf(s.x) - AX, y0 = GROUND - FIG_H;
  bool blaze = eyeFlare > 0.3f || s.st == SLAM || s.st == RING || s.st == WAKE || (s.st == DRINK && s.t > 2.2f);
  float pulse = blaze ? 1 : 0.55f + 0.45f * sinf(T * 2.2f);  // the visor's glow breathes
  float dim = s.st == SLEEP ? (s.peek > 0 ? 0.7f : 0.32f + 0.06f * sinf(T * 0.8f)) : s.st == SAD || s.st == HUNGRY ? 0.7f : 1;
  if (s.st == GLOAT) pulse = int(s.t / 0.16f) & 1 ? 1 : 0.4f;  // a laugh
  if (s.st == YAWN) pulse = clampf(fminf(s.t, s.dur - s.t) / 0.4f, 0.2f, 1);
  slit(pulse, dim);
  int hdx = 0, hdy = 0;  // the helm's nod / shake; the whole figure's hop
  if (s.st == SLEEP) hdy = 1 + (fmodf(T, 4) < 2);
  if (s.st == SAD || tipped()) hdy = 1;
  if (s.st == REFUSE && s.t < 0.9f) hdx = sinf(s.t * 22) > 0 ? 1 : -1;
  if (s.st == GLOAT) y0 -= int(s.t / 0.16f) & 1;
  if (s.st == WAKE && s.t < 0.3f) y0 -= 2;
  ellipse(s.x, GROUND, 16, 1.6f, SHADOW);
  blit(BODY, FIG_H, 0, x0, y0, hdx, hdy, HEAD_R);
  if (s.st == YAWN && s.t > 0.3f && s.t < s.dur - 0.3f)  // the visor gapes
    fillIf(x0 + 18 + hdx, y0 + 11 + hdy, x0 + 24 + hdx, y0 + 12 + hdy, SLIT), fillIf(x0 + 20, y0 + 12 + hdy, x0 + 22, y0 + 12 + hdy, SLIT2);
  if (sighAge >= 0) disc(x0 + 21 + 2 * sighAge, y0 + 10 - 9 * sighAge, 0.8f + 1.2f * sighAge, sighAge < 0.7f ? SMOKE : SMOKE2);
  if (s.st == SLEEP) zzz(x0 + 27, y0 + 2);
  ringX = x0 + RINGC, ringY = y0 + RINGR;
  if (s.st != RAISE && s.st != SLAM && s.st != RING && s.st != DRINK && s.st != REFUSE && s.st != HUNGRY)
    return blit(ARMS, FIG_H, 0, x0, y0);

  // the mace hand (his left, our right) and the other one, by pose
  float rx = x0 + 31, ry = y0 + 31, mx = x0 + 35, my = GROUND - 4;  // leaning on it, the head down at his side
  float lx = x0 + 11, ly = y0 + 33;                                  // the other hand at his hip
  if (s.st == RAISE) {
    float k = clampf(s.t / 0.3f, 0, 1) * clampf((s.dur - s.t) / 0.4f, 0, 1);
    rx = x0 + 31 + 2 * k, ry = y0 + 31 - 18 * k, mx = rx + 4 + 5 * k, my = ry + 12 - 23 * k;
  }
  if (s.st == SLAM) {
    float k = s.t < 0.4f ? s.t / 0.4f : 0;
    if (s.t < 0.4f) rx = x0 + 31, ry = y0 + 31 - 24 * k, mx = rx - 2 * k, my = ry - 13 * k;  // wind up: overhead
    else if (s.t < 0.5f) rx = x0 + 32, ry = y0 + 18, mx = rx + 10, my = ry;                // the swing
    else rx = x0 + 32, ry = y0 + 31, mx = x0 + 41, my = GROUND - 3;                        // struck in front
    lx = x0 + 22, ly = y0 + 30;
  }
  if (s.st == RING) {
    float k = clampf(fminf(s.t, s.dur - s.t) / 0.5f, 0, 1);
    lx = x0 + 11 + 2 * k, ly = y0 + 33 - 21 * k;
  }
  if (s.st == DRINK) {  // the goblet up to the visor, a draught, down again
    float k = clampf(fminf(s.t, s.dur - s.t - 0.2f) / 0.5f, 0, 1);
    lx = x0 + 11 + 9 * k, ly = y0 + 33 - 19 * k + (s.t > 0.5f && s.t < 2.2f ? int(s.t / 0.35f) & 1 : 0);
  }
  if (s.st == REFUSE) lx = x0 + 10 + 3 * sinf(s.t * 14), ly = y0 + 17;  // palm out: away with it
  if (s.st == HUNGRY) {
    lx = x0 + 13 + (tipped() && sinf(s.t * 30) > 0), ly = y0 + 28;
    if (!tipped() && fmodf(s.t, 0.6f) < 0.15f) my -= 2, ry -= 1;  // drumming the mace on the ground
  }
  ringX = lx, ringY = ly - 1;
  mace(rx, ry, mx, my);
  arm(x0 + SHL, y0 + SHY, lx, ly);
  arm(x0 + SHR, y0 + SHY, rx, ry);
  if (s.st == DRINK || s.st == HUNGRY) goblet(lx, ly - 6 + tipped(), tipped(), s.st == HUNGRY);
  else px(ringX, ringY, GOLD);
}

void drawEye(float EX = sauron::EX, float EY = sauron::EY) {  // the tower's top, or anywhere (the maze's middle)
  float f = eyeFlare, rx = 3 + 1.4f * f, ry = (1.8f + 0.7f * f) * (1 - 0.8f * eyeLid);
  for (int k = 0; k < 10 - 7 * eyeLid; k++) {  // licking flames around it (shut, a few embers)
    float a = k * 0.628f + T * 2.5f, r = rx + 1 + h01(k + int(T * 12)) * (1.2f + 1.8f * f);
    px(EX + r * cosf(a), EY + r * 0.6f * sinf(a), k % 3 ? EYEO : EYER);
  }
  for (int y = floorf(EY - ry - 1); y <= EY + ry + 1; y++)
    for (int x = floorf(EX - rx - 1); x <= EX + rx + 1; x++) {
      float u = (x + 0.5f - EX) / rx, v = (y + 0.5f - EY) / ry, q = u * u + v * v;
      if (q < 1) px(x, y, q > 0.65f ? EYER : q > 0.25f ? EYEO : EYEY);
    }
  int p = lroundf(EX + eyeLook * (rx - 1.5f));  // the slit pupil
  if (eyeLid < 0.6f)
    for (int y = EY - 1 - (f > 0.5f); y <= EY + 1 + (f > 0.5f); y++) px(p, y, INK);
}

void drawBeam() {  // the gaze: a cone of light from the Eye to the plain, lighting what it crosses
  if (beamX < 0) return;
  for (int y = EY + 2; y < N; y++) {
    float k = (y - EY) / (beamY - EY);
    if (k > 1.15f) break;
    float cx = EX + (beamX - EX) * k, w = 0.6f + 3.5f * k;
    for (int x = floorf(cx - w); x <= cx + w; x++) {
      if ((unsigned)x >= N) continue;
      uint8_t c = fb[y * N + x];
      if (c >= INK && c <= GOLD2) continue;  // not over him
      if (fabsf(x + 0.5f - cx) < w * 0.45f || bayer(x, y) < 0.5f) px(x, y, k > 0.95f ? EYEY : BEAM);
    }
  }
}

void drawDoom() {  // smoke from the crater, the fire inside, the cracks
  int n = 10 + 8 * plume;
  for (int k = 0; k < n; k++) {
    float a = fmodf(T * (0.18f + 0.1f * plume) + k / float(n), 1);
    float x = DX + (4 + 6 * plume) * a * sinf(k * 2.1f) - 10 * a * a + (pet::wx == pet::WIND ? 16 * a : 0), y = DY - 1 - a * 24;
    float r = 0.8f + 2.5f * a + 1.5f * plume;
    disc(x, y, r, a > 0.5f ? SMOKE2 : SMOKE);
  }
  for (int x = DX - 2; x <= DX + 2; x++) px(x, DY, fmodf(T * 4 + x, 2) < 1 ? LAVA : LAVA2);
  if (plume > 0) disc(DX, DY - 1, 1 + 2 * plume, LAVA2);
  for (int i = 0; i < 10; i++) {  // the cracks glow, a slow pulse along each
    const Crack &c = cracks[i];
    int n2 = 1 + fabsf(c.x1 - c.x0);
    for (int j = 0; j <= n2; j++) {
      float u = j / float(n2);
      int x = lroundf(c.x0 + (c.x1 - c.x0) * u), y = lroundf(c.y0 + (c.y1 - c.y0) * u);
      float glow = sinf(T * 1.3f + i * 1.7f - u * 3);
      if ((unsigned)x < N && (unsigned)y < N && fb[y * N + x] == CRUST && glow > -0.2f) px(x, y, glow > 0.7f ? LAVA2 : LAVA);
    }
  }
}

void drawNazgul() {  // a fell beast crossing the sky right to left, wings beating
  if (nazT < 0) return;
  float x = 92 - nazT * 22, y = 30 - 4 * sinf(nazT * 0.9f) + sinf(nazT * 5);
  float flap = sinf(nazT * 7);                     // -1 wings down .. 1 up
  capsule(x, y, x + 8, y + 1, 1.2f, INK);          // body and the long tail
  capsule(x + 8, y + 1, x + 12, y + 3, 0.5f, INK);
  capsule(x - 4, y - 2, x, y, 0.8f, INK);          // neck and head
  capsule(x - 6, y - 2, x - 4, y - 2, 0.6f, INK);
  for (int k = -1; k <= 1; k += 2) {               // two bat wings: shoulder -> elbow -> tip, the skin hanging under
    float ex = x + 3 + 3 * k, ey = y - 6 * flap, tx = x + 6 + 8 * k, ty = y - 9 * flap + 2;
    capsule(x + 3, y, ex, ey, 0.6f, INK), capsule(ex, ey, tx, ty, 0.5f, INK);
    for (int i = 1; i < 5; i++) capsule(ex + (tx - ex) * i / 5, ey + (ty - ey) * i / 5, x + 3 + 2 * k, y + 1, 0.5f, INK);
  }
  capsule(x + 2, y - 1, x + 2, y - 4, 0.7f, INK);  // the rider
}

void drawFx() {
  for (auto &b : bits)
    if (b.on) px(b.x, b.y, b.c);
  if (waveAge >= 0) {  // the slam: a ring of fire running out along the ground, cracks flaring
    float r = 4 + 60 * waveAge;
    ring(waveX, GROUND, r, r * 0.2f, waveAge < 0.4f ? LAVA2 : LAVA, clampf(waveAge * 1.3f, 0, 0.9f));
    for (int k = 0; k < 6; k++) {
      float a = k * 1.047f + 0.4f, l = clampf(waveAge * 3, 0, 1) * (6 + 6 * h01(k));
      if (waveAge < 0.8f) capsule(waveX, GROUND, waveX + l * cosf(a), GROUND + l * 0.3f * sinf(a) + 1, 0.4f, waveAge < 0.3f ? LAVA2 : LAVA);
    }
  }
  if (s.st == RING) {  // the Ring shines: a gold halo pulsing out from his hand
    float k = clampf((s.t - 0.4f) / 0.4f, 0, 1) * clampf((s.dur - s.t) / 0.4f, 0, 1);
    if (k > 0) {
      disc(ringX, ringY, 0.6f + k, GOLD);
      for (int j = 0; j < 2; j++) {
        float r = 2 + fmodf(s.t * 6 + j * 4, 8);
        ring(ringX, ringY, r, r, j ? GOLD2 : GOLD, 0.3f + r / 12);
      }
    }
  }
}

// ---- Flight (the music screen; now and then a flight of his own): Sauron on a fell beast over Gorgoroth, side on.
// The Ephel Duath, the ash hills and the plain's lava cracks slide by at their depth; now and then Barad-dur passes far
// off, the Eye on its top. ----
const int FH = 50;        // the flight's horizon
const float FLY_V = 60;   // px/s the plain goes by
bool tunes = false;       // music screen on (music())
float tripLeft = 0, dist = 0, hopY = 0, hopV = 0, cheerT = 0;  // s of flight left (0 = while the music plays); a climb
float vn(float u) {  // smooth value noise, 0..1
  int i = floorf(u);
  float f = u - i;
  f = f * f * (3 - 2 * f);
  return h01(i) * (1 - f) + h01(i + 1) * f;
}
float rangeTop(float x) { float u = (x + dist * 0.07f) / 7; return 33 + 11 * fabsf(2 * vn(u) - 1) + 3 * vn(u * 2.7f + 5); }
float hillTop(float x) { return 52 + 5 * vn((x + dist * 0.3f) / 11); }
void travel(float secs) { rove = true, tripLeft = secs, hopY = hopV = 0, pose(IDLE, 0); }
void endTrip() { rove = false, pose(IDLE, 0); }
void roveStep(float dt) {
  dist += FLY_V * dt;
  hopV += 120 * dt, hopY = fminf(0, hopY + hopV * dt);
  if (hopY == 0) hopV = 0;
  cheerT = fmaxf(0, cheerT - dt);
  if (!tunes && tripLeft > 0 && (tripLeft -= dt) <= 0) endTrip();
}

void tri(float x0, float y0, float x1, float y1, float x2, float y2, uint8_t c) {
  for (int y = floorf(fminf(y0, fminf(y1, y2))); y <= fmaxf(y0, fmaxf(y1, y2)); y++)
    for (int x = floorf(fminf(x0, fminf(x1, x2))); x <= fmaxf(x0, fmaxf(x1, x2)); x++) {
      float qx = x + 0.5f, qy = y + 0.5f;
      float a = (x1 - x0) * (qy - y0) - (y1 - y0) * (qx - x0), b = (x2 - x1) * (qy - y1) - (y2 - y1) * (qx - x1),
            d = (x0 - x2) * (qy - y2) - (y0 - y2) * (qx - x2);
      if ((a >= 0 && b >= 0 && d >= 0) || (a <= 0 && b <= 0 && d <= 0)) px(x, y, c);
    }
}
void outlined(float x0, float y0, float r0, float x1, float y1, float r1, uint8_t c) {
  cone(x0, y0, r0 + 0.8f, x1, y1, r1 + 0.8f, INK), cone(x0, y0, r0, x1, y1, r1, c);
}
// A bat wing from the shoulder (jx, jy); f: -1 down .. 1 up. Three finger bones from the wrist, the skin between them,
// its trailing edge sagging back toward the body.
void wing(float jx, float jy, float f, uint8_t skin, uint8_t skin2) {
  float ex = jx - 9, ey = jy - 16 * f - 2;
  float t[3][2] = {{jx - 36, jy - 30 * f + 2}, {jx - 37, jy - 12 * f + 9}, {jx - 19, jy + 5}};
  tri(jx, jy, ex, ey, t[0][0], t[0][1], skin);
  for (int i = 0; i < 2; i++) {
    float mx = (t[i][0] + t[i + 1][0]) / 2 + 3, my = (t[i][1] + t[i + 1][1]) / 2 + (f > 0 ? 3 : -3);  // the sag
    tri(ex, ey, t[i][0], t[i][1], mx, my, i ? skin2 : skin), tri(ex, ey, mx, my, t[i + 1][0], t[i + 1][1], skin2);
    tri(jx, jy, ex, ey, t[i + 1][0], t[i + 1][1], skin2);
  }
  capsule(jx, jy, ex, ey, 1.1f, INK);
  for (auto &p : t) capsule(ex, ey, p[0], p[1], 0.55f, INK), px(p[0] - 1, p[1], INK);
}
void drawBeast(float bx, float by, float f) {
  wing(bx + 9, by - 6, f * 0.85f, CLOAK2, INK);  // the far wing, a little ahead
  for (int i = 0; i < 6; i++) {                   // the long tail, a barb at its end
    float x0 = bx - 14 - 3.5f * i, y0 = by + 0.6f * i + sinf(T * 4 + i * 0.8f) * 0.5f * i;
    outlined(x0, y0, 2.2f - 0.3f * i, x0 - 3.5f, y0 + 0.6f + sinf(T * 4 + i * 0.8f + 0.8f) * 0.5f, 1.9f - 0.3f * i, CLOAK);
  }
  float tx = bx - 35, ty = by + 4 + sinf(T * 4 + 4.8f) * 3;
  tri(tx - 3, ty, tx + 1, ty - 2, tx + 1, ty + 2, INK);
  ellipse(bx, by, 16, 7.3f, INK);
  for (int y = floorf(by - 6.5f); y <= by + 6.5f; y++)
    for (int x = floorf(bx - 15); x <= bx + 15; x++) {
      float u = (x + 0.5f - bx) / 15, v = (y + 0.5f - by) / 6.5f;
      if (u * u + v * v < 1) px(x, y, v > 0.5f ? LINING2 : v > 0.2f ? CLOAK2 : v < -0.65f ? ARMOR2 : CLOAK);  // lava-lit belly
    }
  outlined(bx - 6, by + 5, 2, bx - 10, by + 11, 1.3f, CLOAK);  // legs tucked, talons
  outlined(bx + 8, by + 5, 1.7f, bx + 11, by + 10, 1.1f, CLOAK);
  px(bx - 12, by + 12, INK), px(bx - 9, by + 12, INK), px(bx + 12, by + 11, INK), px(bx + 10, by + 11, INK);
  outlined(bx + 12, by - 2, 3.6f, bx + 19, by - 9, 2.6f, CLOAK);  // the snake neck, the head with its jaw ajar
  outlined(bx + 19, by - 9, 2.6f, bx + 25, by - 12, 2.1f, CLOAK);
  bool roar = cheerT > 0;
  outlined(bx + 25, by - 12, 2.3f, bx + 33, by - 10 + roar, 1.2f, CLOAK);
  capsule(bx + 26, by - 9, bx + 32, by - 8 + 2 * roar, 0.6f, roar ? LINING : CLOAK2);  // the lower jaw
  px(bx + 28, by - 13, EYEO), px(bx + 27, by - 13, EYER);  // eye
  capsule(bx + 24, by - 14, bx + 21, by - 17, 0.6f, INK), capsule(bx + 26, by - 14, bx + 24, by - 18, 0.5f, INK);  // horns
  for (int k = 0; k < 4; k++) px(bx + 13 + 3.5f * k, by - 6.5f - 1.6f * k, ARMOR2);  // a ridge along the neck
  wing(bx + 3, by - 5, f, ARMOR2, ARMOR3);  // the near wing
}
void notes(float x, float y) {  // music notes rising, blown back
  static const char *const NOTE[2][5] = {{"..##", "..#.", "..#.", "###.", "##.."}, {"####", "#..#", "#..#", "#.##", "#.##"}};
  for (int k = 0; k < 3; k++) {
    float p = fmodf(T * 0.35f + k / 3.f, 1);
    int nx = lroundf(x - 24 * p + 3 * sinf(T * 2 + k * 2)), ny = lroundf(y - 12 * p);
    if (p < 0.85f)
      for (int r = 0; r < 5; r++)
        for (int c = 0; c < 4; c++)
          if (NOTE[k % 2][r][c] == '#') px(nx + c, ny + r, ARMORL);
  }
}
void drawFlight() {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float v = clampf(y, 0, FH) * 3.0f / FH;
      int b = v;
      fb[y * N + x] = b + (v - b > bayer(x, y));
    }
  for (int k = 0; k < 7; k++) {  // ash clouds racing back
    float cx = fmodf(k * 23 - dist * (0.5f + 0.1f * k) + 4000, 120) - 20, cy = 8 + 5 * k % 26;
    for (int j = 0; j < 4; j++) disc(cx + j * 3.5f, cy + (j & 1), 2.6f - 0.3f * j, j & 1 ? CLOUD2 : CLOUD);
  }
  float slot = floorf((dist * 0.07f + 200) / 260);  // Barad-dur far off, every so often
  float bt = slot * 260 - 200 - dist * 0.07f + 20;
  if (h01(int(slot)) > 0.3f && bt > -10 && bt < N + 10) {
    for (int y = 14; y < FH + 4; y++) {
      float w = 1 + (y - 14) * 0.09f + (y > 40 ? (y - 40) * 0.3f : 0);
      fillIf(lroundf(bt - w), y, lroundf(bt + w), y, TOWER);
    }
    px(bt - 2, 12, TOWER), px(bt + 2, 12, TOWER), px(bt - 2, 13, TOWER), px(bt + 2, 13, TOWER);
    px(bt, 12, EYEO), px(bt - 1, 12, EYER), px(bt + 1, 12, EYER);
  }
  for (int x = 0; x < N; x++) {
    float rt = rangeTop(x), ht = hillTop(x);
    for (int y = floorf(rt); y < ht; y++) px(x, y, y < rt + 1 ? FAR : FAR2);
    for (int y = floorf(ht); y < 63; y++) px(x, y, y < ht + 1 ? ROCKL : bayer(x, y) < 0.3f ? MTN2 : MTN);
  }
  for (int k = 0; k < 3; k++) {  // a smoking cone among the hills now and then
    float sl = floorf(dist * 0.3f / 70) + k, cx = sl * 70 + 30 - dist * 0.3f;
    if (h01(int(sl) * 3 + 1) < 0.5f) continue;
    for (int y = 47; y < 63; y++) fillIf(lroundf(cx - (y - 47) * 0.9f - 1), y, lroundf(cx + (y - 47) * 0.9f + 1), y, y < 48 ? MTN2 : MTN);
    px(cx, 46, fmodf(T * 3, 1) < 0.5f ? LAVA : LAVA2), px(cx - 1, 47, LAVA);
    for (int j = 0; j < 4; j++) disc(cx - j * 3, 43 - j * 3, 1 + 0.5f * j, SMOKE);
  }
  for (int y = 63; y < N; y++) {  // the plain: its cracks of lava come faster the nearer they are
    float sp = 0.5f + 0.9f * (y - 63) / 17.f;
    for (int x = 0; x < N; x++) {
      int u = int(x + dist * sp), cell = u / (3 + (y - 63) / 4);
      uint8_t c = y == 63 ? ROCKL : h01(u * 31 + y * 977) < 0.15f ? ROCK2 : ROCK;
      if (h01(cell * 13 + y * 101) < 0.035f) c = h01(cell + y) < 0.5f ? LAVA : CRUST;
      px(x, y, c);
    }
  }
  drawFlash();
  drawDrops();

  float f = sinf(T * 4.5f), bx = s.x - 10, by = 56 + 2 * sinf(T * 1.3f) - 1.2f * f + hopY;
  int x0 = lroundf(bx) - 5 - AX, y0 = lroundf(by) - 35;
  ellipse(bx + 2, 76, 15 + hopY * 0.2f, 1.4f, SHADOW);  // his shadow far below on the plain
  drawBeast(bx, by, f);
  blit(BODY, 30, 0, x0, y0);
  float lx = x0 + 9, ly = y0 + 29, rx = x0 + 34, ry = y0 + 27 - 3 * (cheerT > 0);
  capsule(lx, ly, bx + 16, by - 6, 0.3f, INK);  // the reins
  mace(rx, ry, rx + 4, ry - 14 - 2 * (cheerT > 0));
  arm(x0 + SHL, y0 + SHY, lx, ly), arm(x0 + SHR, y0 + SHY, rx, ry);
  slit(cheerT > 0 ? 1 : 0.55f + 0.45f * sinf(T * 2.2f), 1);
  if (tunes) notes(x0 + 6, y0 + 4);
  drawFx();
  drawLeaves();
}

// ---- Logic ----
float lookT = 0;
// Device shaken: the mace slams, a ring of fire. Asleep: he starts, the Eye flares, then shuts again. Flying: a climb.
void shake() {
  if (game.on) return;
  if (rove) return void(hopV = -40);
  if (s.st == SLEEP) return pose(WAKE, 1.2f);
  pose(SLAM, 1.6f);
}
// Message/button: mace up, the Eye flares. Asleep: the Eye half opens for 2 s. Flying: the beast shrieks, mace up.
void cheer() {
  if (game.on) return;
  if (s.st == SLEEP) return void(s.peek = 2);
  if (rove) return void(cheerT = 1.2f);
  if (s.st == IDLE || s.st == SAD || s.st == HUNGRY) pose(RAISE, 1.8f);
}
void ringUp() { pose(RING, 3.6f); }
void gaze() { pose(GAZE, 5); }
void erupt() { eruptT = 0; }
void nazgul() { nazT = 0; }
float startle = 0;
void thunder() { startle = 0.5f; }
// Pet events (nothing during the game; a flight ends): fed (a goblet of lava), fed while full, petted, too tired to play.
void petAct(int st, float dur) {
  if (game.on) return;
  rove = false;
  pose(st, dur);
}
void eat() { petAct(DRINK, 3); }
void refuse() { petAct(REFUSE, 1.2f); }
void purr() { petAct(GLOAT, 2); }
void yawn() { petAct(YAWN, 1.6f); }
// Music screen on / off (the firmware calls this every frame): he flies; asleep, he sleeps on.
void music(bool on) {
  if (game.on) on = false;
  if (on == tunes) return;
  tunes = on;
  if (on && s.st != SLEEP && mood != pet::SLEEP) travel(0);
  else if (!on && rove && tripLeft <= 0) endTrip();
}

void update(float dt) {
  T += dt, s.t += dt;
  startle = fmaxf(0, startle - dt), s.peek = fmaxf(0, s.peek - dt), yawnT -= dt;
  if (sighAge >= 0 && (sighAge += dt) > 1.5f) sighAge = -1;
  float wantLid = s.st == SLEEP ? (s.peek > 0 || startle > 0 ? 0.5f : 1) : s.st == SAD ? 0.45f : s.st == YAWN ? 0.6f : 0;
  eyeLid += (wantLid - eyeLid) * fminf(1, dt * 3);
  float wantFlare = s.st == RAISE || s.st == WAKE ? 1 : s.st == RING ? 0.8f : s.st == SLAM ? 0.7f : s.st == GLOAT ? 0.6f
                  : s.st == DRINK && s.t > 2.2f ? 0.8f : startle > 0 && s.st != SLEEP ? 1 : 0;
  eyeFlare += (wantFlare - eyeFlare) * fminf(1, dt * 5);
  if ((lookT -= dt) < 0) lookT = 1.5f + 2 * rnd(), eyeLook = rnd() * 2 - 1;  // the Eye roams
  if (s.st == SAD) eyeLook *= 0.5f;
  beamX = -1;
  if (rove) roveStep(dt);
  if (s.st == GAZE) {  // sweeps the plain left to right and back
    float u = s.t / s.dur;
    beamX = 6 + 68 * (0.5f - 0.5f * cosf(u * 6.2832f * 1.5f)), beamY = 62 + 8 * sinf(u * 9);
    eyeLook = clampf((beamX - EX) / 30, -1, 1);
  }
  if (s.st == IDLE && !rove) {
    if (mood == pet::SLEEP) pose(SLEEP, 0);
    else if (mood == pet::HUNGRY) pose(HUNGRY, 0);
    else if (mood == pet::SAD) pose(SAD, 0);
    else if (mood == pet::TIRED && yawnT <= 0) yawnT = 30, pose(YAWN, 1.6f);
    else if (mood == pet::HAPPY && (tripT -= dt) < 0) tripT = 200 + 100 * rnd(), travel(12 + 8 * rnd());
    else if ((trickT -= dt * (mood == pet::TIRED ? 0.5f : 1)) < 0) {  // tired: tricks half as often
      trickT = 10 + 10 * rnd();
      float r = rnd();
      if (r < 0.35f) gaze();
      else if (r < 0.6f) ringUp();
      else if (r < 0.8f) erupt();
      else nazgul();
    }
  }
  if (s.st == SLEEP && mood != pet::SLEEP) pose(YAWN, 1.6f);
  if ((s.st == HUNGRY && mood != pet::HUNGRY) || (s.st == SAD && mood != pet::SAD)) pose(IDLE, 0);
  if (s.st == HUNGRY && fmodf(s.t, 3) > 2 && fmodf(s.t - dt, 3) <= 2)  // the last drop out of the goblet
    for (auto &b : bits)
      if (!b.on) { b = {lroundf(s.x) - AX + 13.f, GROUND - FIG_H + 28.f, 0, 4, 0, LAVA2, true}; break; }
  if (s.st == SAD && fmodf(s.t, 6) > 4 && fmodf(s.t - dt, 6) <= 4) sighAge = 0;
  if (s.st == SLAM && s.t > 0.5f && s.t - dt <= 0.5f) {
    waveAge = 0, waveX = lroundf(s.x) - AX + 41;
    spray(waveX, GROUND - 1, 14, 30, 10, LAVA);
  }
  if (s.st != IDLE && s.dur > 0 && s.t >= s.dur) pose(IDLE, 0);
  if (waveAge >= 0 && (waveAge += dt) > 1.1f) waveAge = -1;
  plume = 0;
  if (eruptT >= 0) {
    eruptT += dt;
    plume = clampf(fminf(eruptT / 0.5f, (5 - eruptT) / 1.5f), 0, 1);
    if (eruptT < 3.5f && fmodf(eruptT, 0.25f) < dt) spray(DX, DY, 3, 26, 18, LAVA);
    if (eruptT > 5) eruptT = -1;
  }
  if (nazT >= 0 && (nazT += dt) > 5.5f) nazT = -1;
  for (auto &b : bits)
    if (b.on) {
      b.age += dt, b.vy += G * dt, b.x += b.vx * dt, b.y += b.vy * dt;
      if (b.y > GROUND + 6 || b.age > 2.5f) b.on = false;
    }
}

void init() {
  s = {}, trickT = 9, yawnT = 0, waveAge = -1, eruptT = -1, nazT = -1, sighAge = -1, eyeLid = 0;
  rove = false, hopY = hopV = cheerT = 0;
  for (auto &b : bits) b.on = false;
  buildBg();
  tint(clockMin);
}

// ---- Game: the Ring's road to the Eye. A round maze seen from above, the Eye in its middle; tilt rolls the Ring
// through it. Reach the Eye and the next maze is a ring wider (it grows past the screen: the view follows the Ring and
// an arrow on the rim points to the Eye). From level 2 the Fellowship walks it at random, one more each level
// (Gandalf, Frodo, Aragorn, Gollum, again): touch one and it's lost. Gollum is quicker and drawn to the Ring. ----
const int MAXR = 11, MAXC = 512, W = 8, C0 = 10;    // rings at most; cells at most; ring width, the Eye's room radius
const int RMAX = C0 + MAXR * W, WN = 2 * RMAX + 4;  // the walls' bitmap: WN x WN world px, the middle at WN / 2
const float SPEED_G = 320, RESP = 10, VMAX = 55, DEAD_G = 0.015f;  // px/s per g of tilt; 1/s to reach it; px/s; dead zone
const float RING_R = 1.5f, HIT_R = 3.5f, INTRO_S = 1.2f, WIN_S = 1.6f;  // Ring and touch radius px; level start, won
const int MAXW = 8;

uint8_t ringN[MAXR + 1];
uint16_t ringOff[MAXR + 2];
uint8_t link_[MAXC];  // bit 0: open to the next cell clockwise, bit 1: open inward; bit 7 visited while building
uint8_t wallBits[(WN * WN + 7) / 8];
struct {
  int level, rings;
  float t, wonT;                  // since the level started; since the Eye was reached, -1 = not yet
  float x, y, vx, vy, camX, camY;  // the Ring, world px from the middle (y down); the view's middle
  float gx, gy, zx, zy;           // tilt in g (from the zero), the zero (the hold at the start)
  bool zeroed;
  int killer;
} mz;
enum { GANDALF_W, FRODO, ARAGORN, GOLLUM };
struct Walker {
  float r, a, tr, ta, speed, phase;  // polar position, target (arc first, then radial)
  int cell, prev;
  uint8_t kind;
  bool on;
} walkers[MAXW];

const float TAU = 6.2832f;
float wrapPi(float a) { return a - TAU * floorf((a + 3.1416f) / TAU); }
float midR(int i) { return i ? C0 + (i - 0.5f) * W : 0; }
int ringOf(int c) {
  int i = 0;
  while (c >= ringOff[i + 1]) i++;
  return i;
}
float cellA(int c) { int i = ringOf(c); return (c - ringOff[i] + 0.5f) * TAU / ringN[i]; }
bool doubled(int i) { return i > 1 && ringN[i] == 2 * ringN[i - 1]; }
int cw(int c) { int i = ringOf(c); return ringOff[i] + (c - ringOff[i] + 1) % ringN[i]; }
int ccw(int c) { int i = ringOf(c); return ringOff[i] + (c - ringOff[i] + ringN[i] - 1) % ringN[i]; }
int parent(int c) { int i = ringOf(c); return i <= 1 ? 0 : ringOff[i - 1] + (c - ringOff[i]) / (doubled(i) ? 2 : 1); }
int adj(int c, int *out) {  // the cells next to c (not the middle), walls or not
  int i = ringOf(c), n = 0;
  if (!i) return 0;
  out[n++] = cw(c), out[n++] = ccw(c);
  if (i > 1) out[n++] = parent(c);
  if (i < mz.rings) {
    int k = c - ringOff[i], d = doubled(i + 1);
    for (int j = 0; j <= d; j++) out[n++] = ringOff[i + 1] + (d ? 2 * k + j : k);
  }
  return n;
}
bool linked(int a, int b) {
  if (b == cw(a) && ringOf(a) == ringOf(b) && (link_[a] & 1)) return true;
  if (a == cw(b) && ringOf(a) == ringOf(b) && (link_[b] & 1)) return true;
  if (ringOf(a) > ringOf(b)) return parent(a) == b && (link_[a] & 2);
  if (ringOf(b) > ringOf(a)) return parent(b) == a && (link_[b] & 2);
  return false;
}
int ways(int c, int *out, bool toEye) {  // open neighbors; the middle only with toEye
  int all[5], n = adj(c, all), m = 0;
  for (int k = 0; k < n; k++)
    if (linked(c, all[k])) out[m++] = all[k];
  if (toEye && ringOf(c) == 1 && (link_[c] & 2)) out[m++] = 0;
  return m;
}
int cellAt(float x, float y) {  // -1 = outside the maze
  float r = hypotf(x, y);
  if (r < C0) return 0;
  int i = 1 + int((r - C0) / W);
  if (i > mz.rings) return -1;
  float a = atan2f(y, x);
  return ringOff[i] + int((a < 0 ? a + TAU : a) / TAU * ringN[i]) % ringN[i];
}
inline bool solid(float x, float y) {
  int u = floorf(x + WN / 2), v = floorf(y + WN / 2);
  return (unsigned)u >= WN || (unsigned)v >= WN || (wallBits[(v * WN + u) >> 3] >> ((v * WN + u) & 7) & 1);
}
float outerR() { return C0 + mz.rings * W; }

void buildMaze(int rings) {  // ring 1 has 8 cells; a ring doubles them when its cells would be 2 wide
  mz.rings = rings, ringN[0] = 1, ringOff[0] = 0, ringOff[1] = 1;
  for (int i = 1; i <= rings; i++) {
    int n = i == 1 ? 8 : ringN[i - 1];
    if (i > 1 && TAU * midR(i) / n >= 2 * W) n *= 2;
    ringN[i] = n, ringOff[i + 1] = ringOff[i] + n;
  }
  int cells = ringOff[rings + 1];
  memset(link_, 0, sizeof link_);
  static uint16_t stack[MAXC];  // depth-first, from a random outer cell: one way between any two cells
  int sp = 0, s0 = ringOff[rings] + int(rnd() * ringN[rings]);
  link_[s0] = 0x80, stack[sp++] = s0;
  while (sp) {
    int c = stack[sp - 1], all[5], free_[5], n = adj(c, all), m = 0;
    for (int k = 0; k < n; k++)
      if (!(link_[all[k]] & 0x80)) free_[m++] = all[k];
    if (!m) { sp--; continue; }
    int b = free_[int(rnd() * m)];
    if (ringOf(b) == ringOf(c)) link_[b == cw(c) ? c : b] |= 1;
    else link_[ringOf(b) > ringOf(c) ? b : c] |= 2;
    link_[b] |= 0x80, stack[sp++] = b;
  }
  link_[ringOff[1] + int(rnd() * ringN[1])] |= 2;  // the one door into the Eye's room
  for (int c = 0; c < cells; c++) link_[c] &= 3;
  memset(wallBits, 0, sizeof wallBits);  // walls ~1.2 px thick, everything outside the maze solid
  float out = outerR(), TH = 0.6f;
  for (int v = 0; v < WN; v++)
    for (int u = 0; u < WN; u++) {
      float x = u + 0.5f - WN / 2, y = v + 0.5f - WN / 2, r = hypotf(x, y);
      bool wall = r > out + TH;
      if (!wall && r > C0 - TH) {
        float a = atan2f(y, x);
        if (a < 0) a += TAU;
        int j = lroundf((r - C0) / W);  // the nearest circle: between ring j and j + 1
        if (fabsf(r - C0 - j * W) < TH) wall = j >= rings || !(link_[ringOff[j + 1] + int(a / TAU * ringN[j + 1]) % ringN[j + 1]] & 2);
        int i = 1 + int((r - C0) / W);
        if (!wall && r >= C0 && i <= rings) {  // the spokes between cells
          float q = a / TAU * ringN[i];
          int kb = lroundf(q);
          if (fabsf(q - kb) * TAU * r / ringN[i] < TH) wall = !(link_[ringOff[i] + (kb + ringN[i] - 1) % ringN[i]] & 1);
        }
      }
      if (wall) wallBits[(v * WN + u) >> 3] |= 1 << ((v * WN + u) & 7);
    }
}

void camFollow() {  // the view: still when it all fits, else on the Ring (no lag: it'd jitter), not past the maze's edge
  bool fits = outerR() < 39;
  float tx = fits ? 0 : mz.x, ty = fits ? 0 : mz.y, lim = fmaxf(0, outerR() - 34), d = hypotf(tx, ty);
  if (d > lim) tx *= lim / d, ty *= lim / d;
  mz.camX = tx, mz.camY = ty;
}
void place(Walker &w, int c) { w.cell = w.prev = c, w.r = w.tr = midR(ringOf(c)), w.a = w.ta = cellA(c), w.phase = rnd() * 6; }
void startLevel() {
  mz.level++, mz.t = 0, mz.wonT = -1;
  buildMaze(2 + mz.level < MAXR ? 2 + mz.level : MAXR);  // 3 rings first: all on screen
  int c0 = ringOff[mz.rings] + int(rnd() * ringN[mz.rings]);  // the Ring starts in the outer ring
  mz.x = midR(mz.rings) * cosf(cellA(c0)), mz.y = midR(mz.rings) * sinf(cellA(c0)), mz.vx = mz.vy = 0;
  int n = mz.level - 1 < MAXW ? mz.level - 1 : MAXW, cells = ringOff[mz.rings + 1];
  for (int k = 0; k < MAXW; k++) {
    Walker &w = walkers[k];
    w.on = k < n;
    if (!w.on) continue;
    w.kind = k % 4, w.speed = fminf(8 + 1.5f * mz.level, 22) * (w.kind == GOLLUM ? 1.35f : 1);
    int c = 1;
    for (int tries = 0; tries < 40; tries++) {  // far from the Ring
      c = 1 + int(rnd() * (cells - 1));
      float r = midR(ringOf(c));
      if (hypotf(r * cosf(cellA(c)) - mz.x, r * sinf(cellA(c)) - mz.y) > 30) break;
    }
    place(w, c);
  }
  camFollow();
}

void walk(Walker &w, float dt) {  // along the corridors: arc to the target's angle, then in or out to its ring
  w.phase += dt * w.speed * 0.6f;
  float step = w.speed * dt, da = wrapPi(w.ta - w.a);
  if (fabsf(da) * w.r > 0.01f) {
    float s = fminf(step / w.r, fabsf(da));
    w.a = wrapPi(w.a + (da > 0 ? s : -s));
    return;
  }
  w.a = w.ta;
  if (fabsf(w.tr - w.r) > 0.01f) {
    w.r += clampf(w.tr - w.r, -step, step);
    return;
  }
  int nb[5], m = ways(w.cell, nb, false), pick = -1;  // there: the next cell, not back unless it's a dead end
  if (w.kind == GOLLUM && rnd() < 0.5f) {  // he smells the Ring: the way that gets closest
    float best = 1e9f;
    for (int k = 0; k < m; k++) {
      float r = midR(ringOf(nb[k])), d = hypotf(r * cosf(cellA(nb[k])) - mz.x, r * sinf(cellA(nb[k])) - mz.y);
      if (d < best) best = d, pick = nb[k];
    }
  } else {
    int fw[5], f = 0;
    for (int k = 0; k < m; k++)
      if (nb[k] != w.prev) fw[f++] = nb[k];
    pick = f ? fw[int(rnd() * f)] : m ? nb[0] : w.cell;
  }
  w.prev = w.cell, w.cell = pick, w.tr = midR(ringOf(pick));
  w.ta = ringOf(pick) < ringOf(w.prev) ? w.a : cellA(pick);  // inward: straight in (its angle is in the parent's span)
}

bool ringHits(float x, float y) {
  if (solid(x, y)) return true;
  for (int k = 0; k < 8; k++)
    if (solid(x + RING_R * cosf(k * 0.785f), y + RING_R * sinf(k * 0.785f))) return true;
  return false;
}

bool unstick(float &x, float &y, bool alongX) {  // free here, or a nudge across the move away; false = blocked
  static const float D[] = {0, 0.35f, -0.35f, 0.7f, -0.7f, 1.05f, -1.05f};
  for (float d : D)
    if (!ringHits(x + alongX * d, y + !alongX * d)) return x += alongX * d, y += !alongX * d, true;
  return false;
}

void finish() {
  game.newBest = pet::gameOver(game.score);
  game.ended = true;
}

void gameStart() {
  game = pet::Game();
  game.on = true, game.labelY = 11, hiOn = true, rove = false, eyeLid = 0;
  mz = {}, mz.wonT = -1;
  startLevel();
}

void gameStop() {  // left with a long hold, or by find; back to Mordor
  if (game.on && !game.dead && game.score > 0) finish();
  game.on = hiOn = false;
  init();
}

void gameInput(bool held) { game.held = held; }  // the button only restarts after a game over (the firmware)
void gameTilt(float) {}
void gameTilt2(float right, float down) {  // gravity along the screen in g; the hold at the start is level
  if (!mz.zeroed) mz.zx = right, mz.zy = down, mz.zeroed = true;
  float x = right - mz.zx, y = down - mz.zy;
  mz.gx = fabsf(x) < DEAD_G ? 0 : x - copysignf(DEAD_G, x), mz.gy = fabsf(y) < DEAD_G ? 0 : y - copysignf(DEAD_G, y);
}

void gameStep(float dt) {
  T += dt, game.t += dt, mz.t += dt;
  eyeFlare += ((mz.wonT >= 0 ? 1 : game.dead ? 0 : 0.2f) - eyeFlare) * fminf(1, dt * 5);
  eyeLook = clampf(mz.x / 12, -1, 1);
  if (game.dead) {
    if (game.t > 10) gameStop();
    return;
  }
  if (mz.wonT >= 0) {  // the Eye takes the Ring
    mz.wonT += dt, mz.x *= expf(-6 * dt), mz.y *= expf(-6 * dt);
    if (mz.wonT > WIN_S) startLevel();
    return;
  }
  float wx = SPEED_G * mz.gx, wy = SPEED_G * mz.gy, w = hypotf(wx, wy), k = fminf(1, RESP * dt);  // tilt = speed
  if (w > VMAX) wx *= VMAX / w, wy *= VMAX / w;
  mz.vx += (wx - mz.vx) * k, mz.vy += (wy - mz.vy) * k;
  int n = 1 + int(hypotf(mz.vx, mz.vy) * dt / 0.4f);
  for (int i = 0; i < n; i++) {  // a step at a time, x then y: it slides along a wall, up to 1 px sideways round a curve
    mz.x += mz.vx * dt / n;
    if (!unstick(mz.x, mz.y, false)) mz.x -= mz.vx * dt / n, mz.vx *= -0.2f;
    mz.y += mz.vy * dt / n;
    if (!unstick(mz.x, mz.y, true)) mz.y -= mz.vy * dt / n, mz.vy *= -0.2f;
  }
  if (hypotf(mz.x, mz.y) < C0 - 4) {
    mz.wonT = 0, game.score++, game.buzz = 1;
    return;
  }
  if (mz.t > INTRO_S)
    for (int j = 0; j < MAXW; j++) {
      Walker &w = walkers[j];
      if (!w.on) continue;
      walk(w, dt);
      if (hypotf(w.r * cosf(w.a) - mz.x, w.r * sinf(w.a) - mz.y) < HIT_R) {
        game.dead = true, game.t = 0, mz.killer = j, game.buzz = 2;
        finish();
        return;
      }
    }
  camFollow();
}

// The Fellowship, 5 x 7 from above-front, and the Ring.
const char *const FOLK[4][7] = {
    {"..R..", ".RRR.", "RRRRR", ".SBS.", ".BBB.", "RBBBR", ".R.R."},   // Gandalf: hat, beard, grey robe
    {".HHH.", "HSSSH", ".SSS.", "GGGGG", "GGGGG", ".GGG.", ".S.S."},   // Frodo: curls, the green cloak, bare feet
    {"HHHHH", "HSSSH", "HSHSH", "LGGGL", "LLLLL", ".LLL.", ".H.H."},   // Aragorn: dark hair, stubble, leather
    {".....", ".PPP.", "PEPEP", ".PPP.", "P.P.P", ".QPQ.", "P...P"}};  // Gollum: big eyes, thin limbs
uint8_t folkC(char ch) {
  switch (ch) {
    case 'R': return ROBE; case 'B': return BEARD; case 'S': return SKIN; case 'G': return CLOAKG;
    case 'H': return HAIR; case 'L': return LEATHER; case 'P': return PALE; case 'Q': return LEATHER; case 'E': return PALE2;
  }
  return INK;
}
int cam2x, cam2y;  // the view's middle in half px
void anchor(float wx, float wy, int &x, int &y) {  // world point -> px() coordinates and the half px shift (hx, hy)
  int vx = lroundf(2 * wx) - cam2x + N, vy = lroundf(2 * wy) - cam2y + N;
  x = vx >> 1, y = vy >> 1, hx = vx & 1, hy = vy & 1;  // >> floors negatives too
}
void drawFolk(const Walker &w) {
  int x, y;
  anchor(w.r * cosf(w.a), w.r * sinf(w.a) - (sinf(w.phase * 3) > 0) * 0.5f, x, y);  // a half px bob as they walk
  for (int r = 0; r < 7; r++)
    for (int c = 0; c < 5; c++)
      if (FOLK[w.kind][r][c] != '.') px(x - 2 + c, y - 4 + r, folkC(FOLK[w.kind][r][c]));
}

void gameDraw() {
  cam2x = lroundf(2 * mz.camX), cam2y = lroundf(2 * mz.camY);
  int u0 = (cam2x - N) >> 1, v0 = (cam2y - N) >> 1, bx, by;  // the world px at the view's top left
  anchor(u0, v0, bx, by);
  float out = outerR();
  for (int j = 0; j <= N; j++)
    for (int i = 0; i <= N; i++) {
      int u = u0 + i, v = v0 + j;
      uint8_t c;
      if (u * u + v * v > (out + 1) * (out + 1)) {  // the lava sea around the maze
        float g = sinf(u * 0.35f + T * 1.1f) + sinf(v * 0.3f - T * 0.8f) + sinf((u + v) * 0.2f + T * 0.5f);
        c = g > 1.4f ? LAVA2 : g > -0.4f + 0.5f * bayer(u, v) ? LAVA : CRUST;
      } else if (solid(u, v)) c = solid(u, v + 1) ? WALL : WALLD;
      else c = h01(u * 131 + v * 7919) < 0.25f ? FLOOR2 : FLOOR;
      px(bx + i, by + j, c);
    }
  int ex, ey;
  anchor(0, 0, ex, ey);
  if (hypotf(mz.camX, mz.camY) < 50) drawEye(ex, ey);
  for (int j = 0; j < MAXW; j++)
    if (walkers[j].on && !(game.dead && j == mz.killer && fmodf(game.t, 0.4f) < 0.2f)) drawFolk(walkers[j]);
  if (!(game.dead && fmodf(game.t, 0.3f) < 0.15f) && !(mz.wonT > 0.6f)) {  // the Ring: gold, its hole, a glint
    int rx, ry;
    anchor(mz.x, mz.y, rx, ry);
    const char *R[4] = {".YY.", "Y..y", "Y..y", ".yy."};
    for (int r = 0; r < 4; r++)
      for (int c = 0; c < 4; c++)
        if (R[r][c] != '.') px(rx - 2 + c, ry - 2 + r, R[r][c] == 'Y' ? GOLD : GOLD2);
    if (fmodf(T, 1.5f) < 0.15f) px(rx - 1, ry - 2, CREAM);
  }
  hx = hy = 0;  // the arrow, and the firmware's bubble after this, on the plain grid
  float d = hypotf(mz.camX, mz.camY);  // the Eye off screen: an arrow on the rim
  if (d > 31) {
    float ux = -mz.camX / d, uy = -mz.camY / d, tx = N / 2 + ux * 36, ty = N / 2 + uy * 36;
    uint8_t c = fmodf(T, 0.8f) < 0.4f ? EYEY : EYEO;
    capsule(tx, ty, tx - 4 * ux + 3 * uy, ty - 4 * uy - 3 * ux, 0.7f, c);
    capsule(tx, ty, tx - 4 * ux - 3 * uy, ty - 4 * uy + 3 * ux, 0.7f, c);
  }
}

// Bubble text: the level as it starts, the result after a game over, else nothing (the maze needs the screen).
void gameLabel(char *out, int cap) {
  int best = pet::best() > game.score ? pet::best() : game.score;
  if (game.dead) snprintf(out, cap, game.newBest ? "NEW BEST %d" : "%d BEST %d", game.score, best);
  else if (mz.t < INTRO_S + 0.6f && mz.wonT < 0) snprintf(out, cap, "LEVEL %d", mz.level);
  else out[0] = 0;
}

void step(float dt) {
  if (game.on) return gameStep(dt), gameDraw();
  update(dt);
  weatherStep(dt);
  if (rove) return drawFlight();
  memcpy(fb, bg, sizeof fb);
  drawDoom();
  drawFlash();
  drawNazgul();
  drawEye();
  drawDrops();
  drawSauron();
  drawBeam();
  drawFx();
  drawLeaves();
}

}  // namespace sauron
