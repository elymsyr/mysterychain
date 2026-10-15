// Teto (Nausicaa's fox-squirrel) plays with a glowing spore in the Sea of Corruption: draws into an 80x80 color-indexed
// buffer (fb), scaled 3x on screen. Same interface as ../cat/cat.h (init, step, shake, cheer, fb, PAL, px); no Arduino dependency.
#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>

namespace teto {

const int N = 80, GROUND = 58;  // logical resolution, ground row (paws stand here)
const float G = 260;            // gravity, px/s²

enum : uint8_t {
  SKY0, SKY1, SKY2, SKY3, FUNG0, FUNG1, POD, MOSS, MOSS2, SPORE, SPORE2, GLOW,
  INK, FUR, FUR2, CREAM, EYE, SHADOW, NCOL
};
const uint8_t PAL[NCOL][3] = {
    {10, 26, 36},   {16, 46, 54},   {28, 74, 76},   {52, 108, 100},  // forest canopy to horizon
    {36, 76, 78},   {18, 40, 46},   {130, 220, 176},                 // far, near fungus trees; glowing pod
    {52, 92, 66},   {32, 62, 48},                                    // moss ground
    {214, 255, 226}, {110, 196, 160}, {255, 255, 240},               // spore, dim spore, glow
    {24, 16, 22},   {250, 190, 60}, {112, 62, 30}, {255, 240, 214}, {60, 200, 90}, {20, 42, 36},  // Teto; shadow
};

// Head faces right and is mirrored when facing left. K outline, O fur, D dark stripe, W cream, G eye. Ears drawn separately.
const int HEAD_H = 9, HEAD_AX = 4, HEAD_AY = 5;  // (HEAD_AX, HEAD_AY): point attached to the neck
const char *const HEAD[3][HEAD_H] = {
    {"...KKKKK.....", "..KOOOOOKK...", ".KOOOOOOOOK..", "KODDOKKKOOOK.", "KOOOKGGWKOOKK",
     "KWOOOKKKOOOOK", "KWWOOOOOOKKK.", ".KWWWOOKK....", "..KKKKK......"},  // eyes open
    {"...KKKKK.....", "..KOOOOOKK...", ".KOOOOOOOOK..", "KODDOOOOOOOK.", "KOOOKKKKKOOKK",
     "KWOOOOOOOOOOK", "KWWOOOOOOKKK.", ".KWWWOOKK....", "..KKKKK......"},  // blink
    {"...KKKKK.....", "..KOOOOOKK...", ".KOOOOOOOOK..", "KODDOOOOOOOK.", "KOOOOKKKOOOKK",
     "KWOOKOOOKOOOK", "KWWOOOOOOKKK.", ".KWWWOOKK....", "..KKKKK......"},  // happy ^
};
enum { EYES_OPEN, EYES_BLINK, EYES_HAPPY };

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

// Paints pixels within r of a line segment (legs, ears, tail; a disc if both ends match).
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

// Body: ellipse rotated by a (nose up = +). Dark cross stripes on the back, cream chest in front. grow>0: outline pass.
void body(float bx, float by, float rx, float ry, float a, int dir, float grow) {
  float ca = cosf(a), sa = sinf(a), R = fmaxf(rx, ry) + grow + 1;
  for (int y = floorf(by - R); y <= by + R; y++)
    for (int x = floorf(bx - R); x <= bx + R; x++) {
      float dx = (x + 0.5f - bx) * dir, dy = y + 0.5f - by;
      float lx = dx * ca - dy * sa, ly = dx * sa + dy * ca;
      float ex = lx / (rx + grow), ey = ly / (ry + grow);
      if (ex * ex + ey * ey > 1) continue;
      bool stripe = ly < ry * 0.3f && fmodf(lx + 30.5f, 3.5f) < 1.2f && lx < 3;
      px(x, y, grow > 0 ? INK : ly > ry * 0.3f && lx > 1 ? CREAM : stripe ? FUR2 : FUR);
    }
}

void shadow(float x, float w) {
  for (int i = -(int)w; i <= (int)w; i++) px(x + i, GROUND, SHADOW);
  for (int i = -(int)(w * 0.6f); i <= (int)(w * 0.6f); i++) px(x + i, GROUND + 1, SHADOW);
}

// ---- Background: canopy, fungus trees, moss; spores slowly drifting down ----
const float BAYER[4] = {0.125f, 0.625f, 0.875f, 0.375f};  // 2x2 dither thresholds
float bayer(int x, int y) { return BAYER[(y & 1) * 2 + (x & 1)]; }

uint8_t skyAt(int x, int y) {
  float v = y * 3.0f / (GROUND - 1);
  int b = v;
  return b + (v - b > bayer(x, y));
}

// Fungus tree: stem thickening downward, half-ellipse cap on top, glowing pods under the cap.
void fungus(float x, float top, float w, uint8_t c) {
  for (int y = top; y < GROUND; y++) {
    float sw = 0.6f + (y - top) * 0.05f;
    for (int i = floorf(x - sw); i <= x + sw; i++) px(i, y, c);
  }
  float h = w * 0.5f;
  for (int y = floorf(top - h); y <= top + 1; y++)
    for (int i = floorf(x - w); i <= x + w; i++) {
      float ex = (i + 0.5f - x) / w, ey = (y + 0.5f - top - 1) / h;
      if (ex * ex + ey * ey < 1) px(i, y, c);
    }
  px(x - w * 0.6f, top + 2, POD), px(x + w * 0.3f, top + 2, POD), px(x + w * 0.75f, top + 1, POD);
}

const int NSP = 22;  // spores floating in the air
struct {
  float x, y, vy, ph;
} spores[NSP];

// ---- Teto and the spore ball ----
enum { SIT, IDLE, RUN, CROUCH, JUMP, LAND, BAT };
struct {
  float x = 28, y = GROUND, vx = 0, vy = 0, t = 0, dur = 1, phase = 0, happy = 0;
  int dir = 1, st = SIT, hops = 0;
  bool booped = false, joy = false;  // joy: joy hop, ignores the ball
} fox;
struct {
  float x = 46, y = 34, vx = 0, vy = 0, tx = 46, ty = 34, re = 0;
} orb;

const int NP = 20;  // sparkles scattered when the ball is touched
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

void go(int st, float dur) { fox.st = st, fox.t = 0, fox.dur = dur, fox.booped = false; }
int side() { return orb.x >= fox.x ? 1 : -1; }

void leap(float xt, float h, int hops) {  // to xt, h high; hops = how many more hops after
  xt = clampf(xt, 16, 64);
  fox.vy = -sqrtf(2 * G * h);
  fox.vx = (xt - fox.x) / (2 * -fox.vy / G);
  fox.hops = hops;
  go(JUMP, 0);
}

void retarget() {  // in play the ball glides to a new spot, away from Teto
  orb.tx = 14 + 52 * rnd(), orb.ty = 24 + 22 * rnd();
  if (fabsf(orb.tx - fox.x) < 14) orb.tx = clampf(fox.x + (orb.tx < fox.x ? -18 : 18), 14, 66);
  orb.re = 1.2f + 1.3f * rnd();
}

// Did the nose or a raised paw touch the ball
bool touch(float reach, float up) { return hypotf(orb.x - (fox.x + fox.dir * reach), orb.y - (fox.y - up)) < 5; }

void boop() {  // touched the ball: it bounces and scatters sparkles
  fox.booped = true, fox.happy = 1.2f;
  orb.vx = fox.dir * (50 + 40 * rnd()), orb.vy = -(50 + 50 * rnd());
  for (int i = 0; i < 10; i++) {
    float a = i * 0.628f, v = 30 + 20 * rnd();
    spark(orb.x, orb.y, v * cosf(a), v * sinf(a), 0.5f);
  }
  retarget();
}

void pounceOrBat() {
  if (orb.y > GROUND - 18) go(BAT, 0.55f);  // low: bats it with a paw
  else fox.hops = 0, go(CROUCH, 0.5f);      // high: crouch + pounce
}

void think() {  // idle wait is over: pick the next game
  fox.dir = side();
  if (rnd() < 0.15f) fox.hops = 3, fox.joy = true, fox.happy = 2.5f, go(CROUCH, 0.2f);  // 3 happy hops in place
  else if (fabsf(orb.x - fox.dir * 10 - fox.x) > 5) go(RUN, 0);
  else pounceOrBat();
}

// Device shaken: spores swirl up, the ball is flung, Teto startles and jumps, plays for a few seconds.
void shake() {
  awake = 6;
  orb.vx += (rnd() < 0.5f ? -1 : 1) * (60 + 60 * rnd()), orb.vy -= 40 + 50 * rnd();
  retarget();
  for (auto &s : spores) s.vy = -(10 + 30 * rnd());
  if (fox.st <= RUN) fox.joy = false, leap(fox.x, 9, 0);
}

// Message/button: Teto hops twice in place for joy.
void cheer() {
  fox.happy = 2.5f;
  if (fox.st <= RUN) fox.hops = 2, fox.joy = true, go(CROUCH, 0.15f);
}

void update(float dt) {
  T += dt, fox.t += dt, fox.happy -= dt, awake -= dt;

  // Ball: glides to its target as a damped spring. Hovers in front of Teto's nose when calm, flees in play.
  if (awake > 0) {
    if ((orb.re -= dt) <= 0) retarget();
  } else {
    orb.tx = fox.x + fox.dir * 13 + 4 * sinf(T * 0.5f), orb.ty = GROUND - 22 + 3 * sinf(T * 1.1f);
  }
  float tx = orb.tx + 2 * sinf(T * 1.9f), ty = orb.ty + 2 * sinf(T * 2.9f);
  orb.vx += (5 * (tx - orb.x) - 2.2f * orb.vx) * dt;
  orb.vy += (5 * (ty - orb.y) - 2.2f * orb.vy) * dt;
  orb.x += orb.vx * dt, orb.y += orb.vy * dt;
  if (orb.x < 10 || orb.x > 70) orb.x = clampf(orb.x, 10, 70), orb.vx *= -0.5f;
  if (orb.y < 14 || orb.y > 52) orb.y = clampf(orb.y, 14, 52), orb.vy *= -0.5f;
  for (auto &s : sparks)
    if (s.life > 0) s.life -= dt, s.x += s.vx * dt, s.y += s.vy * dt, s.vx *= 1 - 3 * dt, s.vy *= 1 - 3 * dt;

  for (int i = 0; i < NSP; i++) {  // spores: fall swaying side to side; a shake lifts them, then they slow again
    auto &s = spores[i];
    s.vy += (4 + i % 3 * 2 - s.vy) * 1.2f * dt;
    s.y += s.vy * dt, s.x += 4 * sinf(T * 0.8f + s.ph) * dt;
    if (s.y > GROUND + 1 || s.y < -12) s.x = N * rnd(), s.y = -1 - 6 * rnd(), s.vy = 4;
  }

  switch (fox.st) {
    case SIT:
      if (fabsf(orb.x - fox.x) > 4) fox.dir = side();
      if (awake > 0) go(IDLE, 0.2f);
      break;
    case IDLE:
      if (fabsf(orb.x - fox.x) > 4) fox.dir = side();
      if (fox.t > fox.dur) awake > 0 ? think() : go(SIT, 0);
      break;
    case RUN: {
      int s = side();
      float tx = clampf(orb.x - s * 10, 16, 64), d = tx - fox.x;
      if (fabsf(d) < 1.5f) {
        fox.dir = s;
        pounceOrBat();
        break;
      }
      fox.dir = d > 0 ? 1 : -1;
      fox.x += fox.dir * fminf(42 * dt, fabsf(d));
      fox.phase += 15 * dt;
      if (fox.t > 3) go(IDLE, 0.3f);  // couldn't catch up: take a breather
      break;
    }
    case CROUCH:
      if (fox.t < fox.dur) break;
      if (fox.hops) leap(fox.x, 7, fox.hops - 1);
      else leap(orb.x - fox.dir * 9, clampf(GROUND - 12 - orb.y, 4, 24), 0);
      break;
    case JUMP:
      fox.vy += G * dt, fox.x += fox.vx * dt, fox.y += fox.vy * dt;
      if (!fox.joy && !fox.booped && touch(10, 13)) boop();
      if (fox.y < GROUND || fox.vy < 0) break;
      fox.y = GROUND;
      go(LAND, fox.hops ? 0.08f : 0.2f);
      break;
    case LAND:
      if (fox.t < fox.dur) break;
      if (fox.hops) leap(fox.x, 7, fox.hops - 1);
      else fox.joy = false, go(IDLE, 0.6f + 1.2f * rnd());
      break;
    case BAT:
      if (!fox.booped && fox.t > 0.15f && fox.t < 0.4f && touch(12, 12)) boop();
      if (fox.t > fox.dur) go(IDLE, 0.4f + 0.6f * rnd());
      break;
  }
}

void drawFox() {
  const int d = fox.dir;
  float bh = 10, a = 0, rx = 7, ry = 3.6f;   // body center height above feet, tilt, radii
  float tb = 1.3f, ta = 0.3f, tw = 2.5f;     // tail: base angle, swing amplitude, speed
  float ear = 0.35f;                         // ears laid back (0 = upright)
  float u[4] = {}, lift[4] = {}, th[4] = {}; // legs: 0 far hind, 1 far front, 2 near hind, 3 near front
  bool air = fox.st == JUMP;

  switch (fox.st) {
    case SIT:  // sits on its haunches, tail raised behind
      a = 0.45f, bh = 8.5f + 0.3f * sinf(T * 2.5f), tb = 1.0f, ta = 0.25f, tw = 1.8f;
      u[0] = u[2] = 3;
      break;
    case IDLE: bh += 0.5f * sinf(T * 3); break;  // breathing
    case RUN:
      bh += 0.8f * fabsf(sinf(fox.phase)), tb = 0.7f, ta = 0.2f, tw = 8, ear = 1.0f;
      for (int i = 0; i < 4; i++) {  // trot: diagonal pairs move together
        float p = fox.phase + (i == 0 || i == 3 ? 0 : 3.1416f);
        u[i] = 3 * sinf(p), lift[i] = 2 * fmaxf(0, cosf(p));
      }
      break;
    case CROUCH: bh = 7, a = -0.1f + 0.06f * sinf(fox.t * 30), rx = 7.3f, ry = 3.4f, tb = 0.6f, ta = 0.5f, tw = 14, ear = 0.8f; break;
    case LAND: bh = 8, a = -0.15f, rx = 7.4f, ry = 3.3f, tb = 0.9f; break;
    case JUMP: {
      bool up = fox.vy < 0;
      a = clampf(-fox.vy / 300, -0.4f, 0.4f) * fminf(1, fabsf(fox.vx) / 25);
      if (up) rx = 7.6f, ry = 3.2f;  // stretch
      tb = 0.3f, ta = 0.1f, tw = 6, ear = 0.9f;
      th[1] = th[3] = up ? 1.0f : 0.6f, th[0] = th[2] = up ? -1.0f : -0.4f;
      break;
    }
    case BAT: {
      float t = fox.t;
      a = 0.12f, bh = 10.5f, tb = 1.3f, ta = 0.3f, tw = 6, ear = 0.15f;
      th[3] = t < 0.25f ? 2.2f * t / 0.25f : t < 0.35f ? 2.2f - 19 * (t - 0.25f) : 0.3f * fmaxf(0, 1 - (t - 0.35f) / 0.2f);
      break;
    }
  }
  if (fox.happy > 0 && !air) ear = 0.1f;  // ears perk up when happy

  float bx = fox.x, by = fox.y - bh, ca = cosf(a), sa = sinf(a);
  auto W = [&](float lx, float ly, float &x, float &y) { x = bx + d * (lx * ca + ly * sa), y = by - lx * sa + ly * ca; };

  float jx[4], jy[4], qx[4], qy[4];  // joint and paw points
  const float JL[4] = {-4, 5, -5.5f, 4};
  for (int i = 0; i < 4; i++) {
    W(JL[i], 2.2f, jx[i], jy[i]);
    bool raised = fox.st == BAT && i == 3;
    if (air || raised) qx[i] = jx[i] + d * 7 * sinf(th[i]), qy[i] = jy[i] + 7 * cosf(th[i]);
    else qx[i] = jx[i] + d * u[i], qy[i] = fox.y - 0.5f - lift[i];
  }

  const int NT = 9;  // tail: thick, fluffy, ringed
  float kx[NT], ky[NT], kr[NT];
  W(-6.5f, -1, kx[0], ky[0]);
  for (int i = 0; i < NT; i++) {
    kr[i] = 1.3f + 1.2f * sinf(3.1416f * (i + 1) / (NT + 1));
    if (!i) continue;
    float g = tb - a + 0.14f * i + ta * sinf(tw * T - 0.5f * i) * i / (NT - 1);
    kx[i] = kx[i - 1] - d * 1.7f * cosf(g), ky[i] = ky[i - 1] - 1.7f * sinf(g);
  }

  shadow(fox.x, 9 * fmaxf(0.3f, 1 - (GROUND - fox.y) / 30));
  for (int i = 1; i < NT; i++) capsule(kx[i - 1], ky[i - 1], kx[i], ky[i], kr[i] + 1, INK);
  for (int i = 1; i < NT; i++) capsule(kx[i - 1], ky[i - 1], kx[i], ky[i], kr[i], (i + 1) / 2 % 2 || i == NT - 1 ? FUR2 : FUR);
  for (int i = 0; i < 2; i++) capsule(jx[i], jy[i], qx[i], qy[i], 1.6f, INK), capsule(jx[i], jy[i], qx[i], qy[i], 0.7f, FUR2);
  body(bx, by, rx, ry, a, d, 1);
  for (int i = 2; i < 4; i++) capsule(jx[i], jy[i], qx[i], qy[i], 1.6f, INK);
  body(bx, by, rx, ry, a, d, 0);
  for (int i = 2; i < 4; i++) capsule(jx[i], jy[i], qx[i], qy[i], 0.7f, FUR), disc(qx[i], qy[i] - 0.3f, 0.8f, FUR2);

  float hx, hy, rfx, rfy;
  W(8.5f, -4.5f, hx, hy);
  W(6.5f, 0.5f, rfx, rfy);
  disc(rfx, rfy, 2.9f, INK), disc(rfx, rfy, 2.0f, CREAM);  // fluffy cream chest ruff

  int hxi = lroundf(hx), hyi = lroundf(hy);
  for (int k = 0; k < 2; k++) {  // ears: behind the head, long, dark tips; the front ear twitches now and then
    float e = ear + (k ? -0.15f : 0.2f) + (k && fmodf(T, 4.3f) < 0.15f ? 0.35f : 0);
    float x0 = hxi + d * (3.5f + 3 * k + 0.5f - HEAD_AX), y0 = hyi - HEAD_AY + 1.5f, vx = -d * sinf(e), vy = -cosf(e), L = k ? 9.5f : 8.5f;
    float x1 = x0 + vx * L * 0.55f, y1 = y0 + vy * L * 0.55f, x2 = x0 + vx * L, y2 = y0 + vy * L;
    capsule(x0, y0, x1, y1, 2.8f, INK), capsule(x1, y1, x2, y2, 2.0f, INK), px(x2 + vx * 1.8f, y2 + vy * 1.8f, INK);
    capsule(x0, y0, x1, y1, 1.9f, FUR), capsule(x1, y1, x2, y2, 1.1f, FUR2);
  }
  int eyes = fox.happy > 0 ? EYES_HAPPY : fmodf(T, 3.3f) < 0.15f ? EYES_BLINK : EYES_OPEN;
  for (int r = 0; r < HEAD_H; r++)
    for (int c = 0; HEAD[eyes][r][c]; c++) {
      char ch = HEAD[eyes][r][c];
      if (ch == '.') continue;
      uint8_t v = ch == 'O' ? FUR : ch == 'D' ? FUR2 : ch == 'W' ? CREAM : ch == 'G' ? EYE : INK;
      px(d > 0 ? hxi + c - HEAD_AX : hxi - 1 - c + HEAD_AX, hyi - HEAD_AY + r, v);
    }
}

void drawOrb() {  // dandelion-like spore: bright core, spinning ring of fluff
  int x = lroundf(orb.x), y = lroundf(orb.y);
  for (int k = 0; k < 8; k++) {
    float a = T * 2 + k * 0.785f;
    px(x + lroundf(3.2f * cosf(a)), y + lroundf(3.2f * sinf(a)), SPORE2);
  }
  px(x - 1, y, SPORE), px(x + 1, y, SPORE), px(x, y - 1, SPORE), px(x, y + 1, SPORE), px(x, y, GLOW);
  for (auto &s : sparks)
    if (s.life > 0) px(s.x, s.y, s.life > 0.25f ? GLOW : SPORE);
}

void init() {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      uint8_t c = skyAt(x, y);
      if (y >= GROUND) c = y < GROUND + 2 || (y < GROUND + 4 && bayer(x, y) > 0.5f) ? MOSS : MOSS2;
      fb[y * N + x] = c;
    }
  for (int x = 0; x < N; x++)  // low fungus mounds in the distance
    for (int y = 50 + 2 * sinf(x * 0.3f) + 1.5f * sinf(x * 0.17f + 1); y < GROUND; y++) px(x, y, FUNG0);
  fungus(16, 24, 7, FUNG0), fungus(36, 33, 4, FUNG0), fungus(52, 20, 9, FUNG0), fungus(69, 29, 6, FUNG0);
  fungus(9, 34, 6, FUNG1), fungus(73, 38, 5, FUNG1), fungus(60, 44, 3, FUNG1);
  for (int x = 4; x < N - 4; x += 7 + x % 5) {  // small mushrooms on the ground
    px(x, GROUND - 1, FUNG1), px(x - 1, GROUND - 2, FUNG1), px(x, GROUND - 2, FUNG1), px(x + 1, GROUND - 2, FUNG1);
    if (x % 3 == 0) px(x, GROUND - 3, POD);
  }
  memcpy(bg, fb, sizeof bg);
  for (auto &s : spores) s.x = N * rnd(), s.y = GROUND * rnd(), s.vy = 4, s.ph = 6.28f * rnd();
}

void step(float dt) {
  update(dt);
  memcpy(fb, bg, sizeof fb);
  for (auto &s : spores) px(s.x, s.y, fmodf(T * 0.7f + s.ph, 2) < 1 ? SPORE : SPORE2);
  drawFox();
  drawOrb();
}

}  // namespace teto
namespace scene = teto;  // firmware.ino talks to whichever scene is included through this name
