// Cat scene: draws into an 80x80 color-indexed buffer (fb), scaled 3x on screen.
// No Arduino dependency: also compiles on a PC for frame-by-frame testing.
#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>

namespace kitty {

const int N = 80, GROUND = 58;  // logical resolution, ground row (paws stand here)
const float G = 260;            // gravity, px/s²

enum : uint8_t { SKY0, SKY1, SKY2, SKY3, GRASS, GRASS2, STAR, MOON, INK, FUR, FUR2, CREAM, PINK, YARN, YARN2, SHADOW, NCOL };
const uint8_t PAL[NCOL][3] = {
    {14, 16, 44},   {28, 26, 78},   {58, 42, 104},  {104, 60, 120},  // sky, top to horizon
    {70, 150, 70},  {38, 96, 50},   {255, 240, 180}, {250, 230, 160},  // grass, dark grass, star, moon
    {30, 18, 22},   {255, 150, 40}, {200, 95, 25},  {255, 236, 210}, {255, 140, 160},  // cat
    {80, 140, 255}, {40, 70, 200},  {26, 70, 40},   // yarn ball, shadow
};

// Head: K outline, O fur, W cream, P pink. Symmetric, no mirroring needed when turning.
const int HW = 12, HH = 10;
const char *const HEAD[3][HH] = {
    {".K........K.", "KPK......KPK", "KPPK....KPPK", "KOOKKKKKKOOK", "KOOOOOOOOOOK",
     "KOKWOOOOKWOK", "KOKKOOOOKKOK", "KOOOWPPWOOOK", ".KOWWWWWWOK.", "..KKKKKKKK.."},  // eyes open (with highlight)
    {".K........K.", "KPK......KPK", "KPPK....KPPK", "KOOKKKKKKOOK", "KOOOOOOOOOOK",
     "KOOOOOOOOOOK", "KOKKOOOOKKOK", "KOOOWPPWOOOK", ".KOWWWWWWOK.", "..KKKKKKKK.."},  // blink
    {".K........K.", "KPK......KPK", "KPPK....KPPK", "KOOKKKKKKOOK", "KOOOOOOOOOOK",
     "KOOKOOOOKOOK", "KOKOKOOKOKOK", "KOOOWPPWOOOK", ".KOWWPPWWOK.", "..KKKKKKKK.."},  // happy ^^, mouth open
};
enum { EYES_OPEN, EYES_BLINK, EYES_HAPPY };

uint8_t fb[N * N], bg[N * N];
float T = 0, awake = 0;  // awake > 0: cat is in play mode (refilled by a shake)
uint32_t rs = 2463534242u;
float rnd() {
  rs ^= rs << 13, rs ^= rs >> 17, rs ^= rs << 5;
  return (rs >> 8) / 16777216.0f;
}
float clampf(float v, float lo, float hi) { return fminf(fmaxf(v, lo), hi); }

inline void px(int x, int y, uint8_t c) {
  if ((unsigned)x < N && (unsigned)y < N) fb[y * N + x] = c;
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

// ---- Stars and background ----
const int NS = 16;
uint8_t sx[NS], sy[NS];
float sp[NS];

uint8_t skyAt(int x, int y) {  // gradient with 2x2 Bayer dithering
  static const float B[4] = {0.125f, 0.625f, 0.875f, 0.375f};
  float v = y * 3.0f / (GROUND - 1);
  int b = v;
  return b + (v - b > B[(y & 1) * 2 + (x & 1)]);
}

// ---- Cat and yarn ball ----
enum { IDLE, RUN, CROUCH, JUMP, LAND, BAT };
struct {
  float x = 26, y = GROUND, vx = 0, vy = 0, t = 0, dur = 1, phase = 0, happy = 0;
  int dir = 1, st = IDLE, hops = 0;
  bool kicked = false;
} cat;
struct {
  float x = 56, y = GROUND - 3, vx = 0, vy = 0, rot = 0;
} ball;

void go(int st, float dur) { cat.st = st, cat.t = 0, cat.dur = dur, cat.kicked = false; }
int side() { return ball.x >= cat.x ? 1 : -1; }
void kick(float vx, float vy) { ball.vx = vx, ball.vy = vy, cat.happy = 1.2f; }

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

// Device shaken: the yarn flies, the cat startles and jumps, plays for a few seconds.
void shake() {
  awake = 6;
  kick((rnd() < 0.5f ? -1 : 1) * (40 + 60 * rnd()), -(90 + 80 * rnd()));
  if (cat.st == IDLE || cat.st == RUN) leap(cat.x, 10, 0);
}

// Message/button: the cat hops twice in place for joy.
void cheer() {
  cat.happy = 2.5f;
  if (cat.st == IDLE || cat.st == RUN) cat.hops = 2, go(CROUCH, 0.15f);
}

void update(float dt) {
  T += dt, cat.t += dt, cat.happy -= dt, awake -= dt;

  ball.vy += G * dt, ball.x += ball.vx * dt, ball.y += ball.vy * dt;
  if (ball.y >= GROUND - 3) {
    ball.y = GROUND - 3;
    ball.vy = ball.vy > 50 ? -ball.vy * 0.45f : 0;
    ball.vx -= ball.vx * fminf(2.5f * dt, 1);  // rolling friction
    if (fabsf(ball.vx) < 2) ball.vx = 0;
  }
  if (ball.x < 8 || ball.x > 72) ball.x = clampf(ball.x, 8, 72), ball.vx *= -0.6f;  // bounce off the screen edge
  ball.rot += ball.vx * dt / 3;

  switch (cat.st) {
    case IDLE:
      if (fabsf(ball.x - cat.x) > 4) cat.dir = side();
      if (awake > 0 && cat.t > cat.dur && ball.vx == 0 && ball.vy == 0) think();  // stays put unless shaken
      break;
    case RUN: {
      int s = side();
      float tx = clampf(ball.x - s * 12, 16, 64), d = tx - cat.x;
      if (fabsf(d) < 1.5f) {
        cat.dir = s;
        if (rnd() < 0.6f) go(BAT, 0.55f);
        else cat.hops = 0, go(CROUCH, 0.5f);
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
      else leap(ball.x - cat.dir * 8, fminf(10 + 0.25f * fabsf(ball.x - cat.x), 18), 0);
      break;
    case JUMP:
      cat.vy += G * dt, cat.x += cat.vx * dt, cat.y += cat.vy * dt;
      if (cat.y < GROUND || cat.vy < 0) break;
      cat.y = GROUND;
      if (fabsf(ball.x - (cat.x + cat.dir * 8)) < 7 && ball.y > GROUND - 6)  // paws landed on the yarn
        kick(cat.dir * (50 + 40 * rnd()), -(60 + 60 * rnd()));
      go(LAND, cat.hops ? 0.08f : 0.2f);
      break;
    case LAND:
      if (cat.t < cat.dur) break;
      if (cat.hops) leap(cat.x, 7, cat.hops - 1);
      else go(IDLE, 0.6f + 1.2f * rnd());
      break;
    case BAT:
      if (!cat.kicked && cat.t > 0.28f) {  // hit the yarn on the paw's downswing
        cat.kicked = true;
        if (fabsf(ball.x - (cat.x + cat.dir * 12)) < 6) kick(cat.dir * (45 + 35 * rnd()), -(30 + 50 * rnd()));
      }
      if (cat.t > cat.dur) go(IDLE, 0.4f + 0.6f * rnd());
      break;
  }
}

void drawCat() {
  const int d = cat.dir;
  float bh = 9, a = 0, rx = 8, ry = 4.5f;   // body center height above feet, tilt, radii
  float tb = 1.2f, ta = 0.35f, tw = 2.5f;    // tail: base angle, swing amplitude, speed
  float u[4] = {}, lift[4] = {}, th[4] = {};  // legs: 0 far hind, 1 far front, 2 near hind, 3 near front
  bool air = cat.st == JUMP;

  switch (cat.st) {
    case IDLE: bh += 0.5f * sinf(T * 3); break;  // breathing
    case RUN:
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
  }

  float bx = cat.x, by = cat.y - bh, ca = cosf(a), sa = sinf(a);
  auto W = [&](float lx, float ly, float &x, float &y) { x = bx + d * (lx * ca + ly * sa), y = by - lx * sa + ly * ca; };

  float jx[4], jy[4], qx[4], qy[4];  // joint and paw points
  const float JL[4] = {-4, 6, -5.5f, 4.5f};
  for (int i = 0; i < 4; i++) {
    W(JL[i], 2.5f, jx[i], jy[i]);
    bool raised = cat.st == BAT && i == 3;
    if (air || raised) qx[i] = jx[i] + d * 6 * sinf(th[i]), qy[i] = jy[i] + 6 * cosf(th[i]);
    else qx[i] = jx[i] + d * u[i], qy[i] = cat.y - 0.5f - lift[i];
  }

  float kx[8], ky[8];
  W(-7.5f, -1.5f, kx[0], ky[0]);
  for (int i = 1; i < 8; i++) {
    float g = tb - a + 0.13f * i + ta * sinf(tw * T - 0.55f * i) * i / 7;
    kx[i] = kx[i - 1] - d * 1.5f * cosf(g), ky[i] = ky[i - 1] - 1.5f * sinf(g);
  }

  shadow(cat.x, 9 * fmaxf(0.3f, 1 - (GROUND - cat.y) / 30));
  for (int i = 1; i < 8; i++) capsule(kx[i - 1], ky[i - 1], kx[i], ky[i], 2.0f, INK);
  for (int i = 1; i < 8; i++) capsule(kx[i - 1], ky[i - 1], kx[i], ky[i], 1.0f, i >= 6 ? FUR2 : FUR);
  for (int i = 0; i < 2; i++) capsule(jx[i], jy[i], qx[i], qy[i], 1.8f, INK), capsule(jx[i], jy[i], qx[i], qy[i], 0.8f, FUR2);
  body(bx, by, rx, ry, a, d, 1);
  for (int i = 2; i < 4; i++) capsule(jx[i], jy[i], qx[i], qy[i], 1.8f, INK);
  body(bx, by, rx, ry, a, d, 0);
  for (int i = 2; i < 4; i++) capsule(jx[i], jy[i], qx[i], qy[i], 0.8f, FUR), disc(qx[i], qy[i] - 0.3f, 0.8f, CREAM);

  float hx, hy;
  W(7.5f, -5, hx, hy);
  int eyes = cat.happy > 0 ? EYES_HAPPY : fmodf(T, 3.3f) < 0.15f ? EYES_BLINK : EYES_OPEN;
  int x0 = lroundf(hx - HW / 2), y0 = lroundf(hy - HH / 2);
  for (int r = 0; r < HH; r++)
    for (int c = 0; c < HW; c++) {
      char ch = HEAD[eyes][r][c];
      if (ch != '.') px(x0 + c, y0 + r, ch == 'K' ? INK : ch == 'O' ? FUR : ch == 'W' ? CREAM : PINK);
    }
}

void drawBall() {
  shadow(ball.x, 3 * fmaxf(0.3f, 1 - (GROUND - 3 - ball.y) / 30));
  disc(ball.x, ball.y, 3.6f, INK);
  disc(ball.x, ball.y, 2.7f, YARN);
  float c = 2.4f * cosf(ball.rot), s = 2.4f * sinf(ball.rot);  // spinning yarn strand
  capsule(ball.x - c, ball.y - s, ball.x + c, ball.y + s, 0.5f, YARN2);
  capsule(ball.x - s * 0.6f, ball.y + c * 0.6f, ball.x + s * 0.2f, ball.y - c * 0.2f, 0.4f, YARN2);
}

void init() {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) fb[y * N + x] = y < GROUND ? skyAt(x, y) : uint8_t(y < GROUND + 3 ? GRASS : GRASS2);
  for (int y = 10; y < 25; y++)  // crescent moon
    for (int x = 53; x < 68; x++)
      if (hypotf(x + 0.5f - 60, y + 0.5f - 17) < 4.6f && hypotf(x + 0.5f - 62.5f, y + 0.5f - 15.5f) > 4.2f) px(x, y, MOON);
  for (int x = 0; x < N; x++) {
    if (rnd() < 0.3f) px(x, GROUND - 1, GRASS);  // grass tips
    if (rnd() < 0.4f) px(x, GROUND + 3, GRASS);
  }
  memcpy(bg, fb, sizeof bg);
  for (int i = 0; i < NS; i++) sx[i] = 12 + rnd() * 56, sy[i] = 4 + rnd() * 34, sp[i] = rnd() * 6.28f;
}

void step(float dt) {
  update(dt);
  memcpy(fb, bg, sizeof fb);
  for (int i = 0; i < NS; i++)  // twinkling stars (never over the moon)
    if (sinf(T * 1.7f + sp[i]) > -0.2f && bg[sy[i] * N + sx[i]] != MOON) px(sx[i], sy[i], STAR);
  drawBall();
  drawCat();
}

}  // namespace kitty
