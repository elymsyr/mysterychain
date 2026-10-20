// Cat scene: draws into an 80x80 color-indexed buffer (fb), scaled 3x on screen.
// No Arduino dependency: also compiles on a PC for frame-by-frame testing.
// The cat is a pet (pet.h): its mood picks what it does, the sky follows the clock (tint), and it has a runner game (game.h).
// Travel (travel()): the cat runs to the right through a scrolling landscape (parallax layers, tinted by the clock) and
// jumps what's in its way. The music screen shows it; now and then it also sets off on its own.
#pragma once
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pet.h"

namespace kitty {

const int N = 80, GROUND = 58;  // logical resolution, ground row (paws stand here)
const float G = 260;            // gravity, px/s²

enum : uint8_t { SKY0, SKY1, SKY2, SKY3, GRASS, GRASS2, STAR, MOON, INK, FUR, FUR2, CREAM, PINK, YARN, YARN2, SHADOW,
                 SUN, GREY, CLAY, CLOUD, FAR, HILL, TREE, DROP, FLAKE, FLASH, NCOL };
uint8_t PAL[NCOL][3] = {  // night look; tint() rewrites the sky, grass, star, moon, shadow and sun entries
    {14, 16, 44},   {28, 26, 78},   {58, 42, 104},  {104, 60, 120},  // sky, top to horizon
    {70, 150, 70},  {38, 96, 50},   {255, 240, 180}, {250, 230, 160},  // grass, dark grass, star, moon
    {30, 18, 22},   {255, 150, 40}, {200, 95, 25},  {255, 236, 210}, {255, 140, 160},  // cat
    {80, 140, 255}, {40, 70, 200},  {26, 70, 40},   // yarn ball, shadow
    {255, 214, 90}, {178, 176, 186}, {196, 98, 60}, {40, 48, 84},  // sun, mouse/bird grey, flower pot clay, cloud
    {48, 40, 92},   {30, 62, 56},   {20, 44, 40},   // travel: far mountains, hills, trees
    {205, 222, 255}, {250, 252, 255}, {214, 212, 250},  // weather: raindrop, snow, lightning flash
};

// ---- Time of day (K6): the palette indices stay, their RGB follows the clock (pet::look) ----
const uint8_t TINTED[] = {SKY0, SKY1, SKY2, SKY3, GRASS, GRASS2, STAR, MOON, SHADOW, SUN, CLOUD, FAR, HILL, TREE};
const int NT = sizeof TINTED;
const uint8_t LOOK[4][NT][3] = {  // pet::NIGHT, DAWN, DAY, DUSK
    {{14, 16, 44}, {28, 26, 78}, {58, 42, 104}, {104, 60, 120}, {70, 150, 70}, {38, 96, 50},
     {255, 240, 180}, {250, 230, 160}, {26, 70, 40}, {255, 214, 90}, {40, 48, 84}, {48, 40, 92}, {30, 62, 56}, {20, 44, 40}},
    {{70, 80, 150}, {150, 110, 165}, {235, 140, 140}, {255, 185, 130}, {84, 150, 72}, {46, 100, 56},
     {255, 240, 200}, {250, 235, 200}, {34, 80, 46}, {255, 240, 170}, {255, 206, 196}, {170, 120, 160}, {96, 122, 92}, {62, 96, 70}},
    {{74, 144, 226}, {104, 170, 236}, {142, 196, 244}, {186, 222, 250}, {96, 186, 80}, {54, 128, 62},
     {255, 240, 200}, {240, 240, 240}, {44, 110, 52}, {255, 236, 140}, {250, 252, 255}, {132, 168, 214}, {84, 156, 92}, {46, 116, 62}},
    {{44, 46, 112}, {130, 66, 128}, {226, 98, 92}, {255, 158, 84}, {72, 132, 62}, {40, 88, 48},
     {255, 240, 190}, {250, 225, 170}, {30, 72, 42}, {255, 226, 150}, {250, 160, 130}, {112, 66, 120}, {68, 92, 66}, {44, 68, 50}},
};
using pet::SUNRISE;
using pet::SUNSET;

int clockMin = -1;   // minute of day, -1 = clock unknown (night look, moon at its old spot)
float nightW = 1;    // 1 = full night (all stars), 0 = none

void tint(int minuteOfDay) {
  clockMin = minuteOfDay;
  int a, b;
  float f = pet::look(clockMin, a, b);
  for (int k = 0; k < NT; k++)
    for (int c = 0; c < 3; c++) PAL[TINTED[k]][c] = lroundf(LOOK[a][k][c] * (1 - f) + LOOK[b][k][c] * f);
  nightW = (a == pet::NIGHT ? 1 - f : 0) + (b == pet::NIGHT ? f : 0);
  // Weather: the sky and far layers grey over (toward their own brightness), a storm or rain also darkens the land.
  static const uint8_t AIR[] = {SKY0, SKY1, SKY2, SKY3, FAR}, LAND[] = {GRASS, GRASS2, HILL, TREE, SHADOW};
  float ov = pet::OVERCAST[pet::wx], dark = pet::gloom();
  for (uint8_t k : AIR) pet::dim(PAL[k], ov, dark);
  pet::dim(PAL[CLOUD], ov, dark * 0.6f);
  for (uint8_t k : LAND) pet::dim(PAL[k], ov * 0.3f, dark);
}
bool sunUp() { return clockMin >= SUNRISE && clockMin < SUNSET; }

// Head: K outline, O fur, W cream, P pink. Symmetric, no mirroring needed when turning.
const int HW = 12, HH = 10;
#define EARS ".K........K.", "KPK......KPK", "KPPK....KPPK", "KOOKKKKKKOOK"
#define MOUTH "KOOOWPPWOOOK", ".KOWWWWWWOK.", "..KKKKKKKK.."
const char *const HEAD[][HH] = {
    {EARS, "KOOOOOOOOOOK", "KOKWOOOOKWOK", "KOKKOOOOKKOK", MOUTH},                  // eyes open (with highlight)
    {EARS, "KOOOOOOOOOOK", "KOOOOOOOOOOK", "KOKKOOOOKKOK", MOUTH},                  // blink
    {EARS, "KOOOOOOOOOOK", "KOOKOOOOKOOK", "KOKOKOOKOKOK", "KOOOWPPWOOOK", ".KOWWPPWWOK.", "..KKKKKKKK.."},  // happy ^^, mouth open
    {EARS, "KOOOOOOOOOOK", "KOKKKOOKKKOK", "KOOKOOOOKOOK", MOUTH},                  // half-closed (bored)
    {EARS, "KOOOOOOOOOOK", "KOKOKOOKOKOK", "KOOKOOOOKOOK", MOUTH},                  // closed (asleep)
    {EARS, "KOOOOOOOOOOK", "KOKWOOOKOKOK", "KOKKOOOOKOOK", MOUTH},                  // one eye open (peek)
    {EARS, "KOOOOOOOOOOK", "KOKOKOOKOKOK", "KOOKOOOOKOOK", "KOOOKKKKOOOK", ".KOWKPPKWOK.", "..KKKKKKKK.."},  // yawn
    {EARS, "KOKOOOOOOKOK", "KOOKOOOOKOOK", "KOOKKOOKKOOK", MOUTH},                  // frown (angry)
    {EARS, "KOKOKOOKOKOK", "KOOKOOOOKOOK", "KOKOKOOKOKOK", MOUTH},                  // x x (game over)
    {EARS, "KOKKOOOOKKOK", "KOKWOOOOKWOK", "KOKKOOOOKKOK", "KOOOWKKWOOOK", ".KOWWKKWWOK.", "..KKKKKKKK.."},  // wide, "o" (thunder)
    {EARS, "KOOOOOOOOOOK", "KKKKKKKKKKKK", "KOKKKOOKKKOK", MOUTH},                  // sunglasses
};
#undef EARS
#undef MOUTH
enum { EYES_OPEN, EYES_BLINK, EYES_HAPPY, EYES_HALF, EYES_CLOSED, EYES_PEEK, EYES_YAWN, EYES_FROWN, EYES_DIZZY, EYES_WIDE,
       EYES_SHADES };

// Small sprites: same letters as the head, plus Y grey, C clay, g/G grass, F dark fur, b/B yarn, # = the caller's color.
const char *const BOWL_FULL[] = {"..F.F.F..", "KFFFFFFFK", "KBBBBBBBK", ".KBBBBBK.", "..KKKKK.."};
const char *const BOWL_EMPTY[] = {".........", "KbbbbbbbK", "KBBBBBBBK", ".KBBBBBK.", "..KKKKK.."};
const char *const ZZ[] = {"####", "..#.", ".#..", "####"};
const char *const LOVE[] = {".P.P.", "PPPPP", ".PPP.", "..P.."};
const char *const NOTE[2][5] = {{"..##", "..#.", "..#.", "###.", "##.."},   // eighth note, beamed pair
                                {".####", ".#..#", ".#..#", "##.##", "#..#."}};
const char *const FLY[2][5] = {{"K.....K", ".K...K.", "KPK.KPK", "KPPKPPK", ".KK.KK."},   // butterfly, wings open
                               {".K...K.", "..K.K..", ".KPKPK.", ".KPKPK.", "..K.K.."}};  // wings up
const char *const RAT[2][4] = {{".....KP.", "K.KKKYYK", ".KYYYYKY", "..K..K.."},  // facing right, two run frames
                                 {".....KP.", "K.KKKYYK", ".KYYYYKY", ".K..K..."}};

uint8_t fb[N * N], bg[N * N];
// The runner game is seen from farther away: a GN x GN view (~1.67x on screen instead of 3x) over the same world coordinates,
// shifted by OX, OY (world x = view x - OX). px() draws into whichever view is on (step() picks it).
const int GN = 144, OX = 32, OY = 31;  // OX: the bubble (80 px coordinates) stays centered
uint8_t gfb[GN * GN];
uint8_t *view = fb;
int VN = N, vx0 = 0, vy0 = 0;  // view size and shift
float T = 0, awake = 0;  // awake > 0: cat is in play mode (refilled by a shake)
bool tunes = false;      // music screen on (music()): the cat travels
bool rove = false;       // travelling (travel())
float tripLeft = 0, dist = 0;  // s of travel left (0 = until the music stops), px the world has scrolled
uint32_t rs = 2463534242u;
float rnd() {
  rs ^= rs << 13, rs ^= rs >> 17, rs ^= rs << 5;
  return (rs >> 8) / 16777216.0f;
}
float clampf(float v, float lo, float hi) { return fminf(fmaxf(v, lo), hi); }

inline void px(int x, int y, uint8_t c) {
  x += vx0, y += vy0;
  if ((unsigned)x < (unsigned)VN && (unsigned)y < (unsigned)VN) view[y * VN + x] = c;
}
void setView(bool wide) { view = wide ? gfb : fb, VN = wide ? GN : N, vx0 = wide ? OX : 0, vy0 = wide ? OY : 0; }

void art(const char *const *rows, int h, int x, int y, int flip = 1, uint8_t hash = INK) {
  static const char K[] = "KOWPYCgGFbB#";
  const uint8_t C[] = {INK, FUR, CREAM, PINK, GREY, CLAY, GRASS, GRASS2, FUR2, YARN, YARN2, hash};
  int w = strlen(rows[0]);
  for (int r = 0; r < h; r++)
    for (int c = 0; c < w; c++) {
      const char *k = strchr(K, rows[r][flip > 0 ? c : w - 1 - c]);
      if (k && *k) px(x + c, y + r, C[k - K]);
    }
}

// Paints pixels within r of a line segment (legs, tail; a disc if both ends match).
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

// Body: ellipse rotated by a (nose up = +). grow>0: outline pass.
void body(float bx, float by, float rx, float ry, float a, int dir, float grow) {
  float ca = cosf(a), sa = sinf(a), R = fmaxf(rx, ry) + grow + 1;
  for (int y = floorf(by - R); y <= by + R; y++)
    for (int x = floorf(bx - R); x <= bx + R; x++) {
      float dx = (x + 0.5f - bx) * dir, dy = y + 0.5f - by;
      float lx = dx * ca - dy * sa, ly = dx * sa + dy * ca;
      float ex = lx / (rx + grow), ey = ly / (ry + grow);
      if (ex * ex + ey * ey > 1) continue;
      bool stripe = ly < -ry * 0.15f && lx > -7 && lx < 3 && fmodf(lx + 30, 3) < 1;
      px(x, y, grow > 0 ? INK : ly > ry * 0.35f ? CREAM : stripe ? FUR2 : FUR);
    }
}

void shadow(float x, float w) {
  for (int i = -(int)w; i <= (int)w; i++) px(x + i, GROUND, SHADOW);
  for (int i = -(int)(w * 0.6f); i <= (int)(w * 0.6f); i++) px(x + i, GROUND + 1, SHADOW);
}

// ---- Stars, sun, moon and background ----
const int NS = 16;
uint8_t sx[NS], sy[NS];
float sp[NS];

uint8_t ramp(int x, int y, int h) {  // 4-step gradient over h rows (0..3) with 2x2 Bayer dithering
  static const float B[4] = {0.125f, 0.625f, 0.875f, 0.375f};
  float v = y * 3.0f / (h - 1);
  int b = v;
  return b + (v - b > B[(y & 1) * 2 + (x & 1)]);
}
uint8_t skyAt(int x, int y) { return SKY0 + ramp(x, y, GROUND); }
bool isSky(int x, int y) {  // past the background's edge (the game's wider view): sky above the grass
  return (unsigned)x < N && (unsigned)y < N ? bg[y * N + x] <= SKY3 : y < GROUND - 1;
}
float wrap(float v, float m) { return v - m * floorf(v / m); }  // v mod m, never negative
float h01(int i) {  // a fixed random number in 0..1 per integer (scenery along the trip)
  uint32_t h = i * 2654435761u;
  h ^= h >> 15, h *= 2246822519u, h ^= h >> 13;
  return (h & 0xffff) / 65536.0f;
}

void skyDisc(float cx, float cy, float r, uint8_t c, float cutX = 0, float cutY = 0, float cutR = 0) {  // only over sky
  for (int y = floorf(cy - r); y <= cy + r; y++)
    for (int x = floorf(cx - r); x <= cx + r; x++)
      if (isSky(x, y) && hypotf(x + 0.5f - cx, y + 0.5f - cy) < r && (cutR == 0 || hypotf(x + 0.5f - cutX, y + 0.5f - cutY) > cutR))
        px(x, y, c);
}

// Clouds drift by day, fading in by density at dawn and out at dusk. On a trip they slide back slowly (far away).
// Weather: more of them (also at night), wind blows them fast.
void clouds() {
  static const float B[4] = {0.125f, 0.625f, 0.875f, 0.375f};
  static const float CY[6] = {8, 26, 36, 17, 31, 4}, CW[6] = {7, 5, 6, 8, 6, 9}, SPD[6] = {1.6f, 2.4f, 1.1f, 1.9f, 1.3f, 2.1f},
                     X0[6] = {10, 60, 35, 85, 0, 48};
  int n = pet::wx == pet::CLEAR ? 3 : pet::wx == pet::WIND ? 4 : 6;
  float fast = pet::wx == pet::WIND ? 8 : pet::wx == pet::STORM ? 3 : 1;
  float show = fmaxf(1 - nightW, n == 6 ? 0.9f : 0);  // share of cloud pixels drawn (dithered)
  for (int k = 0; k < n; k++) {
    float cx = wrap(X0[k] + T * SPD[k] * fast - dist * 0.06f, N + 30) - 15, w = CW[k];
    for (int y = CY[k] - 4; y <= CY[k] + 2; y++)
      for (int x = cx - w - 3; x <= cx + w + 3; x++) {
        if (!isSky(x, y) || show <= B[((y + 1) & 1) * 2 + (x & 1)]) continue;
        float dx = x + 0.5f - cx, dy = y + 0.5f - CY[k];
        if (dy < 1.5f && (hypotf(dx / (w + 2), dy / 2) < 1 || hypotf(dx + 2, dy + 1.5f) < 2.8f || hypotf(dx - w / 3, dy + 2) < 2.5f))
          px(x, y, CLOUD);
      }
  }
}

void drawSky() {  // over the background already in the view (memcpy of bg, or the game's own)
  float mx = 60, my = 17;  // moon at its old spot while the clock is unknown
  bool moon = true;
  if (clockMin >= 0) {  // sun or moon on an arc from left (rise) to right (set)
    moon = !sunUp();
    float p = moon ? float((clockMin - SUNSET + 1440) % 1440) / (1440 - SUNSET + SUNRISE) : float(clockMin - SUNRISE) / (SUNSET - SUNRISE);
    mx = 12 + 56 * p, my = GROUND - 2 - 28 * sinf(3.1416f * p);
  }
  float ov = pet::OVERCAST[pet::wx];  // overcast hides stars by count, a thick one the sun and moon too
  for (int i = 0; i < NS; i++)  // twinkling stars, fading in by count at dusk
    if (i < NS * nightW * (1 - ov) && sinf(T * 1.7f + sp[i]) > -0.2f && !(moon && hypotf(sx[i] - mx, sy[i] - my) < 6))
      px(sx[i], sy[i], STAR);
  if (ov < 0.6f && moon) skyDisc(mx, my, 4.6f, MOON, mx + 2.5f, my - 1.5f, 4.2f);  // crescent
  else if (ov < 0.6f) {
    skyDisc(mx, my, 4.2f, SUN);
    if (pet::wx == pet::CLEAR)  // clear: slowly turning rays
      for (int k = 0; k < 8; k++) {
        float a = k * 0.785f + T * 0.4f, r = 6 + (k & 1);
        if (isSky(mx + r * cosf(a), my + r * sinf(a))) px(mx + r * cosf(a), my + r * sinf(a), SUN);
      }
  }
  clouds();
}

// ---- Weather (pet::wx, weather.h): rain plants an umbrella the cat sits under ----
const int UMB_X = 40, UMB_CX = UMB_X + 3, UMB_R = 14, UMB_Y = GROUND - 22;  // where the cat sits, canopy center, radius, rim row
const int WIND_DIR = 1;
float canopy(float x);
float landRow() { return GROUND - 1 + (N - 4 - GROUND) * rnd() * rnd(); }  // mostly toward the back
float drift();
bool airy(uint8_t c) { return c <= SKY3 || c == CLOUD || c == STAR; }
void thunder();
#include "weather.h"
bool umbrella() { return wet() && !rove; }
float canopy(float x) {  // the umbrella's top at x (rain stops here), or a big number off it
  float u = (x - UMB_CX) / UMB_R;
  return umbrella() && fabsf(u) < 1 ? UMB_Y - 7 * sqrtf(1 - u * u) : 1e9f;
}

// ---- Cat, yarn ball, bowl and visitors ----
enum { IDLE, RUN, CROUCH, JUMP, LAND, BAT, SLEEP, EAT, REFUSE, PURR, YAWN, BORED, HUNGRY, ANGRY, KO, GROOM, SPIN, DANCE,
       SEEK, SHELTER, SUNBATHE, SNOWPLAY };  // weather: walk to the umbrella, sit under it, shades on, paw at snowflakes
struct {
  float x = 26, y = GROUND, vx = 0, vy = 0, t = 0, dur = 1, phase = 0, happy = 0, peek = 0, scare = 0;  // scare: thunder
  int dir = 1, st = IDLE, hops = 0;
  bool kicked = false, wake = false;  // wake: yawn after this landing (startled out of sleep)
} cat;
struct {
  float x = 56, y = GROUND - 3, vx = 0, vy = 0, rot = 0;
} ball;
struct {
  float x = 0;
  bool on = false, food = false;
} bowl;
enum { BUTTERFLY, MOUSE, BLOWN };  // BLOWN: a leaf in the wind
struct {
  float x = 0, y = 0, vx = 0, vy = 0, t = 0, home = 0;
  int kind = BUTTERFLY;
  bool on = false, flee = false;
} vis;

pet::Mood mood = pet::HAPPY;
float visitT = 150, yawnT = 0, funT = 12, tripT = 200;  // s until the next visitor / tired yawn / idle trick / trip

void setMood(pet::Mood m) { mood = m; }

void go(int st, float dur) {
  cat.st = st, cat.t = 0, cat.dur = dur, cat.kicked = false;
  if (st != EAT && st != HUNGRY) bowl.on = false;
}
void pose(int st, float dur) {  // a ground state from wherever the cat is (ends a trip)
  rove = false, cat.y = GROUND, cat.vx = cat.vy = 0, cat.hops = 0, cat.wake = false, cat.peek = 0;
  go(st, dur);
}
bool grounded() { return cat.st != CROUCH && cat.st != JUMP && cat.st != LAND && cat.st != BAT && cat.st != ANGRY && cat.st != KO; }
bool chasing() { return vis.on && !vis.flee; }
float target() { return chasing() ? vis.x : ball.x; }
int side() { return target() >= cat.x ? 1 : -1; }
void kick(float vx, float vy) { ball.vx = vx, ball.vy = vy, cat.happy = 1.2f; }
void showBowl(bool food) {  // in front of the cat, on the side toward the middle
  cat.dir = cat.x < N / 2 ? 1 : -1;
  bowl.x = cat.x + cat.dir * 13, bowl.on = true, bowl.food = food;
}

void leap(float xt, float h, int hops) {  // to xt, h high; hops = how many more hops after
  xt = clampf(xt, 16, 64);
  cat.vy = -sqrtf(2 * G * h);
  cat.vx = (xt - cat.x) / (2 * -cat.vy / G);
  cat.hops = hops;
  go(JUMP, 0);
}

void think() {  // idle wait is over: pick the next game
  float r = rnd();
  cat.dir = side();
  if (r < 0.2f) cat.hops = 3, cat.happy = 2.5f, go(CROUCH, 0.2f);                // 3 happy hops in place
  else if (fabsf(ball.x - cat.x) > 24 && r < 0.6f) cat.hops = 0, go(CROUCH, 0.7f);  // stalk from afar + pounce
  else go(RUN, 0);
}

// A butterfly (day) or a mouse (night) comes in, waits near the cat; the cat stalks and pounces.
void visit() {
  if (vis.on) return;
  vis.on = true, vis.flee = false, vis.t = 0;
  vis.kind = pet::wx == pet::WIND ? BLOWN : sunUp() ? BUTTERFLY : MOUSE;
  int open = cat.x < N / 2 ? 1 : -1;  // the roomier side of the cat
  vis.x = open > 0 ? 84 : -4;
  vis.home = clampf(cat.x + open * (24 + 10 * rnd()), 18, 62);
  vis.y = vis.kind == MOUSE ? GROUND - 1 : vis.kind == BLOWN ? GROUND - 10 : GROUND - 18;
}
void flee() {
  vis.flee = true;
  vis.vx = (vis.kind == BLOWN ? 1 : vis.x >= cat.x ? 1 : -1) * 70, vis.vy = vis.kind == MOUSE ? 0 : -30;  // a leaf goes with the wind
}

void travel(float secs);
void roveStep(float dt);

void idleDone() { go(IDLE, (0.6f + 1.2f * rnd()) * (mood == pet::TIRED ? 2 : 1)); }  // tired: plays half as often

void update(float dt) {
  T += dt, cat.t += dt, cat.happy -= dt, cat.peek -= dt, cat.scare -= dt, awake -= dt, yawnT -= dt;
  if (cat.st == IDLE && mood == pet::HAPPY && !vis.on && !rove) funT -= dt, tripT -= dt;
  if (rove) return roveStep(dt);

  ball.vy += G * dt, ball.x += ball.vx * dt, ball.y += ball.vy * dt;
  if (ball.y >= GROUND - 3) {
    ball.y = GROUND - 3;
    ball.vy = ball.vy > 50 ? -ball.vy * 0.45f : 0;
    ball.vx -= ball.vx * fminf(2.5f * dt, 1);  // rolling friction
    if (fabsf(ball.vx) < 2) ball.vx = 0;
  }
  if (ball.x < 8 || ball.x > 72) ball.x = clampf(ball.x, 8, 72), ball.vx *= -0.6f;  // bounce off the screen edge
  ball.rot += ball.vx * dt / 3;

  if (vis.on) {
    vis.t += dt;
    if (vis.flee) {
      vis.x += vis.vx * dt, vis.y += vis.vy * dt;
      if (vis.x < -10 || vis.x > N + 10 || vis.y < -10) vis.on = false;
    } else {
      float sp = (vis.kind == MOUSE ? 40 : 25) * dt;
      vis.x += clampf(vis.home - vis.x, -sp, sp);
      if (vis.t > 12) flee();  // the cat never came
    }
  }
  bool rest = ball.vx == 0 && ball.vy == 0;
  if (mood == pet::HAPPY && cat.st == IDLE && awake <= 0 && rest && !vis.on && !wet() && !snowing() && (visitT -= dt) <= 0)
    visit(), visitT = 120 + 180 * rnd();

  switch (cat.st) {
    case IDLE:
      if (fabsf(target() - cat.x) > 4) cat.dir = side();
      if (cat.t <= cat.dur || !rest) break;  // stays put unless shaken, or the mood says otherwise
      if (chasing()) {
        if (fabsf(vis.home - vis.x) < 1) go(RUN, 0);
      } else if (awake > 0) think();
      else if (wet() && fabsf(cat.x - UMB_X) > 1.5f) go(SEEK, 0);  // rain: home under the umbrella first
      else if (mood == pet::SLEEP) go(SLEEP, 0);
      else if (mood == pet::HUNGRY) showBowl(false), go(HUNGRY, 0);
      else if (mood == pet::SAD) go(BORED, 0);
      else if (mood == pet::TIRED && yawnT <= 0) yawnT = 30, go(YAWN, 1.5f);
      else if (wet()) go(SHELTER, 0);  // happy or tired: sits the rain out
      else if (tripT <= 0) tripT = 180 + 240 * rnd(), travel(15 + 10 * rnd());
      else if (funT <= 0) {
        float r = rnd(), w = rnd();
        funT = (snowing() ? 4 : 10) + 15 * rnd();
        if (snowing() && w < 0.75f) go(SNOWPLAY, 2.4f);
        else if (pet::wx == pet::CLEAR && sunUp() && w < 0.4f) go(SUNBATHE, 8);
        else r < 0.45f ? go(GROOM, 3.2f) : r < 0.75f ? go(SPIN, 2.2f) : go(DANCE, 6);
      }
      break;
    case RUN: {
      int s = side();
      float tx = clampf(target() - s * (chasing() ? 14 : 12), 16, 64), d = tx - cat.x;
      if (fabsf(d) < 1.5f) {
        cat.dir = s;
        if (!chasing() && rnd() < 0.6f) go(BAT, 0.55f);
        else cat.hops = 0, go(CROUCH, chasing() ? 0.6f : 0.5f);
        break;
      }
      cat.dir = d > 0 ? 1 : -1;
      cat.x += cat.dir * fminf(40 * dt, fabsf(d));
      cat.phase += 15 * dt;
      break;
    }
    case CROUCH:
      if (cat.t < cat.dur) break;
      if (cat.hops) leap(cat.x, 7, cat.hops - 1);
      else if (chasing()) leap(vis.x - cat.dir * 8, vis.kind == BUTTERFLY ? 14 : vis.kind == BLOWN ? 11 : 8, 0);
      else leap(ball.x - cat.dir * 8, fminf(10 + 0.25f * fabsf(ball.x - cat.x), 18), 0);
      break;
    case JUMP:
      cat.vy += G * dt, cat.x += cat.vx * dt, cat.y += cat.vy * dt;
      if (cat.y < GROUND || cat.vy < 0) break;
      cat.y = GROUND;
      if (fabsf(ball.x - (cat.x + cat.dir * 8)) < 7 && ball.y > GROUND - 6)  // paws landed on the yarn
        kick(cat.dir * (50 + 40 * rnd()), -(60 + 60 * rnd()));
      if (chasing() && !cat.hops) {  // the pounce: 30 % of the time the visitor is caught
        if (fabsf(vis.x - (cat.x + cat.dir * 8)) < 8 && rnd() < 0.3f) vis.on = false, pet::joy(3), cat.happy = 2.5f, cat.hops = 2;
        else flee();
      }
      go(LAND, cat.hops ? 0.08f : 0.2f);
      break;
    case LAND:
      if (cat.t < cat.dur) break;
      if (cat.hops) leap(cat.x, 7, cat.hops - 1);
      else if (cat.wake) cat.wake = false, go(YAWN, 1.5f);
      else idleDone();
      break;
    case BAT:
      if (!cat.kicked && cat.t > 0.28f) {  // hit the yarn on the paw's downswing
        cat.kicked = true;
        if (fabsf(ball.x - (cat.x + cat.dir * 12)) < 6) kick(cat.dir * (45 + 35 * rnd()), -(30 + 50 * rnd()));
      }
      if (cat.t > cat.dur) go(IDLE, 0.4f + 0.6f * rnd());
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
    case ANGRY:  // one hop back, then glare
      if (cat.y < GROUND || cat.vy < 0) {
        cat.vy += G * dt, cat.x = clampf(cat.x + cat.vx * dt, 16, 64), cat.y += cat.vy * dt;
        if (cat.y >= GROUND) cat.y = GROUND, cat.vy = 0;
      }
      if (cat.t > cat.dur) go(IDLE, 1);
      break;
    case EAT:
    case REFUSE:
    case PURR:
      if (cat.t > cat.dur) go(IDLE, 1 + rnd());
      break;
    case YAWN:
      if (cat.t > cat.dur) go(IDLE, 2 + 2 * rnd());
      break;
    case GROOM:
      if (cat.t > cat.dur) go(IDLE, 1 + rnd());
      break;
    case SPIN:  // chasing its tail: turns every 0.16 s, dizzy for the last 0.6 s
      if (cat.t < cat.dur - 0.6f) cat.dir = fmodf(cat.t, 0.32f) < 0.16f ? 1 : -1, cat.phase += 18 * dt;
      if (cat.t > cat.dur) go(IDLE, 1 + rnd());
      break;
    case DANCE:
      if (cat.t > cat.dur) go(IDLE, 1 + rnd());
      break;
    case SEEK: {  // trots to the umbrella, then turns to the open side
      float d = UMB_X - cat.x;
      cat.dir = d > 0 ? 1 : -1, cat.x += cat.dir * fminf(30 * dt, fabsf(d)), cat.phase += 12 * dt;
      if (fabsf(d) < 1) cat.dir = 1, go(IDLE, 0.3f);
      break;
    }
    case SHELTER:
      if (!wet() || (mood != pet::HAPPY && mood != pet::TIRED) || (mood == pet::TIRED && yawnT <= 0)) go(IDLE, 1);
      break;
    case SUNBATHE:
      if (cat.t > cat.dur || pet::wx != pet::CLEAR || !sunUp()) go(IDLE, 1 + rnd());
      break;
    case SNOWPLAY:  // ends with a hop at the flakes
      if (cat.t > cat.dur) leap(cat.x, 9, 0);
      break;
  }
}

// Music notes rising from (x, y), drifting sideways by `drift` px over their life (on a trip: left, behind the cat).
void notes(float x, float y, float drift) {
  for (int k = 0; k < 3; k++) {
    float p = fmodf(T * 0.35f + k / 3.0f, 1);
    int nx = lroundf(x + (drift ? drift * p : (k - 1) * 16) + 3 * sinf(T * 2 + k * 2)), ny = lroundf(y - (drift ? 14 : 18) * p);
    if (p < 0.85f || fmodf(T, 0.2f) < 0.1f) art(NOTE[k % 2], 5, nx - 2, ny, 1, INK);  // blinks out at the top
  }
}

void drawCat() {
  const int d = cat.dir, st = cat.st;
  float bh = 9, a = 0, rx = 8, ry = 4.5f;   // body center height above feet, tilt, radii
  float tb = 1.2f, ta = 0.35f, tw = 2.5f;    // tail: base angle, swing amplitude, speed
  float u[4] = {}, lift[4] = {}, th[4] = {};  // legs: 0 far hind, 1 far front, 2 near hind, 3 near front
  float hdx = 7.5f, hdy = -5;                 // head center in body coordinates
  int eyes = -1;                              // -1: open / blink / happy as usual
  float raise = 0;                            // near front paw lifted by this angle (0 = standing on it)
  bool air = st == JUMP || ((st == ANGRY || st == KO) && cat.y < GROUND - 0.5f);
  bool tuck = false, front = false, curl = false;  // legs hidden / stretched forward lying down; tail curled on the ground

  switch (st) {
    case IDLE: bh += 0.5f * sinf(T * 3); break;  // breathing
    case RUN:
    case SEEK:
      bh += 0.8f * fabsf(sinf(cat.phase)), tb = 0.7f, ta = 0.2f, tw = 8;
      for (int i = 0; i < 4; i++) {  // trot: diagonal pairs move together
        float p = cat.phase + (i == 0 || i == 3 ? 0 : 3.1416f);
        u[i] = 3 * sinf(p), lift[i] = 2 * fmaxf(0, cosf(p));
      }
      break;
    case CROUCH: bh = 6, a = -0.1f + 0.06f * sinf(cat.t * 30), rx = 8.3f, ry = 4.3f, tb = 0.5f, ta = 0.5f, tw = 14; break;
    case LAND: bh = 7, a = -0.15f, rx = 8.4f, ry = 4.2f, tb = 0.9f; break;
    case JUMP: {
      bool up = cat.vy < 0;
      a = clampf(-cat.vy / 300, -0.4f, 0.4f) * fminf(1, fabsf(cat.vx) / 25);
      if (up) rx = 8.6f, ry = 4.1f;  // stretch
      tb = 0.3f, ta = 0.1f, tw = 6;
      th[1] = th[3] = up ? 1.0f : 0.6f, th[0] = th[2] = up ? -1.0f : -0.4f;
      break;
    }
    case BAT: {
      float t = cat.t;
      a = 0.12f, bh = 9.5f, tb = 1.3f, ta = 0.3f, tw = 6;
      th[3] = t < 0.25f ? 2.2f * t / 0.25f : t < 0.35f ? 2.2f - 19 * (t - 0.25f) : 0.3f * fmaxf(0, 1 - (t - 0.35f) / 0.2f);
      break;
    }
    case SLEEP:  // curled up, breathing 1 px every 2 s
      bh = 4.2f + 0.5f * sinf(T * 3.1416f), rx = 8, ry = 4.2f, tuck = curl = true, hdx = 7, hdy = -0.8f;
      eyes = cat.peek > 0 ? EYES_PEEK : EYES_CLOSED;
      break;
    case BORED:  // lying like a sphinx, tail tip flicking
    case KO:
    case SUNBATHE:  // the same, chin up, shades on
      bh = 4.5f, rx = 8.3f, ry = 4.2f, front = !air, hdx = 8.5f, hdy = -3, tb = 0.1f, ta = 0.6f, tw = 2;
      eyes = st == KO ? EYES_DIZZY : st == SUNBATHE ? EYES_SHADES : EYES_HALF;
      if (st == SUNBATHE) hdy = -4, tw = 1;
      if (air) bh = 9, front = false;
      break;
    case HUNGRY:  // sitting by the empty bowl, now and then looking into it
    case PURR:
    case SHELTER:  // under the umbrella, now and then looking up at it
      a = 0.4f, bh = 7.5f, tb = 0.6f, ta = 0.15f, tw = 1.5f;
      if (st == PURR) eyes = EYES_HAPPY;
      else if (st == SHELTER) {
        if (fmodf(cat.t, 5) > 3.8f) hdx -= 1, hdy -= 1.5f;
      } else if (fmodf(cat.t, 4) > 3.2f) hdx += 1.5f, hdy += 2.5f;
      break;
    case EAT:  // head in the bowl, chewing
      a = -0.12f, hdx = 8.5f, hdy = -1.5f + 0.7f * sinf(T * 12), eyes = EYES_HAPPY;
      break;
    case REFUSE:  // head shake
      hdx += 1.2f * sinf(cat.t * 20), eyes = EYES_BLINK;
      break;
    case YAWN: {  // stretch, front low, mouth wide
      float k = fminf(1, 1.5f * sinf(fminf(cat.t / cat.dur, 1) * 3.1416f));
      a = -0.2f * k, rx = 8 + k, bh = 9 - k;
      if (cat.t > 0.25f && cat.t < 1.25f) eyes = EYES_YAWN;
      break;
    }
    case ANGRY:  // puffed up, tail straight up
      rx = 9, ry = 5.5f, bh = 10, tb = 1.5f, ta = 0.05f, tw = 20, eyes = EYES_FROWN;
      break;
    case GROOM: {  // sitting, licks the raised front paw, then wipes its face with it
      float k = fmodf(cat.t, 1.6f);
      a = 0.4f, bh = 7.5f, tb = 0.6f, ta = 0.15f, tw = 1.5f, eyes = EYES_HAPPY;
      raise = k < 1.3f ? 2.5f : 0.4f;
      if (k < 0.9f) hdy += 0.8f + 0.7f * sinf(cat.t * 14);  // lick lick
      else if (k < 1.3f) hdx -= 1, hdy += 0.5f, raise = 2.9f;  // over the ear
      break;
    }
    case SPIN: {
      bool dizzy = cat.t > cat.dur - 0.6f;
      bh += dizzy ? 0 : 1.2f * fabsf(sinf(cat.phase)), tb = 0.4f, ta = 0.5f, tw = 16;
      if (dizzy) eyes = EYES_DIZZY;
      else
        for (int i = 0; i < 4; i++) u[i] = 2 * sinf(cat.phase + (i == 0 || i == 3 ? 0 : 3.1416f));
      break;
    }
    case SNOWPLAY: {  // sitting, looking up, swatting at the flakes
      float k = fmodf(cat.t, 0.8f);
      a = 0.45f, bh = 7.5f, tb = 0.6f, ta = 0.3f, tw = 4, hdx -= 0.5f, hdy -= 1.5f;
      raise = k < 0.45f ? 2.2f + 0.9f * sinf(k / 0.45f * 3.1416f) : 0;
      eyes = k < 0.45f ? EYES_OPEN : EYES_HAPPY;
      break;
    }
    case DANCE: {  // to a 120 bpm beat: 4 beats sitting and swaying, 4 beats stepping paw to paw
      float b = T * 2, ph = fmodf(b, 1);
      int beat = (int)b;
      float bounce = fabsf(sinf(3.1416f * ph));
      eyes = beat % 2 ? EYES_HAPPY : EYES_OPEN;
      tb = 1.0f, ta = 0.6f, tw = 6.2832f;  // tail swings once a beat
      if (beat % 8 < 4) {
        a = 0.4f, bh = 7.5f + 0.8f * bounce, hdx += (beat % 2 ? 1.2f : -1.2f) * bounce;
        raise = beat % 4 == 3 ? 2.6f * bounce : 0;  // a paw up on the fourth beat
      } else {
        bh += 1.2f * bounce;
        lift[beat % 2 ? 3 : 1] = 2.5f * bounce, lift[beat % 2 ? 0 : 2] = 2.5f * bounce;
        hdy -= 0.8f * bounce;
      }
      break;
    }
  }

  if (cat.scare > 0 && !air && st != SLEEP)  // thunder: puffed up, tail straight up, shivering
    rx = 9, ry = 5.3f, bh += 1, tb = 1.5f, ta = 0.05f, tw = 20, eyes = EYES_WIDE;
  if (pet::wx == pet::WIND && !curl) ta = fmaxf(ta, 0.3f + 0.4f * gust), tw = fmaxf(tw, 9);  // the tail flutters
  if (gust > 0.6f && eyes < 0 && cat.happy <= 0) eyes = EYES_HALF;  // squints into a gust
  float bx = cat.x + (cat.scare > 0 && !air ? 0.6f * sinf(T * 60) : 0), by = cat.y - bh, ca = cosf(a), sa = sinf(a);
  auto W = [&](float lx, float ly, float &x, float &y) { x = bx + d * (lx * ca + ly * sa), y = by - lx * sa + ly * ca; };

  float jx[4], jy[4], qx[4], qy[4];  // joint and paw points
  const float JL[4] = {-4, 6, -5.5f, 4.5f};
  if (raise > 0) th[3] = raise;
  for (int i = 0; i < 4; i++) {
    W(JL[i], 2.5f, jx[i], jy[i]);
    bool raised = i == 3 && (st == BAT || raise > 0);
    if (air || raised) qx[i] = jx[i] + d * 6 * sinf(th[i]), qy[i] = jy[i] + 6 * cosf(th[i]);
    else if (front) qx[i] = jx[i] + d * (i & 1 ? 5 : 3.5f), qy[i] = cat.y - 1;
    else qx[i] = jx[i] + d * u[i], qy[i] = cat.y - 0.5f - lift[i];
  }

  float kx[8], ky[8];
  if (curl) {  // around the body along the ground, tip in front
    static const float CX[8] = {-7.5f, -9.5f, -9, -6.5f, -2, 3, 7, 9.5f}, CY[8] = {-1.5f, 1, 3, 4, 4.3f, 4.3f, 4, 2.8f};
    for (int i = 0; i < 8; i++) W(CX[i], CY[i] + (i == 7 ? 0.5f * sinf(T * 2) : 0), kx[i], ky[i]);
  } else {
    W(-7.5f, -1.5f, kx[0], ky[0]);
    for (int i = 1; i < 8; i++) {
      float g = tb - a + 0.13f * i + ta * sinf(tw * T - 0.55f * i) * i / 7;
      kx[i] = kx[i - 1] - d * 1.5f * cosf(g), ky[i] = ky[i - 1] - 1.5f * sinf(g);
    }
  }

  shadow(cat.x, 9 * fmaxf(0.3f, 1 - (GROUND - cat.y) / 30));
  for (int i = 1; i < 8; i++) capsule(kx[i - 1], ky[i - 1], kx[i], ky[i], 2.0f, INK);
  for (int i = 1; i < 8; i++) capsule(kx[i - 1], ky[i - 1], kx[i], ky[i], 1.0f, i >= 6 ? FUR2 : FUR);
  if (!tuck)
    for (int i = 0; i < 2; i++) capsule(jx[i], jy[i], qx[i], qy[i], 1.8f, INK), capsule(jx[i], jy[i], qx[i], qy[i], 0.8f, FUR2);
  body(bx, by, rx, ry, a, d, 1);
  if (!tuck)
    for (int i = 2; i < 4; i++) capsule(jx[i], jy[i], qx[i], qy[i], 1.8f, INK);
  body(bx, by, rx, ry, a, d, 0);
  if (!tuck)
    for (int i = 2; i < 4; i++) capsule(jx[i], jy[i], qx[i], qy[i], 0.8f, FUR), disc(qx[i], qy[i] - 0.3f, 0.8f, CREAM);

  float hx, hy;
  W(hdx, hdy, hx, hy);
  if (eyes < 0) eyes = cat.happy > 0 ? EYES_HAPPY : fmodf(T, 3.3f) < 0.15f ? EYES_BLINK : EYES_OPEN;
  art(HEAD[eyes], HH, lroundf(hx - HW / 2), lroundf(hy - HH / 2));
  if (raise > 0)  // a paw up at the face (grooming, dancing) is in front of the head
    capsule(jx[3], jy[3], qx[3], qy[3], 1.8f, INK), capsule(jx[3], jy[3], qx[3], qy[3], 0.8f, FUR), disc(qx[3], qy[3] - 0.3f, 0.8f, CREAM);

  if (st == SLEEP && cat.peek <= 0)  // z z z floating up
    for (int k = 0; k < 3; k++) {
      float p = fmodf(T * 0.3f + k / 3.0f, 1);
      art(ZZ, 4, lroundf(hx + d * (2 + 10 * p)) - 2, lroundf(hy - 8 - 20 * p), 1, nightW > 0.5f ? CREAM : INK);
    }
  if (st == PURR)  // little hearts rising
    for (int k = 0; k < 3; k++) {
      float p = fmodf(cat.t * 0.9f + k / 3.0f, 1);
      art(LOVE, 4, lroundf(hx + d * (2 + 3 * k) + sinf(T * 4 + k)) - 2, lroundf(hy - 10 - 9 * p));
    }
  if (st == DANCE) notes(cat.x, cat.y - 19, 0);
  if (rove && tunes) notes(cat.x + 6, cat.y - 22, -26);  // over its head, blown back
}

void drawBall() {
  shadow(ball.x, 3 * fmaxf(0.3f, 1 - (GROUND - 3 - ball.y) / 30));
  disc(ball.x, ball.y, 3.6f, INK);
  disc(ball.x, ball.y, 2.7f, YARN);
  float c = 2.4f * cosf(ball.rot), s = 2.4f * sinf(ball.rot);  // spinning yarn strand
  capsule(ball.x - c, ball.y - s, ball.x + c, ball.y + s, 0.5f, YARN2);
  capsule(ball.x - s * 0.6f, ball.y + c * 0.6f, ball.x + s * 0.2f, ball.y - c * 0.2f, 0.4f, YARN2);
}

void drawVisitor() {
  if (!vis.on) return;
  int x = lroundf(vis.x), facing = vis.flee ? (vis.vx > 0 ? 1 : -1) : (cat.x > vis.x ? 1 : -1);
  if (vis.kind == BUTTERFLY) art(FLY[sinf(T * 20) > 0], 5, x - 3, lroundf(vis.y + 2 * sinf(T * 3)) - 2);
  else if (vis.kind == BLOWN) art(LEAF_ART[sinf(T * 9) > 0], 3, x - 2, lroundf(vis.y + 3 * sinf(T * 4)) - 1);
  else art(RAT[vis.x != vis.home && fmodf(T, 0.2f) < 0.1f], 4, x - 4, GROUND - 4, facing);
}

// For the runner (game.h): the cat is the hero.
auto &hero = cat;
void drawHero() { drawCat(); }
const int BIRD_TOP = GROUND - 36, LABEL_Y = GROUND + 4, SCROLL_Y = GROUND - 1;
const float HEAD_X0 = 1, HEAD_X1 = 12, HEAD_H = 16;
#include "game.h"  // the runner game uses the drawing above

// ---- Travel: the cat runs on the spot, the world scrolls left in layers (far mountains, hills with trees, the ground,
// grass in front), each slower the farther it is. Rocks, logs, pots and bushes come along; the cat jumps them. ----
const float TRIP_V = 40, TRIP_X = 26;  // ground speed px/s, where the cat runs
const char *const ROCK_ART[] = {"..KKK..", ".KYYYK.", "KYWYYYK", "KYYYYYK", "KKKKKKK"};
const char *const LOG_ART[] = {".KKKKKKKK.", "KCFCCCCKCK", "KCCCCFKCFK", "KCCFCCCKCK", ".KKKKKKKK."};
const char *const BUSH_ART[] = {"..KKKK..", ".KgggGK.", "KgGgggGK", "KggGgggK", "KGggGgGK", ".KKKKKK."};
const char *const *const TRIP_ART[] = {ROCK_ART, LOG_ART, POT_ART, BUSH_ART};
const int TRIP_H[] = {5, 5, 6, 6}, TRIP_W[] = {7, 10, 6, 8};
struct {
  float x;  // left edge on screen
  uint8_t kind;
} way[2];

void wayNew(int i, float after) { way[i] = {after + 45 + 70 * rnd(), uint8_t(rnd() * 4)}; }

void travel(float secs) {  // secs: how long; 0 = while the music plays
  tripLeft = secs;
  if (rove) return;
  pose(RUN, 0);
  rove = true, vis.on = false, awake = 0, cat.x = TRIP_X, cat.dir = 1;
  wayNew(0, N), wayNew(1, way[0].x);
}

void tripHop(float h) {  // the jump arc's apex is over what the cat jumps
  cat.vy = -sqrtf(2 * G * h), cat.vx = TRIP_V;  // vx only tilts the jump pose
  go(JUMP, 0);
}

void roveStep(float dt) {
  dist += TRIP_V * dt;
  for (int i = 0; i < 2; i++)
    if ((way[i].x -= TRIP_V * dt) < -12) wayNew(i, fmaxf(way[0].x, way[1].x));
  if (cat.st == JUMP) {
    cat.vy += G * dt, cat.y += cat.vy * dt;
    if (cat.y >= GROUND && cat.vy > 0) cat.y = GROUND, cat.vy = 0, go(LAND, 0.08f);
  } else if (cat.st == LAND) {
    if (cat.t > cat.dur) go(RUN, 0);
  } else {
    cat.phase += 15 * dt;
    for (auto &o : way) {
      float h = TRIP_H[o.kind] + 8, half = sqrtf(2 * h / G) * TRIP_V;  // jump height, run-up to the apex
      if (o.x + TRIP_W[o.kind] / 2.0f - cat.x < half && o.x + TRIP_W[o.kind] > cat.x) tripHop(h);
    }
  }
  if (!tunes && (tripLeft -= dt) <= 0) go(IDLE, 1), rove = false;
}

// ---- Weather: the cat's side of it (weather.h) ----
float drift() { return rove ? -TRIP_V * 0.6f : 0; }  // on a trip the rain blows back
void thunder() {  // startled; asleep it only peeks
  if (cat.st == SLEEP) cat.peek = 1.5f;
  else if (grounded()) cat.scare = 1.1f;
}

void drawSnowCover() {  // white over the grass, densest at the top rows; scrolls along on a trip
  if (snowCover <= 0) return;
  int off = rove ? floorf(dist) : 0;
  for (int y = GROUND - 1; y < N; y++)
    for (int x = 0; x < N; x++) {
      uint8_t &c = view[(y + vy0) * VN + x + vx0];
      if ((c == GRASS || c == GRASS2) && h01((x + off) * 977 + y * 131) < snowCover * (y < GROUND + 3 ? 1.2f : 0.97f)) c = FLAKE;
    }
}

void drawUmbrellaPole() {  // planted at a slant behind the cat
  if (umbrella()) capsule(UMB_X - 10, GROUND, UMB_CX, UMB_Y - 6, 0.6f, INK);
}
void drawCanopy() {  // blue and dark blue panels, outlined, a little scallop at the rim
  if (!umbrella()) return;
  for (int x = UMB_CX - UMB_R; x <= UMB_CX + UMB_R; x++) {
    int top = ceilf(canopy(x + 0.5f)), panel = (x - UMB_CX + UMB_R + 3) / 7;
    if (top > UMB_Y) continue;
    for (int y = top; y <= UMB_Y; y++) px(x, y, y == top ? INK : panel & 1 ? YARN2 : YARN);
    if ((x - UMB_CX + UMB_R) % 7 >= 2 && (x - UMB_CX + UMB_R) % 7 <= 4) px(x, UMB_Y + 1, INK);
    else px(x, UMB_Y, INK);
  }
  px(UMB_CX, UMB_Y - 8, INK), px(UMB_CX, UMB_Y - 9, INK);  // the tip
}

void drawRove() {
  memcpy(fb, bg, sizeof fb);
  drawSky();
  for (int x = 0; x < N; x++) {  // far mountains and near hills: ridge lines over the scrolled x
    float u = x + dist * 0.1f, v = x + dist * 0.35f;
    int far = 40 + 4 * sinf(u * 0.06f) + 2.5f * sinf(u * 0.17f + 1), hill = 49 + 3 * sinf(v * 0.09f + 2) + 1.5f * sinf(v * 0.23f);
    for (int y = far; y < GROUND; y++) fb[y * N + x] = y < hill ? FAR : HILL;
  }
  for (int k = floorf(dist * 0.35f / 13) - 1, end = k + N / 13 + 3; k < end; k++) {  // trees on the hills
    if (h01(k) > 0.55f) continue;
    float x = k * 13 + 6 * h01(k + 50) - dist * 0.35f, v = x + dist * 0.35f, r = 2 + 1.5f * h01(k + 9);
    float top = 49 + 3 * sinf(v * 0.09f + 2) + 1.5f * sinf(v * 0.23f);
    capsule(x, top, x, top - 2 - r, 0.5f, TREE), disc(x, top - 2 - r, r, TREE);
  }
  for (int x = 0; x < N; x++) {  // the ground scrolls with the cat's speed: grass tips, flowers, darker speckles
    int u = floorf(x + dist);
    float r = h01(u);
    for (int y = GROUND; y < N; y++) fb[y * N + x] = y < GROUND + 3 ? GRASS : h01(u * 7 + y) < 0.1f ? GRASS : GRASS2;
    if (r < 0.35f) px(x, GROUND - 1, GRASS);
    if (r > 0.97f) px(x, GROUND - 1, r > 0.985f ? PINK : CREAM), px(x, GROUND - 2, GRASS);  // a flower now and then
  }
  drawFlash();
  drawSnowCover();
  drawDrops();
  for (auto &o : way) art(TRIP_ART[o.kind], TRIP_H[o.kind], lroundf(o.x), GROUND - TRIP_H[o.kind]);
  drawCat();
  for (int k = floorf(dist * 1.6f / 5) - 1, end = k + N / 5 + 3; k < end; k++) {  // tall grass right in front, fastest
    if (h01(k + 300) > 0.45f) continue;
    float x = k * 5 + 3 * h01(k + 301) - dist * 1.6f, top = 71 - 4 * h01(k + 302);
    capsule(x, N, x - 1.5f, top, 0.6f, GRASS2), px(x - 1.5f, top, GRASS);
  }
  drawLeaves();
}

// ---- Events from the firmware ----

// Device shaken: the yarn flies, the cat startles and jumps, plays for a few seconds.
// Asleep: a startled hop, a yawn, back to sleep. Starving: angry, no play.
void shake() {
  if (game.on) return;
  kick((rnd() < 0.5f ? -1 : 1) * (40 + 60 * rnd()), -(90 + 80 * rnd()));
  if (cat.st == SLEEP) {
    awake = 0, cat.peek = 0, leap(cat.x, 10, 0), cat.wake = true;
    return;
  }
  if (pet::s.full < 10 && grounded()) {
    awake = 0, pose(ANGRY, 1.2f);
    cat.vy = -sqrtf(2 * G * 5), cat.vx = -cat.dir * 22;
    return;
  }
  if (rove) {  // on a trip: a big leap for joy, then on it runs
    if (cat.st != JUMP) tripHop(18), cat.happy = 1.5f;
    return;
  }
  awake = 6;
  if (grounded()) leap(cat.x, 10, 0);
}

// Music screen on / off (the firmware calls this every frame): the cat sets off on a trip; asleep, it sleeps on.
void music(bool on) {
  if (game.on) on = false;
  if (on == tunes) return;
  tunes = on;
  if (on && cat.st != SLEEP && mood != pet::SLEEP) travel(0);
  else if (!on && rove && tripLeft <= 0) go(IDLE, 1), rove = false;
}

// Message/button: the cat hops twice in place for joy. Asleep: one eye opens for 2 s.
void cheer() {
  if (game.on) return;
  if (cat.st == SLEEP) {
    cat.peek = 2;
    return;
  }
  cat.happy = 2.5f;
  if (rove) {
    if (cat.st != JUMP) tripHop(10);
  } else if (grounded() && cat.st != EAT) cat.hops = 2, go(CROUCH, 0.15f);
}

// Pet events (nothing during the game): feed accepted (also wakes it), feed while full, petted, too tired to play.
void eat() {
  if (!game.on) pose(EAT, 3), showBowl(true);
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

void init() {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) fb[y * N + x] = y < GROUND ? skyAt(x, y) : uint8_t(y < GROUND + 3 ? GRASS : GRASS2);
  for (int x = 0; x < N; x++) {
    if (rnd() < 0.3f) px(x, GROUND - 1, GRASS);  // grass tips
    if (rnd() < 0.4f) px(x, GROUND + 3, GRASS);
  }
  memcpy(bg, fb, sizeof bg);
  for (int i = 0; i < NS; i++) sx[i] = 12 + rnd() * 56, sy[i] = 4 + rnd() * 34, sp[i] = rnd() * 6.28f;
}

void step(float dt) {
  setView(game.on);  // stays on for the firmware's bubble, drawn after this
  if (game.on) {
    gameStep(dt);
    gameDraw();
    return;
  }
  update(dt);
  weatherStep(dt);
  if (rove) return drawRove();
  memcpy(fb, bg, sizeof fb);
  drawSky();
  drawFlash();
  drawSnowCover();
  drawDrops();
  drawUmbrellaPole();
  if (bowl.on) art(bowl.food ? BOWL_FULL : BOWL_EMPTY, 5, lroundf(bowl.x) - 4, GROUND - 5);
  drawBall();
  if (vis.kind == MOUSE) drawVisitor();  // on the grass, behind the cat
  drawCat();
  if (vis.kind != MOUSE) drawVisitor();
  drawCanopy();
  drawLeaves();
}

}  // namespace kitty
#ifndef SCENE_PICK
namespace scene = kitty;  // firmware.ino talks to whichever scene is included through this name (pets.h: to both)
#endif
#define SCENE_PET         // firmware.ino: this scene has the pet features (pet.h, game, day/night)
