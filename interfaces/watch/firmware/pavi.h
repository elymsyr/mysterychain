// Pavi (Seven Havens protagonist, purple-haired girl) and her fairy: draws into an 80x80 color-indexed buffer (fb),
// scaled 3x on screen. No Arduino dependency: also compiles on a PC for frame-by-frame testing (interfaces/watch/test).
// Pavi is a pet like the cat (pet.h): her mood picks what she does, the sky follows the clock (tint: clouds and sun by day,
// aurora, moon and stars at night), and her game is the travel road steered by tilting (gameTilt, below).
// Travel (travel()): seen from behind, she bends her way down a road (earth: a rock slab, water: a wave on a river,
// air: an air ball, fire: jets from her feet), dodging left and right; the layers shift by depth (parallax). The music
// screen shows it; now and then she also sets off on her own.
#pragma once
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifndef PET_NAME
#define PET_NAME "Pavi"  // alone; with the cat (pets.h) the pet keeps the cat's default name
#endif
#include "pet.h"

namespace pavi {

const int N = 80, GROUND = 58;  // logical resolution, ground row (feet stand here)
const float G = 260;            // gravity, px/s²

enum : uint8_t {
  SKY0, SKY1, SKY2, SKY3, AUR0, AUR1, AUR2, STAR, ROOF, ROOF2, LAMP, WALL, WALL2,
  INK, SKIN, HAIR, HAIR2, HAIRL, BAND, EYE, CREAM, MOUTH, CLOAK, CLOAK2, SHIRT, BELT, PANTS,
  WING, FAIRY, GLOW, SHADOW, SUN, MOON, CLOUD, PINK, APPLE, LEAF, LEAF2, GREY, CLAY,
  GRASS, GRASS2, HAZE, ROAD, ROAD2, RIVER, RIVER2, HILL, FLAME, FLAME2, DROP, FLAKE, FLASH, NCOL
};
uint8_t PAL[NCOL][3] = {  // night look; tint() rewrites the sky, roofs, wall, star, sun, moon, cloud and shadow entries
    {8, 12, 36},    {12, 30, 64},   {16, 52, 86},   {28, 80, 104},  // night sky, top to horizon
    {64, 48, 124},  {30, 150, 120}, {130, 240, 196},                // aurora: purple top, green, bright lower edge
    {230, 240, 255}, {50, 28, 54},  {34, 20, 42},   {255, 200, 110},  // star, distant roofs, window light
    {124, 54, 64},  {80, 32, 48},                                   // the wall she stands on
    {24, 14, 30},   {134, 80, 54},  {84, 62, 150},  {50, 38, 100}, {130, 110, 200},  // outline, skin, hair
    {238, 198, 92}, {80, 190, 110}, {245, 238, 222}, {160, 60, 70},  // hairband, eye, cream, mouth
    {170, 182, 100}, {116, 130, 64}, {228, 126, 104}, {150, 86, 50}, {72, 46, 60},  // cloak, shirt, belt, trousers
    {150, 225, 255}, {60, 100, 210}, {235, 255, 255}, {52, 22, 40},  // fairy wing, body, glow; shadow
    {255, 214, 90}, {240, 236, 210}, {40, 48, 84},                   // sun, moon, cloud
    {255, 140, 170}, {214, 48, 56}, {96, 186, 80}, {54, 128, 62}, {178, 176, 186}, {196, 98, 60},  // props
    {40, 86, 62},   {28, 64, 48},   {34, 64, 80},    // travel: grass, dark grass, far ground haze
    {74, 58, 60},   {56, 42, 48},   {30, 60, 110},  {80, 120, 170}, {26, 40, 62},  // road, its edge, river, ripple, hills
    {255, 130, 40}, {255, 228, 120},                                 // fire, its core
    {205, 222, 255}, {250, 252, 255}, {214, 212, 250},               // weather: raindrop, snow, lightning flash
};

// ---- Time of day (K6): the palette indices stay, their RGB follows the clock (pet::look) ----
const uint8_t TINTED[] = {SKY0, SKY1, SKY2, SKY3, STAR, ROOF, ROOF2, LAMP, WALL, WALL2, SHADOW, SUN, MOON, CLOUD,
                          GRASS, GRASS2, HAZE, ROAD, ROAD2, RIVER, RIVER2, HILL};
const int NT = sizeof TINTED;
const uint8_t LOOK[4][NT][3] = {  // pet::NIGHT, DAWN, DAY, DUSK; by day the roofs fade into the haze, the windows go dark
    {{8, 12, 36}, {12, 30, 64}, {16, 52, 86}, {28, 80, 104}, {230, 240, 255}, {50, 28, 54}, {34, 20, 42},
     {255, 200, 110}, {124, 54, 64}, {80, 32, 48}, {52, 22, 40}, {255, 214, 90}, {240, 236, 210}, {40, 48, 84},
     {40, 86, 62}, {28, 64, 48}, {34, 64, 80}, {74, 58, 60}, {56, 42, 48}, {30, 60, 110}, {80, 120, 170}, {26, 40, 62}},
    {{70, 80, 150}, {150, 110, 165}, {235, 140, 140}, {255, 185, 130}, {255, 240, 220}, {120, 72, 104}, {92, 56, 90},
     {255, 214, 140}, {176, 86, 86}, {122, 58, 66}, {96, 44, 58}, {255, 240, 170}, {250, 240, 220}, {255, 206, 196},
     {92, 140, 80}, {70, 112, 66}, {190, 150, 150}, {170, 130, 110}, {140, 100, 90}, {120, 130, 190}, {220, 200, 230}, {136, 104, 140}},
    {{74, 144, 226}, {104, 170, 236}, {142, 196, 244}, {186, 222, 250}, {255, 255, 255}, {124, 128, 176}, {100, 104, 152},
     {100, 104, 152}, {196, 100, 92}, {142, 68, 72}, {116, 52, 62}, {255, 236, 140}, {240, 240, 240}, {250, 252, 255},
     {104, 176, 84}, {78, 148, 68}, {160, 196, 206}, {196, 160, 112}, {164, 128, 88}, {70, 140, 220}, {170, 220, 250}, {118, 152, 176}},
    {{44, 46, 112}, {130, 66, 128}, {226, 98, 92}, {255, 158, 84}, {255, 240, 200}, {78, 36, 70}, {56, 26, 54},
     {255, 200, 110}, {150, 62, 70}, {98, 40, 54}, {66, 28, 46}, {255, 226, 150}, {250, 225, 170}, {250, 160, 130},
     {80, 120, 66}, {60, 96, 54}, {156, 110, 110}, {160, 106, 86}, {128, 82, 70}, {110, 100, 170}, {230, 170, 170}, {104, 70, 108}},
};
using pet::SUNRISE;
using pet::SUNSET;

int clockMin = -1;  // minute of day, -1 = clock unknown (night look, moon at a fixed spot)
float nightW = 1;   // 1 = full night (aurora, all stars), 0 = day (clouds)

void tint(int minuteOfDay) {
  clockMin = minuteOfDay;
  int a, b;
  float f = pet::look(clockMin, a, b);
  for (int k = 0; k < NT; k++)
    for (int c = 0; c < 3; c++) PAL[TINTED[k]][c] = lroundf(LOOK[a][k][c] * (1 - f) + LOOK[b][k][c] * f);
  nightW = (a == pet::NIGHT ? 1 - f : 0) + (b == pet::NIGHT ? f : 0);
  // Weather: the sky and the far layers grey over (toward their own brightness), a storm or rain also darkens the land.
  static const uint8_t AIR[] = {SKY0, SKY1, SKY2, SKY3, ROOF, ROOF2, HAZE, HILL},
                       LAND[] = {WALL, WALL2, SHADOW, GRASS, GRASS2, ROAD, ROAD2, RIVER, RIVER2};
  float ov = pet::OVERCAST[pet::wx], dark = pet::gloom();
  for (uint8_t k : AIR) pet::dim(PAL[k], ov, dark);
  pet::dim(PAL[CLOUD], ov, dark * 0.6f);
  for (uint8_t k : LAND) pet::dim(PAL[k], ov * 0.3f, dark);
}
bool sunUp() { return clockMin >= SUNRISE && clockMin < SUNSET; }

// Sprites face right and are mirrored when facing left. K outline, S skin, H/h/L hair, Y band, E eye, W cream,
// M mouth, C/c cloak, P shirt, B belt. Head in 3/4 view: face in front, hair mass falls behind. Rows 9-14 are the face.
const int HEAD_H = 19, HEAD_AX = 9;  // HEAD_AX: head column that sits over the body center
#define TOP ".........KK.....", "........KHK.....", ".....KKKHKKKK...", "...KKHHHHHHHHKK.", "..KHHHLHHHHHHHHK", \
            ".KHHYYYYYYYYYYHK", ".KHHHHHHHHHHHHHK", "KHHHHLHHHHHHHHHK", "KHHHHHSHHSSSHSHK"
#define CHIN "KhHHHHHKKKKKK...", "KHhHHHK.........", ".KHhHK..........", "..KKK..........."
#define CHEEK "KhHHHSSSSSSSSSSK"
#define SMILE "KhHHHHSSSSMMSSK.", "KhHHHHHSSSSSSK.."
const char *const HEAD[][HEAD_H] = {
    {TOP, "KhHHHSSKKSSSKKSK", "KhHHHSSWESSSWESK", "KhHHHSSEESSSEESK", CHEEK, SMILE, CHIN},  // eyes open
    {TOP, CHEEK, CHEEK, "KhHHHSSKKSSSKKSK", CHEEK, SMILE, CHIN},                             // blink
    {TOP, CHEEK, "KhHHHSSKKSSSKKSK", "KhHHHSKSSKSKSSKK", CHEEK, "KhHHHHSSSKMMKSK.", "KhHHHHHSSSKKSK..", CHIN},  // happy ^^
    {TOP, CHEEK, "KhHHHSSKKSSSKKSK", "KhHHHSSEESSSEESK", CHEEK, "KhHHHHSSSSKKSSK.", "KhHHHHHSSSSSSK..", CHIN},  // half (bored)
    {TOP, CHEEK, "KhHHHSKSSKSKSSKK", "KhHHHSSKKSSSKKSK", CHEEK, "KhHHHHSSSSSMSSK.", "KhHHHHHSSSSSSK..", CHIN},  // closed
    {TOP, "KhHHHSSSSSSSKKSK", "KhHHHSKSSKSSWESK", "KhHHHSSKKSSSEESK", CHEEK, "KhHHHHSSSSSMSSK.", "KhHHHHHSSSSSSK..", CHIN},  // peek
    {TOP, CHEEK, "KhHHHSSKKSSSKKSK", "KhHHHSKSSKSKSSKK", "KhHHHSSSSKKKKSSK", "KhHHHHSSSKMMKSK.", "KhHHHHHSSKKKSK..", CHIN},  // yawn
    {TOP, "KhHHHSSKSSSSSKSK", "KhHHHSSEKSSSKESK", "KhHHHSSEESSSEESK", CHEEK, "KhHHHHSSSSKKSSK.", "KhHHHHHSSKSSKK..", CHIN},  // frown
    {TOP, "KhHHHSKSKSSKSKSK", "KhHHHSSKSSSSKSSK", "KhHHHSKSKSSKSKSK", CHEEK, "KhHHHHSSSSKKSSK.", "KhHHHHHSSSSSSK..", CHIN},  // x x
    {TOP, "KhHHHSSKKSSSKKSK", "KhHHHSSWESSSWESK", "KhHHHSSEESSSEESK", "KhHHHSSKKSSSKKSK", "KhHHHHSSSKMKSSK.", "KhHHHHHSSSKSSK..",
     CHIN},  // wide, "o" (thunder)
};
#undef TOP
#undef CHIN
#undef CHEEK
#undef SMILE
enum { EYES_OPEN, EYES_BLINK, EYES_HAPPY, EYES_HALF, EYES_CLOSED, EYES_PEEK, EYES_YAWN, EYES_FROWN, EYES_DIZZY, EYES_WIDE };

const int TORSO_H = 9, TORSO_AX = 6;
const char *const TORSO[TORSO_H] = {
    "....KSSK....", "..KKCSSCKK..", ".KCCCPPCCCK.", "KCcPPPPPPcCK", "KCcPPPPPPcCK",
    "KCcBBBBBBcCK", ".KcPPPPPPcK.", ".KcPPPPPPcK.", ".KKKKKKKKKK.",
};

// Props, in the cat's sprite letters (art()): K outline, O apple, W cream, P pink, Y grey, C clay, g/G leaf, F belt brown,
// b/B fairy blue, # = the caller's color.
const char *const ZZ[] = {"####", "..#.", ".#..", "####"};
const char *const LOVE[] = {".P.P.", "PPPPP", ".PPP.", "..P.."};
const char *const NOTE[2][5] = {{"..##", "..#.", "..#.", "###.", "##.."},
                                {".####", ".#..#", ".#..#", "##.##", "#..#."}};
const char *const TWINKLE[] = {".#.", "###", ".#."};
const char *const FLY[2][5] = {{"K.....K", ".K...K.", "KPK.KPK", "KPPKPPK", ".KK.KK."},   // butterfly, wings open
                               {".K...K.", "..K.K..", ".KPKPK.", ".KPKPK.", "..K.K.."}};  // wings up
const char *const APPLE_ART[4][5] = {{".KFg.", "KOOOK", "KOWOK", "KOOOK", ".KKK."},  // whole, then bitten from her side
                                     {".KFg.", ".KOOK", "..KOK", ".KOOK", ".KKK."},
                                     {"..Fg.", "..KOK", "...KK", "..KOK", "..KK."},
                                     {"..Fg.", "..K..", "..K..", "..K..", "....."}};
const char *const PLATE[] = {"KWWWWWWK", ".KYYYYK.", "..KKKK.."};

uint8_t fb[N * N], bgCity[N * N], bgRoad[N * N];
uint8_t *bg = bgCity;            // the static background: her wall, or the open sky on a trip
float T = 0, DT = 0, awake = 0;  // awake > 0: play mode (refilled by a shake)
bool tunes = false;              // music screen on (music()): she travels
bool rove = false;               // travelling (travel())
float tripLeft = 0;              // s of travel left, 0 = until the music stops
uint32_t rs = 2463534242u;
float rnd() {
  rs ^= rs << 13, rs ^= rs >> 17, rs ^= rs << 5;
  return (rs >> 8) / 16777216.0f;
}
float clampf(float v, float lo, float hi) { return fminf(fmaxf(v, lo), hi); }

inline void px(int x, int y, uint8_t c) {
  if ((unsigned)x < N && (unsigned)y < N) fb[y * N + x] = c;
}

// Paints pixels within r of a line segment (arms, legs, hair; a disc if both ends match).
void capsule(float x0, float y0, float x1, float y1, float r, uint8_t c) {
  float dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy + 1e-6f;
  for (int y = floorf(fminf(y0, y1) - r); y <= ceilf(fmaxf(y0, y1) + r); y++)
    for (int x = floorf(fminf(x0, x1) - r); x <= ceilf(fmaxf(x0, x1) + r); x++) {
      float qx = x + 0.5f - x0, qy = y + 0.5f - y0, t = clampf((qx * dx + qy * dy) / l2, 0, 1);
      qx -= t * dx, qy -= t * dy;
      if (qx * qx + qy * qy <= r * r) px(x, y, c);
    }
}
void disc(float x, float y, float r, uint8_t c) { capsule(x, y, x, y, r, c); }

uint8_t code(char ch) {
  switch (ch) {
    case 'S': return SKIN;  case 'H': return HAIR;  case 'h': return HAIR2; case 'L': return HAIRL;
    case 'Y': return BAND;  case 'E': return EYE;   case 'W': return CREAM; case 'M': return MOUTH;
    case 'C': return CLOAK; case 'c': return CLOAK2; case 'P': return SHIRT; case 'B': return BELT;
  }
  return INK;
}

// Sprite column ax lands on x; mirrored horizontally if d < 0.
void blit(const char *const *rows, int h, int ax, int x, int y0, int d) {
  for (int r = 0; r < h; r++)
    for (int c = 0; rows[r][c]; c++)
      if (rows[r][c] != '.') px(d > 0 ? x + c - ax : x - 1 - c + ax, y0 + r, code(rows[r][c]));
}

void art(const char *const *rows, int h, int x, int y, int flip = 1, uint8_t hash = INK) {  // props: top-left at x, y
  static const char K[] = "KOWPYCgGFbB#";
  const uint8_t C[] = {INK, APPLE, CREAM, PINK, GREY, CLAY, LEAF, LEAF2, BELT, WING, FAIRY, hash};
  int w = strlen(rows[0]);
  for (int r = 0; r < h; r++)
    for (int c = 0; c < w; c++) {
      const char *k = strchr(K, rows[r][flip > 0 ? c : w - 1 - c]);
      if (k && *k) px(x + c, y + r, C[k - K]);
    }
}

void shadow(float x, float w) {
  for (int i = -(int)w; i <= (int)w; i++) px(x + i, GROUND, SHADOW);
  for (int i = -(int)(w * 0.6f); i <= (int)(w * 0.6f); i++) px(x + i, GROUND + 1, SHADOW);
}

// ---- Background: sky (clouds and sun by day; aurora, moon and stars at night), distant roofs, wall ----
const float BAYER[4] = {0.125f, 0.625f, 0.875f, 0.375f};  // 2x2 dither thresholds
float bayer(int x, int y) { return BAYER[(y & 1) * 2 + (x & 1)]; }

uint8_t ramp(int x, int y, int h) {  // 4-step gradient over h rows (0..3)
  float v = y * 3.0f / (h - 1);
  int b = v;
  return b + (v - b > bayer(x, y));
}
uint8_t skyAt(int x, int y) { return SKY0 + ramp(x, y, GROUND); }
float h01(int i) {  // a fixed random number in 0..1 per integer (scenery along the trip)
  uint32_t h = i * 2654435761u;
  h ^= h >> 15, h *= 2246822519u, h ^= h >> 13;
  return (h & 0xffff) / 65536.0f;
}

// ---- Weather (pet::wx, weather.h): in the rain Pavi holds a water dome over herself; it goes where she goes ----
const int WIND_DIR = 1;
float canopy(float x);
float landRow();
float drift() { return 0; }  // her trip goes into the screen
bool airy(uint8_t c) { return c <= STAR || c == CLOUD; }  // sky, aurora, stars, clouds
void thunder();
#include "weather.h"

// Aurora curtain: bright lower edge in each column, turning purple and fading upward. Draws over sky only; fades out
// (by density) as the night does, and behind clouds (gone once it rains or snows).
void aurora() {
  float show = nightW * fmaxf(0, 1 - 2 * pet::OVERCAST[pet::wx]);
  for (int x = 0; x < N; x++) {
    float c = 20 + 4 * sinf(x * 0.08f + T * 0.35f) + 2 * sinf(x * 0.21f - T * 0.6f);  // lower edge row
    float len = 10 + 3 * sinf(x * 0.13f + T * 0.5f), shine = 0.5f + 0.5f * sinf(x * 0.45f - T * 1.6f);
    for (int y = c - len; y <= c; y++) {
      if (y < 0 || bg[y * N + x] > SKY3 || show <= bayer(x + 1, y)) continue;
      float t = (c - y) / len, b = bayer(x, y);  // t: 0 lower edge, 1 top
      if (t < 0.15f) px(x, y, shine > b ? AUR2 : AUR1);
      else if (t < 0.55f) { if (0.8f > b) px(x, y, AUR1); }
      else if ((1 - t) / 0.45f > b) px(x, y, AUR0);
    }
  }
}

void skyDisc(float cx, float cy, float r, uint8_t c, float cutX = 0, float cutY = 0, float cutR = 0) {  // only over sky
  for (int y = floorf(cy - r); y <= cy + r; y++)
    for (int x = floorf(cx - r); x <= cx + r; x++)
      if ((unsigned)x < N && (unsigned)y < N && bg[y * N + x] <= SKY3 && hypotf(x + 0.5f - cx, y + 0.5f - cy) < r &&
          (cutR == 0 || hypotf(x + 0.5f - cutX, y + 0.5f - cutY) > cutR))
        fb[y * N + x] = c;
}

// Clouds drift by day, fading in by density at dawn and out at dusk. Weather: more of them (also at night), wind blows
// them fast.
void clouds() {
  static const float CY[6] = {9, 19, 30, 14, 25, 4}, CW[6] = {7, 5, 6, 8, 6, 9}, SPD[6] = {1.6f, 2.4f, 1.1f, 1.9f, 1.3f, 2.1f},
                     X0[6] = {10, 60, 35, 85, 0, 48};
  int n = pet::wx == pet::CLEAR ? 3 : pet::wx == pet::WIND ? 4 : 6;
  float fast = pet::wx == pet::WIND ? 8 : pet::wx == pet::STORM ? 3 : 1, show = fmaxf(1 - nightW, n == 6 ? 0.9f : 0);
  for (int k = 0; k < n; k++) {
    float cx = fmodf(X0[k] + T * SPD[k] * fast, N + 30) - 15, w = CW[k];
    for (int y = CY[k] - 4; y <= CY[k] + 2; y++)
      for (int x = cx - w - 3; x <= cx + w + 3; x++) {
        if ((unsigned)x >= N || (unsigned)y >= N || bg[y * N + x] > SKY3 || show <= bayer(x, y + 1)) continue;
        float dx = x + 0.5f - cx, dy = y + 0.5f - CY[k];
        if (dy < 1.5f && (hypotf(dx / (w + 2), dy / 2) < 1 || hypotf(dx + 2, dy + 1.5f) < 2.8f || hypotf(dx - w / 3, dy + 2) < 2.5f))
          fb[y * N + x] = CLOUD;
      }
  }
}

const int NS = 14;
uint8_t sx[NS], sy[NS];
float sp[NS];

void drawSky() {
  memcpy(fb, bg, sizeof fb);
  aurora();
  float ov = pet::OVERCAST[pet::wx];  // overcast hides stars by count, a thick one the sun and moon too
  for (int i = 0; i < NS; i++)  // twinkling stars (never over the aurora), fading in by count at dusk
    if (i < NS * nightW * (1 - ov) && sinf(T * 1.7f + sp[i]) > -0.2f && fb[sy[i] * N + sx[i]] <= SKY3) px(sx[i], sy[i], STAR);
  float mx = 62, my = 9;  // moon at a fixed spot while the clock is unknown
  bool moon = true;
  if (clockMin >= 0) {  // sun or moon on an arc from left (rise) to right (set)
    moon = !sunUp();
    float p = moon ? float((clockMin - SUNSET + 1440) % 1440) / (1440 - SUNSET + SUNRISE) : float(clockMin - SUNRISE) / (SUNSET - SUNRISE);
    mx = 12 + 56 * p, my = GROUND - 14 - 40 * sinf(3.1416f * p);
  }
  if (ov < 0.6f && moon) skyDisc(mx, my, 4.6f, MOON, mx + 2.5f, my - 1.5f, 4.2f);  // crescent
  else if (ov < 0.6f) {
    skyDisc(mx, my, 4.2f, SUN);
    if (pet::wx == pet::CLEAR) rays(mx, my);
  }
  clouds();
}

// ---- Girl, fairy, visitors ----
enum { IDLE, RUN, CROUCH, JUMP, LAND, REACH, SLEEP, EAT, REFUSE, PURR, YAWN, BORED, HUNGRY, ANGRY, KO, WAVE, SIT, TWIRL, DANCE,
       SHIELD, CATCH, BASK };  // weather: holding the dome up in the rain, catching snowflakes, sitting in the sun
struct {
  float x = 30, y = GROUND, vx = 0, vy = 0, t = 0, dur = 1, phase = 0, happy = 0, peek = 0, scare = 0;  // scare: thunder
  int dir = 1, st = IDLE, hops = 0;
  bool booped = false, yay = false, wake = false;  // yay: both arms up during a joy hop; wake: yawn after this landing
} girl;
struct {
  float x = 44, y = 30, vx = 0, vy = 0, tx = 44, ty = 30, re = 0, emit = 0;
} fairy;
struct {
  float x = 0;
  bool on = false;
} bowl;  // the empty plate while she's hungry
enum { BUTTERFLY, FIREFLY, BLOWN };  // BLOWN: a leaf in the wind
struct {
  float x = 0, y = 0, vx = 0, vy = 0, t = 0, home = 0;
  int kind = BUTTERFLY;
  bool on = false, flee = false;
} vis;

pet::Mood mood = pet::HAPPY;
float visitT = 150, yawnT = 0, funT = 12, tripT = 200;  // s until the next visitor / tired yawn / idle trick / trip

void setMood(pet::Mood m) { mood = m; }

const int NP = 24;  // fairy sparkle particles
struct {
  float x, y, vx, vy, life;
} sparks[NP];
void spark(float x, float y, float vx, float vy, float life) {
  for (auto &s : sparks)
    if (s.life <= 0) {
      s = {x, y, vx, vy, life};
      return;
    }
}
void burst(float x, float y) {
  for (int i = 0; i < 10; i++) {
    float a = i * 0.628f, v = 30 + 20 * rnd();
    spark(x, y, v * cosf(a), v * sinf(a), 0.5f);
  }
}

void go(int st, float dur) {
  girl.st = st, girl.t = 0, girl.dur = dur, girl.booped = false;
  if (st != HUNGRY) bowl.on = false;
}
void endTrip() { rove = false, bg = bgCity; }
void pose(int st, float dur) {  // a ground state from wherever she is (ends a trip)
  endTrip();
  girl.y = GROUND, girl.vx = girl.vy = 0, girl.hops = 0, girl.wake = false, girl.yay = false, girl.peek = 0;
  go(st, dur);
}
bool grounded() { return girl.st != CROUCH && girl.st != JUMP && girl.st != LAND && girl.st != REACH && girl.st != ANGRY && girl.st != KO; }
bool chasing() { return vis.on && !vis.flee; }
float aimX() { return chasing() ? vis.x : fairy.x; }  // what she's after: a visitor, else the fairy
float aimY() { return chasing() ? vis.y : fairy.y; }
int side() { return aimX() >= girl.x ? 1 : -1; }

void leap(float xt, float h, int hops) {  // to xt, h high; hops = how many more hops after
  xt = clampf(xt, 16, 64);
  girl.vy = -sqrtf(2 * G * h);
  girl.vx = (xt - girl.x) / (2 * -girl.vy / G);
  girl.hops = hops;
  go(JUMP, 0);
}

void retarget() {  // in play the fairy darts to a new spot, away from the girl
  fairy.tx = 14 + 52 * rnd(), fairy.ty = 22 + 20 * rnd();
  if (fabsf(fairy.tx - girl.x) < 14) fairy.tx = clampf(girl.x + (fairy.tx < girl.x ? -18 : 18), 14, 66);
  fairy.re = 1.2f + 1.3f * rnd();
}

// A butterfly (day), a firefly (night) or a leaf (wind) comes in and hovers near her; she reaches or jumps for it.
void visit() {
  if (vis.on) return;
  vis.on = true, vis.flee = false, vis.t = 0;
  vis.kind = pet::wx == pet::WIND ? BLOWN : sunUp() ? BUTTERFLY : FIREFLY;
  int open = girl.x < N / 2 ? 1 : -1;  // the roomier side
  vis.x = open > 0 ? 84 : -4;
  vis.home = clampf(girl.x + open * (24 + 10 * rnd()), 18, 62);
  vis.y = vis.kind == FIREFLY ? GROUND - 27 : GROUND - 18;  // the butterfly in reach, the firefly needs a jump
}
void flee() {
  vis.flee = true;
  vis.vx = (vis.kind == BLOWN ? WIND_DIR : vis.x >= girl.x ? 1 : -1) * 60, vis.vy = -30;  // a leaf goes with the wind
}

// The water dome: a half ellipse standing on the wall, its top under the bubble band. Rain stops on it (canopy()).
const float DOME_RX = 17, DOME_RY = 33;
float domeX = 30, domeK = 0;  // its middle (follows her), how far it's up 0..1
float canopy(float x) {
  float u = (x - domeX) / DOME_RX;
  return domeK > 0 && !rove && fabsf(u) < 1 ? GROUND - DOME_RY * domeK * sqrtf(1 - u * u) : 1e9f;
}
void thunder() {  // hands over her head; asleep she only peeks
  if (girl.st == SLEEP) girl.peek = 1.5f;
  else if (grounded()) girl.scare = 1.1f;
}

// Did the tip of the reaching front hand touch what she's after
bool touch() { return hypotf(aimX() - (girl.x + girl.dir * 6.5f), aimY() - (girl.y - 17)) < 5; }

void boop() {  // touched it. The fairy giggles, flees and scatters sparkles; a visitor is caught 30 % of the time
  girl.booped = true, girl.happy = 1.2f;
  if (chasing()) {
    burst(vis.x, vis.y);
    if (rnd() < 0.3f) vis.on = false, pet::joy(3), girl.happy = 2.5f, girl.yay = true, girl.hops = 2;
    else flee();
    return;
  }
  fairy.vx = girl.dir * (50 + 40 * rnd()), fairy.vy = -(50 + 50 * rnd());
  burst(fairy.x, fairy.y);
  retarget();
}

void reachOrJump() {
  if (aimY() > GROUND - 21) go(REACH, 0.55f);  // reachable standing up
  else girl.hops = 0, go(CROUCH, 0.3f);
}

void think() {  // idle wait is over (or a visitor arrived): pick the next move
  girl.dir = side();
  if (!chasing() && rnd() < 0.15f) girl.hops = 2, girl.yay = true, girl.happy = 2, go(CROUCH, 0.15f);  // joy hop
  else if (fabsf(aimX() - girl.dir * 6 - girl.x) > 4) go(RUN, 0);
  else reachOrJump();
}

void travel(float secs);
void roveStep(float dt);
float tripX = 40, tripTop = 30;  // on a trip: her screen x and head top (the fairy keeps near)

void idleDone() { go(IDLE, (0.5f + 1.0f * rnd()) * (mood == pet::TIRED ? 2 : 1)); }  // tired: plays half as often

void update(float dt) {
  T += dt, DT = dt, girl.t += dt, girl.happy -= dt, girl.peek -= dt, girl.scare -= dt, awake -= dt, yawnT -= dt;
  if (girl.st == IDLE && mood == pet::HAPPY && !vis.on && !rove) funT -= dt, tripT -= dt;
  domeK = clampf(domeK + (wet() && !rove ? 1.6f : -1.6f) * dt, 0, 1);
  domeX += (girl.x - domeX) * fminf(1, 4 * dt);
  const int d = girl.dir;

  // Fairy: glides to its target as a damped spring. Circles above the girl's head when calm, flees in play, keeps her
  // company in the other states (rests on her head while she sleeps).
  float wob = 1;
  if (awake > 0) {
    if ((fairy.re -= dt) <= 0) retarget();
  } else {
    float hx = girl.x + 10 * sinf(T * 0.6f), hy = girl.y - 32 + 3 * sinf(T * 1.2f);
    switch (girl.st) {
      case SLEEP: hx = girl.x - d, hy = girl.y - 27, wob = 0; break;
      case SIT: case BASK: hx = girl.x + d * 9, hy = GROUND - 3, wob = 0.3f; break;
      case SHIELD: hx = girl.x + d * 8, hy = girl.y - 24, wob = 0.5f; break;  // under the dome with her
      case DANCE: hx = girl.x + 14 * cosf(T * 2.5f), hy = girl.y - 20 + 8 * sinf(T * 2.5f); break;
      case PURR: hx = girl.x + 9 * cosf(T * 5), hy = girl.y - 27 + 3 * sinf(T * 5), wob = 0.3f; break;
      case BORED: case HUNGRY: hx = girl.x + d * 9, hy = girl.y - 15 + sinf(T * 0.8f), wob = 0.3f; break;
      case WAVE: hx = girl.x + d * 13, hy = girl.y - 30; break;
    }
    if (domeK > 0.5f) hy = fmaxf(hy, girl.y - 26);              // the rain: inside the dome
    if (girl.scare > 0) hx = girl.x - d * 6, hy = girl.y - 20, wob = 0;  // thunder: hides behind her
    if (rove) hx = tripX + 13, hy = tripTop + 4 + 3 * sinf(T * 1.2f);
    fairy.tx = hx, fairy.ty = hy;
  }
  float tx = fairy.tx + wob * 2.5f * sinf(T * 2.3f), ty = fairy.ty + wob * 2 * sinf(T * 3.7f);
  fairy.vx += (6 * (tx - fairy.x) - 2.4f * fairy.vx) * dt;
  fairy.vy += (6 * (ty - fairy.y) - 2.4f * fairy.vy) * dt;
  fairy.vx += WIND_DIR * 70 * gust * dt;  // a gust pushes it off, it struggles back
  fairy.x += fairy.vx * dt, fairy.y += fairy.vy * dt;
  if (fairy.x < 10 || fairy.x > 70) fairy.x = clampf(fairy.x, 10, 70), fairy.vx *= -0.5f;
  if (fairy.y < 12 || fairy.y > 56) fairy.y = clampf(fairy.y, 12, 56), fairy.vy *= -0.5f;
  if ((fairy.emit -= dt) <= 0) {  // sparkle trail; denser when fast, rare while she sleeps
    spark(fairy.x, fairy.y + 1, 6 * rnd() - 3, 6 + 8 * rnd(), 0.6f);
    fairy.emit = girl.st == SLEEP ? 0.8f : hypotf(fairy.vx, fairy.vy) > 15 ? 0.04f : 0.12f;
  }
  for (auto &s : sparks)
    if (s.life > 0) s.life -= dt, s.x += s.vx * dt, s.y += s.vy * dt, s.vx *= 1 - 3 * dt, s.vy *= 1 - 3 * dt;

  if (vis.on) {
    vis.t += dt;
    if (vis.flee) {
      vis.x += vis.vx * dt, vis.y += vis.vy * dt;
      if (vis.x < -10 || vis.x > N + 10 || vis.y < -10) vis.on = false;
    } else {
      float sp = 25 * dt;
      vis.x += clampf(vis.home - vis.x, -sp, sp);
      if (vis.t > 12) flee();  // she never came
    }
  }
  if (rove) return roveStep(dt);
  if (mood == pet::HAPPY && girl.st == IDLE && awake <= 0 && !vis.on && !wet() && !snowing() && (visitT -= dt) <= 0)
    visit(), visitT = 120 + 180 * rnd();

  switch (girl.st) {
    case IDLE:
      if (fabsf(aimX() - girl.x) > 3) girl.dir = side();
      if (girl.t <= girl.dur) break;
      if (chasing()) {
        if (fabsf(vis.home - vis.x) < 1) think();
      } else if (awake > 0) think();
      else if (mood == pet::SLEEP) go(SLEEP, 0);
      else if (mood == pet::HUNGRY) bowl.x = clampf(girl.x + (girl.x < N / 2 ? 12 : -12), 14, 66), bowl.on = true, go(HUNGRY, 0);
      else if (mood == pet::SAD) go(BORED, 0);
      else if (mood == pet::TIRED && yawnT <= 0) yawnT = 30, go(YAWN, 1.5f);
      else if (wet()) go(SHIELD, 0);  // happy or tired: holds the dome up until the rain stops
      else if (tripT <= 0) tripT = 180 + 240 * rnd(), travel(15 + 10 * rnd());
      else if (funT <= 0) {
        float r = rnd(), w = rnd();
        funT = (snowing() ? 4 : 10) + 15 * rnd();
        if (snowing() && w < 0.75f) go(CATCH, 2.6f);
        else if (pet::wx == pet::CLEAR && sunUp() && w < 0.4f) go(BASK, 8);
        else if (r < 0.3f) go(WAVE, 2.4f);
        else if (r < 0.55f) go(SIT, 6 + 4 * rnd());
        else if (r < 0.8f) go(TWIRL, 1.6f);
        else go(DANCE, 6);
      }
      break;
    case RUN: {
      int s = side();
      float tx = clampf(aimX() - s * 6, 16, 64), dd = tx - girl.x;
      if (fabsf(dd) < 1.5f) {
        girl.dir = s;
        reachOrJump();
        break;
      }
      girl.dir = dd > 0 ? 1 : -1;
      girl.x += girl.dir * fminf(36 * dt, fabsf(dd));
      girl.phase += 14 * dt;
      if (girl.t > 3) go(IDLE, 0.3f);  // couldn't catch up: take a breather
      break;
    }
    case CROUCH:
      if (girl.t < girl.dur) break;
      if (girl.hops) leap(girl.x, 6, girl.hops - 1);
      else leap(aimX() - girl.dir * 6, clampf(GROUND - 16 - aimY(), 4, 22), 0);
      break;
    case JUMP:
      girl.vy += G * dt, girl.x += girl.vx * dt, girl.y += girl.vy * dt;
      if (!girl.yay && !girl.booped && !girl.wake && touch()) boop();
      if (girl.y < GROUND || girl.vy < 0) break;
      girl.y = GROUND;
      go(LAND, girl.hops ? 0.08f : 0.18f);
      break;
    case LAND:
      if (girl.t < girl.dur) break;
      if (girl.hops) leap(girl.x, 6, girl.hops - 1);
      else if (girl.wake) girl.wake = false, go(YAWN, 1.5f);
      else girl.yay = false, idleDone();
      break;
    case REACH:
      if (!girl.booped && girl.t > 0.15f && girl.t < 0.4f && touch()) boop();
      if (girl.t > girl.dur) girl.hops ? (girl.dir = side(), go(CROUCH, 0.15f)) : go(IDLE, 0.3f + 0.5f * rnd());
      break;
    case SLEEP:
      if (mood != pet::SLEEP) go(YAWN, 1.5f);
      break;
    case HUNGRY:
      if (mood != pet::HUNGRY) go(IDLE, 1);
      break;
    case BORED:
      if (mood != pet::SAD) go(IDLE, 1);
      break;
    case ANGRY:  // one hop back, then a stomp and a glare
      if (girl.y < GROUND || girl.vy < 0) {
        girl.vy += G * dt, girl.x = clampf(girl.x + girl.vx * dt, 16, 64), girl.y += girl.vy * dt;
        if (girl.y >= GROUND) girl.y = GROUND, girl.vy = 0;
      }
      if (girl.t > girl.dur) go(IDLE, 1);
      break;
    case TWIRL:  // spins on the spot: turns every 0.2 s
      if (girl.t < girl.dur - 0.2f) girl.dir = fmodf(girl.t, 0.4f) < 0.2f ? 1 : -1;
      // fall through
    case EAT:
    case REFUSE:
    case PURR:
    case WAVE:
    case SIT:
      if (girl.t > girl.dur) go(IDLE, 1 + rnd());
      break;
    case YAWN:
      if (girl.t > girl.dur) go(IDLE, 2 + 2 * rnd());
      break;
    case DANCE: {  // the last two beats of every 8 are a twirl
      float b = fmodf(T * 2, 8);
      if (b >= 6) girl.dir = fmodf(b, 0.5f) < 0.25f ? 1 : -1;
      if (girl.t > girl.dur) go(IDLE, 1 + rnd());
      break;
    }
    case SHIELD:
      if (!wet() || (mood != pet::HAPPY && mood != pet::TIRED) || (mood == pet::TIRED && yawnT <= 0)) go(IDLE, 1);
      break;
    case CATCH:  // a flake lands in her hand and sparkles
      if (!girl.booped && girl.t > 1.5f) girl.booped = true, girl.happy = 1, burst(girl.x + girl.dir * 7.6f, girl.y - 16);
      if (girl.t > girl.dur) go(IDLE, 1 + rnd());
      break;
    case BASK:
      if (girl.t > girl.dur || pet::wx != pet::CLEAR || !sunUp()) go(IDLE, 1 + rnd());
      break;
  }
}

// Ponytail: chain hanging from its tie point (Verlet + length constraint). Trails when running, swings when jumping.
const int NH = 6;
float hx[NH], hy[NH], hpx[NH], hpy[NH];
bool hairReady = false;
void ponytail(float ax, float ay) {
  float wind = pet::wx == pet::WIND ? WIND_DIR * (90 + 200 * gust) : 0;  // px/s²: it streams out in a gust
  if (!hairReady)
    for (int i = 0; i < NH; i++) hx[i] = hpx[i] = ax, hy[i] = hpy[i] = ay + i * 1.6f, hairReady = true;
  float jump = ax - hx[0];
  if (fabsf(jump) > 3)  // turned: tie point jumped to the other side of the head; move the hair along so it doesn't whip
    for (int i = 0; i < NH; i++) hx[i] += jump, hpx[i] += jump;
  hx[0] = ax, hy[0] = ay;
  for (int i = 1; i < NH; i++) {
    float vx = (hx[i] - hpx[i]) * 0.9f, vy = (hy[i] - hpy[i]) * 0.9f;
    hpx[i] = hx[i], hpy[i] = hy[i];
    hx[i] += vx + wind * DT * DT, hy[i] += vy + G * DT * DT;
    float dx = hx[i] - hx[i - 1], dy = hy[i] - hy[i - 1], l = sqrtf(dx * dx + dy * dy) + 1e-6f;
    hx[i] = hx[i - 1] + dx * 1.6f / l, hy[i] = hy[i - 1] + dy * 1.6f / l;
  }
  for (int i = 1; i < NH; i++) capsule(hx[i - 1], hy[i - 1], hx[i], hy[i], 1.9f - 0.15f * i, INK);
  for (int i = 1; i < NH; i++) capsule(hx[i - 1], hy[i - 1], hx[i], hy[i], 0.9f - 0.08f * i, i == 4 ? HAIRL : HAIR);
  disc(hx[1], hy[1], 0.9f, BAND);  // hair tie
}

void leg(float x0, float y0, float x1, float y1) {
  capsule(x0, y0, x1, y1, 1.6f, INK);
  capsule(x0, y0, x1, y1, 0.7f, PANTS);
}

// Music notes rising from (x, y), drifting sideways by `drift` px over their life.
void notes(float x, float y, float drift) {
  for (int k = 0; k < 3; k++) {
    float p = fmodf(T * 0.35f + k / 3.0f, 1);
    int nx = lroundf(x + (drift ? drift * p : (k - 1) * 18) + 3 * sinf(T * 2 + k * 2)), ny = lroundf(y - 14 * p);
    if (p < 0.85f || fmodf(T, 0.2f) < 0.1f) art(NOTE[k % 2], 5, nx - 2, ny, 1, INK);  // blinks out at the top
  }
}

void drawGirl() {
  const int d = girl.dir, st = girl.st;
  float hip = 5, th[2] = {0.25f, 0.25f}, fx[2] = {}, fl[2] = {};  // hip height; arm angle (0 down, π up); foot slide/lift
  // array index: 0 back side, 1 front (facing) side
  float hdx = 0, hdy = 0, tap = 0;  // head offset; sitting with legs out: feet tapping
  int eyes = -1;                    // -1: open / blink / happy as usual
  enum { STAND, KNEES, OUT } legs = STAND;  // sitting: knees up / legs stretched out in front
  int apple = -1;                              // apple in the front hand: bite stage
  bool air = st == JUMP || ((st == ANGRY || st == KO) && girl.y < GROUND - 0.5f);

  switch (st) {
    case IDLE: hip += 0.6f * sinf(T * 3), th[0] = th[1] = 0.25f + 0.06f * sinf(T * 3); break;  // breathing
    case RUN:
      hip += 0.8f * fabsf(sinf(girl.phase));
      for (int i = 0; i < 2; i++) {
        float p = girl.phase + i * 3.1416f;
        fx[i] = d * 2.2f * sinf(p), fl[i] = 1.5f * fmaxf(0, cosf(p)), th[i] = 0.45f + 0.35f * sinf(p + 3.1416f);
      }
      break;
    case CROUCH: hip = 3, th[0] = th[1] = 0.7f, fx[0] = -d, fx[1] = d; break;
    case LAND: hip = 3.5f, th[0] = th[1] = 0.6f; break;
    case JUMP: {
      bool up = girl.vy < 0;
      fl[0] = fl[1] = up ? 0 : 1.5f, fx[0] = -d * 0.8f, fx[1] = d * 0.8f;
      th[0] = 0.9f, th[1] = 2.7f;  // front arm reaches for the fairy
      break;
    }
    case REACH: {
      float t = girl.t;
      hip = 5.5f, th[0] = 0.4f;  // on tiptoe
      th[1] = t < 0.2f ? 0.25f + 2.6f * t / 0.2f : t < 0.35f ? 2.85f : fmaxf(0.25f, 2.85f - 2.6f * (t - 0.35f) / 0.2f);
      break;
    }
    case SLEEP:  // sitting, hugging her knees, head down, breathing
      legs = KNEES, hip = 1.5f, th[0] = -0.5f, th[1] = 0.55f, hdy = 2 + 0.5f * sinf(T * 3.1416f);
      eyes = girl.peek > 0 ? EYES_PEEK : EYES_CLOSED;
      break;
    case BORED:  // sitting, legs out, head down, a foot moving now and then
      legs = OUT, hip = 1.5f, th[0] = -0.3f, th[1] = 0.15f, hdy = 1.5f, tap = 1.2f * sinf(T * 1.2f), eyes = EYES_HALF;
      break;
    case SIT:  // sitting, legs out, feet tapping, leaning back on her hands
      legs = OUT, hip = 1.5f, th[0] = 0.5f, th[1] = 0.15f, tap = 1.5f * sinf(T * 5);
      if (fmodf(girl.t, 3.5f) > 2.6f) eyes = EYES_HAPPY;
      break;
    case HUNGRY:  // hands on her belly next to the empty plate, now and then looking at it
      th[0] = -0.55f, th[1] = -0.3f, hip += 0.3f * sinf(T * 2);
      if (fmodf(girl.t, 4) > 3.0f) hdy = 1, eyes = EYES_HALF;
      break;
    case EAT: {  // an apple, bite by bite
      float t = girl.t;
      th[1] = 2.5f + 0.1f * sinf(T * 12), apple = t < 0.7f ? 0 : t < 1.5f ? 1 : t < 2.3f ? 2 : 3;
      eyes = fmodf(t, 0.3f) < 0.15f ? EYES_HAPPY : EYES_CLOSED;  // chewing
      break;
    }
    case REFUSE:  // head shake, hand up: no thanks
      hdx = 1.2f * sinf(girl.t * 20), th[1] = 1.6f + 0.2f * sinf(girl.t * 20), eyes = EYES_BLINK;
      break;
    case PURR:  // petted: hands to her cheeks, giggling
      th[0] = -2.5f, th[1] = 2.5f, hip += 0.5f * fabsf(sinf(T * 8)), eyes = EYES_HAPPY;
      break;
    case YAWN: {  // stretch, both arms up, mouth wide
      float k = fminf(1, 1.5f * sinf(fminf(girl.t / girl.dur, 1) * 3.1416f));
      hip = 5 + 0.8f * k, th[0] = th[1] = 0.25f + 2.6f * k;
      if (girl.t > 0.25f && girl.t < 1.25f) eyes = EYES_YAWN;
      break;
    }
    case ANGRY:  // fists down and back, frown
      th[0] = th[1] = -0.35f, eyes = EYES_FROWN;
      if (!air) hip = 4.5f;
      break;
    case KO:  // game over: tumbles down, then sits dazed with stars
      eyes = EYES_DIZZY;
      if (air) th[0] = 2.4f, th[1] = 2.0f, fl[0] = fl[1] = 1.5f;
      else legs = KNEES, hip = 1.5f, th[0] = -0.5f, th[1] = 0.5f, hdy = 1;
      break;
    case WAVE:  // waves at you
      th[1] = 2.7f + 0.35f * sinf(girl.t * 14), eyes = EYES_HAPPY;
      break;
    case TWIRL:
      th[0] = th[1] = 1.5f, hip += 0.6f * fabsf(sinf(girl.t * 15)), eyes = EYES_HAPPY;
      break;
    case SHIELD:  // a hand up to the dome, now and then looking up at it
      th[1] = 2.55f + 0.1f * sinf(T * 3), th[0] = 0.3f;
      if (fmodf(girl.t, 5) > 3.8f) hdy = -1;
      break;
    case CATCH: {  // arms up taking turns, face up, glad once she has one
      float k = sinf(girl.t * 5);
      th[0] = 2.3f + 0.35f * k, th[1] = 2.3f - 0.35f * k, hip += 0.4f * fabsf(k), hdy = -1;
      eyes = girl.t > 1.5f ? EYES_HAPPY : EYES_OPEN;
      break;
    }
    case BASK:  // sitting in the sun, leaning back on her hands, eyes shut
      legs = OUT, hip = 1.5f, th[0] = 0.55f, th[1] = 0.15f, tap = 0.8f * sinf(T * 1.5f), hdy = -1, eyes = EYES_HAPPY;
      break;
    case DANCE: {  // to a 120 bpm beat: arms take turns, then both up and side steps, then a twirl (update())
      float b = T * 2, ph = fmodf(b, 1), bounce = fabsf(sinf(3.1416f * ph));
      int beat = (int)b % 8;
      hip = 4.4f + 0.9f * bounce;
      if (beat < 4) th[beat % 2] = 2.6f, th[1 - beat % 2] = 0.5f, fl[beat % 2] = 1.5f * bounce;
      else if (beat < 6) th[0] = th[1] = 2.5f + 0.3f * sinf(T * 12), fx[0] = fx[1] = (beat % 2 ? 1.5f : -1.5f) * bounce;
      else th[0] = th[1] = 1.5f;
      eyes = beat % 2 ? EYES_HAPPY : -1;
      break;
    }
  }
  if (girl.yay) th[0] = th[1] = 2.6f + 0.25f * sinf(T * 18);
  bool scared = girl.scare > 0 && !air && st != SLEEP;
  if (scared) th[0] = th[1] = 2.85f, hip -= 0.8f, eyes = EYES_WIDE;  // thunder: hands to her head, ducking, shivering
  if (gust > 0.6f && eyes < 0 && girl.happy <= 0) eyes = EYES_HALF;  // squints into a gust

  int cx = lroundf(girl.x + (scared ? 0.6f * sinf(T * 60) : 0)), hy = lroundf(girl.y - hip), ty = hy - 8;  // hip, torso top
  int hx0 = cx + lroundf(hdx), ky = ty - 16 + lroundf(hdy);             // head
  shadow(cx, 7 * fmaxf(0.3f, 1 - (GROUND - girl.y) / 30));
  ponytail(hx0 - d * 7.0f, ky + 14.5f);
  for (int i = 0; i < 2 && legs == STAND; i++) {
    float s = i ? d : -d;
    float lx = cx + s * 2, fx1 = lx + fx[i], fy1 = girl.y - 1 - fl[i];
    leg(lx, hy, fx1, fy1);
    disc(fx1 + d * 0.5f, fy1 + 0.2f, 0.8f, BELT);  // boot
  }
  blit(TORSO, TORSO_H, TORSO_AX, cx, ty, d);
  for (int i = 0; i < 2 && legs != STAND; i++) {  // sitting: legs in front of the body
    float o = i ? 0 : -1.2f, fy = girl.y - 1;
    if (legs == KNEES) {
      float kx = cx + d * (4 + o), kyy = hy - 3.5f;
      leg(cx + d * o, hy, kx, kyy), leg(kx, kyy, cx + d * (7 + o), fy);
    } else {
      fy -= fmaxf(0, i ? tap : -tap);
      leg(cx + d * o, hy, cx + d * (5 + o), girl.y - 1.5f), leg(cx + d * (5 + o), girl.y - 1.5f, cx + d * (8.5f + o), fy);
    }
    disc(cx + d * ((legs == KNEES ? 7 : 8.5f) + o + 0.5f), fy + 0.2f, 0.8f, BELT);
  }
  if (eyes < 0) eyes = girl.happy > 0 ? EYES_HAPPY : fmodf(T, 3.1f) < 0.14f ? EYES_BLINK : EYES_OPEN;
  blit(HEAD[eyes], HEAD_H, HEAD_AX, hx0, ky, d);
  for (int i = 0; i < 2; i++) {  // arms after the head: stay in front of the hair when raised
    float s = i ? d : -d, sx = cx + s * 4.5f, sy = ty + 2.5f;
    float ex = sx + s * 6 * sinf(th[i]), ey = sy + 6 * cosf(th[i]), mx = sx + (ex - sx) * 0.35f, my = sy + (ey - sy) * 0.35f;
    capsule(sx, sy, ex, ey, 1.5f, INK), disc(ex, ey, 1.6f, INK);
    capsule(sx, sy, mx, my, 0.8f, SHIRT), capsule(mx, my, ex, ey, 0.8f, SKIN), disc(ex, ey, 0.8f, SKIN);
    px(sx + (ex - sx) * 0.75f, sy + (ey - sy) * 0.75f, BAND);  // gold bracelet
    if (i && apple >= 0) art(APPLE_ART[apple], 5, lroundf(ex) - 2, lroundf(ey) - 4, d);
  }

  if (st == SLEEP && girl.peek <= 0)  // z z z floating up
    for (int k = 0; k < 3; k++) {
      float p = fmodf(T * 0.3f + k / 3.0f, 1);
      art(ZZ, 4, lroundf(hx0 + d * (4 + 9 * p)) - 2, lroundf(ky + 1 - 11 * p), 1, nightW > 0.5f ? CREAM : INK);
    }
  if (st == PURR)  // little hearts rising
    for (int k = 0; k < 3; k++) {
      float p = fmodf(girl.t * 0.9f + k / 3.0f, 1);
      art(LOVE, 4, lroundf(hx0 + d * (2 + 4 * k) + sinf(T * 4 + k)) - 2, lroundf(ky + 2 - 9 * p));
    }
  if (st == DANCE) notes(girl.x, GROUND - 22, 0);
  if (st == KO && !air)  // stars around her head
    for (int k = 0; k < 2; k++) {
      float a = T * 6 + k * 3.1416f;
      art(TWINKLE, 3, lroundf(hx0 - d * 1 + 7 * cosf(a)) - 1, lroundf(ky + 2 + 1.5f * sinf(a)) - 1, 1, BAND);
    }
}

void drawFairy() {
  bool sleep = girl.st == SLEEP && awake <= 0;  // the fairy dozes too: wings still, glow pulsing slowly
  int x = lroundf(fairy.x), y = lroundf(fairy.y), up = !sleep && fmodf(T, 0.12f) < 0.06f ? -1 : 0;  // wing flap
  for (int s = -1; s <= 1; s += 2) px(x + s, y + up, WING), px(x + 2 * s, y + up, WING), px(x + s, y + up + 1, WING);
  px(x, y, sleep && sinf(T * 1.5f) < 0 ? WING : GLOW), px(x, y + 1, FAIRY);
  for (auto &s : sparks)
    if (s.life > 0) px(s.x, s.y, s.life > 0.3f ? GLOW : fmodf(s.life * 20, 2) < 1 ? WING : AUR2);
}

void drawVisitor() {
  if (!vis.on) return;
  int x = lroundf(vis.x), y = lroundf(vis.y + 2 * sinf(T * 3));
  if (vis.kind == BUTTERFLY) art(FLY[sinf(T * 20) > 0], 5, x - 3, y - 2);
  else if (vis.kind == BLOWN) art(LEAF_ART[sinf(T * 9) > 0], 3, x - 2, y - 1);
  else {  // firefly: wings and a glowing tail that pulses
    px(x - 1, y - 1, WING), px(x + 1, y - 1, WING), px(x, y, INK);
    px(x, y + 1, sinf(T * 5) > -0.3f ? LAMP : BELT);
  }
}

void drawDome() {  // a shimmering water outline with gaps running along it and a glint; rain splashes on it
  if (domeK <= 0 || rove) return;
  for (int k = 0; k <= 96; k++) {
    float a = k * 3.1416f / 96, x = domeX + DOME_RX * cosf(a), y = GROUND - DOME_RY * domeK * sinf(a);
    if ((k + int(T * 10)) % 5) px(x, y, sinf(a * 3 - T * 1.5f) > 0.85f ? CREAM : WING);
  }
}

void drawSnowCover() {  // on the wall top, caps on the roofs (a trip: drawRove snows its ground)
  if (snowCover <= 0) return;
  for (int y = 1; y < N; y++)
    for (int x = 0; x < N; x++) {
      uint8_t &c = fb[y * N + x];
      bool cap = (c == ROOF || c == ROOF2) && airy(fb[(y - 1) * N + x]);
      if ((c == WALL || cap) && h01(x * 977 + y * 131) < snowCover * 1.1f) c = FLAKE;
    }
}

// Pavi's game is the trip below, steered by tilting (not game.h's runner); it keeps its score in the shared runner state.
using pet::Game;
Game &game = pet::runner;

// ---- Travel: a road into the screen, seen from behind her. Row y below the horizon is depth z = FOC / (y - HZ) units
// (the camera is 1 unit up); a point (wx, z) lands on x = 40 + (wx - camX) * FOC / z. The road changes every SEG units
// (earth, water = a river, air, fire) and she bends with whatever she's on. Rocks, trees and ruined pillars stand in
// the lanes; she side-steps them (in the game: you steer), the camera follows her partway, so the city and the hills
// shift less than the road (parallax). Trees along the verges just rush past. High horizon, long lens, her low on the
// screen: her head ends below the horizon, so the road ahead of her shows from ~5 units out. ----
const int HZ = 26;                                            // horizon row
const float FOC = 50, ZP = 1.2f, LANE = 0.8f, SPEED = 7, SEG = 35;  // focal px, her depth, lane width, units/s, element stretch
enum { EARTH, WATER, AIR, FIRE };
enum { BOULDER, TREE, PILLAR };
struct Thing {
  float w, wx;  // world: forward, sideways
  uint8_t kind;
};
const int NOB = 3, NSIDE = 6;
Thing obs[NOB], verge[NSIDE];  // in the lanes (dodged), on the verges (scenery)
float dist = 0, camX = 0, pX = 0, pV = 0, hopY = 0, hopV = 0;  // travelled; camera, her x and speed sideways; hop
int lane = 0, elem = EARTH;

struct Bit {  // spray, dust, embers, wisps
  float x, y, vx, vy, life;
  uint8_t c;
};
const int NB = 28;
Bit bits[NB];
void puff(float x, float y, float vx, float vy, float life, uint8_t c) {
  for (auto &b : bits)
    if (b.life <= 0) {
      b = {x, y, vx, vy, life, c};
      return;
    }
}

int elemAt(float w) { return int(w / SEG) & 3; }
float landRow() { return rove ? HZ + 6 + (N - HZ - 6) * rnd() : GROUND - 1 + 2.5f * rnd(); }  // the road, or the wall top
void obNew(Thing &o, float after) { o = {after + 6 + 6 * rnd(), (int(rnd() * 3) - 1) * LANE, uint8_t(rnd() * 3)}; }
void sideNew(Thing &o, float after) {
  o = {after + 1.5f + 3 * rnd(), (rnd() < 0.5f ? -1 : 1) * (2.2f + 2.5f * rnd()), uint8_t(rnd() < 0.8f ? TREE : PILLAR)};
}

void travel(float secs) {  // secs: how long; 0 = while the music plays
  tripLeft = secs;
  if (rove) return;
  pose(IDLE, 0);
  rove = true, bg = bgRoad, vis.on = false, awake = 0;
  dist = SEG * int(4 * rnd()) + 2, camX = pX = pV = hopY = hopV = 0, lane = 0, elem = elemAt(dist + ZP);
  float far = dist + 8;
  for (auto &o : obs) obNew(o, far), far = o.w;
  far = dist;
  for (auto &o : verge) sideNew(o, far), far = o.w;
}

void tripHop(float v) {
  if (hopY == 0) hopV = -v;
}

void roveStep(float dt) {
  dist += (game.on ? game.speed : SPEED) * dt;
  float me = dist + ZP, farOb = 0, farSide = 0;
  for (auto &o : obs) farOb = fmaxf(farOb, o.w);
  for (auto &o : verge) farSide = fmaxf(farSide, o.w);
  for (auto &o : obs)
    if (o.w < dist + 0.5f) {  // behind her: passed (a point in the game), a new one far ahead
      if (game.on) game.score++;
      obNew(o, farOb), farOb = o.w;
    }
  for (auto &o : verge)
    if (o.w < dist + 0.5f) sideNew(o, farSide), farSide = o.w;

  auto blocked = [&](int l) {  // something in lane l within 6 units ahead
    for (auto &o : obs)
      if (fabsf(o.wx - l * LANE) < 0.1f && o.w > me - 0.3f && o.w < me + 6) return true;
    return false;
  };
  if (!game.on && blocked(lane)) {  // side-step to a free lane, the near one first (the game: gameTilt picks it)
    int a = lane == 0 ? (rnd() < 0.5f ? -1 : 1) : 0, b = lane == 0 ? -a : -lane;
    if (!blocked(a)) lane = a;
    else if (!blocked(b)) lane = b;
  }
  float to = lane * LANE - pX;  // glide sideways, easing in and out
  pV += (clampf(to * (game.on ? 10 : 6), -(game.on ? 5 : 3.2f), game.on ? 5 : 3.2f) - pV) * fminf(1, 10 * dt);  // the game: a snappier lane change
  pX += pV * dt;
  camX += (0.6f * pX - camX) * fminf(1, 3 * dt);  // 0.6: a side lane stays inside the round screen
  if (hopY < 0 || hopV < 0) {
    hopV += G * dt, hopY += hopV * dt;
    if (hopY >= 0) hopY = hopV = 0;
  }

  int e = elemAt(me);
  if (e != elem) {  // a new stretch: a burst in its color, and a joy hop
    elem = e, girl.happy = 1;
    static const uint8_t C[4] = {CLAY, WING, CREAM, FLAME};
    for (int i = 0; i < 12; i++) {
      float a = i * 0.524f;
      puff(tripX, tripTop + 26, 35 * cosf(a), 25 * sinf(a), 0.5f, C[e]);
    }
    tripHop(55);
  }
  for (auto &b : bits)
    if (b.life > 0) b.life -= dt, b.x += b.vx * dt, b.y += b.vy * dt, b.vy += (b.c == WING ? 120 : 0) * dt;
  if (!game.on && !tunes && (tripLeft -= dt) <= 0) pose(IDLE, 1);
}

// Pavi from behind: hair, band and ponytail; the cloak over her back.
const char *const BACK[17] = {
    ".......KK.......", "......KHHK......", "....KKHHHHKK....", "...KHHHHLHHHK...", "..KHHHHHHLHHHK..",
    ".KHHHHHHHHHHHHK.", ".KYYYYYYYYYYYYK.", "KHHHHHHHHHHHHHHK", "KHHLHHHHHHHHHHHK", "KHHHHHHHHHHHHHHK",
    "KhHHHHHHHHHHHHhK", "KhHHHHHHHHHHHHhK", ".KhHHHHHHHHHHhK.", ".KhhHHHHHHHHhhK.", "..KhhHHHHHHhhK..",
    "...KKhhhhhhKK...", ".....KKKKKK.....",
};
const char *const BACK_TORSO[9] = {
    "....KSSK....", "..KKCCCCKK..", ".KCCCCCCCCK.", "KCcCCCCCCcCK", "KCcCCcCCCcCK",
    "KCcBBBBBBcCK", ".KcPPPPPPcK.", ".KcPPPPPPcK.", ".KKKKKKKKKK.",
};

void oval(float cx, float cy, float rx, float ry, uint8_t c) {
  for (int y = floorf(cy - ry); y <= cy + ry; y++)
    for (int x = floorf(cx - rx); x <= cx + rx; x++) {
      float dx = (x + 0.5f - cx) / rx, dy = (y + 0.5f - cy) / ry;
      if (dx * dx + dy * dy <= 1) px(x, y, c);
    }
}

void drawThing(const Thing &o) {
  float z = o.w - dist;
  if (z < 0.5f || z > 30) return;
  float sc = FOC / z, gx = 40 + (o.wx - camX) * sc, gy = HZ + sc;  // px per unit, foot of it on screen
  if (gx < -sc || gx > N + sc) return;
  if (o.kind == BOULDER) {
    float r = 0.3f * sc;
    disc(gx, gy - r * 0.8f, r + 0.8f, INK), disc(gx, gy - r * 0.8f, r, GREY);
    disc(gx - r * 0.35f, gy - r * 1.15f, r * 0.3f, CREAM);
  } else if (o.kind == TREE) {
    float r = 0.42f * sc, t = fmaxf(0.4f, 0.07f * sc);
    capsule(gx, gy, gx, gy - 0.6f * sc, t + 0.8f, INK), capsule(gx, gy, gx, gy - 0.6f * sc, t, BELT);
    disc(gx, gy - 0.95f * sc, r + 0.8f, INK), disc(gx, gy - 0.95f * sc, r, GRASS2);
    disc(gx - r * 0.3f, gy - 0.95f * sc - r * 0.3f, r * 0.45f, GRASS);
  } else {
    float r = fmaxf(0.4f, 0.13f * sc), top = gy - 1.0f * sc;
    capsule(gx, gy, gx, top, r + 0.8f, INK), capsule(gx, gy, gx, top, r, WALL);
    capsule(gx - r * 1.4f, top, gx + r * 1.4f, top, fmaxf(0.5f, 0.06f * sc) + 0.8f, INK);  // capital
    capsule(gx - r * 1.4f, top, gx + r * 1.4f, top, fmaxf(0.5f, 0.06f * sc), WALL2);
  }
}

void drawBack(float cx, float fy) {  // her back, feet on row fy, bending with `elem`
  float hip = 5, th[2] = {0.4f, 0.4f}, spread = 3, lean = clampf(pV * 0.8f, -1.5f, 1.5f);  // arm angle out from down
  bool sit = false;
  for (int i = 0; i < 2; i++) {
    float p = T * 3 + i * 3.1416f;
    switch (elem) {
      case EARTH: spread = 4, th[i] = 0.55f + 0.3f * sinf(T * 6 + i * 3.1416f); break;  // pushing the slab on
      case WATER: hip = 4.5f, th[i] = 1.1f + 0.6f * sinf(p); break;                     // arms flowing with the wave
      case AIR: sit = true, hip = 1.5f, th[i] = 1.35f + 0.15f * sinf(T * 4); break;     // balancing on the air ball
      case FIRE: spread = 1.5f, th[i] = 0.3f; break;                                    // jets from feet and fists
    }
  }
  int x = lroundf(cx), hy = lroundf(fy - hip), ty = hy - 8, ky = ty - 16, hx = x + lroundf(lean);
  for (int i = -1; i <= 1; i += 2) {  // legs
    if (sit) leg(x + i * 2, hy, x + i * 5.5f, hy + 1), leg(x + i * 5.5f, hy + 1, x + i * 1.5f, hy + 2);  // cross-legged
    else leg(x + i * 2, hy, x + i * spread, fy - 1), disc(x + i * spread, fy - 0.8f, 0.8f, BELT);
  }
  blit(BACK_TORSO, 9, 6, x, ty, 1);
  blit(BACK, 17, 8, hx, ky, 1);
  ponytail(hx, ky + 11.5f);  // hangs over her cloak
  for (int i = 0; i < 2; i++) {
    float s = i ? 1 : -1, sx = x + s * 4.5f, sy = ty + 2.5f;
    float ex = sx + s * 6 * sinf(th[i]), ey = sy + 6 * cosf(th[i]), mx = sx + (ex - sx) * 0.35f, my = sy + (ey - sy) * 0.35f;
    capsule(sx, sy, ex, ey, 1.5f, INK), disc(ex, ey, 1.6f, INK);
    capsule(sx, sy, mx, my, 0.8f, SHIRT), capsule(mx, my, ex, ey, 0.8f, SKIN), disc(ex, ey, 0.8f, SKIN);
    if (elem == FIRE) {  // fists push flame down and back
      float f = 2 + 2 * rnd();
      capsule(ex, ey + 1, ex + s, ey + 1 + f, 1.0f, FLAME), px(ex, ey + 1.5f, FLAME2);
    }
  }
  tripX = cx, tripTop = ky;
}

void drawRove() {
  drawSky();  // sky down to the horizon (bgRoad), sun and moon rise from it
  for (int x = 0; x < N; x++) {  // far: the havens' towers, lamps lit at night; near: low hills. Each shifts with the camera
    float u = x + camX * 3 + 200, v = x + camX * 9 + 500;
    int b = floorf(u / 4), tall = 3 + 11 * h01(b) * h01(b), hill = HZ - 3 - 2 * sinf(v * 0.11f) - 1.5f * sinf(v * 0.27f + 1);
    if (h01(b + 77) < 0.35f) tall = 0;  // a gap in the skyline
    for (int y = HZ - tall; y < HZ; y++) fb[y * N + x] = y >= hill ? HILL : ROOF;
    for (int y = hill; y < HZ; y++) fb[y * N + x] = HILL;
    if (tall > 6 && fmodf(u, 4) < 1 && (int)(HZ - tall + 3) < hill) px(x, HZ - tall + 3, LAMP);  // a window
  }
  const float ripple = fmodf(T * 2, 6.2831853f);  // wrapped: sinf of a big angle (T grows from boot) takes a slow reduction path, per pixel
  for (int y = HZ; y < N; y++) {  // the ground: stripes and the road (or river) rushing at her, haze far off
    float z = FOC / (y - HZ + 0.5f), w = dist + z;
    bool stripe = fmodf(w, 1.6f) < 0.8f;
    for (int x = 0; x < N; x++) {
      float wx = camX + (x + 0.5f - 40) * z / FOC, aw = fabsf(wx);
      uint8_t c;
      if (aw < 1.25f) {
        if (elemAt(w) == WATER) c = fmodf(w * 1.3f + 0.25f * sinf(wx * 5 + ripple), 1) < 0.2f ? RIVER2 : RIVER;
        else c = h01(int(w * 3) * 131 + int(floorf(wx * 5))) < 0.18f ? ROAD2 : ROAD;
      } else c = aw < 1.4f ? ROAD2 : stripe ? GRASS : GRASS2;
      float snow = game.on || c == RIVER || c == RIVER2 ? 0 : snowCover * (aw < 1.4f ? 0.5f : 0.9f);  // less on the road
      if (snow > 0 && h01(int(w * 4) * 7919u + int(floorf(wx * 6)) * 31u) < snow) c = FLAKE;  // snow, fixed to the ground
      if (z > 14 && (z - 14) / 10 > bayer(x, y)) c = HAZE;
      fb[y * N + x] = c;
    }
  }
  if (!game.on) drawFlash(), drawDrops();  // no weather in the game
  const Thing *all[NOB + NSIDE];  // far to near, she goes in at her depth
  int n = 0;
  for (auto &o : obs) all[n++] = &o;
  for (auto &o : verge) all[n++] = &o;
  for (int i = 1; i < n; i++)
    for (int j = i; j > 0 && all[j]->w > all[j - 1]->w; j--) {
      const Thing *t = all[j];
      all[j] = all[j - 1], all[j - 1] = t;
    }
  int i = 0;
  for (; i < n && all[i]->w - dist > ZP; i++) drawThing(*all[i]);

  float cx = 40 + (pX - camX) * FOC / ZP, gy = HZ + FOC / ZP, fy = gy - 3 + hopY;  // on the vehicle
  if (elem == AIR || elem == FIRE) capsule(cx - 5, gy, cx + 5, gy, 1, ROAD2);  // floating: her shadow on the road
  switch (elem) {
    case EARTH:  // a rock slab skimming the ground, dust off its back corners
      fy = gy - 3 + hopY;
      capsule(cx - 8, fy + 1, cx + 8, fy + 1, 2.2f, INK), capsule(cx - 7.5f, fy + 0.6f, cx + 7.5f, fy + 0.6f, 1.3f, CLAY);
      if (rnd() < 0.5f) puff(cx + (rnd() < 0.5f ? -8 : 8), fy + 2, 30 * rnd() - 15, 15 + 15 * rnd(), 0.35f, rnd() < 0.5f ? CLAY : GREY);
      break;
    case WATER:  // riding a wave: blue mound, light rim, foam on top, spray
      fy = gy - 4 + hopY;
      oval(cx, fy + 3, 11, 4.5f, INK), oval(cx, fy + 3, 10, 3.6f, FAIRY), oval(cx, fy + 2.2f, 8, 2, WING);
      for (int k = -7; k <= 7; k += 2) px(cx + k + (int(T * 12) & 1), fy + 0.5f + 0.5f * sinf(T * 9 + k), CREAM);
      if (rnd() < 0.6f) puff(cx + (rnd() < 0.5f ? -9 : 9), fy + 1, 40 * rnd() - 20, -30 - 20 * rnd(), 0.5f, WING);
      break;
    case AIR:  // sitting on a spinning ball of air, floating
      fy = gy - 9 + hopY + 1.2f * sinf(T * 3);
      disc(cx, fy + 3, 6, WING);
      for (int k = 0; k < 3; k++)
        for (int j = 0; j < 7; j++) {
          float a = T * 9 + k * 2.1f + j * 0.28f, r = 5.5f - j * 0.5f;
          px(cx + r * cosf(a), fy + 3 + r * 0.75f * sinf(a), CREAM);
        }
      if (rnd() < 0.3f) puff(cx + 12 * rnd() - 6, fy + 6, 0, 20, 0.3f, CREAM);
      break;
    case FIRE:  // floating on flame jets from her feet, embers falling behind
      fy = gy - 7 + hopY + 0.8f * sinf(T * 7);
      for (int s = -1; s <= 1; s += 2) {
        float f = 3 + 3 * rnd();
        capsule(cx + s * 1.5f, fy, cx + s * 2, fy + f, 1.6f, FLAME), capsule(cx + s * 1.5f, fy, cx + s * 1.7f, fy + f * 0.6f, 0.8f, FLAME2);
      }
      if (rnd() < 0.6f) puff(cx + 4 * rnd() - 2, fy + 5, 30 * rnd() - 15, 20 + 20 * rnd(), 0.35f, rnd() < 0.5f ? FLAME : FLAME2);
      break;
  }
  drawBack(cx, fy);
  if (tunes) notes(cx + 10, fy - 30, 12);
  for (; i < n; i++) drawThing(*all[i]);
  for (auto &b : bits)
    if (b.life > 0) px(b.x, b.y, b.c);
  if (!game.on) drawLeaves();
}

// ---- Game: the trip, steered. Tilt the keychain left / right for that lane, hold it level for the middle one.
// Whatever stands in her lane comes at her: one hit and it's over. Score = obstacles passed; it speeds up as it goes.
// The firmware feeds the sideways tilt (gameTilt), restarts after a game over and reads `game.ended`. ----
const float GSPEED0 = 4, GSPEED_UP = 0.1f, GSPEED_MAX = 12;  // units/s; +GSPEED_UP every second played
const float TILT_IN = 0.17f, TILT_OUT = 0.09f;  // g sideways: past IN = side lane (~10°), back under OUT = middle

void finish() {  // stats and the event for the firmware
  game.newBest = pet::gameOver(game.score);
  game.ended = true;
}

void gameStart() {
  game = Game();
  game.on = true, game.speed = GSPEED0;
  endTrip();  // a fresh road, even from the middle of a trip
  travel(0);
}

void gameStop() {  // left with a long hold, or by find
  if (game.on && !game.dead && game.score > 0) finish();
  game.on = false;
  pose(IDLE, 1);
}

void gameInput(bool) {}  // the button isn't used here: the tilt steers

void gameTilt(float g) {  // sideways gravity in g, + = tilted right; hysteresis so a wobble doesn't flip lanes
  if (game.on && !game.dead) lane = g > (lane == 1 ? TILT_OUT : TILT_IN) ? 1 : g < -(lane == -1 ? TILT_OUT : TILT_IN) ? -1 : 0;
}

void gameStep(float dt) {
  T += dt, game.t += dt;
  if (game.dead) {  // knocked to a stop: the dust settles; the firmware restarts on a press, gives up after 10 s
    roveStep(dt);
    if (game.t > 10) game.on = false, pose(IDLE, 1);
    return;
  }
  if (!game.fixedSpeed) game.speed = fminf(GSPEED0 + GSPEED_UP * game.t, GSPEED_MAX);
  float me0 = dist + ZP;
  roveStep(dt);
  for (auto &o : obs)  // swept over this frame's travel: at top speed a frame moves her farther than an obstacle is deep
    if (o.w > me0 - 0.25f && o.w < dist + ZP + 0.25f && fabsf(o.wx - pX) < 0.45f) {
      game.dead = true, game.t = 0, game.speed = 0, girl.happy = 0;
      dist -= 2.5f;  // bounced back: what she hit stands in front of her instead of filling the screen
      tripHop(70);
      for (int i = 0; i < 14; i++) {
        float a = i * 0.449f;
        puff(tripX, tripTop + 26, 40 * cosf(a), 30 * sinf(a), 0.6f, i & 1 ? GREY : CREAM);
      }
      finish();
      return;
    }
}

void gameDraw() { drawRove(); }

// Bubble text: score and high score while running, the result after a game over.
void gameLabel(char *out, int cap) {
  int best = pet::best() > game.score ? pet::best() : game.score;
  if (!game.dead) snprintf(out, cap, "%d HI %d", game.score, best);
  else if (game.newBest) snprintf(out, cap, "NEW BEST %d", game.score);
  else snprintf(out, cap, "%d BEST %d", game.score, best);
}

// ---- Events from the firmware ----

// Device shaken: the fairy startles and darts off, the girl jumps and chases it for a few seconds.
// Asleep: a startled hop, a yawn, back to sleep. Starving: angry, no play.
void shake() {
  if (game.on) return;
  fairy.vx += (rnd() < 0.5f ? -1 : 1) * (60 + 60 * rnd()), fairy.vy -= 40 + 50 * rnd();
  retarget();
  if (girl.st == SLEEP) {
    awake = 0, girl.peek = 0, leap(girl.x, 6, 0), girl.wake = true;
    return;
  }
  if (pet::s.full < 10 && grounded()) {
    awake = 0, pose(ANGRY, 1.2f);
    girl.vy = -sqrtf(2 * G * 5), girl.vx = -girl.dir * 22;
    return;
  }
  if (rove) return tripHop(80);  // on a trip: a big hop, on she goes
  awake = 6;
  if (grounded()) leap(girl.x, 6, 0);
}

// Message/button: the girl raises her arms and hops twice for joy. Asleep: one eye opens for 2 s.
void cheer() {
  if (game.on) return;
  if (girl.st == SLEEP) {
    girl.peek = 2;
    return;
  }
  girl.happy = 2.5f;
  if (rove) tripHop(60);
  else if (grounded() && girl.st != EAT) pose(CROUCH, 0.15f), girl.hops = 2, girl.yay = true;
}

// Pet events (nothing during the game): feed accepted (also wakes her), feed while full, petted, too tired to play.
void eat() {
  if (!game.on) pose(EAT, 3);
}
void refuse() {
  if (!game.on) pose(REFUSE, 1);
}
void purr() {
  if (!game.on) pose(PURR, 2);
}
void yawn() {
  if (!game.on) pose(YAWN, 1.5f);
}

// Music screen on / off (the firmware calls this every frame): she sets off on a trip; asleep, she sleeps on.
void music(bool on) {
  if (game.on) on = false;
  if (on == tunes) return;
  tunes = on;
  if (on && girl.st != SLEEP && mood != pet::SLEEP) travel(0);
  else if (!on && rove && tripLeft <= 0 && !game.on) pose(IDLE, 1);
}

void init() {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) fb[y * N + x] = y < GROUND ? skyAt(x, y) : uint8_t(y < GROUND + 2 ? WALL : WALL2);
  for (int y = GROUND + 2; y < N; y++)  // wall stones: mortar line every 5 rows, staggered per course
    for (int x = 0; x < N; x++)
      if ((y - GROUND - 2) % 5 == 0 || (x + ((y - GROUND - 2) / 5 % 2) * 4) % 8 == 0) px(x, y, SHADOW);
  for (int y = 36; y < GROUND; y++)  // distant roofs: pointed roof left, mast middle, dome + tower right
    for (int x = 0; x < N; x++) {
      bool peak = y > 38 + fabsf(x - 14) * 1.1f && x < 28;
      bool mast = y > 40 && (x == 47 || (y > 50 && x > 41 && x < 54));
      bool dome = hypotf(x + 0.5f - 66, y + 0.5f - 52) < 8 || (x == 66 && y > 39) || (y > 50 && x > 56);
      if (peak || mast || dome) px(x, y, peak || dome ? ROOF : ROOF2);
    }
  px(14, 47, LAMP), px(10, 51, LAMP), px(63, 53, LAMP), px(68, 53, LAMP);  // window lights
  memcpy(bgCity, fb, sizeof bgCity);
  for (int y = 0; y < N; y++)  // the trip's sky ends at its horizon
    for (int x = 0; x < N; x++) bgRoad[y * N + x] = y < HZ ? SKY0 + ramp(x, y, HZ) : ROAD;
  for (int i = 0; i < NS; i++) sx[i] = 10 + rnd() * 60, sy[i] = 3 + rnd() * 30, sp[i] = rnd() * 6.28f;
}

void step(float dt) {
  if (game.on) {
    gameStep(dt);
    gameDraw();
    return;
  }
  update(dt);
  weatherStep(dt);
  if (rove) {
    drawRove();
    drawFairy();
    return;
  }
  drawSky();
  drawFlash();
  drawSnowCover();
  drawDrops();
  if (bowl.on) art(PLATE, 3, lroundf(bowl.x) - 4, GROUND - 3);
  drawGirl();
  drawVisitor();
  drawFairy();
  drawDome();
  drawLeaves();
}

}  // namespace pavi
#ifndef SCENE_PICK
namespace scene = pavi;  // firmware.ino talks to whichever scene is included through this name (pets.h: to both)
#endif
#define SCENE_PET        // firmware.ino: this scene has the pet features (pet.h, game, day/night)
