// Nausicaa (Valley of the Wind) and Teto, the fox-squirrel on her shoulder: draws into an 80x80 color-indexed buffer
// (fb), scaled 3x on screen. No Arduino dependency: also compiles on a PC for frame-by-frame testing (interfaces/watch/test).
// A pet like the cat and Pavi (pet.h): her mood picks what she does, the windy valley follows the clock (tint), and her
// game is an air fight on the Mehve, seen from above (gameTilt + button, below). Shake: Teto jumps off, she chases him,
// then kneels and he climbs back.
// Travel (travel()): she flies her glider (the Mehve) over the valley, the layers scroll by depth (parallax). The music
// screen shows it; now and then she also takes off on her own.
#pragma once
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifndef PET_NAME
#define PET_NAME "Nausicaa"  // alone; with the others (pets.h) the pet keeps the cat's default name
#endif
#include "pet.h"

namespace nausicaa {

const int N = 80, GROUND = 58;  // logical resolution, ground row (feet stand here)
const float G = 260;            // gravity, px/s²

enum : uint8_t {
  SKY0, SKY1, SKY2, SKY3, CLOUD, CLOUD2, HILL, HILL2, MILL, GRASS, GRASS2, GRASSL, WIND, STAR, SUN, MOON, SHADOW, TREE,
  INK, SKIN, HAIR, HAIR2, HAIRL, EYE, CREAM, MOUTH, EARRING, TUNIC, TUNIC2, BELT, PANTS, FUR, FUR2, TEYE,
  PINK, APPLE, LEAF, LEAF2, GREY, CLAY, SPORE, HULL, HULL2, JET, METAL, METAL2, BLOOD, BLOOD2, SHOT, FLAME, NCOL
};
uint8_t PAL[NCOL][3] = {  // day look; tint() rewrites everything up to TREE
    {44, 96, 170},  {74, 134, 200}, {128, 176, 222}, {196, 212, 222},  // sky, top to horizon (haze at the horizon)
    {252, 246, 232}, {232, 196, 176}, {86, 120, 140}, {60, 92, 106}, {40, 54, 66},  // cloud, its shade; ridges, windmills
    {104, 166, 72}, {64, 118, 56},  {172, 214, 104}, {255, 255, 255},  // grass, dark grass, grass tip; wind streak
    {255, 255, 255}, {255, 236, 140}, {240, 240, 240}, {50, 100, 44}, {50, 100, 60},  // star, sun, moon, shadow; forest
    {30, 22, 30},   {246, 200, 166}, {186, 78, 58},  {124, 44, 38}, {226, 118, 82},  // outline, skin, hair
    {64, 72, 110},  {250, 240, 220}, {190, 90, 90},  {220, 40, 50},  // eye, cream, mouth, earring
    {80, 150, 220}, {46, 96, 170},  {120, 70, 40},  {214, 190, 140},  // tunic, its shade, belt/boots, trousers
    {250, 190, 60}, {112, 62, 30},  {60, 200, 90},                   // Teto fur, stripes, eye
    {255, 140, 160}, {214, 48, 56}, {96, 186, 80}, {54, 128, 62}, {178, 176, 186}, {196, 98, 60},  // props
    {200, 255, 230},                                                 // glowing spore (night visitor)
    {244, 240, 226}, {190, 184, 172}, {70, 50, 90},                  // glider hull, its shade, jet intake
    {176, 168, 124}, {100, 96, 72}, {196, 40, 36}, {108, 22, 26},       // game: Tolmekian olive, its shade; Pejite red, shade
    {255, 236, 120}, {255, 140, 40},                                 // her shots, fire
};

// ---- Time of day (K6): the palette indices stay, their RGB follows the clock (pet::look) ----
const int NT = TREE + 1;  // SKY0..TREE are tinted
const uint8_t LOOK[4][NT][3] = {  // pet::NIGHT, DAWN, DAY, DUSK
    {{10, 14, 40}, {20, 28, 70}, {40, 44, 96}, {70, 62, 112}, {60, 66, 100}, {44, 48, 82}, {34, 44, 76}, {24, 34, 58},
     {16, 20, 34}, {44, 90, 60}, {28, 62, 44}, {70, 120, 80}, {120, 130, 170}, {255, 250, 220}, {255, 214, 90},
     {250, 240, 200}, {20, 44, 32}, {20, 44, 36}},
    {{70, 80, 150}, {150, 110, 165}, {235, 140, 140}, {255, 185, 130}, {255, 214, 200}, {220, 150, 150}, {120, 100, 140},
     {90, 76, 112}, {50, 40, 64}, {96, 150, 72}, {58, 106, 56}, {170, 190, 110}, {255, 230, 220}, {255, 240, 220},
     {255, 240, 170}, {250, 235, 200}, {40, 80, 46}, {46, 84, 56}},
    {{44, 96, 170}, {74, 134, 200}, {128, 176, 222}, {196, 212, 222}, {252, 246, 232}, {232, 196, 176}, {86, 120, 140},
     {60, 92, 106}, {40, 54, 66}, {104, 166, 72}, {64, 118, 56}, {172, 214, 104}, {255, 255, 255}, {255, 255, 255},
     {255, 236, 140}, {240, 240, 240}, {50, 100, 44}, {50, 100, 60}},
    {{44, 46, 112}, {130, 66, 128}, {226, 98, 92}, {255, 158, 84}, {255, 170, 130}, {200, 100, 100}, {100, 64, 100},
     {70, 46, 80}, {40, 26, 44}, {84, 132, 62}, {50, 90, 50}, {150, 160, 90}, {255, 200, 170}, {255, 240, 190},
     {255, 226, 150}, {250, 225, 170}, {34, 70, 40}, {40, 70, 46}},
};
using pet::SUNRISE;
using pet::SUNSET;

int clockMin = -1;  // minute of day, -1 = clock unknown (night look, moon at a fixed spot)
float nightW = 1;   // 1 = full night (all stars), 0 = day

void tint(int minuteOfDay) {
  clockMin = minuteOfDay;
  int a, b;
  float f = pet::look(clockMin, a, b);
  for (int k = 0; k < NT; k++)
    for (int c = 0; c < 3; c++) PAL[k][c] = lroundf(LOOK[a][k][c] * (1 - f) + LOOK[b][k][c] * f);
  nightW = (a == pet::NIGHT ? 1 - f : 0) + (b == pet::NIGHT ? f : 0);
}
bool sunUp() { return clockMin >= SUNRISE && clockMin < SUNSET; }

// Sprites face right and are mirrored when facing left. K outline, S skin, H/h/L hair, E eye, W cream, M mouth,
// R earring, T/t tunic, B belt; Teto: O fur, D dark stripe, G eye. Head in 3/4 view, short hair with bangs; rows 8-14 the face.
const int HEAD_H = 17, HEAD_AX = 9;  // HEAD_AX: head column that sits over the body center
#define TOP ".....KKKKKK.....", "...KKHHHHHHKK...", "..KHHHLLHHHHHK..", ".KHHHHHHLLHHHHK.", ".KHHHHHHHHHHHHHK", \
            "KHHHHHHHHHHHHHHK", "KHHHHHHHHSHHHSHK", "KhHHHHHSSSSHSSSK"
#define CHEEK "KhHHHhSSSSSSSSSK"
#define LID "KhHHHHSSSSSSSSSK"
#define MOUTH_ "KhHHHhSSSSSSSSK.", "KhHHHhSSSSSSMSK.", "KhHhhRSSSSSSSK.."
#define FLAT "KhHHHhSSSSSSSSK.", "KhHHHhSSSSSKKSK.", "KhHhhRSSSSSSSK.."
#define CHIN ".KhKKKKSSSSSK...", "..K....KKKKK...."
const char *const HEAD[][HEAD_H] = {
    {TOP, "KhHHHHSKKSSSKKSK", "KhHHHHSWESSSWESK", "KhHHHhSEESSSEESK", CHEEK, MOUTH_, CHIN},  // eyes open
    {TOP, LID, LID, "KhHHHhSKKSSSKKSK", CHEEK, MOUTH_, CHIN},                             // blink
    {TOP, LID, "KhHHHHSKKSSSKKSK", "KhHHHhKSSKSKSSKK", CHEEK, "KhHHHhSSSSSSSSK.", "KhHHHhSSSSKMMKK.",
     "KhHhhRSSSSSKKK..", CHIN},                                                            // happy ^^, mouth open
    {TOP, LID, "KhHHHHSKKSSSKKSK", "KhHHHhSEESSSEESK", CHEEK, FLAT, CHIN},                // half (bored)
    {TOP, LID, "KhHHHHKSSKSKSSKK", "KhHHHhSKKSSSKKSK", CHEEK, MOUTH_, CHIN},              // closed (asleep)
    {TOP, "KhHHHHSSSSSSKKSK", "KhHHHHKSSKSSWESK", "KhHHHhSKKSSSEESK", CHEEK, MOUTH_, CHIN},  // one eye open (peek)
    {TOP, LID, "KhHHHHKSSKSKSSKK", "KhHHHhSKKSSSKKSK", "KhHHHhSSSSKKKKSK", "KhHHHhSSSKMMMKK.", "KhHHHhSSSKMMKSK.",
     "KhHhhRSSSSKKSK..", CHIN},                                                            // yawn
    {TOP, "KhHHHHSKSSSSSKSK", "KhHHHHSEKSSSKESK", "KhHHHhSEESSSEESK", CHEEK, FLAT, CHIN},  // frown
    {TOP, "KhHHHHKSKSSKSKSK", "KhHHHHSKSSSSKSSK", "KhHHHhKSKSSKSKSK", CHEEK, FLAT, CHIN},  // x x (game over)
};
#undef TOP
#undef CHEEK
#undef LID
#undef MOUTH_
#undef FLAT
#undef CHIN
enum { EYES_OPEN, EYES_BLINK, EYES_HAPPY, EYES_HALF, EYES_CLOSED, EYES_PEEK, EYES_YAWN, EYES_FROWN, EYES_DIZZY };

const int TORSO_H = 11, TORSO_AX = 6;  // tunic covers the hips below the belt
const char *const TORSO[TORSO_H] = {
    "....KSSK....", "..KKtTTtKK..", ".KTTTttTTTK.", "KTTTTTTTTTTK", "KTtTTTTTTtTK", "KTtTTTTTTtTK",
    ".KBBBBBBBBK.", ".KTTTTTTTTK.", "KTTTTTTTTTTK", "KtTTTTTTTTtK", "KKKKKKKKKKKK",
};

const int TSIT_H = 12, TSIT_AX = 5, TRUN_H = 10, TRUN_AX = 6;  // Teto: sitting, running (2 frames: stretched, gathered)
const char *const TSIT[TSIT_H] = {
    "..K..K....", ".KDKKDK...", ".KOKKOK...", "..KOKOK...", "..KOOOOK..", "..KOOOGOK.",
    "..KOOOOOOK", "..KDWWWKK.", ".KODOWWK..", "KOOOKWOK..", "KDODKOOK..", ".KKKKKKK..",
};
const char *const TRUN[2][TRUN_H] = {
    {"......K..K..", ".....KDKKDK.", ".....KOKKOK.", "......KOOOK.", ".KKKKKKOOGOK",
     "KODODOOOOOOK", "KOOOOOOWWKK.", ".KOKKKKOK...", "KOK....KOK..", "KK......KK.."},
    {"......K..K..", ".....KDKKDK.", ".....KOKKOK.", "......KOOOK.", ".KKKKKKOOGOK",
     "KODODOOOOOOK", "KOOOOOOWWKK.", ".KKOKKOKK...", "..KOK.KOK...", "..KK..KK...."},
};

// Props, in the cat's sprite letters (art()): K outline, O apple, W cream, P pink, Y grey, C clay, g/G leaf, F belt brown,
// b/B tunic blue, # = the caller's color.
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

uint8_t fb[N * N], bg[N * N], bgAir[N * N];  // the valley; open sky for the flight
float T = 0, DT = 0, awake = 0;  // awake > 0: play mode (refilled by a shake)
bool tunes = false;              // music screen on (music()): she flies
bool rove = false;               // flying (travel())
float tripLeft = 0;              // s of flight left, 0 = until the music stops
uint32_t rs = 2463534242u;
float rnd() {
  rs ^= rs << 13, rs ^= rs >> 17, rs ^= rs << 5;
  return (rs >> 8) / 16777216.0f;
}
float clampf(float v, float lo, float hi) { return fminf(fmaxf(v, lo), hi); }
float wrap(float v, float m) { return v - m * floorf(v / m); }  // v mod m, never negative
float h01(int i) {  // a fixed random number in 0..1 per integer (scenery along the flight)
  uint32_t h = i * 2654435761u;
  h ^= h >> 15, h *= 2246822519u, h ^= h >> 13;
  return (h & 0xffff) / 65536.0f;
}

inline void px(int x, int y, uint8_t c) {
  if ((unsigned)x < N && (unsigned)y < N) fb[y * N + x] = c;
}

// Paints pixels within r of a line segment (arms, legs, tail, wings; a disc if both ends match).
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
    case 'S': return SKIN;  case 'H': return HAIR;   case 'h': return HAIR2; case 'L': return HAIRL;
    case 'E': return EYE;   case 'W': return CREAM;  case 'M': return MOUTH; case 'R': return EARRING;
    case 'T': return TUNIC; case 't': return TUNIC2; case 'B': return BELT;
    case 'O': return FUR;   case 'D': return FUR2;   case 'G': return TEYE;
  }
  return INK;
}

// Sprite column ax lands on x; mirrored horizontally if d < 0. shut: Teto's eye closed.
void blit(const char *const *rows, int h, int ax, int x, int y0, int d, bool shut = false) {
  for (int r = 0; r < h; r++)
    for (int c = 0; rows[r][c]; c++) {
      char ch = rows[r][c];
      if (ch != '.') px(d > 0 ? x + c - ax : x - 1 - c + ax, y0 + r, code(shut && ch == 'G' ? 'K' : ch));
    }
}

void art(const char *const *rows, int h, int x, int y, int flip = 1, uint8_t hash = INK) {  // props: top-left at x, y
  static const char K[] = "KOWPYCgGFbB#";
  const uint8_t C[] = {INK, APPLE, CREAM, PINK, GREY, CLAY, LEAF, LEAF2, BELT, TUNIC, TUNIC2, hash};
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

// ---- Background: sky (sun or moon, stars), drifting clouds, wind streaks, windmills, grass ----
const float BAYER[4] = {0.125f, 0.625f, 0.875f, 0.375f};  // 2x2 dither thresholds
float bayer(int x, int y) { return BAYER[(y & 1) * 2 + (x & 1)]; }

uint8_t skyAt(int x, int y) {
  float v = fminf(y, GROUND - 1) * 3.0f / (GROUND - 1);
  int b = v;
  return b + (v - b > bayer(x, y));
}

// Cloud: a few blobs with a flat base; bottom strip shaded. Draws over the sky only (behind the ridges).
void cloud(float cx, float cy, float s) {
  static const float P[4][3] = {{-4, 0.5f, 2.6f}, {-1, -1.5f, 3.4f}, {3, -0.5f, 3}, {6, 1, 2.2f}};  // x, y, radius
  for (int y = floorf(cy - 5 * s); y <= cy + 2 * s; y++)
    for (int x = floorf(cx - 7 * s); x <= cx + 9 * s; x++) {
      if ((unsigned)x >= N || (unsigned)y >= N || fb[y * N + x] > SKY3) continue;
      for (auto &p : P) {
        float dx = x + 0.5f - (cx + p[0] * s), dy = y + 0.5f - (cy + p[1] * s);
        if (dx * dx + dy * dy < p[2] * p[2] * s * s) {
          px(x, y, y >= cy + s ? CLOUD2 : CLOUD);
          break;
        }
      }
    }
}
const float CLOUDS[3][4] = {{8, 12, 1.0f, 2.0f}, {58, 21, 0.8f, 1.4f}, {30, 31, 1.3f, 1.0f}};  // x, y, scale, speed

const int NW = 4;  // wind streaks rush past right to left
struct {
  float x, y, len, v;
} gusts[NW];
void gust(int i, float x) { gusts[i] = {x, 6 + 34 * rnd(), 3 + 4 * rnd(), 45 + 35 * rnd()}; }

const float MILLS[2][5] = {{63, 27, 6, 1.6f, MILL}, {18, 36, 4, 2.3f, HILL2}};  // hub x, y, blade length, angular speed, color
void blades(float x, float y, float len, float speed, uint8_t c) {
  for (int b = 0; b < 4; b++) {
    float a = T * speed + b * 1.5708f;
    capsule(x + 0.5f, y + 0.5f, x + 0.5f + len * cosf(a), y + 0.5f + len * sinf(a), 0.6f, c);
  }
}

void grassTips() {  // grass tips bending left in the wind
  for (int x = 0; x < N; x++) {
    int h = (x * 7 % 5 < 2) + (x * 13 % 7 == 0), lean = sinf(T * 2.5f + x * 0.3f) > 0.3f ? -1 : 0;
    for (int k = 1; k <= h; k++) px(x + (k == h ? lean : 0), GROUND - k, k == h ? GRASSL : GRASS);
  }
}

const int NS = 14;
uint8_t sx[NS], sy[NS];
float sp[NS];

void skyDisc(float cx, float cy, float r, uint8_t c, float cutX = 0, float cutY = 0, float cutR = 0) {  // only over sky
  for (int y = floorf(cy - r); y <= cy + r; y++)
    for (int x = floorf(cx - r); x <= cx + r; x++)
      if ((unsigned)x < N && (unsigned)y < N && fb[y * N + x] <= SKY3 && hypotf(x + 0.5f - cx, y + 0.5f - cy) < r &&
          (cutR == 0 || hypotf(x + 0.5f - cutX, y + 0.5f - cutY) > cutR))
        fb[y * N + x] = c;
}

void drawWind(float speed) {  // streaks over the sky and clouds only
  for (auto &g : gusts)
    for (int k = 0; k < g.len * speed; k++) {
      int x = g.x + k, y = g.y;
      if ((unsigned)x < N && fb[y * N + x] <= CLOUD2) px(x, y, WIND);
    }
}

// Stars, then sun or moon on an arc from left (rise) to right (set). Over the sky in fb, so draw it before the land.
void sunMoon(float horizon) {
  for (int i = 0; i < NS; i++)
    if (i < NS * nightW && sinf(T * 1.7f + sp[i]) > -0.2f) px(sx[i], sy[i], STAR);
  float mx = 62, my = 9;  // moon at a fixed spot while the clock is unknown
  bool moon = true;
  if (clockMin >= 0) {
    moon = !sunUp();
    float p = moon ? float((clockMin - SUNSET + 1440) % 1440) / (1440 - SUNSET + SUNRISE) : float(clockMin - SUNRISE) / (SUNSET - SUNRISE);
    mx = 12 + 56 * p, my = horizon - 34 * sinf(3.1416f * p);
  }
  if (moon) skyDisc(mx, my, 4.6f, MOON, mx + 2.5f, my - 1.5f, 4.2f);  // crescent
  else skyDisc(mx, my, 4.2f, SUN);
}

void drawSky() {
  memcpy(fb, bgAir, sizeof fb);
  sunMoon(42);
  for (int i = 0; i < N * N; i++)  // the valley over the sky
    if (bg[i] > SKY3) fb[i] = bg[i];
  for (auto &c : CLOUDS) {  // drift left with the wind, wrap from left edge to right
    float x = fmodf(c[0] - T * c[3], 110);
    cloud(x < 0 ? x + 95 : x - 15, c[1], c[2]);
  }
  drawWind(1);
  for (auto &m : MILLS) blades(m[0], m[1], m[2], m[3], m[4]);
  grassTips();
}

// ---- Nausicaa, Teto, visitors ----
enum { IDLE, RUN, CROUCH, JUMP, LAND, KNEEL, REACH, SLEEP, EAT, REFUSE, PURR, YAWN, BORED, HUNGRY, ANGRY, KO, WAVE, SIT,
       BREEZE, TWIRL, DANCE };
struct {
  float x = 32, y = GROUND, vx = 0, vy = 0, t = 0, dur = 1, phase = 0, happy = 0, peek = 0, blow = 0;  // blow: hair flutter 0..1
  int dir = 1, st = IDLE, hops = 0;
  bool booped = false, yay = false, wake = false;  // yay: both arms up during a joy hop; wake: yawn after this landing
} girl;

enum { SHOULDER, HAND, LEAP, SCAMPER, PERCH };  // LEAP and later: Teto is off the girl
struct {
  float x = 0, y = 0, vx = 0, vy = 0, t = 0, dur = 1, tx = 0, ty = GROUND, phase = 0;
  int dir = 1, st = SHOULDER;
  bool toHand = false, home = false;  // toHand: the leap lands on her hand; home: running back to her hand
} teto;
float shX, shY, hdX, hdY;  // her shoulder and reaching hand (updated while drawing each frame)

struct {
  float x = 0;
  bool on = false;
} bowl;  // the empty plate while she's hungry
enum { BUTTERFLY, GLOWSPORE };
struct {
  float x = 0, y = 0, vx = 0, vy = 0, t = 0, home = 0;
  int kind = BUTTERFLY;
  bool on = false, flee = false;
} vis;

pet::Mood mood = pet::HAPPY;
float visitT = 150, yawnT = 0, funT = 12, tripT = 200;  // s until the next visitor / tired yawn / idle trick / flight

void setMood(pet::Mood m) { mood = m; }

void go(int st, float dur) {
  girl.st = st, girl.t = 0, girl.dur = dur, girl.booped = false;
  if (st != HUNGRY) bowl.on = false;
}
void tgo(int st, float dur) { teto.st = st, teto.t = 0, teto.dur = dur; }
void pose(int st, float dur) {  // a ground state from wherever she is (ends a flight)
  rove = false;
  girl.y = GROUND, girl.vx = girl.vy = 0, girl.hops = 0, girl.wake = false, girl.yay = false, girl.peek = 0;
  go(st, dur);
}
bool grounded() { return girl.st != CROUCH && girl.st != JUMP && girl.st != LAND && girl.st != REACH && girl.st != ANGRY && girl.st != KO; }
bool loose() { return teto.st >= LEAP; }
bool chasing() { return vis.on && !vis.flee; }
float aimX() { return chasing() ? vis.x : teto.x; }  // what she's after: a visitor, else Teto
float aimY() { return chasing() ? vis.y : teto.y; }
int side() { return aimX() >= girl.x ? 1 : -1; }

void leap(float xt, float h, int hops) {  // to xt, h high; hops = how many more hops after
  xt = clampf(xt, 16, 64);
  girl.vy = -sqrtf(2 * G * h);
  girl.vx = (xt - girl.x) / (2 * -girl.vy / G);
  girl.hops = hops;
  go(JUMP, 0);
}

// Teto leaps to (tx, ty); apex is h above the higher of start and target.
void hop(float tx, float ty, float h, bool toHand) {
  tx = clampf(tx, 8, 72);
  float top = fminf(teto.y, ty) - h, t1 = sqrtf(2 * (teto.y - top) / G), t2 = sqrtf(2 * (ty - top) / G);
  teto.vy = -G * t1, teto.vx = (tx - teto.x) / (t1 + t2);
  teto.tx = tx, teto.ty = ty, teto.toHand = toHand, teto.home = false;
  if (fabsf(tx - teto.x) > 1) teto.dir = tx > teto.x ? 1 : -1;
  tgo(LEAP, 0);
}

float roamX() {  // a random spot away from the girl
  float x = 10 + 60 * rnd();
  if (fabsf(x - girl.x) < 16) x = clampf(girl.x + (x < girl.x ? -20 : 20), 10, 70);
  return x;
}

void roam() {  // in play Teto runs or leaps somewhere
  float x = roamX();
  if (rnd() < 0.35f && fabsf(x - teto.x) < 30) hop(x, GROUND, 6 + 6 * rnd(), false);
  else teto.tx = x, teto.home = false, tgo(SCAMPER, 0);
}

// A butterfly (day) or a glowing spore (night) comes in and hovers near her; she reaches or jumps for it.
void visit() {
  if (vis.on) return;
  vis.on = true, vis.flee = false, vis.t = 0;
  vis.kind = sunUp() ? BUTTERFLY : GLOWSPORE;
  int open = girl.x < N / 2 ? 1 : -1;  // the roomier side
  vis.x = open > 0 ? 84 : -4;
  vis.home = clampf(girl.x + open * (24 + 10 * rnd()), 18, 62);
  vis.y = vis.kind == BUTTERFLY ? GROUND - 20 : GROUND - 30;  // the butterfly in reach, the spore needs a jump
}
void flee() {
  vis.flee = true;
  vis.vx = (vis.x >= girl.x ? 1 : -1) * 60, vis.vy = -30;
}

// Did the tip of the reaching front hand touch the visitor
bool touch() { return chasing() && hypotf(vis.x - (girl.x + girl.dir * 6.5f), vis.y - (girl.y - 19)) < 5; }

void boop() {  // caught 30 % of the time, else it flies off
  girl.booped = true, girl.happy = 1.2f;
  if (rnd() < 0.3f) vis.on = false, pet::joy(3), girl.happy = 2.5f, girl.yay = true, girl.hops = 2;
  else flee();
}

void reachOrJump() {
  if (vis.y > GROUND - 23) go(REACH, 0.55f);  // reachable standing up
  else girl.hops = 0, go(CROUCH, 0.3f);
}

void think() {  // idle wait is over: go after the visitor, or follow Teto and hold out her hand
  girl.dir = side();
  if (chasing()) fabsf(vis.x - girl.dir * 6 - girl.x) > 4 ? go(RUN, 0) : reachOrJump();
  else if (awake > 0 && rnd() < 0.15f) girl.hops = 2, girl.yay = true, girl.happy = 2, go(CROUCH, 0.15f);  // joy hop
  else if (fabsf(teto.x - girl.dir * 12 - girl.x) > 3) go(RUN, 0);
  else go(KNEEL, 1.0f);
}

void travel(float secs);
void roveStep(float dt);

void idleDone() { go(IDLE, (0.5f + 1.0f * rnd()) * (mood == pet::TIRED ? 2 : 1)); }  // tired: plays half as often

void update(float dt) {
  T += dt, DT = dt, girl.t += dt, teto.t += dt, girl.happy -= dt, girl.peek -= dt, awake -= dt, yawnT -= dt;
  if (girl.st == IDLE && mood == pet::HAPPY && !vis.on && !loose() && !rove) funT -= dt, tripT -= dt;
  float blow = rove ? 1 : girl.st == RUN || girl.st == BREEZE ? 1 : girl.st == JUMP ? 0.8f : 0;  // hair lifts fast, settles slowly
  girl.blow += (blow - girl.blow) * fminf(1, (blow > girl.blow ? 8 : 3) * dt);
  for (int i = 0; i < NW; i++)
    if ((gusts[i].x -= gusts[i].v * (rove ? 2 : 1) * dt) < -gusts[i].len * 2) gust(i, N + 40 * rnd());
  if (rove) return roveStep(dt);

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
  if (mood == pet::HAPPY && girl.st == IDLE && awake <= 0 && !vis.on && !loose() && (visitT -= dt) <= 0)
    visit(), visitT = 120 + 180 * rnd();

  switch (girl.st) {
    case IDLE:
      if ((chasing() || loose()) && fabsf(aimX() - girl.x) > 3) girl.dir = side();
      if (girl.t <= girl.dur) break;
      if (chasing()) {
        if (fabsf(vis.home - vis.x) < 1) think();
      } else if (loose()) {
        if (teto.st != LEAP) think();
      } else if (mood == pet::SLEEP) go(SLEEP, 0);
      else if (mood == pet::HUNGRY) bowl.x = clampf(girl.x + (girl.x < N / 2 ? 12 : -12), 14, 66), bowl.on = true, go(HUNGRY, 0);
      else if (mood == pet::SAD) go(BORED, 0);
      else if (mood == pet::TIRED && yawnT <= 0) yawnT = 30, go(YAWN, 1.5f);
      else if (tripT <= 0) tripT = 180 + 240 * rnd(), travel(15 + 10 * rnd());
      else if (funT <= 0) {
        float r = rnd();
        funT = 10 + 15 * rnd();
        if (r < 0.2f) go(WAVE, 2.4f);
        else if (r < 0.4f) go(SIT, 6 + 4 * rnd());
        else if (r < 0.6f) go(BREEZE, 3.5f);
        else if (r < 0.8f) go(TWIRL, 1.6f);
        else go(DANCE, 6);
      } else if (girl.t > girl.dur + 5) girl.dir = -girl.dir, girl.t = 0;  // looks around now and then
      break;
    case RUN: {
      int s = side();
      float tx = clampf(aimX() - s * (chasing() ? 6 : 12), 14, 66), d = tx - girl.x;
      if (fabsf(d) < 1.5f) {
        girl.dir = s;
        chasing() ? reachOrJump() : go(KNEEL, 1.0f);
        break;
      }
      girl.dir = d > 0 ? 1 : -1;
      girl.x += girl.dir * fminf(34 * dt, fabsf(d));
      girl.phase += 14 * dt;
      if (girl.t > 3 || (!chasing() && !loose())) go(IDLE, 0.3f);  // couldn't catch up: take a breather
      break;
    }
    case CROUCH:
      if (girl.t < girl.dur) break;
      if (girl.hops) leap(girl.x, 6, girl.hops - 1);
      else leap(vis.x - girl.dir * 6, clampf(GROUND - 18 - vis.y, 4, 22), 0);
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
    case KNEEL:  // in play waits briefly; after play, until Teto is on her shoulder
      if (teto.st == SHOULDER || (awake > 0 && girl.t > girl.dur) || girl.t > 6) go(IDLE, 1 + rnd());
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
    case ANGRY:  // one hop back, then a glare
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
    case BREEZE:
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
  }

  switch (teto.st) {
    case SHOULDER: break;  // pinned to the shoulder while drawing
    case HAND:
      if (teto.t > 0.75f) tgo(SHOULDER, 0);
      break;
    case LEAP:
      teto.vy += G * dt, teto.x += teto.vx * dt, teto.y += teto.vy * dt;
      if (teto.vy < 0 || teto.y < teto.ty) break;
      teto.x = teto.tx, teto.y = teto.ty;
      if (teto.toHand) tgo(HAND, 0);
      else tgo(PERCH, 0.3f + 0.8f * rnd());
      break;
    case SCAMPER: {
      float d = teto.tx - teto.x;
      teto.dir = d > 0 ? 1 : -1;
      teto.x += teto.dir * fminf(48 * dt, fabsf(d));
      teto.phase += 20 * dt;
      if (fabsf(d) > 0.5f) break;
      if (teto.home && girl.st == KNEEL) teto.dir = -girl.dir, hop(hdX, hdY, 3, true);
      else tgo(PERCH, 0.3f + 0.8f * rnd());
      break;
    }
    case PERCH:
      if (girl.st == KNEEL && awake <= 0) {  // play is over: comes to the reaching hand
        if (girl.t > 0.25f) teto.tx = clampf(hdX + girl.dir * 4, 8, 72), teto.home = true, tgo(SCAMPER, 0);
      } else if (girl.st == KNEEL && fabsf(teto.x - hdX) < 18) {  // in play: leaps over the hand and her head and runs off
        if (girl.t > 0.2f) hop(girl.x - girl.dir * 16, GROUND, 28, false);
      } else if (awake > 0 && teto.t > teto.dur) roam();
      else if (fabsf(girl.x - teto.x) > 2) teto.dir = girl.x > teto.x ? 1 : -1;
      break;
  }
}

// Two locks at the nape fly back with the wind; `a` tilts them (+ = up).
void hairLocks(float cx, int ky, int d, float a0) {
  if (girl.blow < 0.2f) return;
  float rx[2], ry[2], tx[2], ty[2], len = 3 * girl.blow;  // root, tip
  for (int i = 0; i < 2; i++) {
    float a = a0 + 0.25f * sinf(T * 14 + i * 2.1f);
    rx[i] = cx + d * (1.5f - HEAD_AX), ry[i] = ky + 7.5f + i * 4;
    tx[i] = rx[i] - d * len * cosf(a), ty[i] = ry[i] - len * sinf(a);
    capsule(rx[i], ry[i], tx[i], ty[i], 1.0f, INK);
  }
  for (int i = 0; i < 2; i++) capsule(rx[i] + d * 1.5f, ry[i], tx[i], ty[i], 0.5f, HAIR);
}

void leg(float x0, float y0, float x1, float y1) {  // trousers on top, boot below
  float mx = (x0 + x1) / 2, my = (y0 + y1) / 2;
  capsule(x0, y0, x1, y1, 1.6f, INK);
  capsule(x0, y0, mx, my, 0.7f, PANTS), capsule(mx, my, x1, y1, 0.8f, BELT);
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
  float hip = 7, th[2] = {0.25f, 0.25f}, fx[2] = {}, fl[2] = {};  // hip height; arm angle (0 down, π up); foot slide/lift
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
    case CROUCH: hip = 5, th[0] = th[1] = 0.7f, fx[0] = -d, fx[1] = d; break;
    case LAND: hip = 5.5f, th[0] = th[1] = 0.6f; break;
    case JUMP: {
      bool up = girl.vy < 0;
      fl[0] = fl[1] = up ? 0 : 1.5f, fx[0] = -d * 0.8f, fx[1] = d * 0.8f;
      th[0] = 1.1f, th[1] = girl.yay ? 1.1f : 2.7f;  // front arm reaches for the visitor
      break;
    }
    case REACH: {
      float t = girl.t;
      hip = 7.5f, th[0] = 0.4f;  // on tiptoe
      th[1] = t < 0.2f ? 0.25f + 2.6f * t / 0.2f : t < 0.35f ? 2.85f : fmaxf(0.25f, 2.85f - 2.6f * (t - 0.35f) / 0.2f);
      break;
    }
    case KNEEL: hip = 4, fx[0] = -d * 3, fx[1] = d * 2, th[0] = 0.3f, th[1] = fminf(1.4f, girl.t * 6); break;  // hand toward Teto
    case SLEEP:  // sitting, hugging her knees, head down, breathing
      legs = KNEES, hip = 3, th[0] = -0.5f, th[1] = 0.55f, hdy = 2 + 0.5f * sinf(T * 3.1416f);
      eyes = girl.peek > 0 ? EYES_PEEK : EYES_CLOSED;
      break;
    case BORED:  // sitting, legs out, head down, a foot moving now and then
      legs = OUT, hip = 3, th[0] = -0.3f, th[1] = 0.15f, hdy = 1.5f, tap = 1.2f * sinf(T * 1.2f), eyes = EYES_HALF;
      break;
    case SIT:  // sitting, legs out, feet tapping, leaning back on her hands
      legs = OUT, hip = 3, th[0] = 0.5f, th[1] = 0.15f, tap = 1.5f * sinf(T * 5);
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
      hip = 7 + 0.8f * k, th[0] = th[1] = 0.25f + 2.6f * k;
      if (girl.t > 0.25f && girl.t < 1.25f) eyes = EYES_YAWN;
      break;
    }
    case ANGRY:  // fists down and back, frown
      th[0] = th[1] = -0.35f, eyes = EYES_FROWN;
      if (!air) hip = 6.5f;
      break;
    case KO:  // game over: tumbles down, then sits dazed with stars
      eyes = EYES_DIZZY;
      if (air) th[0] = 2.4f, th[1] = 2.0f, fl[0] = fl[1] = 1.5f;
      else legs = KNEES, hip = 3, th[0] = -0.5f, th[1] = 0.5f, hdy = 1;
      break;
    case WAVE:  // waves at you
      th[1] = 2.7f + 0.35f * sinf(girl.t * 14), eyes = EYES_HAPPY;
      break;
    case BREEZE:  // reads the wind: eyes closed, arms open, hair flying
      th[0] = th[1] = 1.2f + 0.08f * sinf(T * 2), eyes = EYES_CLOSED;
      break;
    case TWIRL:
      th[0] = th[1] = 1.5f, hip += 0.6f * fabsf(sinf(girl.t * 15)), eyes = EYES_HAPPY;
      break;
    case DANCE: {  // to a 120 bpm beat: arms take turns, then both up and side steps, then a twirl (update())
      float b = T * 2, ph = fmodf(b, 1), bounce = fabsf(sinf(3.1416f * ph));
      int beat = (int)b % 8;
      hip = 6.4f + 0.9f * bounce;
      if (beat < 4) th[beat % 2] = 2.6f, th[1 - beat % 2] = 0.5f, fl[beat % 2] = 1.5f * bounce;
      else if (beat < 6) th[0] = th[1] = 2.5f + 0.3f * sinf(T * 12), fx[0] = fx[1] = (beat % 2 ? 1.5f : -1.5f) * bounce;
      else th[0] = th[1] = 1.5f;
      eyes = beat % 2 ? EYES_HAPPY : -1;
      break;
    }
  }
  if (girl.yay) th[0] = th[1] = 2.6f + 0.25f * sinf(T * 18);

  int cx = lroundf(girl.x), hy = lroundf(girl.y - hip), ty = hy - 8;  // hip and torso top rows
  int hx0 = cx + lroundf(hdx), ky = ty - 17 + lroundf(hdy);             // head
  shadow(cx, 7 * fmaxf(0.3f, 1 - (GROUND - girl.y) / 30));
  for (int i = 0; i < 2 && legs == STAND; i++) {
    float s = i ? d : -d, lx = cx + s * 2, fx1 = lx + fx[i], fy1 = girl.y - 1 - fl[i];
    leg(lx, hy, fx1, fy1);
    disc(fx1 + d * 0.5f, fy1 + 0.2f, 0.8f, BELT);
  }
  blit(TORSO, TORSO_H, TORSO_AX, cx, ty, d);
  for (int i = 0; i < 2 && legs != STAND; i++) {  // sitting: legs in front of the tunic
    float o = i ? 0 : -1.2f, fy = girl.y - 1;
    if (legs == KNEES) {
      float kx = cx + d * (5 + o), kyy = hy - 3.5f;
      leg(cx + d * o, hy, kx, kyy), leg(kx, kyy, cx + d * (8 + o), fy);
    } else {
      fy -= fmaxf(0, i ? tap : -tap);
      leg(cx + d * o, hy + 1, cx + d * (5 + o), girl.y - 1.5f), leg(cx + d * (5 + o), girl.y - 1.5f, cx + d * (9 + o), fy);
    }
    disc(cx + d * ((legs == KNEES ? 8 : 9) + o + 0.5f), fy + 0.2f, 0.8f, BELT);
  }
  if (eyes < 0) eyes = girl.happy > 0 ? EYES_HAPPY : fmodf(T, 3.1f) < 0.14f ? EYES_BLINK : EYES_OPEN;
  blit(HEAD[eyes], HEAD_H, HEAD_AX, hx0, ky, d);
  hairLocks(hx0, ky, d, st == JUMP ? clampf(-girl.vy / 120, -0.9f, 1.0f) : -0.15f);
  for (int i = 0; i < 2; i++) {  // arms after the head: stay in front of the hair when raised
    float s = i ? d : -d, ax = cx + s * 4.5f, ay = ty + 2.5f;
    float ex = ax + s * 6 * sinf(th[i]), ey = ay + 6 * cosf(th[i]), mx = ax + (ex - ax) * 0.8f, my = ay + (ey - ay) * 0.8f;
    capsule(ax, ay, ex, ey, 1.5f, INK), disc(ex, ey, 1.6f, INK);
    capsule(ax, ay, mx, my, 0.8f, TUNIC), disc(ex, ey, 0.8f, SKIN);  // long-sleeved tunic, hand
    if (i) hdX = ex, hdY = ey - 1;
    if (i && apple >= 0) art(APPLE_ART[apple], 5, lroundf(ex) - 2, lroundf(ey) - 4, d);
  }
  shX = cx - d * 5, shY = ty + 5;

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
  if (st == DANCE) notes(girl.x, GROUND - 24, 0);
  if (st == KO && !air)  // stars around her head
    for (int k = 0; k < 2; k++) {
      float a = T * 6 + k * 3.1416f;
      art(TWINKLE, 3, lroundf(hx0 - d * 1 + 7 * cosf(a)) - 1, lroundf(ky + 2 + 1.5f * sinf(a)) - 1, 1, EARRING);
    }
}

void attach() {  // pin Teto on the shoulder/hand to the girl's pose this frame
  if (teto.st == SHOULDER) teto.x = shX, teto.y = shY, teto.dir = fmodf(T, 7) > 6.2f ? -girl.dir : girl.dir;  // looks back now and then
  if (teto.st == HAND) {  // sits on the hand for 0.35 s, then climbs up the arm to the shoulder
    float k = clampf((teto.t - 0.35f) / 0.4f, 0, 1);
    teto.x = hdX + (shX - hdX) * k, teto.y = hdY + (shY - hdY) * k - 2 * sinf(k * 3.1416f), teto.dir = -girl.dir;
  }
}

void drawTeto(bool fly = false) {  // fly: clinging on in the glider's wind
  const int d = teto.dir;
  bool run = fly || teto.st == SCAMPER || teto.st == LEAP || (teto.st == HAND && teto.t > 0.35f);
  const char *const *spr = run ? TRUN[teto.st == SCAMPER && (int(teto.phase / 3.1416f) & 1)] : TSIT;
  int h = run ? TRUN_H : TSIT_H, ax = run ? TRUN_AX : TSIT_AX, x = lroundf(teto.x), y = lroundf(teto.y);
  if (loose()) shadow(x, 3 * fmaxf(0.3f, 1 - (GROUND - teto.y) / 30));

  float kx[5], ky[5];  // tail: raised, swaying, striped chain from the sprite's rear
  kx[0] = x + d * (0.5f - ax), ky[0] = y - h + (run ? 5.5f : 8.5f);
  for (int i = 1; i < 5; i++) {
    float g = fly ? -0.1f + 0.2f * sinf(T * 16 - i) : (run ? 0.35f : 1.0f) + 0.3f * i + (run ? 0.15f : 0.35f) * sinf(T * (run ? 12 : 4) - 0.7f * i);
    kx[i] = kx[i - 1] - d * 1.5f * cosf(g), ky[i] = ky[i - 1] - 1.5f * sinf(g);
  }
  for (int i = 1; i < 5; i++) capsule(kx[i - 1], ky[i - 1], kx[i], ky[i], 1.5f - 0.1f * i, INK);
  for (int i = 1; i < 5; i++) capsule(kx[i - 1], ky[i - 1], kx[i], ky[i], 0.7f, i & 1 ? FUR : FUR2);
  bool asleep = girl.st == SLEEP && teto.st == SHOULDER;
  blit(spr, h, ax, x, y - h, d, asleep || fmodf(T + 1.3f, 3.7f) < 0.15f);
}

void drawVisitor() {
  if (!vis.on) return;
  int x = lroundf(vis.x), y = lroundf(vis.y + 2 * sinf(T * 3));
  if (vis.kind == BUTTERFLY) art(FLY[sinf(T * 20) > 0], 5, x - 3, y - 2);
  else {  // a glowing spore from the Sea of Corruption: pulsing halo, soft core
    bool bright = sinf(T * 4) > 0;
    for (int k = 0; k < 4; k++) px(x + (k == 0) - (k == 1), y + (k == 2) - (k == 3), bright ? SPORE : LEAF);
    px(x, y, CREAM);
  }
}

// ---- Travel: she flies the Mehve to the right, lying on it; Teto clings to her back. The valley scrolls under her in
// layers, each slower the farther it is: far ridges, hills with windmills, the forest just below; clouds near and far,
// wind streaks rushing past. She drifts up and down on the wind. ----
float dist = 0, gy = 43, gv = 0, lift = 0;  // px flown (the forest layer); glider height, its speed; shake: a climb

void travel(float secs) {  // secs: how long; 0 = while the music plays
  tripLeft = secs;
  if (rove) return;
  pose(IDLE, 0);
  rove = true, vis.on = false, awake = 0, bowl.on = false, gy = 43, gv = 0, lift = 0;
  tgo(SHOULDER, 0);
}

void roveStep(float dt) {
  dist += 30 * dt;
  lift = fmaxf(0, lift - 14 * dt);
  float to = 43 + 5 * sinf(T * 0.45f) + 2 * sinf(T * 1.15f) - lift;  // riding the air
  gv += (5 * (to - gy) - 3 * gv) * dt;
  gy += gv * dt;
  if (!tunes && (tripLeft -= dt) <= 0) pose(IDLE, 1);
}

void drawGlider() {
  const float gx = 40, a = clampf(-gv * 0.03f, -0.22f, 0.22f), ca = cosf(a), sa = sinf(a);  // nose up when climbing
  auto W = [&](float lx, float ly, float &x, float &y) { x = gx + lx * ca + ly * sa, y = gy - lx * sa + ly * ca; };
  auto bar = [&](float x0, float y0, float x1, float y1, float r, uint8_t c) {
    float ax, ay, bx, by;
    W(x0, y0, ax, ay), W(x1, y1, bx, by);
    capsule(ax, ay, bx, by, r, c);
  };
  auto hull = [&](float x0, float y0, float x1, float y1, float r, uint8_t c) { bar(x0 * 1.4f, y0, x1 * 1.4f, y1, r, c); };
  float flap = 0.6f * sinf(T * 2.2f);  // the wings flex a little
  hull(-1, -1, -14, -6 - flap, 1.5f, INK), hull(-1, -1, -14, -6 - flap, 0.7f, HULL2);    // far wing, behind
  hull(-12, 0, -15, -3, 1.0f, INK), hull(-12, 0, -15, -3, 0.4f, HULL);                   // tail fin
  hull(-11, 0, 9, 0, 3.2f, INK), hull(-10, -0.3f, 8.5f, -0.3f, 2.3f, HULL), hull(-9, 1.4f, 7, 1.4f, 0.9f, HULL2);  // hull
  float jx, jy;
  W(9.3f * 1.4f, 0.6f, jx, jy), disc(jx, jy, 1.4f, JET);                                 // jet intake at the nose
  // Nausicaa lying on top: legs back, body along the hull, arm to the handlebar, head up looking ahead.
  bar(-4, -4, -12, -3.2f, 1.6f, INK), bar(-4, -4, -8, -3.6f, 0.7f, PANTS), bar(-8, -3.6f, -12, -3.2f, 0.8f, BELT);
  bar(-4, -4.6f, 3, -5.6f, 2.3f, INK), bar(-4, -4.6f, 3, -5.6f, 1.5f, TUNIC), bar(-3.5f, -4.4f, -3.5f, -4.4f, 1.0f, BELT);
  bar(5, -2, 6.5f, -8, 0.5f, INK);                                                         // handlebar
  bar(3, -6, 6.5f, -8, 1.3f, INK), bar(3, -6, 5.5f, -7.4f, 0.7f, TUNIC);
  float hx, hy;
  W(6.5f, -8, hx, hy), disc(hx, hy, 0.8f, SKIN);
  W(4.5f, -6.5f, hx, hy);
  int eyes = girl.happy > 0 ? EYES_HAPPY : fmodf(T, 3.1f) < 0.14f ? EYES_BLINK : EYES_OPEN, ky = lroundf(hy) - 15;
  blit(HEAD[eyes], HEAD_H, HEAD_AX, lroundf(hx), ky, 1);
  hairLocks(hx, ky, 1, 0.1f);
  W(-8, -5.5f, teto.x, teto.y);  // on her back
  teto.dir = 1, drawTeto(true);
  hull(1, 1, -16, 10 + flap, 2.4f, INK), hull(1, 0.8f, -15.6f, 9.8f + flap, 1.5f, HULL);  // near wing, in front
  hull(-15.6f, 9.8f + flap, -17, 7 + flap, 1.0f, INK);                                   // its tip fin
  if (tunes) notes(gx - 12, gy - 18, -18);
}

void drawRove() {
  memcpy(fb, bgAir, sizeof fb);
  sunMoon(46);
  for (int k = 0; k < 2; k++)  // far clouds, slow
    cloud(wrap(10 + k * 55 - dist * 0.05f - T * 1.5f, 120) - 20, 10 + k * 9, 0.8f - 0.2f * k);
  for (int x = 0; x < N; x++) {  // far ridges, hills, the forest below: each layer scrolls slower the farther it is
    float u = x + dist * 0.1f, v = x + dist * 0.35f, w = x + dist;
    int far = 46 + 3 * sinf(u * 0.07f) + 2 * sinf(u * 0.19f + 1), hill = 57 + 3 * sinf(v * 0.1f + 2) + 1.5f * sinf(v * 0.25f);
    int wood = 71 - 2.5f * fabsf(sinf(w * 0.28f)) - 1.5f * fabsf(sinf(w * 0.11f + 1));  // treetop bumps
    for (int y = far; y < N; y++) fb[y * N + x] = y < hill ? HILL : y < wood ? HILL2 : y < wood + 2 ? GRASS2 : TREE;
  }
  for (int k = floorf(dist * 0.35f / 31) - 1, end = k + 5; k < end; k++) {  // windmills on the hills
    float x = k * 31 + 12 * h01(k) - dist * 0.35f, v = x + dist * 0.35f;
    float top = 57 + 3 * sinf(v * 0.1f + 2) + 1.5f * sinf(v * 0.25f);
    capsule(x, top, x, top - 7, 0.5f, MILL), blades(x, top - 7, 4, 1.8f + h01(k + 5), MILL);
  }
  drawWind(2.5f);
  drawGlider();
  float cx = wrap(90 - dist * 1.4f, 260) - 30;  // now and then a big cloud rushes by right in front
  cloud(cx, 64, 1.6f);
}

// ---- Game: the Mehve against the air fleets, seen from above, the valley scrolling under her (instead of the runner).
// She flies along the bottom: the sideways tilt sets where (gameTilt, level = middle). The button fires her gun, held =
// auto-fire. Every shot heats it; full heat jams it until it has cooled all the way (the bar under her). Planes come
// down from the top: Pejite gunships (red one-seaters, fins at the wing tips: quick, weaving, 1 hit), Tolmekian armored
// corvettes (four wings: aimed shots, 3 hits, from 5 points), Bumblecrow transports (huge, four engines: a 3-way spread,
// 8 hits, from 15 points). A bullet or a crash and it's over; a downed plane scores its hits. Planes that get past
// don't count. The firmware feeds the button level and the tilt, restarts after a game over and reads `game.ended`. ----
using pet::Game;
Game &game = pet::runner;

enum { GUNSHIP, CORVETTE, CROW };
const int PY = 67;                                        // her row (the glider's middle)
const float REACH_X = 17, TILT_FULL = 0.25f, GLIDE_V = 120;  // px either side of the middle; tilt (g) for the edge; px/s
const float SHOT_V = 120, FIRE_DT = 0.14f;                // her shots px/s; auto-fire interval
const float HEAT_SHOT = 0.11f, HEAT_COOL = 0.3f, JAM_COOL = 0.7f;  // heat a shot; cooling /s, and while jammed (~1.4 s)
const float BULLET_V = 38, SCROLL_V = 22;                 // enemy shots, ground px/s
const int SHOT_TOP = 25;  // the score bubble ends above this row: planes start firing below it

// Nose down (they fly at her). K outline, M / m olive and its shade, R / r red and its shade, W glass.
const char *const GUNSHIP_ART[] = {
    "K.......K", "rK.KKK.Kr", "RRKRRRKRR", "KRRRRRRRK", ".KKRWRKK.", "...KRK...", "....K....",
};
const char *const CORVETTE_ART[] = {
    "......K......", ".....KmK.....", "..KKKKMKKKK..", "..KmmMMMmmK..", "...KKKMKKK...", ".....KMK.....",
    "KKKKKKMKKKKKK", "KMMMMMMMMMMMK", "KmmmmMWMmmmmK", ".KKKKKWKKKKK.", ".....KWK.....", "......K......",
};
const char *const CROW_ART[] = {
    "..........KKKKK..........", ".........KmKMKmK.........", ".........KmKMKmK.........",
    "...KK....KKMMMKK....KK...", "....KK...KmMMMmK...KK....", ".....KK..KmMMMmK..KK.....",
    "..KKKKKKKKmMMMmKKKKKKKK..", ".KMMMMMMMMMMMMMMMMMMMMMK.", "KMMMMMMMMMMMMMMMMMMMMMMMK",
    "KmmmmmKmKmmMMMmmKmKmmmmmK", ".KmmmmKmKmmMMMmmKmKmmmmK.", "..KKKKKmKKmMWMmKKmKKKKK..",
    "......KKK.KMWMK.KKK......", "..........KMMMK..........", "...........KKK...........",
};
const struct {
  const char *const *art;
  int h, hp;
  float v;  // px/s down
} PLANE[] = {{GUNSHIP_ART, 7, 1, 30}, {CORVETTE_ART, 12, 3, 16}, {CROW_ART, 15, 8, 8}};

struct Plane {
  float x, y, x0, t, hit, fire;  // x0: weave middle; hit: flash left; fire: s to its next shot
  int8_t hp;
  uint8_t kind;
  bool on;
} planes[6];
struct Dot {
  float x, y, vx, vy;
  bool on;
} shots[8], bullets[16];
struct Puff {
  float x, y, vx, vy, life;
  uint8_t c;
} puffs[32];
float mx = 40, mv = 0, tiltTo = 40, aim = 40, bank = 0, heat = 0, fireT = 0, flashT = 0, spawnT = 0, scrollY = 0;
bool jam = false;

int planeW(const Plane &p) { return strlen(PLANE[p.kind].art[0]); }
void burst(float x, float y, int n, float v, float life, uint8_t c0, uint8_t c1) {
  for (int i = 0; i < n; i++)
    for (auto &p : puffs)
      if (p.life <= 0) {
        float a = 6.2832f * rnd(), s = v * (0.3f + 0.7f * rnd());
        p = {x, y, s * cosf(a), s * sinf(a), life * (0.6f + 0.4f * rnd()), rnd() < 0.5f ? c0 : c1};
        break;
      }
}
void fireAt(float x, float y, float vx, float vy) {
  for (auto &b : bullets)
    if (!b.on) {
      b = {x, y, vx, vy, true};
      return;
    }
}

void spawn() {
  float r = rnd();
  bool crowUp = false;
  for (auto &p : planes) crowUp |= p.on && p.kind == CROW;
  uint8_t k = game.score >= 15 && r < 0.15f && !crowUp ? CROW : game.score >= 5 && r < 0.5f ? CORVETTE : GUNSHIP;
  for (auto &p : planes)
    if (!p.on) {
      float x = k == CROW ? 34 + 12 * rnd() : 24 + 32 * rnd();
      p = {x, -(float)PLANE[k].h, x, 0, 0, 1 + rnd(), (int8_t)PLANE[k].hp, k, true};
      return;
    }
}

void finish() {  // stats and the event for the firmware
  game.newBest = pet::gameOver(game.score);
  game.ended = true;
}

void gameStart() {
  pose(IDLE, 0);  // ends a flight
  tgo(SHOULDER, 0);
  game = Game();
  game.on = true;
  mx = tiltTo = aim = 40, mv = bank = heat = fireT = flashT = 0, jam = false, spawnT = 1;
  for (auto &p : planes) p.on = false;
  for (auto &s : shots) s.on = false;
  for (auto &b : bullets) b.on = false;
  for (auto &p : puffs) p.life = 0;
}

void gameStop() {  // left with a long hold, or by find
  if (game.on && !game.dead && game.score > 0) finish();
  game.on = false;
  pose(IDLE, 1);
}

void gameInput(bool held) {  // button level, every frame: a press fires at once, held = auto-fire
  if (held && !game.held) fireT = 0;
  game.held = held;
}

void gameTilt(float g) { tiltTo = 40 + clampf(g / TILT_FULL, -1, 1) * REACH_X; }  // sideways gravity in g, + = right

void down() {  // she's hit: the end
  game.dead = true, game.t = 0, game.held = false;
  burst(mx, PY, 16, 30, 0.8f, FLAME, SHOT);
  finish();
}

void gameStep(float dt) {
  T += dt, game.t += dt, scrollY += SCROLL_V * dt;
  float lvl = 0.6f + fminf(game.t, 60) / 60;  // calm at first, everything quicker as the game goes on (x1.6 after a minute)
  for (auto &p : puffs)
    if (p.life > 0) p.life -= dt, p.x += p.vx * dt, p.y += (p.vy + SCROLL_V) * dt, p.vx *= 1 - 2 * dt, p.vy *= 1 - 2 * dt;
  for (auto &s : shots)
    if (s.on && (s.y -= SHOT_V * dt) < -3) s.on = false;  // up to the top edge (under the bubble)
  for (auto &b : bullets)
    if (b.on) {
      b.x += b.vx * dt, b.y += b.vy * dt;
      if (b.y > N + 2 || b.x < -2 || b.x > N + 2) b.on = false;
      else if (!game.dead && fabsf(b.x - mx) < 2.5f && fabsf(b.y - (PY - 1)) < 3) b.on = false, down();
    }
  for (auto &p : planes) {
    if (!p.on) continue;
    p.t += dt, p.hit -= dt, p.y += PLANE[p.kind].v * lvl * dt;
    int w = planeW(p), h = PLANE[p.kind].h;
    if (p.kind == GUNSHIP) p.x = p.x0 + 9 * sinf(p.t * 2.5f);
    if (p.kind == CORVETTE) p.x += clampf(mx - p.x, -6, 6) * dt;  // edges toward her
    if (p.y > N + 2) {
      p.on = false;
      continue;
    }
    if (!game.dead && p.y > SHOT_TOP && p.y < PY - 16 && (p.fire -= dt * lvl) <= 0) {
      float ny = p.y + h;
      if (p.kind == CORVETTE) {  // aimed
        float dx = mx - p.x, dy = PY - ny, l = sqrtf(dx * dx + dy * dy);
        fireAt(p.x, ny, BULLET_V * dx / l, BULLET_V * dy / l), p.fire = 2.2f;
      } else if (p.kind == CROW) {  // a spread from the nose
        for (int i = -1; i <= 1; i++) fireAt(p.x, ny, BULLET_V * sinf(0.4f * i), BULLET_V * cosf(0.4f * i));
        p.fire = 2.6f;
      } else p.fire = 1e9f;  // the gunships only ram
    }
    for (auto &s : shots)
      if (s.on && fabsf(s.x - p.x) < w / 2.f && s.y > p.y && s.y < p.y + h) {
        s.on = false, p.hit = 0.08f;
        burst(s.x, s.y, 2, 20, 0.2f, SHOT, CREAM);
        if (--p.hp <= 0) {
          p.on = false, game.score += PLANE[p.kind].hp;
          burst(p.x, p.y + h / 2.f, 6 + 2 * h, 10 + 2 * h, 0.7f, FLAME, p.kind == GUNSHIP ? BLOOD : GREY);
          break;
        }
      }
    if (p.on && !game.dead && fabsf(p.x - mx) < w / 2.f + 1.5f && PY - 1 > p.y - 2 && PY - 1 < p.y + h + 2) {
      p.on = false, burst(p.x, p.y + h / 2.f, 12, 25, 0.7f, FLAME, GREY);
      down();
    }
  }
  if (game.dead) {  // she goes down trailing smoke; the firmware restarts on a press, gives up after 10 s
    if (rnd() < 0.5f) burst(mx, PY, 1, 6, 0.6f, GREY, CREAM);
    if (game.t > 10) game.on = false, pose(IDLE, 1);
    return;
  }
  // Smooth flight: the aim follows the tilt (hand tremor filtered), the glide eases in and out, the wings bank slowly:
  // a jittery tilt no longer rocks her left-right.
  aim += (tiltTo - aim) * fminf(1, 8 * dt);
  mv += (clampf((aim - mx) * 7, -GLIDE_V, GLIDE_V) - mv) * fminf(1, 16 * dt);
  mx += mv * dt;
  bank += (clampf(mv / 60, -1, 1) - bank) * fminf(1, 6 * dt);
  flashT -= dt, fireT -= dt;
  if ((heat -= (jam ? JAM_COOL : HEAT_COOL) * dt) <= 0) heat = 0, jam = false;
  if (game.held && !jam && fireT <= 0) {
    for (auto &s : shots)
      if (!s.on) {
        s = {mx, PY - 7.f, 0, 0, true};
        break;
      }
    fireT = FIRE_DT, flashT = 0.05f;
    if ((heat += HEAT_SHOT) >= 1) heat = 1, jam = true;
  }
  if ((spawnT -= dt * lvl) <= 0) spawn(), spawnT = 1.4f + 1.2f * rnd() - fminf(game.score, 40) * 0.02f;
}

// The valley from above: grass, a river mirroring the sky, clumps of forest; all of it rolls down the screen.
void drawGround() {
  for (int y = 0; y < N; y++) {
    int r = int(scrollY) + N - 1 - y;  // world row
    float rc = 40 + 18 * sinf(r * 0.021f) + 7 * sinf(r * 0.057f + 1), hw = 3.5f + 1.5f * sinf(r * 0.04f);
    for (int x = 0; x < N; x++) {
      float d = fabsf(x + 0.5f - rc);
      uint8_t c = h01(r / 4 * 97 + x / 4) < 0.15f ? GRASS2 : GRASS;  // fields, 4 px patches
      if (d < hw) c = (r * 7 + x * 3) % 23 == 0 ? SKY3 : SKY2;
      else if (d < hw + 1) c = GRASSL;
      fb[y * N + x] = c;
    }
  }
  for (int b = int(scrollY) / 12 - 2, e = (int(scrollY) + N) / 12 + 2; b < e; b++) {  // a clump every 12 rows, off the river
    float wr = b * 12 + 12 * h01(b * 5 + 2), cx = 80 * h01(b * 3), rad = 3 + 4 * h01(b * 3 + 1);
    float rc = 40 + 18 * sinf(wr * 0.021f) + 7 * sinf(wr * 0.057f + 1);
    if (fabsf(cx - rc) < rad + 6) cx += cx < rc ? -12 : 12;
    float y = scrollY + N - 1 - wr;
    for (int k = 0; k < 3; k++) disc(cx + (k - 1) * rad * 0.7f, y + (k == 1 ? -rad * 0.4f : 0), rad * (k == 1 ? 1 : 0.75f), TREE);
    disc(cx - rad * 0.3f, y - rad * 0.6f, rad * 0.35f, GRASS2);
  }
}

void drawPlane(const Plane &p) {
  const char *const *a = PLANE[p.kind].art;
  int w = planeW(p), x0 = lroundf(p.x - w / 2.f), y0 = lroundf(p.y);
  for (int r = 0; r < PLANE[p.kind].h; r++)  // shadow on the ground first
    for (int c = 0; c < w; c++)
      if (a[r][c] != '.') px(x0 + c + 4, y0 + r + 6, SHADOW);
  for (int r = 0; r < PLANE[p.kind].h; r++)
    for (int c = 0; c < w; c++) {
      char ch = a[r][c];
      if (ch == '.') continue;
      uint8_t col = ch == 'M' ? METAL : ch == 'm' ? METAL2 : ch == 'R' ? BLOOD : ch == 'r' ? BLOOD2 : ch == 'W' ? CREAM : INK;
      px(x0 + c, y0 + r, p.hit > 0 && ch != 'K' ? uint8_t(CREAM) : col);
    }
}

void drawMehve() {  // nose up, banking into the turn; she lies on top, Teto on her back
  float x = mx, y = PY, span = 9 - 2 * fabsf(bank);
  if (game.dead) span = 9 * cosf(game.t * 9), y += fminf(game.t, 2) * 3;  // spinning down
  for (int s = -1; s <= 1; s += 2) {
    float tx = x + s * span, ty = y + 2.5f + s * 1.5f * bank;
    capsule(x, y - 1, tx, ty, 2.0f, INK);
  }
  capsule(x, y - 6, x, y + 5, 1.8f, INK);
  for (int s = -1; s <= 1; s += 2) {
    float tx = x + s * span, ty = y + 2.5f + s * 1.5f * bank;
    capsule(x, y - 1, tx, ty, 1.1f, HULL), capsule(x, y + 0.2f, tx, ty + 0.6f, 0.5f, HULL2);
  }
  capsule(x, y - 5.5f, x, y + 4.5f, 1.0f, HULL), px(x, y - 6, JET);
  capsule(x, y + 1, x, y + 4, 0.7f, PANTS);   // legs back
  capsule(x, y - 2, x, y + 1, 1.1f, TUNIC);   // body
  disc(x, y - 3.5f, 1.3f, HAIR), px(x, y - 4, HAIRL);
  px(x + 1, y, FUR), px(x + 1, y + 1, FUR2);  // Teto
  if (flashT > 0) px(x, y - 8, SHOT), px(x - 1, y - 7, SHOT), px(x + 1, y - 7, SHOT);
  if (jam && rnd() < 0.3f) burst(x, y - 7, 1, 8, 0.4f, CREAM, GREY);  // the gun steams while it cools
}

void gameDraw() {
  drawGround();
  for (auto &p : planes)
    if (p.on) drawPlane(p);
  for (auto &b : bullets)
    if (b.on) disc(b.x, b.y, 1.1f, BLOOD2), px(b.x, b.y, FLAME);
  for (auto &s : shots)
    if (s.on) px(s.x, s.y, SHOT), px(s.x, s.y + 1, SHOT), px(s.x, s.y + 2, FLAME);
  drawMehve();
  for (auto &p : puffs)
    if (p.life > 0) px(p.x, p.y, p.c);
  const int L = 31, W = 18, Y = 75;  // the heat bar: green, amber, red; blinking while jammed
  for (int x = L - 1; x <= L + W; x++) px(x, Y - 1, INK), px(x, Y + 2, INK);
  px(L - 1, Y, INK), px(L - 1, Y + 1, INK), px(L + W, Y, INK), px(L + W, Y + 1, INK);
  uint8_t c = jam ? (fmodf(T, 0.25f) < 0.12f ? EARRING : CREAM) : heat < 0.6f ? LEAF : heat < 0.85f ? FUR : EARRING;
  for (int x = 0; x < W; x++) {
    uint8_t f = x < heat * W ? c : uint8_t(METAL2);
    px(L + x, Y, f), px(L + x, Y + 1, f);
  }
}

// Bubble text: score and high score while running, the result after a game over.
void gameLabel(char *out, int cap) {
  int best = pet::best() > game.score ? pet::best() : game.score;
  if (!game.dead) snprintf(out, cap, "%d HI %d", game.score, best);
  else if (game.newBest) snprintf(out, cap, "NEW BEST %d", game.score);
  else snprintf(out, cap, "%d BEST %d", game.score, best);
}

// ---- Events from the firmware ----

// Device shaken: Teto leaps off her shoulder, she startles and jumps, tries to catch him for a few seconds.
// Asleep: a startled hop, a yawn, back to sleep. Starving: angry, no play. Flying: a climb on the wind.
void shake() {
  if (game.on) return;
  if (rove) {
    lift = 14, girl.happy = 1.5f;
    return;
  }
  if (girl.st == SLEEP) {
    awake = 0, girl.peek = 0, leap(girl.x, 6, 0), girl.wake = true;
    return;
  }
  if (pet::s.full < 10 && grounded()) {
    awake = 0, pose(ANGRY, 1.2f);
    girl.vy = -sqrtf(2 * G * 5), girl.vx = -girl.dir * 22;
    return;
  }
  awake = 6, vis.on = false;
  if (teto.st != LEAP) hop(roamX(), GROUND, 8 + 8 * rnd(), false);
  if (grounded()) girl.yay = false, leap(girl.x, 5, 0);
}

// Message/button: the girl raises her arms and hops twice for joy (a sitting Teto hops too). Asleep: one eye opens.
void cheer() {
  if (game.on) return;
  if (girl.st == SLEEP) {
    girl.peek = 2;
    return;
  }
  girl.happy = 2.5f;
  if (rove) lift = fmaxf(lift, 6);
  else if (grounded() && girl.st != EAT) pose(CROUCH, 0.15f), girl.hops = 2, girl.yay = true;
  if (teto.st == PERCH) hop(teto.x, GROUND, 5, false);
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

// Music screen on / off (the firmware calls this every frame): she takes off on the glider; asleep, she sleeps on.
void music(bool on) {
  if (game.on) on = false;
  if (on == tunes) return;
  tunes = on;
  if (on && girl.st != SLEEP && mood != pet::SLEEP) travel(0);
  else if (!on && rove && tripLeft <= 0) pose(IDLE, 1);
}

void init() {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float far = 42 + 3 * sinf(x * 0.09f) + 2 * sinf(x * 0.23f + 1), near = 49 + 2 * sinf(x * 0.14f + 2) + 1.5f * sinf(x * 0.31f);
      uint8_t c = skyAt(x, y);
      bgAir[y * N + x] = c;
      if (y >= far) c = HILL;
      if (y >= near) c = HILL2;
      if (y >= GROUND) c = y < GROUND + 3 || (y < GROUND + 5 && bayer(x, y) > 0.5f) ? GRASS : GRASS2;
      fb[y * N + x] = c;
    }
  for (auto &m : MILLS)  // windmill towers: from hub down to the ridge
    for (int y = m[1] + 1; fb[y * N + (int)m[0]] <= SKY3; y++) px(m[0], y, m[4]);
  memcpy(bg, fb, sizeof bg);
  for (int i = 0; i < NW; i++) gust(i, N * rnd());
  for (int i = 0; i < NS; i++) sx[i] = 10 + rnd() * 60, sy[i] = 3 + rnd() * 30, sp[i] = rnd() * 6.28f;
}

void step(float dt) {
  if (game.on) {
    gameStep(dt);
    gameDraw();
    return;
  }
  update(dt);
  if (rove) return drawRove();
  drawSky();
  if (bowl.on) art(PLATE, 3, lroundf(bowl.x) - 4, GROUND - 3);
  drawGirl();
  attach();
  drawTeto();
  drawVisitor();
}

}  // namespace nausicaa
#ifndef SCENE_PICK
namespace scene = nausicaa;  // firmware.ino talks to whichever scene is included through this name (pets.h: to all)
#endif
#define SCENE_PET            // firmware.ino: this scene has the pet features (pet.h, game, day/night)
