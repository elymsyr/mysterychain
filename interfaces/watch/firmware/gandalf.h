// Gandalf: a pet like the others (pet.h, pets.h look 3). 80x80 color-indexed buffer drawn 3x like the other scenes, no
// Arduino dependency (test/lotr_preview.cpp dumps clips).
// The weather picks him and the land (pet::wx): clear = Gandalf the White before Minas Tirith (Gondor); cloudy or wind =
// the Grey at Bag End (the Shire); rain or snow = the Grey in Rivendell; storm = the Grey on the Deeping Wall (Helm's
// Deep). A change between Grey and White plays the transformation, the land swaps inside its flash.
// Animations: idle (blink, robe in the wind), pipe with smoke rings, cheer (staff up, the tip lights), shake ("You shall
// not pass": raise, slam, a ring of light), a firework from the staff (Shire), the Grey -> White flash.
// Pet: his mood picks what he does (asleep on his staff, peering into an empty tankard, sighing, yawning); fed, he drinks
// an ale; full, he waves it off; petted, he chuckles. The music screen: he rides Shadowfax across Rohan (see "Ride").
// Game (hold 1-3 s): the Bridge of Khazad-dum against the Balrog, see "Game" below.
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
// The ride (music screen) is always across Rohan: far peaks x2, downs x2, grass x3; road x2, rock x2, leaf, trunk;
// snow on the peaks, bush; shade.
const uint8_t RIDE[NB][3] = {
    {128, 146, 176}, {98, 112, 146}, {170, 170, 100}, {136, 140, 78}, {170, 178, 92}, {134, 146, 66}, {206, 206, 124},
    {190, 164, 116}, {150, 124, 86}, {150, 146, 138}, {104, 100, 96}, {74, 120, 62}, {92, 66, 44}, {238, 242, 250},
    {100, 132, 60}, {92, 106, 50}};
const uint8_t SKYLOOK[4][MOON + 1][3] = {  // pet::NIGHT, DAWN, DAY, DUSK: sky, top to horizon; cloud, its shade; star, sun, moon
    {{10, 14, 40}, {20, 28, 70}, {40, 44, 96}, {70, 62, 112}, {60, 66, 100}, {44, 48, 82}, {255, 250, 220}, {255, 214, 90}, {250, 240, 200}},
    {{70, 80, 150}, {150, 110, 165}, {235, 140, 140}, {255, 185, 130}, {255, 214, 200}, {220, 150, 150}, {255, 240, 220}, {255, 240, 170}, {250, 235, 200}},
    {{44, 96, 170}, {74, 134, 200}, {128, 176, 222}, {196, 212, 222}, {252, 246, 232}, {214, 206, 206}, {255, 255, 255}, {255, 236, 140}, {240, 240, 240}},
    {{44, 46, 112}, {130, 66, 128}, {226, 98, 92}, {255, 158, 84}, {255, 170, 130}, {200, 100, 100}, {255, 240, 190}, {255, 226, 150}, {250, 225, 170}},
};
const float FILT[4][4] = {{16, 22, 58, 0.62f}, {255, 160, 140, 0.2f}, {0, 0, 0, 0}, {230, 100, 80, 0.28f}};  // land at each look

// The game's Moria, all fire (over SKY0..SHADOW while it runs; the garb stays, the Grey's darkened to a silhouette).
const uint8_t MORIA[SHADOW + 1][3] = {
    {40, 8, 6},     {96, 22, 8},    {170, 52, 12},  {232, 112, 30},   // the haze: dark at the edges, bright behind him
    {255, 176, 64}, {204, 82, 20},  {58, 20, 14},   {110, 40, 20},    // flames behind him x2; smoke x2
    {255, 238, 176},                                                  // white-hot: whip core, embers
    {46, 22, 18},   {24, 12, 12},   {206, 92, 32},  {112, 46, 22},    // basalt columns, their gaps; lit tops, rims
    {200, 164, 122}, {142, 110, 80}, {242, 216, 172},                 // the bridge, its joints, its lit far end
    {66, 32, 22},   {30, 13, 10},   {255, 110, 24}, {255, 222, 120},  // Balrog: body, shade, lava, its core
    {72, 26, 20},   {30, 12, 10},                                     // his wings, their bones
    {150, 36, 10},  {70, 16, 8},    {14, 6, 6},                       // the chasm's glow x2, shade
};
const uint8_t SILHOUETTE[] = {HAT, HAT2, HAT3, ROBE, ROBE2, ROBE3, MANT, MANT2};
pet::Game &game = pet::runner;  // the game below: the bridge (score shared with the other scenes)

uint8_t biome = SHIRE;
bool rove = false;  // riding (the music screen, or a ride of his own): Rohan's colors
const int RH = 44, RG = 77;  // the ride's horizon, the row under the hooves
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
      float v = (rove ? RIDE : BIO[biome])[k][c];
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
  if (game.on) {
    memcpy(PAL, MORIA, sizeof MORIA);
    if (white < 0.5f)  // the Grey: dark against the fire
      for (uint8_t k : SILHOUETTE) PAL[k][0] = PAL[k][0] * 0.5f, PAL[k][1] = PAL[k][1] * 0.4f, PAL[k][2] = PAL[k][2] * 0.4f;
  }
  pet::palDirty = true;  // the transformation and the game retint mid-minute
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
// solid: every pixel in one color (the transformation's silhouette). Rows above headR move by (hdx, hdy) (a nod, a head
// shake; hdy >= 0, or a gap opens): drawn bottom-up so the lowered head goes over the robe.
void blit(const char *const *rows, int h, int ax, int x, int y0, int solid = -1, float sway = 0, int r0 = 0, int hdx = 0,
          int hdy = 0, int headR = 0) {
  int n = 0;
  while (n < h && rows[n]) n++;  // a short patch ends at its last row
  for (int r = n - 1; r >= 0; r--) {
    float k = r > r0 ? float(r - r0) / (h - r0) : 0;
    int off = lroundf(sway * k * k) + (r < headR ? hdx : 0), y = y0 + r + (r < headR ? hdy : 0);
    for (int c = 0; rows[r][c]; c++)
      if (rows[r][c] != '.') px(x + c - ax + off, y, solid >= 0 ? solid : code(rows[r][c]));
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
float landRow() { return rove ? 57 + 22 * rnd() : GROUND - 6 + 14 * rnd(); }
float drift() { return rove ? -55 : 0; }  // riding into the rain
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
// Face patches (rows from 15, columns from 7, '.' keeps the pixel): blink, happy, shouting, yawning, one eye open
// (asleep), wide-eyed (startled).
const char *const FACE[6][7] = {
    {"", ".ss...ss.."},
    {"", ".KK...KK..", "", "", "", "...sDDs..."},
    {"sWW.ss.WWs", ".KEW.WKE..", "", "", "...KDDK...", "...KDDK...", "....KK...."},
    {"", ".ss...ss..", "", "", "...KDDK...", "...KDDK...", "....KK...."},
    {"", ".ss......."},
    {"sWW.ss.WWs", ".KEW.WKE.."},
};
enum { F_OPEN = -1, F_BLINK, F_HAPPY, F_SHOUT, F_YAWN, F_PEEK, F_WIDE };
const int HEAD_R = 21;  // sprite rows that nod with his head: hat, face, the top of the beard
// A tankard of ale (P wood, Y iron hoops, W foam), the handle on the right; column 0 at the left rim.
const char *const MUG[7] = {".WWWW..", "KWWWWK.", "KPPPpKKK", "KYYYYK.K", "KPPPpK.K", "KPPPpKKK", "KKKKKK.."};
const char *const HAND[4] = {".KKK.", "KSSSK", "KSSsK", ".KKK."};  // column 2 on the staff
// Staff heads, column 3 on the staff, the last row just over the shaft's top: the Grey's crook, the White's cage of light.
const char *const CROOK[4] = {".KKK", "KGGgK", "KGKKgK", "KK.KGK"};
const char *const CAGE[7] = {"..KKK", ".KGGGK", "KGKLKGK", "KGLLLGK", "KGKLKGK", ".KGGGK", "..KgK"};
const int SHAFT_TOP[2] = {4, 7};  // sprite row where the shaft starts above the head

// Pet states (SLEEP, HUNGRY, SAD: dur 0 = while the mood lasts); WAKE: shaken awake, he dozes off again.
enum { IDLE, PIPING, RAISE, SLAM, TRANSFORM, FIREWORK, SLEEP, DRINK, REFUSE, CHUCKLE, YAWN, HUNGRY, SAD, WAKE };
struct {
  float x = 38, t = 0, dur = 0, blinkT = 3, peek = 0;  // peek: s one eye stays open (a message while asleep)
  int st = IDLE;
} g;
void pose(int st, float dur) { g.st = st, g.t = 0, g.dur = dur; }
float trickT = 8, yawnT = 0, tripT = 240;  // s to the next idle trick / tired yawn / ride of his own
pet::Mood mood = pet::HAPPY;
void setMood(pet::Mood m) { mood = m; }
bool freeHand() { return g.st == PIPING || g.st == DRINK || g.st == REFUSE || g.st == HUNGRY; }  // the staff stands alone
bool tipped() { return g.st == HUNGRY && fmodf(g.t, 3) > 1.6f && fmodf(g.t, 3) < 2.4f; }  // shaking the empty tankard

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
  rest = k == 0 && g.st != SLAM && !freeHand();
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

// A tankard with its left rim at x, top at y; upside down, or empty (the foam rows dark).
void mug(int x, int y, bool flip, bool empty, int solid) {
  for (int r = 0; r < 7; r++)
    for (int c = 0; MUG[r][c]; c++) {
      char ch = MUG[flip ? 6 - r : r][c];
      if (ch == '.' || (empty && ch == 'W' && (flip ? 6 - r : r) == 0)) continue;
      px(x + c, y + r, solid >= 0 ? solid : empty && ch == 'W' ? INK : code(ch));
    }
}
void zzz(float x, float y) {  // z z z floating up, cream at night, ink by day
  static const char *const Z[4] = {"####", "..#.", ".#..", "####"};
  for (int k = 0; k < 3; k++) {
    float p = fmodf(T * 0.3f + k / 3.f, 1);
    int zx = lroundf(x + 7 * p + 1.5f * sinf(T * 2 + k)), zy = lroundf(y - 13 * p);
    if (p < 0.85f)
      for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
          if (Z[r][c] == '#') px(zx + c, zy + r, nightW > 0.5f ? CREAM : INK);
  }
}

void drawGandalf() {
  int w = white > 0.5f;
  int x0 = lroundf(g.x) - AX, y0 = GROUND - FIG_H;
  int face = g.blinkT < 0.15f ? F_BLINK : F_OPEN;
  if (g.st == PIPING && g.t > 0.4f && g.t < g.dur - 0.4f) face = F_BLINK;
  if (g.st == RAISE || g.st == FIREWORK || g.st == CHUCKLE) face = F_HAPPY;
  if (g.st == SLAM) face = F_SHOUT;
  if (g.st == SLEEP) face = g.peek > 0 ? F_PEEK : F_BLINK;
  if (g.st == WAKE) face = F_WIDE;
  if (g.st == YAWN) face = g.t > 0.25f && g.t < g.dur - 0.3f ? F_YAWN : F_BLINK;
  if (g.st == REFUSE || g.st == SAD || g.st == HUNGRY) face = F_BLINK;  // eyes shut / looking down
  if (g.st == DRINK) face = g.t > 0.5f && g.t < 2.3f ? F_BLINK : g.t >= 2.3f ? F_HAPPY : face;
  int hdx = 0, hdy = 0;  // the head's nod / shake; the whole figure's hop
  if (g.st == SLEEP) hdy = 1 + (fmodf(T, 4) < 2);
  if (g.st == SAD || tipped()) hdy = 1;
  if (g.st == REFUSE && g.t < 0.9f) hdx = sinf(g.t * 22) > 0 ? 1 : -1;
  if (g.st == CHUCKLE) y0 -= int(g.t / 0.16f) & 1;
  if (g.st == WAKE && g.t < 0.3f) y0 -= 2;
  int solid = g.st == TRANSFORM && g.t > 1.1f && g.t < 1.6f ? LIGHT : -1;  // the flash: a white silhouette
  float wind = pet::wx == pet::WIND ? 1 + gust : pet::wx == pet::STORM ? 1 : 0;

  float hx, hy, tilt;
  bool rest;
  staffPose(hx, hy, tilt, rest);
  float top = hy - (HAND_R + 1 - SHAFT_TOP[w]), foot = hy + (FIG_H - HAND_R - 1);
  tipX = hx + tilt, tipY = top - (w ? 4 : 3);

  ellipse(g.x + 1, GROUND, 12, 1.6f, SHADOW);
  blit(BODY[w], FIG_H, 0, x0, y0, solid, wind, 32, hdx, hdy, HEAD_R);
  if (face != F_OPEN && solid < 0) blit(FACE[face], 7, -7, x0 + hdx, y0 + 15 + hdy);
  if (g.st == DRINK && g.t > 2.3f) fillIf(x0 + 10, y0 + 19, x0 + 13, y0 + 19, CREAM);  // a foam moustache
  if (g.st == SLEEP) zzz(x0 + 18, y0 + 6);

  if (rest) return blit(ARM[w], FIG_H, 0, x0, y0, solid);
  if (g.st == DRINK || g.st == REFUSE || g.st == HUNGRY) {  // the staff stands by itself, his hand is busy
    capsule(hx, top, hx, foot, 1.5f, INK), capsule(hx, top, hx, foot, 0.6f, STAFF);
    if (w) blit(CAGE, 7, 3, lroundf(hx) - 1, lroundf(top) - 7);
    else blit(CROOK, 4, 3, lroundf(hx) - 1, lroundf(top) - 4);
    float bx = x0 + 21, by = y0 + 29;  // hungry: the empty tankard at his chest
    if (g.st == DRINK) {  // up to his mouth, a sip, down again
      float k = clampf(fminf(g.t, g.dur - g.t - 0.2f) / 0.5f, 0, 1);
      bx = x0 + 22 - 6 * k, by = y0 + 30 - 8 * k + (g.t > 0.5f && g.t < 2.3f ? int(g.t / 0.35f) & 1 : 0);
    }
    if (g.st == REFUSE) bx = x0 + 25 + 1.5f * sinf(g.t * 20), by = y0 + 23;  // palm out, waving it off
    if (tipped()) bx += sinf(g.t * 30) > 0 ? 1 : 0;
    sleeve(x0 + 18, y0 + 21, bx, by + 1, -1);
    if (g.st != REFUSE) mug(lroundf(bx) - 6, lroundf(by) - 4 + (tipped() ? 1 : 0), tipped(), g.st == HUNGRY, -1);
    return blit(HAND, 4, 2, lroundf(bx), lroundf(by) - 2);
  }
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

float canopy(float x) {  // the hat brim, or his head (riding: higher up, a little back)
  float x0 = rove ? lroundf(g.x - 4) - 1 - AX : lroundf(g.x) - AX, y0 = rove ? RG - 16 - 38 : GROUND - FIG_H;
  return white > 0.5f ? (fabsf(x - x0 - 9.5f) < 8 ? y0 + 9 : 1e9f) : (fabsf(x - x0 - 11.5f) < 11 ? y0 + 12 : 1e9f);
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
float cloudRun = 0;  // px the clouds have drifted (faster on a ride)

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
    float x = fmodf(c[0] + cloudRun * c[3], 110);
    if (x < 0) x += 110;
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

// ---- Ride (the music screen; now and then a ride of his own): Gandalf on Shadowfax across Rohan, side on. The far
// peaks, the downs and the road slide by at their depth (parallax), dust flies from the hooves. ----
const float RIDE_V = 55;     // px/s the road goes by
bool tunes = false;          // music screen on (music())
float tripLeft = 0, dist = 0, hopY = 0, hopV = 0, cheerT = 0;  // s of ride left (0 = while the music plays); hop
float vn(float u) {  // smooth value noise, 0..1
  int i = floorf(u);
  float f = u - i;
  f = f * f * (3 - 2 * f);
  return h01(i) * (1 - f) + h01(i + 1) * f;
}
float peakTop(float x) { float u = (x + dist * 0.06f) / 11; return 22 + 15 * vn(u) + 3 * vn(u * 3.3f + 7); }
float downTop(float x) { return 45 + 5 * vn((x + dist * 0.3f) / 13); }
void travel(float secs) {
  rove = true, tripLeft = secs, hopY = hopV = 0;
  pose(IDLE, 0);
  tint(clockMin);
}
void endTrip() {
  rove = false;
  pose(IDLE, 0);
  tint(clockMin);
}
float stride() { return fmodf(dist / 26, 1); }  // gallop phase
void roveStep(float dt) {
  float p0 = stride();
  dist += RIDE_V * dt;
  hopV += 170 * dt, hopY = fminf(0, hopY + hopV * dt);
  if (hopY == 0) hopV = 0;
  cheerT = fmaxf(0, cheerT - dt);
  if (stride() < p0 && hopY == 0)  // a stride: a puff of dust behind
    for (int i = 0, k = 0; i < 40 && k < 4; i++)
      if (!sparks[i].on) sparks[i] = {g.x - 14 + 3 * rnd(), RG - 1.f, -30 - 20 * rnd(), -10 - 14 * rnd(), 0.6f, ACC2, true}, k++;
  if (!tunes && tripLeft > 0 && (tripLeft -= dt) <= 0) endTrip();
}

void outlined(float x0, float y0, float r0, float x1, float y1, float r1, uint8_t c) {
  cone(x0, y0, r0 + 0.8f, x1, y1, r1 + 0.8f, INK), cone(x0, y0, r0, x1, y1, r1, c);
}
// A leg from the hip (hx, hy), phase q: the hoof swings forward lifted, comes back on the ground. Knees fold forward
// on the forelegs, hocks back on the hind ones.
void leg(float hx, float hy, float gy, float q, bool fore, uint8_t c) {
  float s = q * 6.2832f, fx = hx + 6.5f * cosf(s), fy = gy - fmaxf(0, -sinf(s)) * 5 - hopY * 0.15f;
  float dx = fx - hx, dy = fy - hy, d = fmaxf(hypotf(dx, dy), 1), h = sqrtf(fmaxf(0, 42.25f - d * d / 4)) * (fore ? -1 : 1);
  float kx = hx + dx / 2 - dy / d * h, ky = hy + dy / 2 + dx / d * h;
  outlined(hx, hy, 2.2f, kx, ky, 1.3f, c), outlined(kx, ky, 1.2f, fx, fy, 0.9f, c);
  disc(fx, fy, 1.3f, INK);
}
void drawHorse(float bx, float by, float gy) {
  float q = stride();
  leg(bx - 9, by + 3, gy, q + 0.1f, false, HAIR3), leg(bx + 9, by + 3, gy, q + 0.6f, true, HAIR3);  // far side
  for (int i = 0; i < 5; i++) {  // the tail streaming back
    float x0 = bx - 12 - 3 * i, y0 = by - 3 + 1.2f * i + sinf(T * 11 + i) * 0.6f * i;
    outlined(x0, y0, 1.8f - 0.2f * i, x0 - 3, y0 + 1.2f + sinf(T * 11 + i + 1) * 0.6f, 1.6f - 0.2f * i, HAIR2);
  }
  ellipse(bx, by, 14, 7.3f, INK);
  for (int y = floorf(by - 6.5f); y <= by + 6.5f; y++)
    for (int x = floorf(bx - 13); x <= bx + 13; x++) {
      float u = (x + 0.5f - bx) / 13, v = (y + 0.5f - by) / 6.5f;
      if (u * u + v * v < 1) px(x, y, v > 0.45f ? HAIR2 : HAIR1);
    }
  outlined(bx + 10, by - 1, 5.2f, bx + 15, by - 9, 3.3f, HAIR1);    // chest and neck
  outlined(bx + 15.5f, by - 11, 3.3f, bx + 22, by - 7, 2, HAIR1);  // head, the muzzle down
  px(bx + 17, by - 12, INK), px(bx + 22.5f, by - 7, INK), px(bx + 21, by - 5.5f, HAIR2);  // eye, nostril, jaw
  px(bx + 14, by - 15, INK), px(bx + 14, by - 14, HAIR2), px(bx + 15, by - 15, INK), px(bx + 15, by - 14, HAIR2);  // ears
  for (int k = 0; k < 5; k++) {  // the mane flying
    float mx = bx + 14.5f - 1.6f * k, my = by - 13.5f + 2.2f * k;
    capsule(mx, my, mx - 4.5f - sinf(T * 13 + k), my + 0.5f + 0.7f * sinf(T * 9 + k), 0.7f, HAIR2);
  }
  leg(bx - 8, by + 3, gy, q, false, HAIR1), leg(bx + 10, by + 3, gy, q + 0.5f, true, HAIR1);  // near side
}
void notes(float x, float y) {  // music notes rising over him, blown back
  static const char *const NOTE[2][5] = {{"..##", "..#.", "..#.", "###.", "##.."}, {"####", "#..#", "#..#", "#.##", "#.##"}};
  for (int k = 0; k < 3; k++) {
    float p = fmodf(T * 0.35f + k / 3.f, 1);
    int nx = lroundf(x - 24 * p + 3 * sinf(T * 2 + k * 2)), ny = lroundf(y - 12 * p);
    if (p < 0.85f)
      for (int r = 0; r < 5; r++)
        for (int c = 0; c < 4; c++)
          if (NOTE[k % 2][r][c] == '#') px(nx + c, ny + r, nightW > 0.5f ? CREAM : INK);
  }
}
void drawRide() {
  int h0 = horizon;
  horizon = RH;
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) fb[y * N + x] = y < RH ? skyAt(x, y) : LAND;
  drawSky();
  for (int x = 0; x < N; x++) {
    float pt = peakTop(x), dt = downTop(x), sl = peakTop(x + 1) - pt;
    for (int y = floorf(pt); y < dt && y < RH + 6; y++) px(x, y, y < pt + 2 && pt < 30 ? WATER : sl > 0.15f ? FAR2 : FAR);
    for (int y = floorf(dt); y < 57; y++) px(x, y, y < dt + 1 ? LANDL : y > dt + 5 && bayer(x, y) > 0.4f ? MID2 : MID);
  }
  for (int k = 0; k < 6; k++) {  // trees on the downs
    float slot = floorf(dist * 0.3f / 17) + k, tx = slot * 17 - dist * 0.3f;
    if (h01(int(slot) * 7 + 3) > 0.5f) tree(tx + 6 * h01(int(slot)), fminf(downTop(tx) + 6, 56), 3.2f, ACC5, WATER2, LANDL, ACC6, 1.5f);
  }
  for (int y = 57; y < N; y++) {  // the grass, the road; their texture moves faster the nearer it is
    float sp = 0.45f + 0.95f * (y - 57) / 23.f;
    for (int x = 0; x < N; x++) {
      int u = int(x + dist * sp);
      bool road = y >= 67 && y <= 78;
      uint8_t c = road ? ((y == 70 || y == 75) && (u / 3) % 3 == 0 ? ACC2 : ACC1) : y == 57 ? LANDL : LAND;
      if (!road && h01(u * 31 + y * 977) < 0.1f) c = LAND2;
      if (y == 66 || y == 79) c = LAND2;
      px(x, y, c);
    }
  }
  for (int k = 0; k < 4; k++) {  // stones on the verge
    float slot = floorf(dist / 31) + k, sx = slot * 31 - dist;
    if (h01(int(slot) * 5 + 1) > 0.5f) {
      int y = 64;
      fillIf(sx, y - 1, sx + 3, y, ACC3), fillIf(sx + 1, y, sx + 3, y, ACC4), px(sx, y - 2, INK);
    }
  }
  horizon = h0;
  drawFlash();
  drawDrops();

  float bob = sinf(stride() * 12.566f) * 1.2f, bx = g.x - 4, gy = RG + hopY, by = gy - 16 + bob;
  int w = white > 0.5f, x0 = lroundf(bx) - 1 - AX, y0 = lroundf(by) - 38;
  for (int i = 0; i < 4; i++) {  // his mantle flying behind
    float cx = x0 + 6 - 4.5f * i, cy = y0 + 22 + 2.4f * i + sinf(T * 12 + i) * 0.5f * i;
    outlined(cx, cy, 2.6f - 0.4f * i, cx - 4.5f, cy + 2.4f + sinf(T * 12 + i + 1) * 0.5f, 2.2f - 0.4f * i, w ? MANT : ROBE2);
  }
  blit(BODY[w], 33, 0, x0, y0);
  drawHorse(bx, by, gy);
  outlined(bx + 2, by - 5, 2, bx + 3, by + 3, 1.5f, w ? ROBE : ROBE2);  // his leg down the flank, the boot
  disc(bx + 3.5f, by + 4.5f, 1.7f, INK), disc(bx + 3.5f, by + 4.3f, 1, BOOT);
  // the staff held up ahead of him, the hand on it
  float hx = x0 + 24, hy = y0 + 25 + bob * 0.5f, tx = hx + 4, ty = hy - 17;
  sleeve(x0 + 18, y0 + 21, hx - 1, hy + 1, -1);
  capsule(hx - 5, hy + 11, tx, ty, 1.5f, INK), capsule(hx - 5, hy + 11, tx, ty, 0.6f, STAFF);
  if (w) blit(CAGE, 7, 3, lroundf(tx) - 1, lroundf(ty) - 7);
  else blit(CROOK, 4, 3, lroundf(tx) - 1, lroundf(ty) - 4);
  blit(HAND, 4, 2, lroundf(hx) - 1, lroundf(hy) - 2);
  tipX = tx, tipY = ty - (w ? 4 : 3), glow = cheerT > 0 ? fminf(1, cheerT * 2) : 0;
  drawEffects();
  if (tunes) notes(x0 + 4, y0 + 2);
  drawLeaves();
}

// ---- Logic ----
bool wantWhite() { return pet::wx == pet::CLEAR; }

// Device shaken: "You shall not pass!". Asleep: a start, wide-eyed, then he dozes off again. Riding: Shadowfax leaps.
void shake() {
  if (game.on) return;
  if (rove) return void(hopV = -48);
  if (g.st == SLEEP) return pose(WAKE, 1.2f);
  pose(SLAM, 1.6f);
}
// Message/button: the staff up, its tip alight. Asleep: one eye opens for 2 s. Riding: a hop, the staff lights.
void cheer() {
  if (game.on) return;
  if (g.st == SLEEP) return void(g.peek = 2);
  if (rove) return void((hopV = -30, cheerT = 1.2f));
  if (g.st == IDLE || g.st == PIPING || g.st == SAD || g.st == HUNGRY) pose(RAISE, 1.6f);
}
void smoke() { pose(PIPING, 4.2f); }
void firework() { pose(FIREWORK, 3.2f); }
// Pet events (nothing during the game; a ride ends): fed (an ale), fed while full, petted, too tired to play.
void petAct(int st, float dur) {
  if (game.on) return;
  if (rove) rove = false, tint(clockMin);
  pose(st, dur);
}
void eat() { petAct(DRINK, 3); }
void refuse() { petAct(REFUSE, 1.2f); }
void purr() { petAct(CHUCKLE, 2); }
void yawn() { petAct(YAWN, 1.6f); }
// Music screen on / off (the firmware calls this every frame): he rides; asleep, he sleeps on.
void music(bool on) {
  if (game.on) on = false;
  if (on == tunes) return;
  tunes = on;
  if (on && g.st != SLEEP && mood != pet::SLEEP) travel(0);
  else if (!on && rove && tripLeft <= 0) endTrip();
}

void particles(float dt) {  // smoke rings, the rocket, sparks (and the ride's dust), the slam's ring
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

void update(float dt) {
  T += dt, g.t += dt;
  if ((g.blinkT -= dt) < 0) g.blinkT = 2.5f + 3 * rnd();
  startle = fmaxf(0, startle - dt), g.peek = fmaxf(0, g.peek - dt), yawnT -= dt;
  cloudRun += dt * ((pet::wx == pet::WIND ? 3 : pet::wx == pet::STORM ? 2 : 1) - (rove ? 5 : 0));
  glow = 0;
  if (rove) return roveStep(dt), particles(dt);
  uint8_t want = biomeFor(pet::wx);
  if (g.st == SLEEP && (want != biome || (white > 0.5f) != wantWhite())) {  // asleep: no flash, the land just changes
    white = wantWhite(), biome = want, buildBg(), tint(clockMin);
  } else if (g.st != TRANSFORM && (want != biome || (white > 0.5f) != wantWhite())) {
    if ((white > 0.5f) != wantWhite()) pose(TRANSFORM, 2.8f);
    else setBiome(want);
  }
  switch (g.st) {
    case IDLE:
      if (startle > 0) glow = startle;
      if (mood == pet::SLEEP) pose(SLEEP, 0);
      else if (mood == pet::HUNGRY) pose(HUNGRY, 0);
      else if (mood == pet::SAD) pose(SAD, 0);
      else if (mood == pet::TIRED && yawnT <= 0) yawnT = 30, pose(YAWN, 1.6f);
      else if (mood == pet::HAPPY && (tripT -= dt) < 0) tripT = 200 + 100 * rnd(), travel(12 + 8 * rnd());
      else if ((trickT -= dt * (mood == pet::TIRED ? 0.5f : 1)) < 0) {  // tired: tricks half as often
        trickT = 10 + 10 * rnd();
        if (biome == SHIRE && rnd() < 0.4f) firework();
        else smoke();
      }
      break;
    case SLEEP:
      if (startle > 0) g.peek = fmaxf(g.peek, startle);  // thunder: an eye opens
      if (mood != pet::SLEEP) pose(YAWN, 1.6f);
      break;
    case HUNGRY:
      if (mood != pet::HUNGRY) pose(IDLE, 0);
      else if (fmodf(g.t, 3) > 2 && fmodf(g.t - dt, 3) <= 2)  // the last drop out of the tankard
        for (auto &s : sparks)
          if (!s.on) { s = {lroundf(g.x) - AX + 18.f, GROUND - FIG_H + 33.f, 0, 10, 0.8f, DROP, true}; break; }
      break;
    case SAD:
      if (mood != pet::SAD) pose(IDLE, 0);
      else if (fmodf(g.t, 6) > 4 && fmodf(g.t - dt, 6) <= 4) puff(lroundf(g.x) - AX + 15, GROUND - FIG_H + 21);  // a sigh
      break;
    case WAKE: glow = clampf(1 - g.t, 0, 1); break;
    case CHUCKLE: glow = 0.35f; break;
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
  if (g.st != IDLE && g.dur > 0 && g.t >= g.dur) pose(IDLE, 0);
  particles(dt);
}

void init() {
  g = {}, trickT = 8, yawnT = 0, waveAge = -1, rocket.on = false;
  rove = false, hopY = hopV = cheerT = 0;
  for (auto &p : puffs) p.on = false;
  for (auto &s : sparks) s.on = false;
  for (int i = 0; i < NS; i++) sx[i] = 10 + rnd() * 60, sy[i] = 3 + rnd() * 30, sp[i] = rnd() * 6.28f;
  white = wantWhite();
  setBiome(biomeFor(pet::wx));
}

// ---- Game: the Bridge of Khazad-dûm. Seen over Gandalf's shoulder, the Balrog at the far end. Tilt picks his spot
// (left, middle, right). The Balrog marks where he'll strike, one, two or all three lanes: a red ring closes on each
// while he raises his arm and tenses, never at the same pace (fill()), and bursts when it's closed. Then he holds the
// blow for atk.win s, shorter as it gets harder: in a marked lane, a press then (or EARLY s before) blocks (Glamdring
// against the whip, a dome of light against fire). A press only guards for GUARD_S s, and one earlier while he winds up
// is a whiff: no block this blow, so mashing doesn't work. A clean block gives a free strike, tap within counterW s.
// In an unmarked lane his blow misses; a press there in the window strikes him. Between blows he now and then shields
// behind his wings with one side open (a cyan mark): tilt there and tap within duel.win s. HP strikes and he falls (level up) and climbs back, faster. A blow that lands cracks
// the bridge; the third crack breaks it. A streak of STREAK clean blocks or strikes turns him White: points count
// double until he's hit. ----
const float LY0 = 38, LY1 = 57, GY = 74, MY = 46;  // the Balrog's hands, the guard, Gandalf's feet; the marks' row
const float TILT_IN = 0.17f, TILT_OUT = 0.09f;  // as Pavi's: g sideways past IN = side spot, back under OUT = middle
const int HP = 5, STREAK = 6, CRACKS = 3;
const float BURST = 0.3f;  // how long a blow shows
const float GUARD_S = 0.4f, EARLY = 0.1f;  // a press guards this long; this early before the ring closes still counts
float laneX(float l, float y) { return 40 + l * (11 + 4 * (y - LY0) / (LY1 - LY0)); }  // ±11 at his hands, ±15 at Gandalf
float guardX(int l) { return 40 + l * 12.f; }  // the Balrog's sides

enum { FIGHT, GUARD, FALL };  // the Balrog attacks, shields (strike the open side), falls
enum { READY, BLOCK_SWORD, BLOCK_STAFF, STRIKE_SWORD, STRIKE_STAFF, SLAM_DOWN, HURT };
struct {
  bool on, fire;   // fire from the left hand, else the whip in the right
  bool whiff;      // pressed too early: no block this blow
  uint8_t mask;    // bit l + 1: lane l
  uint8_t shape;   // how the ring closes (fill())
  float t, dur;    // winding up: the ring is closed at t = dur, then he holds the blow for win s
  float win;
  float landT;     // when the last one landed
} atk;
struct {
  int phase, hp, cracks, streak, level, sinceOpen;
  int8_t lane, open;
  uint8_t act;  // Gandalf's pose after a block, a strike or a hit (since actT)
  bool tap;     // a press this frame
  float t, nextT, gx, guardT, actT, hitT, whiteT, fallY, fallV, win, counterT, counterW, strikeX;
} duel;
float hard() { return clampf(game.t / 180 + duel.level * 0.12f, 0, 1); }  // 0 -> 1 over 3 min of play, sooner per level
bool hits(uint8_t mask, int l) { return mask >> (l + 1) & 1; }
bool counter() { return duel.phase == FIGHT && T - duel.counterT < duel.counterW; }
bool guarding() { return T - duel.guardT < GUARD_S; }
bool windowOpen() { return atk.on && atk.t >= atk.dur; }  // the ring has closed: he holds the blow, press now
float fill() {  // how far the ring has closed, 0..1, a different pace each blow so it can't be timed by rote
  float u = clampf(atk.t / atk.dur, 0, 1);
  switch (atk.shape) {
    case 1: return u * u * u;                                                // slow, then all at once
    case 2: return 1 - (1 - u) * (1 - u) * (1 - u);                          // fast, then it hangs
    case 3: return u < 0.85f ? 0.6f * fminf(u / 0.4f, 1) : 0.6f + 0.4f * (u - 0.85f) / 0.15f;  // stalls, then snaps
    default: return u;
  }
}

void finish() {  // stats and the event for the firmware
  game.newBest = pet::gameOver(game.score);
  game.ended = true;
}
int mult() { return white > 0.5f ? 2 : 1; }
void act(uint8_t a) { duel.act = a, duel.actT = T; }
void clash(float x, float y, uint8_t c, uint8_t c2, int n, float v) {  // a burst from the sparks pool
  for (auto &s : sparks)
    if (!s.on && n-- > 0) {
      float a = 6.2832f * rnd();
      s = {x, y, v * (0.4f + rnd()) * cosf(a), v * (0.4f + rnd()) * sinf(a) - v * 0.3f, 0, uint8_t(n & 1 ? c : c2), true};
    }
}
void good() {  // a clean block or strike: the streak, and at STREAK the White
  if (++duel.streak >= STREAK && white < 0.5f) white = 1, duel.whiteT = T, tint(clockMin);
}

void next(float gap) { duel.phase = FIGHT, duel.nextT = gap; }
void attack() {  // one, two or three lanes, harder ones later
  float k = hard(), r = rnd();
  int n = r < 0.55f - 0.25f * k ? 1 : r < 0.88f - 0.13f * k ? 2 : 3, l = int(rnd() * 3);
  atk.on = true, atk.whiff = false, atk.fire = rnd() < 0.5f, atk.mask = n == 3 ? 7 : n == 1 ? 1 << l : 7 & ~(1 << l);
  atk.t = 0, atk.dur = (1.5f - 0.6f * k) * (0.6f + 0.8f * rnd()), atk.shape = int(rnd() * 4), atk.win = 0.45f - 0.25f * k;
  duel.sinceOpen++, duel.nextT = 0.5f - 0.25f * k + 0.5f * rnd();  // the gap after it lands
}
void opening() {
  duel.phase = GUARD, duel.t = 0, duel.open = int(rnd() * 3) - 1, duel.win = 1.4f - 0.9f * hard(), duel.sinceOpen = 0;
  game.buzz = 1;
}
void strike(bool hit) {  // Gandalf strikes at his side of the Balrog; HP down, at 0 he falls
  duel.strikeX = guardX(duel.lane), act(rnd() < 0.5f ? STRIKE_SWORD : STRIKE_STAFF);
  if (!hit) return;
  game.score += 3 * mult(), duel.hitT = T, good();
  clash(duel.strikeX, MY, LIGHT, ACC3, 14, 34);
  if (--duel.hp > 0) return;
  duel.phase = FALL, duel.t = 0, duel.level++, duel.hp = HP, game.score += 5 * mult(), duel.counterT = -9;
  atk.on = false, act(SLAM_DOWN);  // "You shall not pass": he slams the bridge, the Balrog falls
}

void hurt() {
  duel.cracks++, duel.streak = 0, act(HURT), game.buzz = 2;
  if (white > 0.5f) white = 0, tint(clockMin);
  if (duel.cracks < CRACKS) return;
  game.dead = true, game.t = 0, duel.fallV = -20;  // the bridge breaks under him
  for (int i = 0; i < 14; i++) clash(duel.gx - 10 + 20 * rnd(), GY + 2, LAND, LAND2, 1, 20);
  finish();
}

void land(bool guard) {
  atk.on = false, atk.landT = T;
  for (int l = -1; l <= 1; l++)
    if (hits(atk.mask, l) && l != duel.lane) clash(laneX(l, LY1), LY1 + 4, FIRE, FIRE2, 6, 22);
  if (!hits(atk.mask, duel.lane)) return guard ? strike(true) : void();  // missed him; a press as he swings strikes
  float x = laneX(duel.lane, LY1);
  if (guard) {
    game.score += mult(), act(atk.fire ? BLOCK_STAFF : BLOCK_SWORD), good(), duel.counterT = T, duel.counterW = 0.9f - 0.45f * hard();
    clash(x, LY1 - 4, atk.fire ? FIRE2 : LIGHT, atk.fire ? LIGHT : LIGHT2, 10, 30);
  } else clash(x, LY1 + 4, FIRE, FIRE2, 12, 26), hurt();
}

void gameStart() {
  game = pet::Game();
  game.on = true, rove = false;
  duel = {}, atk = {};
  duel.hp = HP, duel.gx = laneX(0, LY1), duel.guardT = duel.actT = duel.hitT = duel.whiteT = duel.counterT = atk.landT = -9;
  for (auto &s : sparks) s.on = false;
  white = 0;  // he earns the White in the fight
  next(0.9f);
  for (int y = 0; y < N; y++)  // the haze: brightest behind the Balrog, dark at the edges and down the chasm
    for (int x = 0; x < N; x++) {
      float v = 3.4f - hypotf((x - 40) * 0.75f, y - 28.f) / 12 - fmaxf(0, y - 50) * 0.15f;
      v = clampf(v, 0, 3);
      fb[y * N + x] = int(v) + (v - int(v) > bayer(x, y) && v < 3);
    }
  for (int x = 0; x < N; x++) {  // basalt columns on both sides of the chasm, lower toward the bridge
    int c = x < 40 ? x / 4 : (79 - x) / 4, f = x < 40 ? x % 4 : 3 - (79 - x) % 4;
    if (c > 3) continue;
    int top = 44 + c * 5 + 5 * h01(c + (x < 40) * 9);
    for (int y = top; y < N; y++)
      px(x, y, y < top + 2 ? MID : f == 3 ? FAR2 : (x < 40 ? f == 2 : f == 0) ? MID2 : (y - top) % 7 == 6 ? FAR2 : FAR);
  }
  for (int y = 50; y < N; y++) {  // the bridge: pale cracked flagstones, crumbling edges, no rail
    float hw = 7.5f + (y - 50) * 0.85f + 1.4f * (h01(y * 3) - 0.5f);
    int r = sqrtf((y - 50) * 7.f), r0 = sqrtf((y - 51) * 7.f);
    for (int x = 0; x < N; x++) {
      float d = fabsf(x + 0.5f - 40), u = (x + 0.5f - 40) / (hw + 1) * 2.5f + r * 0.45f;
      if (d >= hw) continue;
      bool joint = y > 50 && r != r0, seam = u - floorf(u) < 0.12f * (1 + 6 / (y - 44.f));
      px(x, y, y < 52 ? LANDL : d > hw - 1 ? LAND2 : joint || seam ? LAND2 : LAND);
    }
  }
  for (int i = 0; i < 6; i++) {  // old cracks in the stone
    float x = 40 + (h01(i * 5) - 0.5f) * (12 + i * 4), y = 56 + i * 4;
    for (int k = 0; k < 4; k++) {
      float nx = x + 2.5f * (h01(i * 9 + k) - 0.5f) * 2, ny = y + 1 + h01(i * 7 + k);
      capsule(x, y, nx, ny, 0.3f, SHADOW), x = nx, y = ny;
    }
  }
  memcpy(bg, fb, sizeof bg);
  tint(clockMin);
}

void gameStop() {  // left with a long hold, or by find; back to the land
  if (game.on && !game.dead && game.score > 0) finish();
  game.on = false;
  init();
}

void gameInput(bool held) {
  if (held && !game.held) duel.tap = true;
  game.held = held;
}

void gameTilt(float g) {  // sideways gravity in g, + = tilted right; hysteresis so a wobble doesn't flip the spot
  int8_t &l = duel.lane;
  if (game.on && !game.dead) l = g > (l == 1 ? TILT_OUT : TILT_IN) ? 1 : g < -(l == -1 ? TILT_OUT : TILT_IN) ? -1 : 0;
}

void gameStep(float dt) {
  T += dt, game.t += dt, duel.t += dt;
  duel.gx += (laneX(duel.lane, LY1) - duel.gx) * fminf(1, 16 * dt);
  for (auto &s : sparks)
    if (s.on) {
      s.age += dt, s.vy += G * dt, s.x += s.vx * dt, s.y += s.vy * dt;
      if (s.age > 1) s.on = false;
    }
  bool tap = duel.tap;
  duel.tap = false;
  if (game.dead) {  // he falls with the bridge; the firmware restarts on a press, gives up after 10 s
    duel.fallV += 2 * G * dt, duel.fallY += duel.fallV * dt;
    if (game.t > 10) gameStop();
    return;
  }
  if (atk.on) {
    atk.t += dt;
    if (tap) duel.guardT = T, atk.whiff |= atk.t < atk.dur - EARLY;
    tap = false;
    if (windowOpen() && guarding() && !atk.whiff) land(true);
    else if (atk.t >= atk.dur + atk.win) land(false);
  }
  if (game.dead) return;
  if (duel.phase == FIGHT) {
    if (tap && counter()) duel.counterT = -9, strike(true);
    else if (!atk.on && !counter() && (duel.nextT -= dt) <= 0) {
      if (duel.sinceOpen >= 4 || (duel.sinceOpen >= 2 && rnd() < 0.35f)) opening();
      else attack();
    }
  } else if (duel.phase == GUARD) {
    if (tap) {
      strike(duel.lane == duel.open);
      if (duel.phase == GUARD) next(0.6f);
    } else if (duel.t > duel.win) next(0.4f);
  } else if (duel.t > 3.4f) next(0.9f);  // FALL: down, and back up the far end
}

int solidC = -1;  // a figure all in one color (a hit, the White's flash)
uint8_t sc(uint8_t c) { return solidC >= 0 ? solidC : c; }

// Balrog: a demon of shadow and flame on the bridge's far end, bat wings spread; by lowers him (staggers, the fall).
void tri(float x0, float y0, float x1, float y1, float x2, float y2, uint8_t c) {
  for (int y = floorf(fminf(y0, fminf(y1, y2))); y <= fmaxf(y0, fmaxf(y1, y2)); y++)
    for (int x = floorf(fminf(x0, fminf(x1, x2))); x <= fmaxf(x0, fmaxf(x1, x2)); x++) {
      float qx = x + 0.5f, qy = y + 0.5f;
      float a = (x1 - x0) * (qy - y0) - (y1 - y0) * (qx - x0), b = (x2 - x1) * (qy - y1) - (y2 - y1) * (qx - x1),
            d = (x0 - x2) * (qy - y2) - (y0 - y2) * (qx - x2);
      if ((a >= 0 && b >= 0 && d >= 0) || (a <= 0 && b <= 0 && d <= 0)) px(x, y, c);
    }
}
void limb(float x0, float y0, float x1, float y1, float r0, float r1, uint8_t c) {
  cone(x0, y0, r0 + 0.9f, x1, y1, r1 + 0.9f, sc(INK)), cone(x0, y0, r0, x1, y1, r1, sc(c));
}
// Bat-wing membrane: finger bones from the joint (jx, jy) to the tips, the skin between them sagging back toward it.
void membrane(float jx, float jy, const float (*tip)[2], int n) {
  for (int i = 0; i + 1 < n; i++)
    for (int j = 0; j < 4; j++) {
      float u0 = j / 4.f, u1 = (j + 1) / 4.f, s0 = 0.28f * sinf(3.1416f * u0), s1 = 0.28f * sinf(3.1416f * u1);
      float ax = tip[i][0] + (tip[i + 1][0] - tip[i][0]) * u0, ay = tip[i][1] + (tip[i + 1][1] - tip[i][1]) * u0;
      float bx = tip[i][0] + (tip[i + 1][0] - tip[i][0]) * u1, by = tip[i][1] + (tip[i + 1][1] - tip[i][1]) * u1;
      tri(jx, jy, ax + (jx - ax) * s0, ay + (jy - ay) * s0, bx + (jx - bx) * s1, by + (jy - by) * s1, sc(ACC5));
    }
  for (int i = 0; i < n; i++) capsule(jx, jy, tip[i][0], tip[i][1], 0.5f, sc(ACC6)), px(tip[i][0], tip[i][1], sc(INK));
}
// His hands: the whip in the right (+1), fire in the left (-1). At rest the right fist is up with the whip, the left low.
// Winding up, the hand rises over his horns and draws back trembling; as he strikes it's flung down, then comes back.
void handAt(int side, float by, float &hx, float &hy) {
  hx = side > 0 ? 61 : 21, hy = (side > 0 ? 39 : 58) + by;
  if (side != (atk.fire ? -1 : 1)) return;
  float since = T - atk.landT;
  if (atk.on) {
    float k = clampf(atk.t / 0.35f, 0, 1), p = fill();
    k = k * k * (3 - 2 * k);
    hx += (40 + side * 17 - hx) * k, hy += (28 + by - hy) * k;
    if (p > 0.55f) hx += side * (p - 0.55f) * 6 + 0.6f * sinf(T * 70), hy -= (p - 0.55f) * 6;  // tensing
  } else if (since < 0.5f) {
    float k = clampf((since - 0.15f) / 0.35f, 0, 1), fx = 40 + side * 6, fy = 48 + by;
    hx = fx + (hx - fx) * k, hy = fy + (hy - fy) * k;
  }
}
// A lash of fire through the points of f(u), u 0..1: a glowing rope with a white-hot core.
template <class F> void lash(F f, int n, float r0, float r1) {
  float x0, y0, x1, y1;
  f(0, x0, y0);
  for (int s = 1; s <= n; s++, x0 = x1, y0 = y1) {
    float u = float(s) / n;
    f(u, x1, y1);
    capsule(x0, y0, x1, y1, r0 + (r1 - r0) * u, FIRE);
    if (s % 3) capsule(x0, y0, x1, y1, (r0 + (r1 - r0) * u) * 0.4f, MOON);
  }
}
// His head, front on: the horns rise from the brow in two arches and sweep down past a narrow face, the fire behind
// showing through between them; a scowl, the maw white-hot. Column 10 on his middle.
const char *const HEAD[16] = {
    "....KoooK...KoooK....", "...KoDDDoK.KoDDDoK...", "..KoDDDDDoKoDDDDDoK..", ".KoDDDDDDDKDDDDDDDoK.",
    "KoDDDDDDDDoDDDDDDDDoK", "KDDDKKKKKdddKKKKKDDDK", "KDDK.KdeEKdKEedK.KDDK", "KDDK..KdddddDDK..KDDK",
    "KDDK..KdddFdDDK..KDDK", "KDDDK.KdTYWYTDK.KDDDK", ".KDDDK.KdYWYDK.KDDDK.", "..KKDDKKdYWYDKKDDKK..",
    "....KK.KdYWYDK.KK....", "........KdFdK........", "........KdddK........", ".........KdK.........",
};
// His body: an inverted triangle of rock lit along its tops by the fire on his back, the domed shoulders higher than
// his neck, the waist melting into flame; the upper arms out to the elbows (rows 15, columns 3.5 and 36.5).
// Column 20 on his middle.
const char *const BALROG[20] = {
    ".......KooooKKKKKK.....KKKKKKooooK.......", "......KoddddooooooK...KooooooddddoK......",
    ".....KodddddddddddKKKKKdddddddddddoK.....", ".....KddddddddddddoooooddddddddddddK.....",
    "....KodddDddddddoooodooooddddddDdddoK....", "....KdddDdddddooKKKKDKKKKoodddddDdddK....",
    "...KoddDdddddKKKddddDddddKKKdddddDddoK...", "...KddddddddddddddddDddddddddddddddddK...",
    "...KddddddddKdDdddddDdddddDdKddddddddK...", "..KoddDdddKK.KdDDDDdddDDDDdK.KKdddDddoK..",
    "..KddDdddK...KddodddodddoddK...KdddDddK..", ".KoddodddK....KddoddEddoddK....KdddoddoK.",
    ".KddodddK.....KdDDDDoDDDDdK.....KdddoddK.", ".KddddddK.....KddddododdddK.....KddddddK.",
    "KodddddK.......KddodddoddK.......KdddddoK", "KddddddK.......KDDDDdDDDDK.......KddddddK",
    ".KddddK.........KddododdK.........KddddK.", "..KddK..........KdddddddK..........KddK..",
    "...KK...........KdddddddK...........KK...", ".................KdddddK.................",
};
// Sprite letters: K outline, D dark rock, d rock, o lit edge or lava, E/e eyes and the furnace, W/Y/F the maw's fire,
// T fangs.
void sprite(const char *const *rows, int h, int col, float cx, float top) {
  static const char K[] = "KDdoEeWYFT";
  const uint8_t C[] = {INK, ACC2, ACC1, ACC3, ACC4, ACC3, MOON, FIRE2, FIRE, LANDL};
  for (int r = 0; r < h; r++)
    for (int c = 0; rows[r][c]; c++)
      if (const char *k = strchr(K, rows[r][c])) px(lroundf(cx) - col + c, lroundf(top) + r, k - K < 3 ? sc(C[k - K]) : C[k - K]);
}
void drawBalrog(float by, bool guard) {
  const float HY = 25 + by, BY = HY + 9;  // the tops of his head and his body
  float flap = sinf(T * 1.6f) * 2.5f;
  for (int s = -1; s <= 1; s += 2) {  // wings: up from the shoulder to the wrist, fingers fanning down
    float ax = 40 + s * 9, ay = BY + 4, wx = 40 + s * 25, wy = 22 + flap + by;
    const float tip[5][2] = {{40 + s * 39.f, 15 + flap * 1.4f + by}, {40 + s * 41.f, 30 + flap * 0.8f + by},
                             {40 + s * 36.f, 44 + by}, {40 + s * 27.f, 52 + by}, {40 + s * 15.f, 54 + by}};
    tri(ax, ay, wx, wy, tip[4][0], tip[4][1], sc(ACC5));
    membrane(wx, wy, tip, 5);
    limb(ax, ay, wx, wy, 1.5f, 0.9f, ACC6);
    capsule(ax, ay - 1.5f, wx, wy - 1, 0.4f, ACC3), capsule(wx, wy - 1, tip[0][0], tip[0][1] - 0.5f, 0.3f, ACC3);  // fire-lit edge
    px(wx, wy - 2.5f, sc(INK)), px(wx + s, wy - 2.5f, sc(INK));  // the claw on the wrist
  }
  for (int y = HY - 6; y < BY + 6; y++)  // the blaze behind him, his head dark against it
    for (int x = 24; x <= 56; x++)
      if (powf((x + 0.5f - 40) / 15, 2) + powf((y + 0.5f - HY - 5) / 11, 2) < 1) px(x, y, (x + y) & 1 ? CLOUD : SKY3);
  for (int k = 0; k < 9; k++) {  // fire roaring up from the back of his head and his shoulders
    float c = (k - 4) / 2.f, fx = 40 + (k - 4) * (fabsf(c) > 1.2f ? 3.4f : 1.8f), fy = HY + 3 + fabsf(c) * 2;
    float h = 7 + 11 * expf(-c * c) + 3 * sinf(T * 9 + k * 1.7f), sw = sinf(T * 5 + k) * 1.4f;
    cone(fx, fy, 2, fx + sw, fy - h, 0.3f, k & 1 ? FIRE : ACC3);
    cone(fx, fy, 0.9f, fx + sw * 0.8f, fy - h * 0.6f, 0.2f, FIRE2);
  }
  sprite(BALROG, 20, 20, 40, BY);
  if (sinf(T * 4) > 0.3f) px(40, BY + 11, MOON);  // the furnace in his chest pulses
  for (int k = 0; k < 7; k++) {  // his waist melting into fire
    float fx = 34 + k * 2 + sinf(T * 3 + k), h = 4 + 3 * sinf(T * 7 + k * 2.1f), sw = sinf(T * 4 + k) * 1.2f;
    cone(fx, BY + 21, 1.8f, fx + sw, BY + 16 - h, 0.3f, k & 1 ? FIRE : ACC3), cone(fx, BY + 21, 0.8f, fx + sw, BY + 18 - h, 0.2f, FIRE2);
  }
  for (int s = -1; s <= 1; s += 2) {  // forearms from the sprite's elbows, big fists
    float hx, hy, ex = 40 + s * 16.5f, ey = BY + 15;
    handAt(s, by, hx, hy);
    limb(ex, ey, hx, hy, 2.6f, 2.2f, ACC1);
    capsule(ex, ey - 1.5f, hx, hy - 1.5f, 0.35f, ACC3);
    disc(hx, hy, 3.2f, sc(INK)), disc(hx, hy, 2.3f, sc(ACC1)), px(hx - 1, hy - 1, ACC3), px(hx + 1, hy - 1, ACC3);
    if (atk.on && atk.fire && s < 0) {  // a fireball growing in the hand while he winds up
      float p = fill();
      disc(hx, hy - 2, 1.5f + 2.5f * p + sinf(T * 20) * 0.5f, FIRE), disc(hx, hy - 2, 0.8f + 1.2f * p, MOON);
    }
  }
  sprite(HEAD, 16, 10, 40, HY);
  if (guard)
    for (int l = -1; l <= 1; l++) {  // shielding: the wings wrapped over his body, one side left open; the head stays up
      if (l == duel.open) continue;
      float cx = guardX(l);
      const float tip[3][2] = {{cx - 6.5f, 57 + by}, {cx, 60 + by}, {cx + 6.5f, 57 + by}};
      tri(cx - 6.5f, 46 + by, cx + 6.5f, 46 + by, cx, 41 + by, sc(ACC5)), fillIf(cx - 5.5f, 46 + by, cx + 5.5f, 50 + by, sc(ACC5));
      membrane(cx, 43 + by, tip, 3);
      capsule(cx - 6.5f, 46 + by, cx, 41 + by, 0.5f, ACC3), capsule(cx, 41 + by, cx + 6.5f, 46 + by, 0.5f, ACC3);
      px(cx, 39 + by, INK);
    }
  if (atk.fire || T - atk.landT > BURST) {  // but as it cracks: his whip hangs from the right fist into the chasm
    float hx, hy;
    handAt(1, by, hx, hy);
    lash([&](float u, float &x, float &y) {
      x = hx + (70 - hx) * u + 5 * sinf(u * 6.3f + T * 2) * u, y = hy + 2 + (80 - hy) * u;
    }, 14, 0.9f, 0.5f);
  }
}

// Gandalf from behind, at z of his scene size: hat (the Grey) or white hair, robe, the staff in his right hand,
// Glamdring in his left.
void swordAt(float x0, float y0, float x1, float y1) {  // hilt to tip
  float dx = x1 - x0, dy = y1 - y0, l = hypotf(dx, dy) + 1e-6f, cx = -dy / l * 1.7f, cy = dx / l * 1.7f;
  capsule(x0, y0, x1, y1, 1, sc(INK)), capsule(x0, y0, x1, y1, 0.4f, sc(BUCKLE));
  capsule(x0 + dx * 0.5f, y0 + dy * 0.5f, x0 + dx * 0.8f, y0 + dy * 0.8f, 0.3f, sc(LIGHT));  // the blade's glint
  capsule(x0 - cx, y0 - cy, x0 + cx, y0 + cy, 0.5f, sc(BELT));
}
void staffAt(float x0, float y0, float x1, float y1) {  // foot to head
  capsule(x0, y0, x1, y1, 1.1f, sc(INK)), capsule(x0, y0, x1, y1, 0.45f, sc(STAFF));
  disc(x1, y1, 1.7f, sc(INK)), disc(x1, y1, 0.9f, sc(white > 0.5f || game.on ? LIGHT : STAFF2));
  if (game.on && solidC < 0) ring(x1, y1, 2.6f + 0.4f * sinf(T * 7), 2.6f, LIGHT2, 0.35f);  // its light against the fire
}
void handOn(float sx, float sy, float hx, float hy) {  // a sleeve from the shoulder, the hand
  sleeve(sx, sy, hx, hy, solidC);
  disc(hx, hy, 1.3f, sc(INK)), disc(hx, hy, 0.7f, sc(SKIN));
}
void drawBack(float x, float y) {
  const float z = 0.8f;
  float since = T - duel.actT, k = clampf(since / 0.12f, 0, 1);
  int a = since < (duel.act == SLAM_DOWN ? 1.2f : 0.4f) ? duel.act : READY;
  if (a == READY && guarding() && atk.on)  // guard up for GUARD_S s after a press
    a = atk.fire ? BLOCK_STAFF : BLOCK_SWORD, k = 1;
  if (a == BLOCK_SWORD || a == BLOCK_STAFF) k = 1;  // a block follows the guard: already up
  if (a == HURT) y += 1.5f * (1 - since / 0.4f), solidC = since < 0.12f ? FIRE : -1;
  if (T - duel.whiteT < 0.15f) solidC = LIGHT;
  if (a == STRIKE_SWORD || a == STRIKE_STAFF) y -= 2 * k;
  auto X = [&](float d) { return x + d * z; };
  auto Y = [&](float d) { return y - d * z; };  // d: up from his feet
  float S = 15, H = 20, tx = duel.strikeX;
  if (!game.dead) ellipse(x, y, 7 * z, 1.4f, SHADOW);
  tri(X(-4.6f), Y(S + 0.5f), X(4.6f), Y(S + 0.5f), X(-7.6f), y, sc(INK)), tri(X(4.6f), Y(S + 0.5f), X(7.6f), y, X(-7.6f), y, sc(INK));
  tri(X(-3.6f), Y(S), X(3.6f), Y(S), X(-6.4f), Y(1), sc(ROBE2)), tri(X(3.6f), Y(S), X(6.4f), Y(1), X(-6.4f), Y(1), sc(ROBE2));
  tri(X(-3.6f), Y(S), X(-0.5f), Y(S), X(-6.4f), Y(1), sc(ROBE)), tri(X(-0.5f), Y(S), X(-2), Y(1), X(-6.4f), Y(1), sc(ROBE));
  for (int r = 3; r < S - 2; r += 3) px(X(1.5f), Y(r), sc(ROBE3)), px(X(4), Y(r - 1), sc(ROBE3));
  ellipse(x, Y(S - 0.5f), 4 * z, 4 * z, sc(INK)), ellipse(x, Y(S - 0.5f), 3.1f * z, 3.2f * z, sc(HAIR2));  // hair down his back
  px(X(-1), Y(S), sc(HAIR1)), px(X(1), Y(S - 1.5f), sc(HAIR1)), px(x, Y(S - 3), sc(HAIR1));
  if (white < 0.5f) {  // the Grey's hat: wide brim, a bent point
    ellipse(x, Y(H - 1), 6.6f * z, 2 * z, sc(INK)), ellipse(x, Y(H - 1), 5.6f * z, 1.1f * z, sc(HAT2));
    cone(x, Y(H - 0.5f), 3.4f * z, X(2), Y(H + 7), 0.9f * z, sc(INK)), cone(x, Y(H - 0.5f), 2.4f * z, X(2), Y(H + 6.5f), 0.3f, sc(HAT));
    cone(X(2), Y(H + 7), 0.9f * z, X(4), Y(H + 9), 0.4f, sc(INK));
  } else disc(x, Y(H - 0.5f), 3.6f * z, sc(INK)), disc(x, Y(H - 0.5f), 2.8f * z, sc(HAIR1)), px(X(1), Y(H - 1), sc(HAIR2));
  float lsx = X(-4), rsx = X(4), sy = Y(S - 1);  // shoulders
  auto restSword = [&] { swordAt(X(-6.5f), Y(11), X(-11), Y(4)), handOn(lsx, sy, X(-6), Y(11)); };
  auto restStaff = [&] { staffAt(X(7), y, X(7.5f), Y(24)), handOn(rsx, sy, X(6.6f), Y(12)); };
  switch (a) {
    case BLOCK_SWORD:  // Glamdring across, over his head
      restStaff(), handOn(lsx, sy, X(-4), Y(H + 3)), swordAt(X(-5), Y(H + 3), X(10), Y(H + 6 * k));
      break;
    case BLOCK_STAFF: {  // the staff stays down, a dome of white light around him
      restSword(), restStaff();
      float rx = 11 * k, ry = 24 * k;
      for (int py = y - ry; py <= y; py++)
        for (int px_ = x - rx; px_ <= x + rx; px_++) {
          float d = hypotf((px_ + 0.5f - x) / rx, (py + 0.5f - y) / ry);
          if (d < 1 && d > 0.88f) px(px_, py, LIGHT);
          else if (d < 0.88f && d > 0.6f && !((px_ + py) & 1) && !(py & 1)) px(px_, py, LIGHT2);  // its shimmer
        }
      break;
    }
    case STRIKE_SWORD:  // thrust up toward the opening
      restStaff(), handOn(lsx, sy, X(-2), Y(H + 2));
      swordAt(X(-2), Y(H + 2), X(-2) + (tx - x) * 0.3f * k, Y(H + 2 + 13 * k));
      break;
    case STRIKE_STAFF:  // staff raised, a bolt of light
    case SLAM_DOWN: {   // raised, then slammed down on the bridge
      float up = a == SLAM_DOWN ? (since < 0.4f ? since / 0.4f : since < 0.5f ? 1 - (since - 0.4f) / 0.1f : 0) : k;
      restSword(), staffAt(X(6), Y(2 + 10 * up), X(6.5f), Y(26 + 10 * up)), handOn(rsx, sy, X(6), Y(13 + 10 * up));
      break;
    }
    default: restSword(), restStaff();
  }
  solidC = -1;
}

// A cyan target on the Balrog at x (strike here), with an arrow over it; left: share of the chance still open.
void mark(float x, float left) {
  float r = 2.5f + 2.5f * clampf(left, 0, 1), b = 1.5f * sinf(T * 10);
  ring(x, MY, r + 0.8f, r + 0.8f, INK), ring(x, MY, r, r, SPARK2), ring(x, MY, r - 0.6f, r - 0.6f, SPARK2);
  disc(x, MY, 1.2f, LIGHT);
  for (int i = 0; i < 4; i++) {  // the arrow, pointing down at it
    float y = MY - r - 4 - i + b;
    for (int dx = -i; dx <= i; dx++) px(x + dx, y, dx == -i || dx == i ? INK : SPARK2);
  }
}
void gameDraw() {
  memcpy(fb, bg, sizeof fb);
  for (int layer = 0; layer < 2; layer++)  // fire roaring up out of the chasm behind him: a tall dark-orange back
    for (int k = 0; k < 24; k++) {           // row, a shorter bright one in front, every tongue its own height
      float fx = k * 3.5f + 3 * h01(k + layer * 40) + sinf(T * 1.3f + k), g = expf(-powf((fx - 40) / 16, 2));
      float h = (6 + 16 * h01(k * 3 + layer * 7) + 20 * g) * (layer ? 0.6f : 1) * (0.75f + 0.25f * sinf(T * (2.5f + h01(k)) + k * 1.3f));
      float base = 56 - 3 * g + 3 * h01(k * 5 + layer), sw = 2.5f * sinf(T * 2.3f + k * 0.7f);
      cone(fx, base, 2.4f + 2 * g, fx + sw, base - h, 0.3f, layer ? CLOUD : CLOUD2);
    }
  for (int k = 0; k < 5; k++) {  // smoke rolling along the top
    float sx = fmodf(h01(k) * 100 + T * (2 + k), 100) - 10, sy = 3 + 3 * k;
    ellipse(sx, sy, 6 + 2 * h01(k + 3), 2.2f, k & 1 ? STAR : SUN);
  }
  for (int i = 0; i < N * N; i++)  // all of that only on the haze: behind the bridge and the cliffs
    if (bg[i] > SKY3) fb[i] = bg[i];
  for (int k = 0; k < 20; k++) {  // embers rising from the abyss
    float sp = 8 + 10 * h01(k + 7), y = N - fmodf(T * sp + h01(k + 3) * N, N), x = h01(k) * N + 2 * sinf(T * 2 + k);
    if (at(x, y) <= MOON) px(x, y, k & 1 ? MOON : LAMP);
  }
  for (int x = 0; x < N; x++)  // the fire's glow flickering up from far below
    for (int y = 76 - 3 * (sinf(T * 3 + x * 0.4f) + 1) * (sinf(x * 0.13f) + 1.2f); y < N; y++)
      if (at(x, y) <= SKY3) px(x, y, (x + y) & 1 ? WATER : WATER2);
  // The Balrog: staggers when struck, falls through the broken far end and climbs back.
  float by = T - duel.hitT < 0.3f ? 2 * (1 - (T - duel.hitT) / 0.3f) : 0, ft = duel.t;
  bool falling = duel.phase == FALL && ft > 0.6f;
  if (falling) by += ft < 2.2f ? 70 * powf((ft - 0.6f) / 1.6f, 2) : 40 * clampf((3.4f - ft) / 1.2f, 0, 1);
  if (by < 40) {
    if (T - duel.hitT < 0.08f) solidC = LIGHT;
    drawBalrog(by, duel.phase == GUARD);
    solidC = -1;
  }
  if (falling)  // he's behind the bridge now; its far end broken away while he drops
    for (int y = 50; y < N; y++)
      for (int x = 0; x < N; x++) {
        uint8_t b = bg[y * N + x];
        if (b >= LAND && b <= LANDL) fb[y * N + x] = y < 53 + 2 * h01(x) && ft < 2.2f ? WATER2 : b;
      }
  if (duel.phase == GUARD) {  // the open side: a cyan mark on him, a dashed path to it; it shrinks as the chance runs out
    mark(guardX(duel.open), 1 - duel.t / duel.win);
    for (float y = 52; y < 70; y += 3)
      if (fmodf(y / 3 - T * 5, 2) < 1) px(laneX(duel.open, y), y, SPARK2);
  }
  if (counter()) mark(guardX(duel.lane), 1 - (T - duel.counterT) / duel.counterW);  // the free strike, right where he stands
  for (int i = 0; i < duel.cracks && i < CRACKS; i++) {  // cracks across the bridge at his feet
    float x = 26 + i * 14 + 4 * h01(i), y = 79;
    for (int s = 0; s < 9; s++) {
      float nx = x + 3 * (h01(i * 31 + s) - 0.5f), ny = y - 1.5f - h01(i * 17 + s);
      capsule(x, y, nx, ny, 0.4f, s & 1 ? SHADOW : INK), x = nx, y = ny;
    }
  }
  if (atk.on)  // where he'll strike: a red ring closing on each marked lane; closed, it bursts white: press now
    for (int l = -1; l <= 1; l++) {
      if (!hits(atk.mask, l)) continue;
      float x = laneX(l, MY), p = fill(), r = 2.4f + 3.6f * (1 - p), w = atk.t - atk.dur;
      ring(x, MY, r + 0.8f, r + 0.8f, INK), ring(x, MY, r, r, w < 0 ? SPARK1 : MOON), ring(x, MY, r - 0.6f, r - 0.6f, SPARK1);
      disc(x, MY, 2.3f, INK), disc(x, MY, 1.5f, w < 0 ? SPARK1 : MOON);
      if (w >= 0 && w < 0.1f) ring(x, MY, 4 + 40 * w, 4 + 40 * w, MOON);  // the mark bursting
    }
  float ls = T - atk.landT;
  if (ls < BURST) {  // the blow: the whip cracking down every marked lane, or fire bursting on them
    float q = ls / BURST, hx, hy;
    handAt(atk.fire ? -1 : 1, 0, hx, hy);
    for (int l = -1; l <= 1; l++) {
      if (!hits(atk.mask, l)) continue;
      float x = laneX(l, LY1), y = LY1 + 1;
      if (atk.fire) {
        if (q < 0.4f) cone(hx, hy, 1, x, y, 2.5f, FIRE), capsule(hx, hy, x, y, 0.6f, FIRE2);
        disc(x, y, 2 + 5 * q, q < 0.5f ? FIRE : ACC3), disc(x, y, 3.5f * (1 - q), FIRE2), disc(x, y, 1.8f * (1 - q), MOON);
      } else {
        lash([&](float u, float &lx, float &ly) {
          lx = hx + (x - hx) * u + 3 * (1 - u) * sinf(u * 9 - T * 24), ly = hy + (y - hy) * u - 7 * sinf(u * 3.1416f);
        }, 12, 0.6f, 1.1f);
        disc(x, y, 2.5f * (1 - q) + 0.8f, FIRE2), disc(x, y, 1.2f * (1 - q), MOON);
      }
    }
  }
  float gy = GY + (game.dead ? duel.fallY : 0);
  if (game.dead)  // the bridge's near end gone: the abyss where he stood
    for (int y = 64; y < N; y++)
      for (int x = 0; x < N; x++)
        if (at(x, y) >= LAND && at(x, y) <= LANDL && fabsf(x - duel.gx) < 10 + (y - 64) * 0.6f + 2 * h01(x + y * 7)) px(x, y, y > 74 ? WATER : WATER2);
  if (gy < N + 30) drawBack(duel.gx, gy);
  float since = T - duel.actT;
  bool landed = T - duel.hitT < 0.3f;
  if (duel.act == STRIKE_STAFF && since < 0.3f && landed)  // the bolt from the staff
    capsule(duel.gx + 5, GY - 29, duel.strikeX, MY, 1.4f * (1 - since / 0.3f), LIGHT2),
        capsule(duel.gx + 5, GY - 29, duel.strikeX, MY, 0.6f, LIGHT);
  if (duel.act == STRIKE_SWORD && since < 0.3f && landed) {  // the slash across him
    float ox = duel.strikeX;
    for (int k = 0; k < 9; k++) {
      float a = -2.4f + k * 0.2f + since * 6;
      px(ox + 6 * cosf(a), MY + 6 * sinf(a), k & 1 ? LIGHT : LIGHT2);
    }
  }
  if (duel.act == SLAM_DOWN && since > 0.45f && since < 1.4f) {  // the ring of light along the bridge
    float r = 4 + 40 * (since - 0.45f);
    ring(duel.gx, GY, r, r * 0.3f, LIGHT, clampf((since - 0.45f) * 1.2f, 0, 0.9f));
    ring(duel.gx, GY, r * 0.8f, r * 0.24f, LIGHT2, clampf((since - 0.45f) * 1.5f, 0, 0.9f));
  }
  if (T - duel.whiteT < 0.7f) {  // the White: rays of light out of him
    float k = (T - duel.whiteT) / 0.7f;
    for (int i = 0; i < 12; i++) {
      float a = i * 0.5236f, r0 = 6 + 20 * k, r1 = r0 + 5 * (1 - k);
      capsule(duel.gx + r0 * cosf(a), GY - 13 + r0 * sinf(a), duel.gx + r1 * cosf(a), GY - 13 + r1 * sinf(a), 0.5f, LIGHT);
    }
  }
  for (auto &s : sparks)
    if (s.on) px(s.x, s.y, s.c);
}

// Bubble text: score and high score; the level as the Balrog falls; the result after a game over.
void gameLabel(char *out, int cap) {
  int best = pet::best() > game.score ? pet::best() : game.score;
  if (game.dead) snprintf(out, cap, game.newBest ? "NEW BEST %d" : "%d BEST %d", game.score, best);
  else if (duel.phase == FALL) snprintf(out, cap, "LEVEL %d", duel.level + 1);
  else snprintf(out, cap, "%d HI %d", game.score, best);
}

void step(float dt) {
  if (game.on) return gameStep(dt), gameDraw();
  update(dt);
  weatherStep(dt);
  if (rove) return drawRide();
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
