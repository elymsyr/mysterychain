// Sauron: DRAFT scene (not a pet yet, not in pets.h). 80x80 color-indexed buffer drawn 3x like the other scenes, no
// Arduino dependency (test/lotr_preview.cpp dumps clips).
// Always Mordor: Barad-dur with the Eye on the left, Mount Doom smoking on the right, lava cracks in the plain of
// Gorgoroth. The clock tints it (day = murky red, night = black with the lava glowing); the weather greys it, snow
// falls as ash, a storm sends lightning.
// Animations: idle (the visor glows, the Eye roams), cheer (mace up, the Eye flares), shake (mace slam, a ring of fire
// and cracks), the Ring (he raises it, it shines), the Eye's gaze (a beam sweeps the plain), Mount Doom erupts, a
// Nazgul on a fell beast flies by.
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
  SMOKE, SMOKE2, CREAM, DROP, FLAKE, FLASH, NCOL
};
uint8_t PAL[NCOL][3] = {};
const uint8_t FIXED[NCOL - LAVA][3] = {
    {255, 112, 20}, {255, 214, 90}, {196, 36, 10}, {255, 124, 20}, {255, 232, 130}, {255, 196, 96},  // lava, the Eye, its gaze
    {20, 16, 22},   {172, 170, 186}, {112, 110, 124}, {64, 62, 72}, {40, 38, 46},   // outline; armor: edge, light, mid, dark
    {52, 44, 54},   {34, 28, 36},    {140, 30, 24},   {92, 20, 18},                  // cloak, its folds, the red lining x2
    {255, 150, 50}, {200, 70, 30},   {240, 196, 70},  {196, 136, 30},               // visor glow x2, the Ring x2
    {84, 74, 74},   {54, 46, 48},    {250, 240, 220},                               // smoke x2, cream (bubble)
    {150, 120, 120}, {150, 144, 140}, {255, 220, 210},                              // rain, ash, lightning flash
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
inline void px(int x, int y, uint8_t c) {
  if ((unsigned)x < N && (unsigned)y < N) fb[y * N + x] = c;
}
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
    case 'O': return SLIT;   case 'o': return SLIT2;  case 'Y': return GOLD;
  }
  return INK;
}
void blit(const char *const *rows, int h, int ax, int x, int y0) {
  for (int r = 0; r < h; r++)
    for (int c = 0; rows[r][c]; c++)
      if (rows[r][c] != '.') px(x + c - ax, y0 + r, code(rows[r][c]));
}
void art(const char *const *rows, int h, int x, int y, int flip = 1) {  // weather.h's blown leaves: here ash flakes
  int w = strlen(rows[0]);
  for (int r = 0; r < h; r++)
    for (int c = 0; c < w; c++)
      if (rows[r][flip > 0 ? c : w - 1 - c] != '.') px(x + c, y + r, rows[r][flip > 0 ? c : w - 1 - c] == 'K' ? INK : SMOKE);
}

// ---- Weather (weather.h): ash drifts right; snow falls as ash (FLAKE is grey) ----
const int WIND_DIR = 1;
float canopy(float) { return 1e9f; }
float landRow() { return GROUND - 10 + 18 * rnd(); }
float drift() { return 0; }
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

enum { IDLE, RAISE, SLAM, RING, GAZE };
struct {
  float x = 44, t = 0, dur = 0;
  int st = IDLE;
} s;
void pose(int st, float dur) { s.st = st, s.t = 0, s.dur = dur; }
float trickT = 9;

float eyeLook = 0, eyeFlare = 0;  // pupil offset -1..1; flare 0..1 (bigger, brighter)
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

float ringX, ringY;
void drawSauron() {
  int x0 = lroundf(s.x) - AX, y0 = GROUND - FIG_H;
  bool blaze = eyeFlare > 0.3f || s.st == SLAM || s.st == RING;
  float pulse = blaze ? 1 : 0.55f + 0.45f * sinf(T * 2.2f);  // the visor's glow breathes
  PAL[SLIT][0] = 200 + 55 * pulse, PAL[SLIT][1] = 70 + 110 * pulse, PAL[SLIT][2] = 30 + 60 * pulse * pulse;
  ellipse(s.x, GROUND, 16, 1.6f, SHADOW);
  blit(BODY, FIG_H, 0, x0, y0);
  ringX = x0 + RINGC, ringY = y0 + RINGR;
  if (s.st == IDLE || s.st == GAZE) return blit(ARMS, FIG_H, 0, x0, y0);

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
  ringX = lx, ringY = ly - 1;
  mace(rx, ry, mx, my);
  arm(x0 + SHL, y0 + SHY, lx, ly);
  arm(x0 + SHR, y0 + SHY, rx, ry);
  px(ringX, ringY, GOLD);
}

void drawEye() {
  float f = eyeFlare, rx = 3 + 1.4f * f, ry = 1.8f + 0.7f * f;
  for (int k = 0; k < 10; k++) {  // licking flames around it
    float a = k * 0.628f + T * 2.5f, r = rx + 1 + h01(k + int(T * 12)) * (1.2f + 1.8f * f);
    px(EX + r * cosf(a), EY + r * 0.6f * sinf(a), k % 3 ? EYEO : EYER);
  }
  for (int y = floorf(EY - ry - 1); y <= EY + ry + 1; y++)
    for (int x = floorf(EX - rx - 1); x <= EX + rx + 1; x++) {
      float u = (x + 0.5f - EX) / rx, v = (y + 0.5f - EY) / ry, q = u * u + v * v;
      if (q < 1) px(x, y, q > 0.65f ? EYER : q > 0.25f ? EYEO : EYEY);
    }
  int p = lroundf(EX + eyeLook * (rx - 1.5f));  // the slit pupil
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

// ---- Logic ----
float lookT = 0;
void shake() { pose(SLAM, 1.6f); }
void cheer() {
  if (s.st == IDLE) pose(RAISE, 1.8f);
}
void ringUp() { pose(RING, 3.6f); }
void gaze() { pose(GAZE, 5); }
void erupt() { eruptT = 0; }
void nazgul() { nazT = 0; }
float startle = 0;
void thunder() { startle = 0.5f; }

void update(float dt) {
  T += dt, s.t += dt;
  startle = fmaxf(0, startle - dt);
  float wantFlare = s.st == RAISE ? 1 : s.st == RING ? 0.8f : s.st == SLAM ? 0.7f : startle > 0 ? 1 : 0;
  eyeFlare += (wantFlare - eyeFlare) * fminf(1, dt * 5);
  if ((lookT -= dt) < 0) lookT = 1.5f + 2 * rnd(), eyeLook = rnd() * 2 - 1;  // the Eye roams
  beamX = -1;
  if (s.st == GAZE) {  // sweeps the plain left to right and back
    float u = s.t / s.dur;
    beamX = 6 + 68 * (0.5f - 0.5f * cosf(u * 6.2832f * 1.5f)), beamY = 62 + 8 * sinf(u * 9);
    eyeLook = clampf((beamX - EX) / 30, -1, 1);
  }
  if (s.st == IDLE && (trickT -= dt) < 0) {
    trickT = 10 + 10 * rnd();
    float r = rnd();
    if (r < 0.35f) gaze();
    else if (r < 0.6f) ringUp();
    else if (r < 0.8f) erupt();
    else nazgul();
  }
  if (s.st == SLAM && s.t > 0.5f && s.t - dt <= 0.5f) {
    waveAge = 0, waveX = lroundf(s.x) - AX + 41;
    spray(waveX, GROUND - 1, 14, 30, 10, LAVA);
  }
  if (s.st != IDLE && s.t >= s.dur) pose(IDLE, 0);
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
  s = {}, trickT = 9, waveAge = -1, eruptT = -1, nazT = -1;
  for (auto &b : bits) b.on = false;
  buildBg();
  tint(clockMin);
}

void step(float dt) {
  update(dt);
  weatherStep(dt);
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
