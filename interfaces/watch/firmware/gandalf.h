// Gandalf: DRAFT scene (not a pet yet, not in pets.h). 80x80 color-indexed buffer drawn 3x like the other scenes, no
// Arduino dependency (test/lotr_preview.cpp dumps clips).
// The weather picks him and the land (pet::wx): clear = Gandalf the White before Minas Tirith (Gondor); cloudy or wind =
// the Grey at Bag End (the Shire); rain or snow = the Grey in Rivendell; storm = the Grey on the Deeping Wall (Helm's
// Deep). A change between Grey and White plays the transformation, the land swaps inside its flash.
// Animations: idle (blink, robe in the wind), pipe with smoke rings, cheer (staff up, the tip lights), shake ("You shall
// not pass": raise, slam, a ring of light), a firework from the staff (Shire), the Grey -> White flash.
#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "pet.h"

namespace gandalf {

const int N = 80, GROUND = 62;
const float G = 60;  // firework sparks fall, px/s²

enum : uint8_t {
  SKY0, SKY1, SKY2, SKY3, CLOUD, CLOUD2, STAR, SUN, MOON,  // sky: tinted by the clock
  FAR, FAR2, MID, MID2, LAND, LAND2, LANDL, ACC1, ACC2, ACC3, ACC4, ACC5, ACC6, WATER, WATER2, SHADOW,  // the biome's
  LAMP, FIRE, FIRE2, INK, SKIN, SKIN2, EYE, MOUTH, BELT, BELT2, BUCKLE, BAG, BAG2, PIPE, SMOKE, LIGHT, LIGHT2, CREAM,
  SPARK1, SPARK2, SPARK3, DROP, FLAKE, FLASH,
  HAIR1, HAIR2, HAIR3, MANT, MANT2, ROBE, ROBE2, ROBE3, HAT, HAT2, HAT3, STAFF, STAFF2, BOOT,  // Grey or White
  NCOL
};
uint8_t PAL[NCOL][3] = {};  // tint() fills the sky, the land and the garb; the rest is fixed
const uint8_t FIXED[HAIR1 - LAMP][3] = {
    {255, 210, 110}, {255, 140, 40}, {255, 226, 120},                                // lit window, fire, its core
    {28, 24, 30},    {236, 192, 162}, {196, 146, 122}, {40, 52, 90}, {96, 36, 40},    // outline, skin, its shade, eye, mouth
    {132, 88, 52},   {92, 60, 36},    {210, 210, 218}, {150, 108, 66}, {104, 72, 44},  // belt, its shade, buckle, bag x2
    {120, 76, 44},   {214, 214, 220}, {255, 250, 220}, {200, 230, 255}, {250, 240, 220},  // pipe, smoke, light x2, cream
    {255, 90, 90},   {120, 220, 255}, {255, 220, 80},                                  // firework sparks
    {205, 222, 255}, {250, 252, 255}, {214, 212, 250},                                 // rain, snow, lightning flash
};
const int NG = NCOL - HAIR1;
const uint8_t GARB[2][NG][3] = {  // hair x3, mantle x2, robe x3, hat x3, staff x2, boots: the Grey, then the White
    {{238, 238, 240}, {196, 196, 204}, {146, 146, 156}, {128, 130, 144}, {96, 98, 112}, {128, 130, 144}, {96, 98, 112},
     {66, 68, 82}, {156, 158, 170}, {118, 120, 134}, {78, 80, 94}, {140, 98, 60}, {96, 64, 38}, {72, 54, 42}},
    {{255, 255, 255}, {222, 222, 230}, {176, 178, 192}, {206, 196, 174}, {166, 156, 136}, {248, 246, 240}, {220, 216, 206},
     {186, 180, 168}, {156, 158, 170}, {118, 120, 134}, {78, 80, 94}, {242, 240, 232}, {188, 184, 174}, {180, 172, 158}},
};

// ---- Lands ----
enum Biome : uint8_t { SHIRE, GONDOR, RIVENDELL, BATTLE };
const int NB = SHADOW - FAR + 1;
const uint8_t BIO[4][NB][3] = {  // day colors, FAR..SHADOW
    // Shire: far downs x2, the Hill x2, lawn x3; door green, brass, wood, glass, path, trunk; flowers red, yellow; shade
    {{150, 188, 146}, {116, 160, 112}, {104, 172, 74}, {74, 138, 58}, {112, 182, 74}, {82, 146, 60}, {176, 216, 104},
     {40, 112, 62}, {244, 204, 60}, {136, 96, 56}, {176, 206, 216}, {210, 182, 122}, {92, 62, 40}, {222, 72, 72},
     {250, 222, 90}, {58, 118, 50}},
    // Gondor: mountain x2, far plain x2, Pelennor x3; white stone, its shade, roofs, road, snow, gate; river x2; shade
    {{116, 124, 150}, {82, 88, 114}, {150, 168, 120}, {122, 140, 98}, {156, 172, 84}, {116, 134, 62}, {196, 204, 112},
     {248, 248, 244}, {190, 196, 210}, {104, 110, 132}, {190, 170, 120}, {236, 242, 250}, {58, 62, 80}, {100, 140, 190},
     {150, 180, 210}, {96, 116, 54}},
    // Rivendell: far valley x2, cliffs x2, terrace x3; walls, copper roof, gold, orange, trunk, pine; water x2; shade
    {{132, 152, 146}, {100, 120, 116}, {112, 126, 108}, {74, 90, 80}, {198, 188, 164}, {152, 144, 124}, {224, 214, 194},
     {240, 232, 212}, {84, 146, 136}, {236, 176, 56}, {206, 104, 44}, {100, 70, 50}, {58, 102, 74}, {206, 234, 250},
     {112, 172, 212}, {124, 114, 100}},
    // Helm's Deep: mountains x2, wall x2, walkway x3; tower, mortar, orcs, spear glint, banner green, its white horse
    {{84, 78, 76}, {58, 54, 54}, {108, 106, 110}, {74, 72, 80}, {152, 148, 142}, {112, 108, 102}, {182, 178, 170},
     {146, 142, 140}, {100, 98, 100}, {34, 28, 26}, {206, 206, 214}, {58, 104, 56}, {232, 228, 212}, {196, 228, 246},
     {112, 172, 212}, {92, 88, 82}},
};
const uint8_t SKYLOOK[4][MOON + 1][3] = {  // pet::NIGHT, DAWN, DAY, DUSK: sky, top to horizon; cloud, its shade; star, sun, moon
    {{10, 14, 40}, {20, 28, 70}, {40, 44, 96}, {70, 62, 112}, {60, 66, 100}, {44, 48, 82}, {255, 250, 220}, {255, 214, 90}, {250, 240, 200}},
    {{70, 80, 150}, {150, 110, 165}, {235, 140, 140}, {255, 185, 130}, {255, 214, 200}, {220, 150, 150}, {255, 240, 220}, {255, 240, 170}, {250, 235, 200}},
    {{44, 96, 170}, {74, 134, 200}, {128, 176, 222}, {196, 212, 222}, {252, 246, 232}, {214, 206, 206}, {255, 255, 255}, {255, 236, 140}, {240, 240, 240}},
    {{44, 46, 112}, {130, 66, 128}, {226, 98, 92}, {255, 158, 84}, {255, 170, 130}, {200, 100, 100}, {255, 240, 190}, {255, 226, 150}, {250, 225, 170}},
};
const float FILT[4][4] = {{16, 22, 58, 0.62f}, {255, 160, 140, 0.2f}, {0, 0, 0, 0}, {230, 100, 80, 0.28f}};  // land at each look

uint8_t biome = SHIRE;
float white = 0;    // 0 Grey, 1 White (the garb colors blend during the flash)
int clockMin = -1;  // minute of day, -1 = unknown (night look)
float nightW = 1;

void tint(int minuteOfDay) {
  clockMin = minuteOfDay;
  int a, b;
  float f = pet::look(clockMin, a, b);
  nightW = (a == pet::NIGHT ? 1 - f : 0) + (b == pet::NIGHT ? f : 0);
  for (int k = 0; k <= MOON; k++)
    for (int c = 0; c < 3; c++) PAL[k][c] = lroundf(SKYLOOK[a][k][c] * (1 - f) + SKYLOOK[b][k][c] * f);
  for (int k = 0; k < NB; k++)
    for (int c = 0; c < 3; c++) {
      float v = BIO[biome][k][c];
      float va = v * (1 - FILT[a][3]) + FILT[a][c] * FILT[a][3], vb = v * (1 - FILT[b][3]) + FILT[b][c] * FILT[b][3];
      PAL[FAR + k][c] = lroundf(va * (1 - f) + vb * f);
    }
  memcpy(PAL[LAMP], FIXED, sizeof FIXED);
  for (int k = 0; k < NG; k++)
    for (int c = 0; c < 3; c++) PAL[HAIR1 + k][c] = lroundf(GARB[0][k][c] * (1 - white) + GARB[1][k][c] * white);
  float ov = pet::OVERCAST[pet::wx], dark = pet::gloom();
  for (int k = SKY0; k <= SKY3; k++) pet::dim(PAL[k], ov, dark);
  pet::dim(PAL[FAR], ov, dark), pet::dim(PAL[FAR2], ov, dark);
  pet::dim(PAL[CLOUD], ov, dark * 0.6f), pet::dim(PAL[CLOUD2], ov, dark * 0.6f);
  for (int k = MID; k <= SHADOW; k++) pet::dim(PAL[k], ov * 0.3f, dark);
}
bool sunUp() { return clockMin >= pet::SUNRISE && clockMin < pet::SUNSET; }

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
inline uint8_t at(int x, int y) { return (unsigned)x < N && (unsigned)y < N ? fb[y * N + x] : INK; }
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
void ring(float cx, float cy, float rx, float ry, uint8_t c, float gap = 0) {  // ellipse outline; gap: dashed share
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
    case 'S': return SKIN;  case 's': return SKIN2; case 'E': return EYE;   case 'D': return MOUTH;
    case 'W': return HAIR1; case 'B': return HAIR2; case 'b': return HAIR3;
    case 'M': return MANT;  case 'm': return MANT2;
    case 'R': return ROBE;  case 'r': return ROBE2; case 'q': return ROBE3;
    case 'H': return HAT;   case 'h': return HAT2;  case 'k': return HAT3;
    case 'T': return BELT;  case 't': return BELT2; case 'Y': return BUCKLE; case 'F': return BOOT;
    case 'P': return BAG;   case 'p': return BAG2;
    case 'G': return STAFF; case 'g': return STAFF2; case 'L': return LIGHT;
  }
  return INK;
}
// Sprite column ax lands on x. Rows from r0 down lean by sway * ((r - r0) / (h - r0))² px (wind).
// solid: every pixel in one color (the transformation's silhouette).
void blit(const char *const *rows, int h, int ax, int x, int y0, int solid = -1, float sway = 0, int r0 = 0) {
  for (int r = 0; r < h && rows[r]; r++) {  // a short patch ends at its last row
    float k = r > r0 ? float(r - r0) / (h - r0) : 0;
    int off = lroundf(sway * k * k);
    for (int c = 0; rows[r][c]; c++)
      if (rows[r][c] != '.') px(x + c - ax + off, y0 + r, solid >= 0 ? solid : code(rows[r][c]));
  }
}
void art(const char *const *rows, int h, int x, int y, int flip = 1) {  // props (weather.h's leaves: C)
  static const char K[] = "KCYGR";
  const uint8_t C[] = {INK, ACC3, ACC4, MID2, WATER};
  int w = strlen(rows[0]);
  for (int r = 0; r < h; r++)
    for (int c = 0; c < w; c++) {
      const char *k = strchr(K, rows[r][flip > 0 ? c : w - 1 - c]);
      if (k && *k) px(x + c, y + r, C[k - K]);
    }
}

// ---- Weather (weather.h): the wind blows left to right; his hat brim keeps the rain off ----
const int WIND_DIR = 1;
float canopy(float x);
float landRow() { return GROUND - 6 + 14 * rnd(); }
float drift() { return 0; }
bool airy(uint8_t c) { return c <= MOON; }
void thunder();
#include "weather.h"

// ---- Gandalf: 27 wide, 46 tall, faces right. The body sprite (hat or hair, face, beard, robe) is drawn as is; at rest
// the ARM sprite adds the bell sleeve, the staff and the hand, in any other pose those are drawn to the pose. ----
const int FIG_H = 46, AX = 11;        // sprite column over g.x
const int STAFF_C = 23, HAND_R = 24;  // at rest: the staff's column, the hand's middle row
const char *const BODY[2][FIG_H] = {  // the Grey, the White: robe, belt, head, beard (no front arm, no staff)
  {
    ".....KKK", "....KhhhKK", ".....KKhhhK", ".......KhhhK", ".......KhhHK", "......KhhhHK", "......KhhhHHK",
    ".....KhhhhHHK", ".....KhhhhHHHK", "....KhhhhhHHHHK", "....KkkkkkkkkkK", "..KKhhhhhhhhhhhhHHHHKK",
    ".KhhhhhhhhhhhhhhhhHHHHK", ".KKkkkkkkkkkkkkkkkkkkKK", "..KBBbbssssssssssbK", "..KBBbbsWWWssWWWsbK",
    "..KBBbbSKESSSKESSbK", "..KBBbbSSSSSsSSSSbK", "..KBBbbSSSSssSSSsbK", "..KBBbbWWBWWWWBWWK",
    "..KBBbWBWWWBWWWBWWbK", "..KBbbBWWWBWWWBWWbK", "..KbBbBWWWBWWWBWWbKK", "..KbbKBWWWBWWWBWWbKK",
    "..KKKbWWWBWWWBWWWbKK", "..KqqKbWWBWWWBWWbKRK", "..KqqKbWWBWWWBWWbKRRK", "..KqqqKbBWWWBWWbKRRRK",
    "..KqqqKbBWWWBWWbKRRRK", ".KqqqqrKbWWWBWbKRRRRK", ".KqqqqrKbWWBWWbKRRRRK", ".KtTTTtTKbWBWbKTTTTTTK",
    ".KPPPtrrqKbBbKRRRRRRRK", ".KPPpPKrqKbWbKRRRRRRRK", ".KPPppKrqqKbKRRrRRRRRK", ".KPPppKrqqrRRRRrRRRRRK",
    ".KKPpKrrqqrRRRRrRRRRRK", ".KKKKqqrrqrrRRRRrRRRRRK", "KqqqqqrrqqrRRRRrrRRRRRK", "KqqqqqrrqqrRRRRrrRRRRRK",
    "KqqqqqrrqqrRRRRrrRRRRRK", "KqqqqqrrqqrRRRRrrRRRRRK", "KqqqqqrrqqrRRRRrrRRRRRK", "KqqqqqrrqqrRRRRrrRRRRRK",
    ".KKKKKKKKKKKKKKKKKKKKKK", ".....KFFFK...KFFFFK",
  },
  {
    "", "", "", "", "", "", "", "", "", "......KKKKKKK", "....KKWWWBWWWKK", "...KWBWWWBWWWWK", "...KBWWBWWWSSSSWK",
    "..KWBWBWSSSSSSSSSWK", "..KWBbBSSSSSSSSSSWK", "..KWBbBsWWWssWWWsWK", "..KWBbBSKESSSKESSWK",
    "..KWBbBSSSSSsSSSSWK", "..KWBbBSSSSssSSSsWK", "..KWBbBWWBWWWWBWWWK", "..KWBbWBWWWBWWWBWWbK",
    "..KWBbBWWWBWWWBWWBbK", "..KWbBBWWWBWWWBWWbKK", "..KBWbBWWWBWWWBWWbKK", "..KBbbbWWBWWWBWWbKMK",
    "..KbBKbWWBWWWBWWbKMK", "..KbKmKbWBWWWBWbKMMMK", "..KKKmKbBWWWBWWbKMMMK", "..KYKmMKbWWWBWbKRRMMK",
    ".KKtKMMrKbWWBbKRRMMMK", ".KYYYKMrKbWBWbKRRMMMK", ".KKkKtTTTKbBbKTTTTTKK", ".KKkKMMrqqKbKRRRRRMMMK",
    ".KKkKMMrqqrRRRRRRRMMMK", ".KKkKMMrqqrRRRRrRRMMMK", ".KKkKMMrqqrRRRRrRRRMMK", ".KKkKMMrqqrRRRRrRRRMMK",
    ".KKkKmMrrqrrRRRRrRRMMMK", "KKkKmMrrqqrRRRRrrRRMMMK", "KKkKmMrrqqrRRRRrrRRMMMK", "KKkKmMrrqqrRRRRrrRRRMMK",
    "KKkKmMrrqqrRRRRrrRRRMMK", "KKkKmMrrqqrRRRRrrRRRMMK", "KKkKmMrrqqrRRRRrrRRRMMK", ".KKKKKKKKKKKKKKKKKKKKKK",
    ".....KFFFK...KFFFFK",
  },
};
const char *const ARM[2][FIG_H] = {  // at rest: the bell sleeve, the staff and the hand on it
  {
    ".....................KKK", "....................KGGgK", "....................KGKKgK",
    "....................KK.KGK", "......................KGK", "......................KGK",
    "......................KGK", "......................KgK", "......................KGK",
    "......................KGK", "......................KGK", "......................KGK",
    ".......................GK", ".......................GK", "......................KgK",
    "......................KGK", "......................KGK", "......................KGK",
    "......................KGK", "......................KGK", "................KRK...KGK",
    "................KRRRK.KgK", ".................KRRRKKGK", ".................KRRRRKKK",
    "...................RRKSSSK", "..................rRRKSSsK", "..................rrRRKKK",
    ".................KqrrrKGK", ".................KqqrrKgK", ".................KqqqrKGK",
    "..................KKqqKGK", "....................K.KGK", "......................KGK",
    "......................KGK", "......................KGK", "......................KgK",
    "......................KGK", ".......................GK", ".......................GK",
    ".......................GK", ".......................GK", ".......................GK",
    ".......................gK", ".......................GK", ".......................GK",
    "......................KGK",
  },
  {
    "......................KKK", ".....................KGGGK", "....................KGKLKGK",
    "....................KGLLLGK", "....................KGKLKGK", ".....................KGGGK",
    "......................KgK", "......................KgK", "......................KGK",
    "......................KGK", "......................KGK", "......................KGK",
    "......................KGK", "......................KGK", "......................KgK",
    "......................KGK", "......................KGK", "......................KGK",
    "......................KGK", "......................KGK", "................KMK...KGK",
    "................KMMMK.KgK", ".................KMMMKKGK", ".................KMMMMKKK",
    "..................KMMKSSSK", "..................mMMKSSsK", ".................KmmMMKKK",
    ".................KmmmmKGK", ".................KmmmmKgK", ".................KmmmmKGK",
    "..................KKmmKGK", ".....................KKGK", "......................KGK",
    "......................KGK", "......................KGK", "......................KgK",
    "......................KGK", ".......................GK", ".......................GK",
    ".......................GK", ".......................GK", ".......................GK",
    ".......................gK", ".......................GK", ".......................GK",
    "......................KGK",
  },
};
// Face patches (rows from 15, columns from 7, '.' keeps the pixel): blink, happy, shouting.
const char *const FACE[3][7] = {
    {"", ".ss...ss.."},
    {"", ".KK...KK..", "", "", "", "...sDDs..."},
    {"sWW.ss.WWs", ".KEW.WKE..", "", "", "...KDDK...", "...KDDK...", "....KK...."},
};
enum { F_OPEN = -1, F_BLINK, F_HAPPY, F_SHOUT };
const char *const HAND[4] = {".KKK.", "KSSSK", "KSSsK", ".KKK."};  // column 2 on the staff
// Staff heads, column 3 on the staff, the last row just over the shaft's top: the Grey's crook, the White's cage of light.
const char *const CROOK[4] = {".KKK", "KGGgK", "KGKKgK", "KK.KGK"};
const char *const CAGE[7] = {"..KKK", ".KGGGK", "KGKLKGK", "KGLLLGK", "KGKLKGK", ".KGGGK", "..KgK"};
const int SHAFT_TOP[2] = {4, 7};  // sprite row where the shaft starts above the head

enum { IDLE, PIPING, RAISE, SLAM, TRANSFORM, FIREWORK };
struct {
  float x = 38, t = 0, dur = 0, blinkT = 3;
  int st = IDLE;
} g;
void pose(int st, float dur) { g.st = st, g.t = 0, g.dur = dur; }
float trickT = 8;  // s to the next idle trick

// Smoke rings, firework sparks, the slam's light ring.
struct Puff { float x, y, r, age; bool on; } puffs[4];
struct Spark { float x, y, vx, vy, age; uint8_t c; bool on; } sparks[40];
struct { float x, y, vy, age; bool on; } rocket;
float waveAge = -1, waveX = 0;  // the slam's ring of light

void puff(float x, float y) {
  for (auto &p : puffs)
    if (!p.on) return void(p = {x, y, 1, 0, true});
}
void burst(float x, float y) {
  const uint8_t C[] = {SPARK1, SPARK2, SPARK3};
  uint8_t c = C[int(rnd() * 3)];
  for (int i = 0; i < 40; i++) {
    float a = 6.2832f * i / 40 + rnd() * 0.1f, v = 16 + 10 * rnd();
    sparks[i] = {x, y, v * cosf(a), v * sinf(a) - 4, 0, uint8_t(i % 5 == 0 ? FIRE2 : c), true};
  }
}

// Where the staff hand is (hx, hy: the middle of the hand) and how far the staff's top leans (tilt, px). rest: the ARM
// sprite covers it.
float tipX, tipY, glow = 0;
void staffPose(float &hx, float &hy, float &tilt, bool &rest) {
  float x0 = lroundf(g.x) - AX, y0 = GROUND - FIG_H;
  float k = 0;
  tilt = 0;
  if (g.st == RAISE || g.st == FIREWORK) k = clampf(g.t / 0.25f, 0, 1) * (g.t < g.dur - 0.3f ? 1 : (g.dur - g.t) / 0.3f);
  if (g.st == TRANSFORM) k = clampf(g.t / 0.4f, 0, 1) * clampf((g.dur - g.t) / 0.5f, 0, 1);
  if (g.st == SLAM) {
    k = 1.2f * (g.t < 0.45f ? g.t / 0.45f : g.t < 0.55f ? 1 - (g.t - 0.45f) / 0.1f : 0);
    tilt = 2.5f * (1 - k);  // the staff tilts forward as it comes down
  }
  rest = k == 0 && g.st != SLAM && g.st != PIPING;
  hx = x0 + STAFF_C + 0.5f + k, hy = y0 + HAND_R + 1 - 14 * k;
}

// Bell sleeve from the shoulder to the hand: narrow up top, wide at the wrist, lit on its upper side.
void sleeve(float sx, float sy, float hx, float hy, int solid) {
  if (solid >= 0) return cone(sx, sy, 2.4f, hx, hy, 3.2f, solid);
  bool w = white > 0.5f;
  cone(sx, sy, 2.4f, hx, hy, 3.2f, INK);
  cone(sx, sy, 1.5f, hx, hy, 2.3f, w ? MANT2 : ROBE2);
  cone(sx, sy - 0.6f, 1, hx - 0.3f * (hx > sx ? 1 : -1), hy - 0.9f, 1.4f, w ? MANT : ROBE);
}

void drawGandalf() {
  int w = white > 0.5f;
  int x0 = lroundf(g.x) - AX, y0 = GROUND - FIG_H;
  int face = g.blinkT < 0.15f ? F_BLINK : F_OPEN;
  if (g.st == PIPING && g.t > 0.4f && g.t < g.dur - 0.4f) face = F_BLINK;
  if (g.st == RAISE || g.st == FIREWORK) face = F_HAPPY;
  if (g.st == SLAM) face = F_SHOUT;
  int solid = g.st == TRANSFORM && g.t > 1.1f && g.t < 1.6f ? LIGHT : -1;  // the flash: a white silhouette
  float wind = pet::wx == pet::WIND ? 1 + gust : pet::wx == pet::STORM ? 1 : 0;

  float hx, hy, tilt;
  bool rest;
  staffPose(hx, hy, tilt, rest);
  float top = hy - (HAND_R + 1 - SHAFT_TOP[w]), foot = hy + (FIG_H - HAND_R - 1);
  tipX = hx + tilt, tipY = top - (w ? 4 : 3);

  ellipse(g.x + 1, GROUND, 12, 1.6f, SHADOW);
  blit(BODY[w], FIG_H, 0, x0, y0, solid, wind, 32);
  if (face != F_OPEN && solid < 0) blit(FACE[face], 7, -7, x0, y0 + 15);

  if (rest) return blit(ARM[w], FIG_H, 0, x0, y0, solid);
  if (g.st == PIPING) {  // the staff stands by itself, his hand brings the pipe up
    float k = clampf(fminf(g.t, g.dur - g.t) / 0.4f, 0, 1);
    float bx = hx - 1 - 3 * k, by = hy - 2 * k;
    capsule(hx, top, hx, foot, 1.5f, solid >= 0 ? solid : INK), capsule(hx, top, hx, foot, 0.6f, solid >= 0 ? solid : STAFF);
    if (w) blit(CAGE, 7, 3, lroundf(hx) - 1, lroundf(top) - 7, solid);
    else blit(CROOK, 4, 3, lroundf(hx) - 1, lroundf(top) - 4, solid);
    int mx = x0 + 14, my = y0 + 20;  // the stem from his mouth, the bowl in his hand, embers in it
    if (k > 0.9f) bx = mx + 4.5f, by = my + 4;
    sleeve(x0 + 18, y0 + 21, bx, by + 1, solid);
    if (k > 0.9f) {
      for (int i = 0; i < 4; i++) px(mx + i, my + i / 2, PIPE);
      fillIf(mx + 4, my, mx + 5, my + 2, PIPE), px(mx + 4, my - 1, INK), px(mx + 5, my - 1, INK);
      if (fmodf(T, 0.6f) < 0.3f) px(mx + 4, my, FIRE), px(mx + 5, my, FIRE2);
    }
    return blit(HAND, 4, 2, lroundf(bx), lroundf(by) - 2, solid);
  }
  sleeve(x0 + 18, y0 + 21, hx - 1, hy + 1, solid);
  capsule(hx + tilt, top, hx - tilt, foot, 1.5f, solid >= 0 ? solid : INK);
  capsule(hx + tilt, top, hx - tilt, foot, 0.6f, solid >= 0 ? solid : STAFF);
  if (w) blit(CAGE, 7, 3, lroundf(hx + tilt) - 1, lroundf(top) - 7, solid);
  else blit(CROOK, 4, 3, lroundf(hx + tilt) - 1, lroundf(top) - 4, solid);
  blit(HAND, 4, 2, lroundf(hx) - 1, lroundf(hy) - 2, solid);
}

float canopy(float x) {  // the hat brim, or his head
  float x0 = lroundf(g.x) - AX;
  return white > 0.5f ? (fabsf(x - x0 - 9.5f) < 8 ? GROUND - FIG_H + 9 : 1e9f)
                      : (fabsf(x - x0 - 11.5f) < 11 ? GROUND - FIG_H + 12 : 1e9f);
}
float startle = 0;
void thunder() { startle = 0.6f; }

// ---- Lands: drawn once into bg, the sky part left as SKY0..3 so the sun, moon, stars and clouds go behind ----
int horizon = 46;
uint8_t skyAt(int x, int y) {
  float v = clampf(y, 0, horizon) * 3.0f / horizon;
  int b = v;
  return b + (v - b > bayer(x, y));
}
struct Box { int8_t x, y, w, h; };
Box windows[24];
int nwin = 0;
Box fires[12];
int nfire = 0;
void window(int x, int y, int w, int h) {
  if (nwin < 24) windows[nwin++] = {(int8_t)x, (int8_t)y, (int8_t)w, (int8_t)h};
}

float wave(float x, float top, float wob, float ph) { return top + wob * sinf(x * 0.09f + ph) + wob * 0.6f * sinf(x * 0.22f + ph * 2); }
void ground(float top, float wob, uint8_t c, uint8_t c2, float ph, int edge = 2) {  // land from a wavy top line down
  for (int x = 0; x < N; x++) {
    float y0 = wave(x, top, wob, ph);
    for (int y = floorf(y0); y < N; y++) px(x, y, y < y0 + edge ? c : c2);
  }
}
void tree(float x, float y, float r, uint8_t leaf, uint8_t leaf2, uint8_t lit, uint8_t trunk, float stem = 0) {
  // round crown on a trunk; stem lifts the crown
  fillIf(x - 1, y - r - stem, x, y, trunk), px(x - 2, y, trunk), px(x + 1, y, trunk);
  y -= stem;
  for (int k = 0; k < 7; k++) {
    float a = k * 0.9f, rr = r * (0.45f + 0.15f * h01(k + int(x)));
    disc(x + r * 0.55f * cosf(a), y - r * 1.2f + r * 0.45f * sinf(a), rr + 1, INK);
  }
  for (int k = 0; k < 7; k++) {
    float a = k * 0.9f, rr = r * (0.45f + 0.15f * h01(k + int(x)));
    disc(x + r * 0.55f * cosf(a), y - r * 1.2f + r * 0.45f * sinf(a), rr, sinf(a) > 0.2f ? leaf2 : leaf);
  }
  for (int k = 0; k < 4; k++) px(x - r * 0.4f + r * 0.5f * h01(k * 3 + int(x)), y - r * 1.7f + r * 0.6f * h01(k * 5 + int(x)), lit);
}

// Round door: dark rim, wood frame, the painted door with planks and a knob in the middle; flat at the ground.
void roundDoor(float cx, float cy, float r, uint8_t paint, int floorY) {
  for (int y = floorf(cy - r - 1); y <= floorY; y++)
    for (int x = floorf(cx - r - 1); x <= cx + r + 1; x++) {
      float d = hypotf(x + 0.5f - cx, y + 0.5f - cy);
      if (d > r + 0.6f) continue;
      px(x, y, d > r - 0.4f ? INK : d > r - 1.4f ? ACC3 : d > r - 2 ? INK : (x - lroundf(cx)) % 2 ? paint : SHADOW);
    }
  disc(cx, cy, 0.9f, ACC2);
}
void roundWindow(float cx, float cy, float r) {
  disc(cx, cy, r + 0.6f, INK), disc(cx, cy, r, ACC3), disc(cx, cy, r - 0.8f, ACC4);
  for (int k = -1; k <= 1; k++) px(cx + k, cy, ACC3), px(cx, cy + k, ACC3);
  window(cx - 1, cy - 1, 1, 1), window(cx + 1, cy + 1, 1, 1);
}
void hill(float cx, float cy, float rx, float ry, int phase) {  // grassy mound, lit rim on top, shade on its right
  for (int y = floorf(cy - ry); y < N; y++)
    for (int x = floorf(cx - rx); x <= cx + rx; x++) {
      float u = (x + 0.5f - cx) / rx, v = (y + 0.5f - cy) / ry, q = u * u + v * v;
      if (q >= 1) continue;
      uint8_t c = q > 0.9f && v < -0.3f ? LANDL : u > 0.35f - 0.4f * v ? MID2 : MID;
      if ((x * 3 + y * 7 + phase) % 13 == 0) c = c == MID ? MID2 : MID;
      px(x, y, c);
    }
}

void shire() {
  horizon = 44;
  ground(38, 2, FAR, FAR, 0.5f);  // the far downs, hedgerow trees along their tops
  for (int i = 0; i < 11; i++) {
    float tx = 2 + i * 7.3f + 4 * h01(i);
    disc(tx, wave(tx, 38, 2, 0.5f) - 0.5f, 1 + 1.1f * h01(i + 9), FAR2);
  }
  ground(45, 1.5f, FAR2, FAR2, 2.2f);  // nearer fields, hedges between them
  for (int x = 0; x < N; x++)
    for (int y = wave(x, 45, 1.5f, 2.2f) + 1; y < 56; y++)
      if ((x * 2 + y * 5) % 29 == 0 || (y % 4 == 0 && h01(x / 7 + y * 13) < 0.6f)) px(x, y, MID2);
  hill(84, 70, 30, 25, 1);  // a neighbour's hole on the right, a yellow door
  roundDoor(68, 54, 3.6f, WATER2, 57);
  roundWindow(76, 51, 1.8f);
  hill(4, 70, 32, 38, 0);  // the Hill, left: Bag End
  tree(13, 33, 6, MID, MID2, LANDL, ACC6);  // the Party Tree on its top
  fillIf(3, 28, 5, 33, ACC3), fillIf(2, 27, 6, 27, INK), fillIf(5, 29, 5, 33, ACC6);  // chimney
  roundDoor(13, 54, 6, ACC1, 59);  // the green door
  roundWindow(25, 48, 2.4f);
  ground(59, 0.6f, LAND, LAND2, 1, 2);  // the lawn
  for (int y = 60; y < N; y++)          // the path from the door, winding down to the lane
    for (int x = 0; x < N; x++) {
      float c = 13 + (y - 60) * 1.4f + 3 * sinf((y - 60) * 0.25f), w = 2.2f + (y - 60) * 0.25f;
      if (fabsf(x + 0.5f - c) < w) px(x, y, fabsf(x + 0.5f - c) > w - 1 ? LAND2 : ACC5);
    }
  fillIf(6, 59, 20, 60, ACC5);  // the step
  for (int x = 56; x < N; x++) {  // the garden fence, right
    if (x % 6 == 0) fillIf(x, 55, x, 62, ACC3), px(x, 54, INK), fillIf(x + 1, 56, x + 1, 62, ACC6);
    else px(x, 57, ACC3), px(x, 60, ACC3), px(x, 58, ACC6), px(x, 61, ACC6);
  }
  for (int i = 0; i < 34; i++) {  // flowers along the hill's foot and in the lawn
    int fx = i < 14 ? 1 + h01(i * 3) * 24 : h01(i * 3) * 80, fy = i < 14 ? 56 + h01(i * 3 + 1) * 3 : 61 + h01(i * 3 + 1) * 18;
    uint8_t c = at(fx, fy);
    if (c != LAND && c != LAND2 && c != MID && c != MID2) continue;
    if (i % 4 == 3) px(fx, fy, LANDL), px(fx + 1, fy - 1, LANDL);
    else px(fx, fy, i % 2 ? WATER : WATER2), px(fx, fy + 1, SHADOW);
  }
}

void gondor() {
  horizon = 48;
  for (int x = 0; x < 44; x++) {  // the mountains of Shadow, far off on the left
    float m = 42 + 2 * fabsf(sinf(x * 0.37f)) + 1.2f * fabsf(sinf(x * 0.91f + 1));
    for (int y = m; y < 49; y++) px(x, y, FAR2);
  }
  for (int y = 0; y < 52; y++)  // Mindolluin behind the city: dark rock, snow on its peak, its right face in shade
    for (int x = 30; x < N; x++) {
      float top = 9 + fabsf(x - 78) * 0.85f + 1.2f * fabsf(sinf(x * 0.6f));
      if (y < top) continue;
      bool shade = x > 78 - (y - 9) * 0.25f || (x + 2 * y) % 13 == 0;
      bool snow = y < 16 + 2 * sinf(x * 0.8f) + sinf(x * 1.9f);
      px(x, y, snow ? (shade ? ACC2 : ACC5) : shade ? FAR2 : FAR);
    }
  ground(48, 0.3f, MID, MID2, 1);  // the far fields, the Anduin glinting on the left
  for (int x = 0; x < 36; x++) px(x, 50 + lroundf(sinf(x * 0.15f)), WATER2), px(x, 51 + lroundf(sinf(x * 0.15f)), WATER);
  // Minas Tirith: seven tiers stepping up the hill. Each is four rows: slate roofs, pale house fronts (lit windows at
  // night), the bright top of its wall, the wall's face; the right end in shade.
  const float CX = 64;
  for (int k = 6; k >= 0; k--) {
    float w = 16 - k * 2.2f;
    int yb = 58 - k * 4;
    for (int x = ceilf(CX - w); x <= CX + w; x++) {
      bool sh = (x - CX) / w > 0.5f;
      int hs = (x + k * 3) % 4;
      px(x, yb - 3, hs < 2 ? ACC3 : hs == 2 ? ACC1 : ACC2);
      px(x, yb - 2, sh ? ACC2 : ACC1);
      if (hs == 2 && h01(k * 50 + x) < 0.5f) window(x, yb - 2, 1, 1);
      px(x, yb - 1, sh ? ACC1 : ACC5);
      px(x, yb, sh ? ACC3 : ACC2);
    }
  }
  for (int y = 30; y < 58; y++) {  // the great prow of rock splitting the tiers
    float w = (y - 30) * 0.07f;
    for (int x = lroundf(CX - 3 - w); x <= lroundf(CX - 3 + w); x++) px(x, y, x < CX - 3 ? ACC5 : ACC2);
    px(lroundf(CX - 4 - w), y, ACC3);
  }
  for (int y = 16; y <= 30; y++) px(CX - 1, y, ACC3), px(CX, y, ACC5), px(CX + 1, y, ACC2), px(CX + 2, y, ACC3);  // the White Tower
  fillIf(CX - 1, 15, CX + 2, 15, ACC3), px(CX, 14, ACC5), px(CX + 1, 14, ACC2), px(CX, 13, ACC3), px(CX + 1, 13, ACC3);
  px(CX, 11, ACC3), px(CX, 12, ACC3), px(CX + 1, 11, ACC1), px(CX + 2, 11, ACC1);  // the banner
  window(CX, 19, 1, 2);
  ground(58, 0.5f, LAND, LAND2, 3);  // the Pelennor
  for (int y = 58; y < N; y++)       // the road from the gate
    for (int x = 0; x < N; x++) {
      float c = CX - 1 - (y - 58) * 1.3f, w = 0.8f + (y - 58) * 0.22f;
      if (fabsf(x + 0.5f - c) < w) px(x, y, fabsf(x + 0.5f - c) > w - 1 ? LAND2 : ACC4);
    }
  for (int y = 55; y < 59; y++)  // the Great Gate
    for (int x = CX - 3; x <= CX + 1; x++)
      if (hypotf(x + 0.5f - (CX - 1), fmaxf(0, 57 - y)) < 2.2f) px(x, y, ACC6);
  for (int i = 0; i < 26; i++) {  // grass tufts
    int fx = h01(i * 5) * 80, fy = 60 + h01(i * 5 + 1) * 20;
    if (at(fx, fy) == LAND) px(fx, fy, LANDL), px(fx, fy - 1, LANDL);
  }
}

float cliffEdge(int y) { return 17 + 1.5f * sinf(y * 0.23f) + sinf(y * 0.5f) + y * 0.04f; }  // Rivendell's left cliff
void rivendell() {
  horizon = 40;
  for (int x = 0; x < N; x++) {  // mountains up the valley, snow on the high ones
    float m = 23 + 6 * fabsf(sinf(x * 0.05f + 1)) + 2 * fabsf(sinf(x * 0.23f));
    for (int y = m; y < 44; y++) px(x, y, y < m + 1.5f && m < 27 ? ACC1 : FAR);
  }
  ground(36, 2, FAR2, FAR2, 0, 1);  // the valley's far side, threads of falling water on it
  for (int k = 0; k < 2; k++)
    for (int y = wave(40 + k * 11, 36, 2, 0) + 1; y < 46; y++) px(40 + k * 11, y, WATER2);
  for (int y = 0; y < N; y++)  // the left cliff: layered rock, its lit edge, pines on the ledges
    for (int x = 0; x < cliffEdge(y); x++) {
      float e = cliffEdge(y);
      px(x, y, x > e - 1.5f ? MID : (y + x / 3) % 6 == 0 ? MID2 : x < e - 7 && (x + y) % 3 == 0 ? MID2 : MID);
    }
  for (int y = 6; y < 50; y += 7) disc(cliffEdge(y) - 1, y, 1.4f, ACC6), px(cliffEdge(y) - 3, y + 1, ACC6);
  for (int y = 44; y < 56; y++)  // a rock ledge under the hall
    for (int x = 54 - (56 - y) / 2; x < N; x++) px(x, y, (x + y) % 5 == 0 ? MID2 : MID);
  // the elven hall: pale walls, tall arched lit windows, a sweeping copper roof, a gilded spire
  fillIf(57, 36, 79, 47, ACC1), fillIf(57, 36, 57, 47, INK), fillIf(58, 47, 79, 47, LAND2);
  for (int x = 54; x < N; x++) {
    float c = 35 - 4.5f * (1 - powf((x - 68) / 14.f, 2)) + (x < 57 ? (57 - x) * 0.6f : 0);
    for (int y = lroundf(c); y <= 35; y++) px(x, y, y == lroundf(c) || y == 35 ? INK : x % 3 ? ACC2 : SHADOW);
  }
  fillIf(67, 25, 68, 30, ACC2), px(67, 24, ACC3), px(68, 24, ACC3), px(68, 23, ACC3);
  for (int i = 0; i < 3; i++) {
    int wx = 61 + i * 6;
    fillIf(wx, 39, wx + 2, 45, FAR2), px(wx + 1, 38, FAR2), window(wx, 40, 3, 5);
  }
  // a small domed pavilion on a ledge of the cliff
  fillIf(2, 41, 11, 47, ACC1), fillIf(4, 42, 4, 47, FAR2), fillIf(9, 42, 9, 47, FAR2), fillIf(1, 48, 12, 48, LAND2);
  for (int x = 1; x <= 12; x++) {
    float c = 41 - 3 * sqrtf(fmaxf(0, 1 - powf((x - 6.5f) / 5.5f, 2)));
    for (int y = lroundf(c); y <= 41; y++) px(x, y, y == lroundf(c) || y == 41 ? INK : ACC2);
  }
  window(6, 43, 2, 3);
  ground(55, 0.3f, LAND, LAND2, 1, 1);  // the stone terrace he stands on
  for (int y = 58; y < N; y += 4)       // flagstone joints
    for (int x = (y / 4) % 2 * 5; x < N; x += 10) fillIf(x, y, x + 8, y, LAND2), fillIf(x + 9, y - 3, x + 9, y, LAND2);
  for (int x = 0; x < N; x++) {  // a low balustrade behind him
    px(x, 50, LANDL), px(x, 51, LAND2);
    if (x % 5 < 2) fillIf(x, 52, x, 54, x % 5 ? LAND2 : LANDL);
  }
  tree(9, 66, 6, ACC3, ACC4, LAMP, ACC5, 3);  // autumn trees
  tree(71, 66, 6, ACC4, ACC3, LAMP, ACC5, 3);
  tree(27, 50, 3, ACC3, ACC4, LAMP, ACC5, 1);
}

void battle() {
  horizon = 40;
  for (int x = 0; x < N; x++)  // the Thrihyrne: dark peaks, their right faces in shade
    for (int y = 0; y < 44; y++) {
      float m = 34 - 9 * fabsf(sinf(x * 0.07f + 0.6f)) - 2.5f * fabsf(sinf(x * 0.3f));
      if (y >= m) px(x, y, (x + y) % 11 == 0 || fmodf(x * 0.07f + 0.6f, 3.1416f) > 1.57f ? FAR2 : FAR);
    }
  fillIf(0, 40, N - 1, 50, FAR2);  // the Deeping-coomb
  for (int i = 0; i < 80; i++) {   // the orc host: spears with glinting tips, torches among them
    int sx = 22 + h01(i) * 58, sy = 42 + h01(i + 99) * 8;
    fillIf(sx, sy - 1, sx + 1, sy, ACC3), fillIf(sx, sy - 4, sx, sy - 2, ACC3), px(sx, sy - 5, ACC4);
  }
  for (int i = 0; i < 6; i++) fires[nfire++] = {(int8_t)(26 + i * 9 + 4 * h01(i * 7)), (int8_t)(44 + 4 * h01(i)), 1, 1};
  for (int y = 4; y < N; y++)  // the Hornburg, left: a round tower of big stones, battlements on top
    for (int x = 0; x < 22; x++) {
      float edge = 18 + 1.5f * sinf(y * 0.08f);
      if (x > edge || (y < 8 && (x % 5 >= 3 || x > edge - 1))) continue;
      bool joint = y % 4 == 0 || (x + (y / 4) * 3) % 6 == 0;
      px(x, y, x > edge - 1 ? INK : joint ? ACC2 : x > edge - 5 ? MID2 : ACC1);
    }
  fillIf(9, 20, 11, 24, INK), fires[nfire++] = {10, 23, 1, 1};  // lit loopholes
  fillIf(14, 36, 15, 39, INK), fires[nfire++] = {14, 38, 1, 1};
  for (int y = 49; y < 56; y++)  // the Deeping Wall's parapet behind him
    for (int x = 20; x < N; x++)
      if (y > 52 || x % 7 < 4) px(x, y, y == 49 || (y < 53 && x % 7 == 0) ? LANDL : x % 7 == 0 || y == 55 ? MID2 : MID);
  ground(56, 0, LAND, LAND, 0);  // the walkway
  for (int y = 59; y < N; y += 3)
    for (int x = (y / 3) % 2 * 3; x < N; x += 7) px(x, y, LAND2), px(x + 1, y, LAND2);
  fillIf(0, 56, N - 1, 56, LANDL);
  fillIf(72, 20, 72, 49, INK);  // Rohan's banner pole; the flag flaps in drawLand
}

void buildBg() {
  nwin = nfire = 0;
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) fb[y * N + x] = skyAt(x, y);
  if (biome == SHIRE) shire();
  if (biome == GONDOR) gondor();
  if (biome == RIVENDELL) rivendell();
  if (biome == BATTLE) battle();
  for (int y = 0; y < N; y++)  // re-grade the sky to this land's horizon
    for (int x = 0; x < N; x++)
      if (fb[y * N + x] <= SKY3) fb[y * N + x] = skyAt(x, y);
  memcpy(bg, fb, sizeof bg);
}
uint8_t biomeFor(uint8_t wx) {
  return wx == pet::CLEAR ? GONDOR : wx == pet::STORM ? BATTLE : wx == pet::RAIN || wx == pet::SNOW ? RIVENDELL : SHIRE;
}
void setBiome(uint8_t b) {
  biome = b;
  buildBg();
  tint(clockMin);
}

// ---- Sky and the moving parts of the land ----
const int NS = 14;
uint8_t sx[NS], sy[NS];
float sp[NS];

void skyDisc(float cx, float cy, float r, uint8_t c, float cutX = 0, float cutY = 0, float cutR = 0) {
  for (int y = floorf(cy - r); y <= cy + r; y++)
    for (int x = floorf(cx - r); x <= cx + r; x++)
      if ((unsigned)x < N && (unsigned)y < N && fb[y * N + x] <= SKY3 && hypotf(x + 0.5f - cx, y + 0.5f - cy) < r &&
          (cutR == 0 || hypotf(x + 0.5f - cutX, y + 0.5f - cutY) > cutR))
        fb[y * N + x] = c;
}
void cloud(float cx, float cy, float s) {
  static const float P[5][3] = {{-6, 0.5f, 3.6f}, {-2, -2.2f, 4.8f}, {3.5f, -1, 4.2f}, {8, 1, 3}, {1, 1.2f, 4}};
  for (int y = floorf(cy - 7 * s); y <= cy + 3 * s; y++)
    for (int x = floorf(cx - 10 * s); x <= cx + 12 * s; x++) {
      if ((unsigned)x >= N || (unsigned)y >= N || fb[y * N + x] > SKY3) continue;
      for (auto &p : P) {
        float dx = x + 0.5f - (cx + p[0] * s), dy = y + 0.5f - (cy + p[1] * s);
        if (dx * dx + dy * dy < p[2] * p[2] * s * s) {
          px(x, y, y >= cy + 1.5f * s || (y >= cy && bayer(x, y) > 0.5f) ? CLOUD2 : CLOUD);
          break;
        }
      }
    }
}
const float CLOUDS[6][4] = {{8, 11, 0.7f, 2.0f}, {58, 17, 0.55f, 1.4f}, {30, 28, 0.75f, 1.0f},
                            {80, 6, 0.7f, 1.7f}, {45, 15, 0.6f, 2.3f}, {15, 24, 0.7f, 1.2f}};
int cloudCount() { return pet::wx == pet::CLEAR ? 2 : pet::wx == pet::WIND ? 4 : 6; }

void drawSky() {
  float ov = pet::OVERCAST[pet::wx];
  for (int i = 0; i < NS; i++)
    if (i < NS * nightW * (1 - ov) && sinf(T * 1.7f + sp[i]) > -0.2f) skyDisc(sx[i] + 0.5f, sy[i] + 0.5f, 0.6f + 0.4f * (i % 4 == 0), STAR);
  float mx = 62, my = 9;
  bool moon = true;
  if (clockMin >= 0) {
    moon = !sunUp();
    float p = moon ? float((clockMin - pet::SUNSET + 1440) % 1440) / (1440 - pet::SUNSET + pet::SUNRISE)
                   : float(clockMin - pet::SUNRISE) / (pet::SUNSET - pet::SUNRISE);
    mx = 12 + 56 * p, my = horizon - 32 * sinf(3.1416f * p);
  }
  if (ov < 0.6f && moon) skyDisc(mx, my, 4.6f, MOON, mx + 2.5f, my - 1.5f, 4.2f);
  else if (ov < 0.6f) {
    skyDisc(mx, my, 4.2f, SUN);
    if (pet::wx == pet::CLEAR) rays(mx, my);
  }
  for (int k = 0; k < cloudCount(); k++) {
    const float *c = CLOUDS[k];
    float x = fmodf(c[0] + T * c[3] * (pet::wx == pet::WIND ? 3 : pet::wx == pet::STORM ? 2 : 1), 110);
    cloud(x - 15, c[1], c[2]);
  }
}

void drawLand() {
  bool lit = nightW > 0.3f || pet::gloom() > 0;
  if (lit)
    for (int i = 0; i < nwin; i++) fillIf(windows[i].x, windows[i].y, windows[i].x + windows[i].w - 1, windows[i].y + windows[i].h - 1, LAMP);
  if (biome == SHIRE)  // chimney smoke curling up
    for (int k = 0; k < 6; k++) {
      float a = fmodf(T * 0.4f + k / 6.f, 1);
      float x = 4 + 3 * sinf(a * 6 + T) * a + 5 * a * (pet::wx == pet::WIND ? 3 : 1), y = 25 - a * 14;
      if (a < 0.85f)
        for (int j = 0; j < 4; j++) {
          int qx = x + (j & 1) * (1 + a), qy = y + (j >> 1) * (1 + a);
          if (at(qx, qy) <= MOON) px(qx, qy, SMOKE);
        }
    }
  if (biome == RIVENDELL) {  // the waterfall down the left cliff, foam at its foot
    for (int y = 4; y < 53; y++)
      for (int k = 0; k < 3; k++) px(16 + k, y, fmodf(y * 0.25f - T * 4 + k * 0.3f, 1) < 0.35f ? WATER : WATER2);
    for (int x = 13; x < 23; x++) px(x, 53, fmodf(x * 0.4f + T * 3, 1) < 0.5f ? WATER : WATER2);
  }
  for (int i = 0; i < nfire; i++) {  // flickering fires
    int x = fires[i].x, y = fires[i].y, h = 1 + (sinf(T * 9 + i * 2) > 0);
    for (int k = 0; k < fires[i].w; k++) px(x + k, y, FIRE), px(x + k, y - 1, FIRE2);
    for (int k = 2; k <= h + 1; k++) px(x + (k & 1), y - k, FIRE);
  }
  if (biome == BATTLE)  // Rohan's banner: green, the white horse, flapping
    for (int x = 73; x <= 80; x++) {
      int wv = lroundf(1.2f * sinf(T * 6 - x * 0.7f) * (x - 72) / 8);
      for (int y = 21; y <= 27; y++) px(x, y + wv, y == 21 || y == 27 || x == 80 ? INK : ACC5);
      if (x >= 74 && x <= 78) px(x, 24 + wv - (x == 75 || x == 76), ACC6), px(x, 25 + wv, x == 74 || x == 78 ? ACC6 : ACC5);
    }
  if (snowCover > 0)  // snow on the first land row of each column
    for (int x = 0; x < N; x++) {
      if (h01(x * 3) > snowCover) continue;
      for (int y = 6; y < GROUND; y++)
        if (fb[y * N + x] > MOON && fb[y * N + x] <= SHADOW) {
          px(x, y, FLAKE);
          if (h01(x * 5) < snowCover * 0.6f) px(x, y + 1, FLAKE);
          break;
        }
    }
}

void drawEffects() {
  for (auto &p : puffs)  // smoke rings: grow, rise, thin out
    if (p.on) ring(p.x, p.y, p.r, p.r * 0.7f, SMOKE, clampf((p.age - 1.5f) / 2, 0, 0.9f));
  if (rocket.on) {
    px(rocket.x, rocket.y, FIRE2);
    for (int k = 1; k < 4; k++) if (h01(k + int(T * 30)) > 0.3f) px(rocket.x, rocket.y + k * 2, FIRE);
  }
  for (auto &s : sparks)
    if (s.on && h01(int(s.x * 7 + s.y)) > s.age / 1.6f) {
      px(s.x, s.y, s.c);
      if (s.age < 0.5f) px(s.x - s.vx * 0.03f, s.y - s.vy * 0.03f, s.c);  // a short trail while fast
    }
  if (waveAge >= 0) {  // the slam: a ring of light along the ground, dashed as it fades
    float r = 6 + 70 * waveAge;
    ring(waveX, GROUND, r, r * 0.22f, LIGHT, clampf(waveAge * 1.2f, 0, 0.9f));
    ring(waveX, GROUND, r * 0.8f, r * 0.18f, LIGHT2, clampf(waveAge * 1.5f, 0, 0.9f));
  }
  if (glow > 0) {  // the staff tip
    disc(tipX, tipY, 1 + glow * 1.5f, LIGHT2), disc(tipX, tipY, 0.7f + glow, LIGHT);
    for (int k = 0; k < 8; k++) {
      float a = k * 0.785f + T * 2, r = 4 + 3 * glow + (k & 1) * 2;
      if (glow > 0.4f) px(tipX + r * cosf(a), tipY + r * sinf(a), LIGHT);
    }
  }
  if (g.st == TRANSFORM) {  // light gathers around him, then flares out as the White
    float t = g.t;
    int cx = g.x, cy = GROUND - 22;
    if (t < 1.2f)
      for (int k = 0; k < 16; k++) {
        float a = k * 0.3927f + t * 3, r = 26 * (1 - fmodf(t * 1.5f + h01(k), 1));
        px(cx + r * cosf(a), cy + r * 1.1f * sinf(a), k % 2 ? LIGHT : LIGHT2);
      }
    if (t > 1.1f && t < 2.6f) {
      float k = (t - 1.1f) / 1.5f;
      for (int i = 0; i < 16; i++) {
        float a = i * 0.3927f, r0 = 22 * k, r1 = r0 + 6 * (1 - k);
        capsule(cx + r0 * cosf(a), cy + r0 * 1.2f * sinf(a), cx + r1 * cosf(a), cy + r1 * 1.2f * sinf(a), 0.5f, k < 0.5f ? LIGHT : LIGHT2);
      }
    }
  }
}

// ---- Logic ----
bool wantWhite() { return pet::wx == pet::CLEAR; }

void shake() { pose(SLAM, 1.6f); }
void cheer() {
  if (g.st == IDLE || g.st == PIPING) pose(RAISE, 1.6f);
}
void smoke() { pose(PIPING, 4.2f); }
void firework() { pose(FIREWORK, 3.2f); }

void update(float dt) {
  T += dt, g.t += dt;
  if ((g.blinkT -= dt) < 0) g.blinkT = 2.5f + 3 * rnd();
  startle = fmaxf(0, startle - dt);
  glow = 0;
  uint8_t want = biomeFor(pet::wx);
  if (g.st != TRANSFORM && (want != biome || (white > 0.5f) != wantWhite())) {
    if ((white > 0.5f) != wantWhite()) pose(TRANSFORM, 2.8f);
    else setBiome(want);
  }
  switch (g.st) {
    case IDLE:
      if ((trickT -= dt) < 0) {
        trickT = 10 + 10 * rnd();
        if (biome == SHIRE && rnd() < 0.4f) firework();
        else smoke();
      }
      if (startle > 0) glow = startle;
      break;
    case PIPING:
      if ((g.t > 1.3f && g.t - dt <= 1.3f) || (g.t > 2.5f && g.t - dt <= 2.5f)) puff(lroundf(g.x) - AX + 19, GROUND - FIG_H + 17);
      break;
    case RAISE: glow = clampf(g.t / 0.3f, 0, 1) * clampf((g.dur - g.t) / 0.4f, 0, 1); break;
    case FIREWORK:
      glow = g.t < 0.6f ? g.t / 0.6f : 0;
      if (g.t > 0.6f && g.t - dt <= 0.6f) rocket = {tipX, tipY - 2, -42, 0, true};
      break;
    case SLAM:
      glow = g.t < 0.45f ? g.t / 0.45f : g.t < 0.9f ? 1 : 0;
      if (g.t > 0.55f && g.t - dt <= 0.55f) waveAge = 0, waveX = g.x + 12;
      break;
    case TRANSFORM: {
      glow = clampf(g.t / 1.1f, 0, 1);
      float w = clampf((g.t - 1.1f) / 0.4f, 0, 1);
      if (!wantWhite()) w = 1 - w;
      if (fabsf(w - white) > 1e-3f || (g.t > 1.35f && biome != biomeFor(pet::wx))) {
        white = wantWhite() ? fmaxf(white, w) : fminf(white, w);
        if (g.t > 1.35f && biome != biomeFor(pet::wx)) biome = biomeFor(pet::wx), buildBg();
        tint(clockMin);
      }
      break;
    }
  }
  if (g.st != IDLE && g.t >= g.dur) pose(IDLE, 0);
  for (auto &p : puffs)
    if (p.on) {
      p.age += dt, p.r += 1.2f * dt, p.y -= 5 * dt, p.x += (pet::wx == pet::WIND ? 8 : 2) * dt;
      if (p.age > 3.5f || p.y < 12) p.on = false;
    }
  if (rocket.on) {
    rocket.y += rocket.vy * dt, rocket.age += dt;
    rocket.x += 3 * sinf(rocket.age * 12) * dt;
    if (rocket.age > 0.55f) rocket.on = false, burst(rocket.x, rocket.y);
  }
  for (auto &s : sparks)
    if (s.on) {
      s.age += dt, s.vy += G * dt, s.x += s.vx * dt, s.y += s.vy * dt, s.vx *= 1 - 1.5f * dt;
      if (s.age > 1.6f) s.on = false;
    }
  if (waveAge >= 0 && (waveAge += dt) > 1) waveAge = -1;
}

void init() {
  g = {}, trickT = 8, waveAge = -1, rocket.on = false;
  for (auto &p : puffs) p.on = false;
  for (auto &s : sparks) s.on = false;
  for (int i = 0; i < NS; i++) sx[i] = 10 + rnd() * 60, sy[i] = 3 + rnd() * 30, sp[i] = rnd() * 6.28f;
  white = wantWhite();
  setBiome(biomeFor(pet::wx));
}

void step(float dt) {
  update(dt);
  weatherStep(dt);
  memcpy(fb, bg, sizeof fb);
  drawSky();
  drawFlash();
  drawLand();
  drawDrops();
  drawGandalf();
  drawEffects();
  drawLeaves();
}

}  // namespace gandalf
