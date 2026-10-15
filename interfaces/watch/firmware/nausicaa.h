// Nausicaa (Valley of the Wind) and Teto, the fox-squirrel on her shoulder: draws into an 80x80 color-indexed buffer
// (fb), scaled 3x on screen. Same interface as ../cat/cat.h (init, step, shake, cheer, fb, PAL, px); no Arduino dependency.
#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>

namespace nausicaa {

const int N = 80, GROUND = 58;  // logical resolution, ground row (feet stand here)
const float G = 260;            // gravity, px/s²

enum : uint8_t {
  SKY0, SKY1, SKY2, SKY3, CLOUD, CLOUD2, HILL, HILL2, MILL, GRASS, GRASS2, GRASSL, WIND,
  INK, SKIN, HAIR, HAIR2, HAIRL, EYE, CREAM, MOUTH, EARRING, TUNIC, TUNIC2, BELT, PANTS,
  FUR, FUR2, TEYE, SHADOW, NCOL
};
const uint8_t PAL[NCOL][3] = {
    {44, 96, 170},  {74, 134, 200}, {128, 176, 222}, {196, 212, 222},  // sky, top to horizon (haze at the horizon)
    {252, 246, 232}, {232, 196, 176}, {86, 120, 140}, {60, 92, 106}, {40, 54, 66},  // cloud, its shade; distant ridges, windmill
    {104, 166, 72}, {64, 118, 56},  {172, 214, 104}, {255, 255, 255},  // grass, dark grass, grass tip; wind streak
    {30, 22, 30},   {246, 200, 166}, {186, 78, 58},  {124, 44, 38}, {226, 118, 82},  // outline, skin, hair
    {64, 72, 110},  {250, 240, 220}, {190, 90, 90},  {220, 40, 50},  // eye, cream, mouth, earring
    {80, 150, 220}, {46, 96, 170},  {120, 70, 40},  {214, 190, 140},  // tunic, its shade, belt/boots, trousers
    {250, 190, 60}, {112, 62, 30},  {60, 200, 90},  {50, 100, 44},   // Teto fur, stripes, eye; shadow
};

// Sprites face right and are mirrored when facing left. K outline, S skin, H/h/L hair, E eye, W cream, M mouth,
// R earring, T/t tunic, B belt; Teto: O fur, D dark stripe, G eye. Head in 3/4 view, short hair with bangs.
const int HEAD_H = 17, HEAD_AX = 9;  // HEAD_AX: head column that sits over the body center
const char *const HEAD[3][HEAD_H] = {
    {".....KKKKKK.....", "...KKHHHHHHKK...", "..KHHHLLHHHHHK..", ".KHHHHHHLLHHHHK.", ".KHHHHHHHHHHHHHK",
     "KHHHHHHHHHHHHHHK", "KHHHHHHHHSHHHSHK", "KhHHHHHSSSSHSSSK", "KhHHHHSKKSSSKKSK", "KhHHHHSWESSSWESK",
     "KhHHHhSEESSSEESK", "KhHHHhSSSSSSSSSK", "KhHHHhSSSSSSSSK.", "KhHHHhSSSSSSMSK.", "KhHhhRSSSSSSSK..",
     ".KhKKKKSSSSSK...", "..K....KKKKK...."},  // eyes open
    {".....KKKKKK.....", "...KKHHHHHHKK...", "..KHHHLLHHHHHK..", ".KHHHHHHLLHHHHK.", ".KHHHHHHHHHHHHHK",
     "KHHHHHHHHHHHHHHK", "KHHHHHHHHSHHHSHK", "KhHHHHHSSSSHSSSK", "KhHHHHSSSSSSSSSK", "KhHHHHSSSSSSSSSK",
     "KhHHHhSKKSSSKKSK", "KhHHHhSSSSSSSSSK", "KhHHHhSSSSSSSSK.", "KhHHHhSSSSSSMSK.", "KhHhhRSSSSSSSK..",
     ".KhKKKKSSSSSK...", "..K....KKKKK...."},  // blink
    {".....KKKKKK.....", "...KKHHHHHHKK...", "..KHHHLLHHHHHK..", ".KHHHHHHLLHHHHK.", ".KHHHHHHHHHHHHHK",
     "KHHHHHHHHHHHHHHK", "KHHHHHHHHSHHHSHK", "KhHHHHHSSSSHSSSK", "KhHHHHSSSSSSSSSK", "KhHHHHSKKSSSKKSK",
     "KhHHHhKSSKSKSSKK", "KhHHHhSSSSSSSSSK", "KhHHHhSSSSSSSSK.", "KhHHHhSSSSKMMKK.", "KhHhhRSSSSSKKK..",
     ".KhKKKKSSSSSK...", "..K....KKKKK...."},  // happy ^^, mouth open
};
enum { EYES_OPEN, EYES_BLINK, EYES_HAPPY };

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

uint8_t fb[N * N], bg[N * N];
float T = 0, awake = 0;  // awake > 0: play mode (refilled by a shake)
uint32_t rs = 2463534242u;
float rnd() {
  rs ^= rs << 13, rs ^= rs >> 17, rs ^= rs << 5;
  return (rs >> 8) / 16777216.0f;
}
float clampf(float v, float lo, float hi) { return fminf(fmaxf(v, lo), hi); }

inline void px(int x, int y, uint8_t c) {
  if ((unsigned)x < N && (unsigned)y < N) fb[y * N + x] = c;
}

// Paints pixels within r of a line segment (arms, legs, tail; a disc if both ends match).
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

// Sprite column ax lands on x; mirrored horizontally if d < 0. shut: Teto blinks.
void blit(const char *const *rows, int h, int ax, int x, int y0, int d, bool shut = false) {
  for (int r = 0; r < h; r++)
    for (int c = 0; rows[r][c]; c++) {
      char ch = rows[r][c];
      if (ch != '.') px(d > 0 ? x + c - ax : x - 1 - c + ax, y0 + r, code(shut && ch == 'G' ? 'K' : ch));
    }
}

void shadow(float x, float w) {
  for (int i = -(int)w; i <= (int)w; i++) px(x + i, GROUND, SHADOW);
  for (int i = -(int)(w * 0.6f); i <= (int)(w * 0.6f); i++) px(x + i, GROUND + 1, SHADOW);
}

// ---- Background: sky, drifting clouds, wind streaks, windmills, grass ----
const float BAYER[4] = {0.125f, 0.625f, 0.875f, 0.375f};  // 2x2 dither thresholds
float bayer(int x, int y) { return BAYER[(y & 1) * 2 + (x & 1)]; }

uint8_t skyAt(int x, int y) {
  float v = y * 3.0f / (GROUND - 1);
  int b = v;
  return b + (v - b > bayer(x, y));
}

// Cloud: a few blobs with a flat base; bottom strip shaded. Draws over sky only (stays behind the ridges).
void cloud(float cx, float cy, float s) {
  static const float P[4][3] = {{-4, 0.5f, 2.6f}, {-1, -1.5f, 3.4f}, {3, -0.5f, 3}, {6, 1, 2.2f}};  // x, y, radius
  for (int y = floorf(cy - 5 * s); y <= cy + 2; y++)
    for (int x = floorf(cx - 7 * s); x <= cx + 9 * s; x++) {
      if ((unsigned)x >= N || (unsigned)y >= N || bg[y * N + x] > SKY3) continue;
      for (auto &p : P) {
        float dx = x + 0.5f - (cx + p[0] * s), dy = y + 0.5f - (cy + p[1] * s);
        if (dx * dx + dy * dy < p[2] * p[2] * s * s) {
          px(x, y, y >= cy + 1 ? CLOUD2 : CLOUD);
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
void blades() {
  for (auto &m : MILLS)
    for (int b = 0; b < 4; b++) {
      float a = T * m[3] + b * 1.5708f, x = m[0] + 0.5f, y = m[1] + 0.5f;
      capsule(x, y, x + m[2] * cosf(a), y + m[2] * sinf(a), 0.6f, m[4]);
    }
}

void grassTips() {  // grass tips bending left in the wind
  for (int x = 0; x < N; x++) {
    int h = (x * 7 % 5 < 2) + (x * 13 % 7 == 0), lean = sinf(T * 2.5f + x * 0.3f) > 0.3f ? -1 : 0;
    for (int k = 1; k <= h; k++) px(x + (k == h ? lean : 0), GROUND - k, k == h ? GRASSL : GRASS);
  }
}

// ---- Nausicaa and Teto ----
enum { IDLE, RUN, CROUCH, JUMP, LAND, KNEEL };
struct {
  float x = 32, y = GROUND, vx = 0, vy = 0, t = 0, dur = 1, phase = 0, happy = 0, blow = 0;  // blow: hair flutter from motion, 0..1
  int dir = 1, st = IDLE, hops = 0;
  bool yay = false;  // both arms up during a joy hop
} girl;

enum { SHOULDER, HAND, LEAP, SCAMPER, SIT };  // LEAP and later: Teto is off the girl
struct {
  float x = 0, y = 0, vx = 0, vy = 0, t = 0, dur = 1, tx = 0, ty = GROUND, phase = 0;
  int dir = 1, st = SHOULDER;
  bool toHand = false, home = false;  // toHand: the leap lands on her hand; home: running back to her hand
} teto;
float shX, shY, hdX, hdY;  // shoulder and reaching hand (updated while drawing each frame)

void go(int st, float dur) { girl.st = st, girl.t = 0, girl.dur = dur; }
void tgo(int st, float dur) { teto.st = st, teto.t = 0, teto.dur = dur; }
bool loose() { return teto.st >= LEAP; }
int side() { return teto.x >= girl.x ? 1 : -1; }

void leap(float xt, float h, int hops) {  // girl to xt, h high; hops = how many more hops after
  xt = clampf(xt, 14, 66);
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

void think() {  // girl idle, Teto on the ground: she follows him, then kneels and holds out her hand
  girl.dir = side();
  if (awake > 0 && rnd() < 0.15f) girl.hops = 2, girl.yay = true, girl.happy = 2, go(CROUCH, 0.15f);
  else if (fabsf(teto.x - girl.dir * 12 - girl.x) > 3) go(RUN, 0);
  else go(KNEEL, 1.0f);
}

// Device shaken: Teto leaps off her shoulder, the girl startles and jumps, tries to catch him for a few seconds.
void shake() {
  awake = 6;
  if (teto.st != LEAP) hop(roamX(), GROUND, 8 + 8 * rnd(), false);
  if (girl.st != JUMP) girl.yay = false, leap(girl.x, 5, 0);
}

// Message/button: the girl raises her arms and hops twice for joy (a sitting Teto hops too).
void cheer() {
  girl.happy = 2.5f;
  if (girl.st == IDLE || girl.st == RUN) girl.hops = 2, girl.yay = true, go(CROUCH, 0.15f);
  if (teto.st == SIT) hop(teto.x, GROUND, 5, false);
}

void update(float dt) {
  T += dt, girl.t += dt, teto.t += dt, girl.happy -= dt, awake -= dt;
  float blow = girl.st == RUN ? 1 : girl.st == JUMP ? 0.8f : 0;  // hair lifts fast, settles slowly when she stops
  girl.blow += (blow - girl.blow) * fminf(1, (blow > girl.blow ? 8 : 3) * dt);
  for (int i = 0; i < NW; i++)
    if ((gusts[i].x -= gusts[i].v * dt) < -gusts[i].len) gust(i, N + 40 * rnd());

  switch (girl.st) {
    case IDLE:
      if (!loose()) {
        if (girl.t > girl.dur + 5) girl.dir = -girl.dir, girl.t = 0;  // looks around now and then
        break;
      }
      if (fabsf(teto.x - girl.x) > 3) girl.dir = side();
      if (teto.st != LEAP && girl.t > girl.dur) think();
      break;
    case RUN: {
      int s = side();
      float tx = clampf(teto.x - s * 12, 14, 66), d = tx - girl.x;
      if (fabsf(d) < 1.5f) {
        girl.dir = s;
        go(KNEEL, 1.0f);
        break;
      }
      girl.dir = d > 0 ? 1 : -1;
      girl.x += girl.dir * fminf(34 * dt, fabsf(d));
      girl.phase += 14 * dt;
      if (girl.t > 3 || !loose()) go(IDLE, 0.3f);  // couldn't catch up: take a breather
      break;
    }
    case CROUCH:
      if (girl.t >= girl.dur) leap(girl.x, 6, girl.hops - 1);
      break;
    case JUMP:
      girl.vy += G * dt, girl.x += girl.vx * dt, girl.y += girl.vy * dt;
      if (girl.y < GROUND || girl.vy < 0) break;
      girl.y = GROUND;
      go(LAND, girl.hops > 0 ? 0.08f : 0.18f);
      break;
    case LAND:
      if (girl.t < girl.dur) break;
      if (girl.hops > 0) leap(girl.x, 6, girl.hops - 1);
      else girl.yay = false, go(IDLE, 0.4f + 0.8f * rnd());
      break;
    case KNEEL:  // in play waits briefly; after play, until Teto is on her shoulder
      if (teto.st == SHOULDER || (awake > 0 && girl.t > girl.dur) || girl.t > 6) go(IDLE, 1 + rnd());
      break;
  }

  switch (teto.st) {
    case SHOULDER: break;  // position is pinned to the shoulder while drawing
    case HAND:
      if (teto.t > 0.75f) tgo(SHOULDER, 0);
      break;
    case LEAP:
      teto.vy += G * dt, teto.x += teto.vx * dt, teto.y += teto.vy * dt;
      if (teto.vy < 0 || teto.y < teto.ty) break;
      teto.x = teto.tx, teto.y = teto.ty;
      if (teto.toHand) tgo(HAND, 0);
      else tgo(SIT, 0.3f + 0.8f * rnd());
      break;
    case SCAMPER: {
      float d = teto.tx - teto.x;
      teto.dir = d > 0 ? 1 : -1;
      teto.x += teto.dir * fminf(48 * dt, fabsf(d));
      teto.phase += 20 * dt;
      if (fabsf(d) > 0.5f) break;
      if (teto.home && girl.st == KNEEL) teto.dir = -girl.dir, hop(hdX, hdY, 3, true);
      else tgo(SIT, 0.3f + 0.8f * rnd());
      break;
    }
    case SIT: {
      if (girl.st == KNEEL && awake <= 0) {  // play is over: comes to the reaching hand
        if (girl.t > 0.25f) teto.tx = clampf(hdX + girl.dir * 4, 8, 72), teto.home = true, tgo(SCAMPER, 0);
      } else if (girl.st == KNEEL && fabsf(teto.x - hdX) < 18) {  // in play: leaps over the hand and her head and runs off
        if (girl.t > 0.2f) hop(girl.x - girl.dir * 16, GROUND, 28, false);
      } else if (awake > 0 && teto.t > teto.dur) roam();
      else if (fabsf(girl.x - teto.x) > 2) teto.dir = girl.x > teto.x ? 1 : -1;
      break;
    }
  }
}

void drawGirl() {
  const int d = girl.dir;
  float hip = 7, th[2] = {0.25f, 0.25f}, fx[2] = {}, fl[2] = {};  // hip height; arm angle (0 down, π up); foot slide/lift
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
    case CROUCH: hip = 5, th[0] = th[1] = 0.7f, fx[0] = -d, fx[1] = d; break;
    case LAND: hip = 5.5f, th[0] = th[1] = 0.6f; break;
    case JUMP:
      fl[0] = fl[1] = girl.vy < 0 ? 0 : 1.5f, fx[0] = -d * 0.8f, fx[1] = d * 0.8f, th[0] = th[1] = 1.1f;
      break;
    case KNEEL: hip = 4, fx[0] = -d * 3, fx[1] = d * 2, th[0] = 0.3f, th[1] = fminf(1.4f, girl.t * 6); break;  // front hand toward Teto
  }
  if (girl.yay) th[0] = th[1] = 2.6f + 0.25f * sinf(T * 18);

  int cx = lroundf(girl.x), hy = lroundf(girl.y - hip), ty = hy - 8, ky = ty - 17;  // hip, torso and head top rows
  shadow(cx, 7 * fmaxf(0.3f, 1 - (GROUND - girl.y) / 30));
  for (int i = 0; i < 2; i++) {  // legs: trousers on top, boots below
    float s = i ? d : -d, lx = cx + s * 2, fx1 = lx + fx[i], fy1 = girl.y - 1 - fl[i];
    float mx = (lx + fx1) / 2, my = (hy + fy1) / 2;
    capsule(lx, hy, fx1, fy1, 1.6f, INK);
    capsule(lx, hy, mx, my, 0.7f, PANTS), capsule(mx, my, fx1, fy1, 0.8f, BELT);
    disc(fx1 + d * 0.5f, fy1 + 0.2f, 0.8f, BELT);
  }
  blit(TORSO, TORSO_H, TORSO_AX, cx, ty, d);
  int eyes = girl.happy > 0 ? EYES_HAPPY : fmodf(T, 3.1f) < 0.14f ? EYES_BLINK : EYES_OPEN;
  blit(HEAD[eyes], HEAD_H, HEAD_AX, cx, ky, d);

  if (girl.blow > 0.2f) {  // two locks at the nape fly back when running/jumping; in a jump they rise/fall with vertical speed
    float rx[2], ry[2], tx[2], ty2[2], len = 3 * girl.blow;  // root, tip
    for (int i = 0; i < 2; i++) {
      float a = (girl.st == JUMP ? clampf(-girl.vy / 120, -0.9f, 1.0f) : 0.15f) + 0.25f * sinf(T * 14 + i * 2.1f);
      rx[i] = cx + d * (1.5f - HEAD_AX), ry[i] = ky + 7.5f + i * 4;
      tx[i] = rx[i] - d * len * cosf(a), ty2[i] = ry[i] + len * sinf(a);
      capsule(rx[i], ry[i], tx[i], ty2[i], 1.0f, INK);
    }
    for (int i = 0; i < 2; i++) capsule(rx[i] + d * 1.5f, ry[i], tx[i], ty2[i], 0.5f, HAIR);
  }

  for (int i = 0; i < 2; i++) {  // arms after the head: stay in front of the hair when raised
    float s = i ? d : -d, ax = cx + s * 4.5f, ay = ty + 2.5f;
    float ex = ax + s * 6 * sinf(th[i]), ey = ay + 6 * cosf(th[i]), mx = ax + (ex - ax) * 0.8f, my = ay + (ey - ay) * 0.8f;
    capsule(ax, ay, ex, ey, 1.5f, INK), disc(ex, ey, 1.6f, INK);
    capsule(ax, ay, mx, my, 0.8f, TUNIC), disc(ex, ey, 0.8f, SKIN);  // long-sleeved tunic, hand
    if (i) hdX = ex, hdY = ey - 1;
  }
  shX = cx - d * 5, shY = ty + 5;
}

void attach() {  // pin Teto on the shoulder/hand to the girl's pose this frame
  if (teto.st == SHOULDER) teto.x = shX, teto.y = shY, teto.dir = fmodf(T, 7) > 6.2f ? -girl.dir : girl.dir;  // looks back now and then
  if (teto.st == HAND) {  // sits on the hand for 0.35 s, then climbs up the arm to the shoulder
    float k = clampf((teto.t - 0.35f) / 0.4f, 0, 1);
    teto.x = hdX + (shX - hdX) * k, teto.y = hdY + (shY - hdY) * k - 2 * sinf(k * 3.1416f), teto.dir = -girl.dir;
  }
}

void drawTeto() {
  const int d = teto.dir;
  bool run = teto.st == SCAMPER || teto.st == LEAP || (teto.st == HAND && teto.t > 0.35f);
  const char *const *spr = run ? TRUN[teto.st == SCAMPER && (int(teto.phase / 3.1416f) & 1)] : TSIT;
  int h = run ? TRUN_H : TSIT_H, ax = run ? TRUN_AX : TSIT_AX, x = lroundf(teto.x), y = lroundf(teto.y);
  if (loose()) shadow(x, 3 * fmaxf(0.3f, 1 - (GROUND - teto.y) / 30));

  float kx[5], ky[5];  // tail: raised, swaying, striped chain from the sprite's rear
  kx[0] = x + d * (0.5f - ax), ky[0] = y - h + (run ? 5.5f : 8.5f);
  for (int i = 1; i < 5; i++) {
    float g = (run ? 0.35f : 1.0f) + 0.3f * i + (run ? 0.15f : 0.35f) * sinf(T * (run ? 12 : 4) - 0.7f * i);
    kx[i] = kx[i - 1] - d * 1.5f * cosf(g), ky[i] = ky[i - 1] - 1.5f * sinf(g);
  }
  for (int i = 1; i < 5; i++) capsule(kx[i - 1], ky[i - 1], kx[i], ky[i], 1.5f - 0.1f * i, INK);
  for (int i = 1; i < 5; i++) capsule(kx[i - 1], ky[i - 1], kx[i], ky[i], 0.7f, i & 1 ? FUR : FUR2);
  blit(spr, h, ax, x, y - h, d, fmodf(T + 1.3f, 3.7f) < 0.15f);
}

void init() {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float far = 42 + 3 * sinf(x * 0.09f) + 2 * sinf(x * 0.23f + 1), near = 49 + 2 * sinf(x * 0.14f + 2) + 1.5f * sinf(x * 0.31f);
      uint8_t c = skyAt(x, y);
      if (y >= far) c = HILL;
      if (y >= near) c = HILL2;
      if (y >= GROUND) c = y < GROUND + 3 || (y < GROUND + 5 && bayer(x, y) > 0.5f) ? GRASS : GRASS2;
      fb[y * N + x] = c;
    }
  for (auto &m : MILLS)  // windmill towers: from hub down to the ridge
    for (int y = m[1] + 1; fb[y * N + (int)m[0]] <= SKY3; y++) px(m[0], y, m[4]);
  memcpy(bg, fb, sizeof bg);
  for (int i = 0; i < NW; i++) gust(i, N * rnd());
}

void step(float dt) {
  update(dt);
  memcpy(fb, bg, sizeof fb);
  for (auto &c : CLOUDS) {  // drift left with the wind, wrap from left edge to right
    float x = fmodf(c[0] - T * c[3], 110);
    cloud(x < 0 ? x + 95 : x - 15, c[1], c[2]);
  }
  for (auto &g : gusts)
    for (int k = 0; k < g.len; k++) {
      int x = g.x + k, y = g.y;
      if ((unsigned)x < N && fb[y * N + x] <= CLOUD2) px(x, y, WIND);
    }
  blades();
  grassTips();
  drawGirl();
  attach();
  drawTeto();
}

}  // namespace nausicaa
namespace scene = nausicaa;  // firmware.ino talks to whichever scene is included through this name
