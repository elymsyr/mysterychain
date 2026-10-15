// Pavi (Seven Havens protagonist, purple-haired girl) and her fairy: draws into an 80x80 color-indexed buffer (fb),
// scaled 3x on screen. Same interface as ../cat/cat.h (init, step, shake, cheer, fb, PAL, px); no Arduino dependency.
#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>

namespace pavi {

const int N = 80, GROUND = 58;  // logical resolution, ground row (feet stand here)
const float G = 260;            // gravity, px/s²

enum : uint8_t {
  SKY0, SKY1, SKY2, SKY3, AUR0, AUR1, AUR2, STAR, ROOF, ROOF2, LAMP, WALL, WALL2,
  INK, SKIN, HAIR, HAIR2, HAIRL, BAND, EYE, CREAM, MOUTH, CLOAK, CLOAK2, SHIRT, BELT, PANTS,
  WING, FAIRY, GLOW, SHADOW, NCOL
};
const uint8_t PAL[NCOL][3] = {
    {8, 12, 36},    {12, 30, 64},   {16, 52, 86},   {28, 80, 104},  // night sky, top to horizon
    {64, 48, 124},  {30, 150, 120}, {130, 240, 196},                // aurora: purple top, green, bright lower edge
    {230, 240, 255}, {50, 28, 54},  {34, 20, 42},   {255, 200, 110},  // star, distant roofs, window light
    {124, 54, 64},  {80, 32, 48},                                   // the wall she stands on
    {24, 14, 30},   {134, 80, 54},  {84, 62, 150},  {50, 38, 100}, {130, 110, 200},  // outline, skin, hair
    {238, 198, 92}, {80, 190, 110}, {245, 238, 222}, {160, 60, 70},  // hairband, eye, cream, mouth
    {170, 182, 100}, {116, 130, 64}, {228, 126, 104}, {150, 86, 50}, {72, 46, 60},  // cloak, shirt, belt, trousers
    {150, 225, 255}, {60, 100, 210}, {235, 255, 255}, {52, 22, 40},  // fairy wing, body, glow; shadow
};

// Sprites face right and are mirrored when facing left. K outline, S skin, H/h/L hair, Y band, E eye, W cream,
// M mouth, C/c cloak, P shirt, B belt. Head in 3/4 view: face in front, hair mass falls behind.
const int HEAD_H = 19, HEAD_AX = 9;  // HEAD_AX: head column that sits over the body center
const char *const HEAD[3][HEAD_H] = {
    {".........KK.....", "........KHK.....", ".....KKKHKKKK...", "...KKHHHHHHHHKK.", "..KHHHLHHHHHHHHK",
     ".KHHYYYYYYYYYYHK", ".KHHHHHHHHHHHHHK", "KHHHHLHHHHHHHHHK", "KHHHHHSHHSSSHSHK", "KhHHHSSKKSSSKKSK",
     "KhHHHSSWESSSWESK", "KhHHHSSEESSSEESK", "KhHHHSSSSSSSSSSK", "KhHHHHSSSSMMSSK.", "KhHHHHHSSSSSSK..",
     "KhHHHHHKKKKKK...", "KHhHHHK.........", ".KHhHK..........", "..KKK..........."},  // eyes open
    {".........KK.....", "........KHK.....", ".....KKKHKKKK...", "...KKHHHHHHHHKK.", "..KHHHLHHHHHHHHK",
     ".KHHYYYYYYYYYYHK", ".KHHHHHHHHHHHHHK", "KHHHHLHHHHHHHHHK", "KHHHHHSHHSSSHSHK", "KhHHHSSSSSSSSSSK",
     "KhHHHSSSSSSSSSSK", "KhHHHSSKKSSSKKSK", "KhHHHSSSSSSSSSSK", "KhHHHHSSSSMMSSK.", "KhHHHHHSSSSSSK..",
     "KhHHHHHKKKKKK...", "KHhHHHK.........", ".KHhHK..........", "..KKK..........."},  // blink
    {".........KK.....", "........KHK.....", ".....KKKHKKKK...", "...KKHHHHHHHHKK.", "..KHHHLHHHHHHHHK",
     ".KHHYYYYYYYYYYHK", ".KHHHHHHHHHHHHHK", "KHHHHLHHHHHHHHHK", "KHHHHHSHHSSSHSHK", "KhHHHSSSSSSSSSSK",
     "KhHHHSSKKSSSKKSK", "KhHHHSKSSKSKSSKK", "KhHHHSSSSSSSSSSK", "KhHHHHSSSKMMKSK.", "KhHHHHHSSSKKSK..",
     "KhHHHHHKKKKKK...", "KHhHHHK.........", ".KHhHK..........", "..KKK..........."},  // happy ^^, mouth open
};
enum { EYES_OPEN, EYES_BLINK, EYES_HAPPY };

const int TORSO_H = 9, TORSO_AX = 6;
const char *const TORSO[TORSO_H] = {
    "....KSSK....", "..KKCSSCKK..", ".KCCCPPCCCK.", "KCcPPPPPPcCK", "KCcPPPPPPcCK",
    "KCcBBBBBBcCK", ".KcPPPPPPcK.", ".KcPPPPPPcK.", ".KKKKKKKKKK.",
};

uint8_t fb[N * N], bg[N * N];
float T = 0, DT = 0, awake = 0;  // awake > 0: play mode (refilled by a shake)
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

void shadow(float x, float w) {
  for (int i = -(int)w; i <= (int)w; i++) px(x + i, GROUND, SHADOW);
  for (int i = -(int)(w * 0.6f); i <= (int)(w * 0.6f); i++) px(x + i, GROUND + 1, SHADOW);
}

// ---- Background: night sky, aurora, distant roofs, wall ----
const float BAYER[4] = {0.125f, 0.625f, 0.875f, 0.375f};  // 2x2 dither thresholds
float bayer(int x, int y) { return BAYER[(y & 1) * 2 + (x & 1)]; }

uint8_t skyAt(int x, int y) {
  float v = y * 3.0f / (GROUND - 1);
  int b = v;
  return b + (v - b > bayer(x, y));
}

// Aurora curtain: bright lower edge in each column, turning purple and fading upward. Draws over sky only.
void aurora() {
  for (int x = 0; x < N; x++) {
    float c = 20 + 4 * sinf(x * 0.08f + T * 0.35f) + 2 * sinf(x * 0.21f - T * 0.6f);  // lower edge row
    float len = 10 + 3 * sinf(x * 0.13f + T * 0.5f), shine = 0.5f + 0.5f * sinf(x * 0.45f - T * 1.6f);
    for (int y = c - len; y <= c; y++) {
      if (y < 0 || bg[y * N + x] > SKY3) continue;
      float t = (c - y) / len, b = bayer(x, y);  // t: 0 lower edge, 1 top
      if (t < 0.15f) px(x, y, shine > b ? AUR2 : AUR1);
      else if (t < 0.55f) { if (0.8f > b) px(x, y, AUR1); }
      else if ((1 - t) / 0.45f > b) px(x, y, AUR0);
    }
  }
}

// ---- Girl and fairy ----
enum { IDLE, RUN, CROUCH, JUMP, LAND, REACH };
struct {
  float x = 30, y = GROUND, vx = 0, vy = 0, t = 0, dur = 1, phase = 0, happy = 0;
  int dir = 1, st = IDLE, hops = 0;
  bool booped = false, yay = false;  // yay: both arms up during a joy hop
} girl;
struct {
  float x = 44, y = 30, vx = 0, vy = 0, tx = 44, ty = 30, re = 0, emit = 0;
} fairy;

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

void go(int st, float dur) { girl.st = st, girl.t = 0, girl.dur = dur, girl.booped = false; }
int side() { return fairy.x >= girl.x ? 1 : -1; }

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

// Did the tip of the reaching front hand touch the fairy
bool touch() { return hypotf(fairy.x - (girl.x + girl.dir * 6.5f), fairy.y - (girl.y - 17)) < 5; }

void boop() {  // touched the fairy: it giggles, flees and scatters sparkles
  girl.booped = true, girl.happy = 1.2f;
  fairy.vx = girl.dir * (50 + 40 * rnd()), fairy.vy = -(50 + 50 * rnd());
  for (int i = 0; i < 10; i++) {
    float a = i * 0.628f, v = 30 + 20 * rnd();
    spark(fairy.x, fairy.y, v * cosf(a), v * sinf(a), 0.5f);
  }
  retarget();
}

void reachOrJump() {
  if (fairy.y > GROUND - 21) go(REACH, 0.55f);  // reachable standing up
  else girl.hops = 0, go(CROUCH, 0.3f);
}

void think() {  // idle wait is over: pick the next move
  girl.dir = side();
  if (rnd() < 0.15f) girl.hops = 2, girl.yay = true, girl.happy = 2, go(CROUCH, 0.15f);  // joy hop
  else if (fabsf(fairy.x - girl.dir * 6 - girl.x) > 4) go(RUN, 0);
  else reachOrJump();
}

// Device shaken: the fairy startles and darts off, the girl jumps and chases it for a few seconds.
void shake() {
  awake = 6;
  fairy.vx += (rnd() < 0.5f ? -1 : 1) * (60 + 60 * rnd()), fairy.vy -= 40 + 50 * rnd();
  retarget();
  if (girl.st == IDLE || girl.st == RUN) leap(girl.x, 6, 0);
}

// Message/button: the girl raises her arms and hops twice for joy.
void cheer() {
  girl.happy = 2.5f;
  if (girl.st == IDLE || girl.st == RUN) girl.hops = 2, girl.yay = true, go(CROUCH, 0.15f);
}

void update(float dt) {
  T += dt, DT = dt, girl.t += dt, girl.happy -= dt, awake -= dt;

  // Fairy: glides to its target as a damped spring. Circles above the girl's head when calm, flees in play.
  if (awake > 0) {
    if ((fairy.re -= dt) <= 0) retarget();
  } else {
    fairy.tx = girl.x + 10 * sinf(T * 0.6f), fairy.ty = girl.y - 32 + 3 * sinf(T * 1.2f);
  }
  float tx = fairy.tx + 2.5f * sinf(T * 2.3f), ty = fairy.ty + 2 * sinf(T * 3.7f);
  fairy.vx += (6 * (tx - fairy.x) - 2.4f * fairy.vx) * dt;
  fairy.vy += (6 * (ty - fairy.y) - 2.4f * fairy.vy) * dt;
  fairy.x += fairy.vx * dt, fairy.y += fairy.vy * dt;
  if (fairy.x < 10 || fairy.x > 70) fairy.x = clampf(fairy.x, 10, 70), fairy.vx *= -0.5f;
  if (fairy.y < 12 || fairy.y > 52) fairy.y = clampf(fairy.y, 12, 52), fairy.vy *= -0.5f;
  if ((fairy.emit -= dt) <= 0) {  // sparkle trail; denser when fast
    spark(fairy.x, fairy.y + 1, 6 * rnd() - 3, 6 + 8 * rnd(), 0.6f);
    fairy.emit = hypotf(fairy.vx, fairy.vy) > 15 ? 0.04f : 0.12f;
  }
  for (auto &s : sparks)
    if (s.life > 0) s.life -= dt, s.x += s.vx * dt, s.y += s.vy * dt, s.vx *= 1 - 3 * dt, s.vy *= 1 - 3 * dt;

  switch (girl.st) {
    case IDLE:
      if (fabsf(fairy.x - girl.x) > 3) girl.dir = side();
      if (awake > 0 && girl.t > girl.dur) think();
      break;
    case RUN: {
      int s = side();
      float tx = clampf(fairy.x - s * 6, 16, 64), d = tx - girl.x;
      if (fabsf(d) < 1.5f) {
        girl.dir = s;
        reachOrJump();
        break;
      }
      girl.dir = d > 0 ? 1 : -1;
      girl.x += girl.dir * fminf(36 * dt, fabsf(d));
      girl.phase += 14 * dt;
      if (girl.t > 3) go(IDLE, 0.3f);  // couldn't catch up: take a breather
      break;
    }
    case CROUCH:
      if (girl.t < girl.dur) break;
      if (girl.hops) leap(girl.x, 6, girl.hops - 1);
      else leap(fairy.x - girl.dir * 6, clampf(GROUND - 16 - fairy.y, 4, 22), 0);
      break;
    case JUMP:
      girl.vy += G * dt, girl.x += girl.vx * dt, girl.y += girl.vy * dt;
      if (!girl.yay && !girl.booped && touch()) boop();
      if (girl.y < GROUND || girl.vy < 0) break;
      girl.y = GROUND;
      go(LAND, girl.hops ? 0.08f : 0.18f);
      break;
    case LAND:
      if (girl.t < girl.dur) break;
      if (girl.hops) leap(girl.x, 6, girl.hops - 1);
      else girl.yay = false, go(IDLE, 0.5f + 1.0f * rnd());
      break;
    case REACH:
      if (!girl.booped && girl.t > 0.15f && girl.t < 0.4f && touch()) boop();
      if (girl.t > girl.dur) go(IDLE, 0.3f + 0.5f * rnd());
      break;
  }
}

// Ponytail: chain hanging from its tie point (Verlet + length constraint). Trails when running, swings when jumping.
const int NH = 6;
float hx[NH], hy[NH], hpx[NH], hpy[NH];
bool hairReady = false;
void ponytail(float ax, float ay) {
  if (!hairReady)
    for (int i = 0; i < NH; i++) hx[i] = hpx[i] = ax, hy[i] = hpy[i] = ay + i * 1.6f, hairReady = true;
  float jump = ax - hx[0];
  if (fabsf(jump) > 3)  // turned: tie point jumped to the other side of the head; move the hair along so it doesn't whip
    for (int i = 0; i < NH; i++) hx[i] += jump, hpx[i] += jump;
  hx[0] = ax, hy[0] = ay;
  for (int i = 1; i < NH; i++) {
    float vx = (hx[i] - hpx[i]) * 0.9f, vy = (hy[i] - hpy[i]) * 0.9f;
    hpx[i] = hx[i], hpy[i] = hy[i];
    hx[i] += vx, hy[i] += vy + G * DT * DT;
    float dx = hx[i] - hx[i - 1], dy = hy[i] - hy[i - 1], l = sqrtf(dx * dx + dy * dy) + 1e-6f;
    hx[i] = hx[i - 1] + dx * 1.6f / l, hy[i] = hy[i - 1] + dy * 1.6f / l;
  }
  for (int i = 1; i < NH; i++) capsule(hx[i - 1], hy[i - 1], hx[i], hy[i], 1.9f - 0.15f * i, INK);
  for (int i = 1; i < NH; i++) capsule(hx[i - 1], hy[i - 1], hx[i], hy[i], 0.9f - 0.08f * i, i == 4 ? HAIRL : HAIR);
  disc(hx[1], hy[1], 0.9f, BAND);  // hair tie
}

void drawGirl() {
  const int d = girl.dir;
  float hip = 5, th[2] = {0.25f, 0.25f}, fx[2] = {}, fl[2] = {};  // hip height; arm angle (0 down, π up); foot slide/lift
  // array index: 0 back side, 1 front (facing) side

  switch (girl.st) {
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
  }
  if (girl.yay) th[0] = th[1] = 2.6f + 0.25f * sinf(T * 18);

  int cx = lroundf(girl.x), hy = lroundf(girl.y - hip), ty = hy - 8, ky = ty - 16;  // hip, torso and head top rows
  shadow(cx, 7 * fmaxf(0.3f, 1 - (GROUND - girl.y) / 30));
  ponytail(cx - d * 7.0f, ky + 14.5f);
  for (int i = 0; i < 2; i++) {
    float s = i ? d : -d, lx = cx + s * 2, fx1 = lx + fx[i], fy1 = girl.y - 1 - fl[i];
    capsule(lx, hy, fx1, fy1, 1.6f, INK);
    capsule(lx, hy, fx1, fy1, 0.7f, PANTS);
    disc(fx1 + d * 0.5f, fy1 + 0.2f, 0.8f, BELT);  // boot
  }
  blit(TORSO, TORSO_H, TORSO_AX, cx, ty, d);
  int eyes = girl.happy > 0 ? EYES_HAPPY : fmodf(T, 3.1f) < 0.14f ? EYES_BLINK : EYES_OPEN;
  blit(HEAD[eyes], HEAD_H, HEAD_AX, cx, ky, d);
  for (int i = 0; i < 2; i++) {  // arms after the head: stay in front of the hair when raised
    float s = i ? d : -d, sx = cx + s * 4.5f, sy = ty + 2.5f;
    float ex = sx + s * 6 * sinf(th[i]), ey = sy + 6 * cosf(th[i]), mx = sx + (ex - sx) * 0.35f, my = sy + (ey - sy) * 0.35f;
    capsule(sx, sy, ex, ey, 1.5f, INK), disc(ex, ey, 1.6f, INK);
    capsule(sx, sy, mx, my, 0.8f, SHIRT), capsule(mx, my, ex, ey, 0.8f, SKIN), disc(ex, ey, 0.8f, SKIN);
    px(sx + (ex - sx) * 0.75f, sy + (ey - sy) * 0.75f, BAND);  // gold bracelet
  }
}

void drawFairy() {
  int x = lroundf(fairy.x), y = lroundf(fairy.y), up = fmodf(T, 0.12f) < 0.06f ? -1 : 0;  // wing flap
  for (int s = -1; s <= 1; s += 2) px(x + s, y + up, WING), px(x + 2 * s, y + up, WING), px(x + s, y + up + 1, WING);
  px(x, y, GLOW), px(x, y + 1, FAIRY);
  for (auto &s : sparks)
    if (s.life > 0) px(s.x, s.y, s.life > 0.3f ? GLOW : fmodf(s.life * 20, 2) < 1 ? WING : AUR2);
}

const int NS = 14;
uint8_t sx[NS], sy[NS];
float sp[NS];

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
  memcpy(bg, fb, sizeof bg);
  for (int i = 0; i < NS; i++) sx[i] = 10 + rnd() * 60, sy[i] = 3 + rnd() * 30, sp[i] = rnd() * 6.28f;
}

void step(float dt) {
  update(dt);
  memcpy(fb, bg, sizeof fb);
  aurora();
  for (int i = 0; i < NS; i++)  // twinkling stars (never over the aurora)
    if (sinf(T * 1.7f + sp[i]) > -0.2f && fb[sy[i] * N + sx[i]] <= SKY3) px(sx[i], sy[i], STAR);
  drawGirl();
  drawFairy();
}

}  // namespace pavi
namespace scene = pavi;  // firmware.ino talks to whichever scene is included through this name
