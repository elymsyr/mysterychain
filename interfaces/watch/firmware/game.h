// Runner mini game (K7), Chrome dino style. Included from the cat scene (cat.h) inside its namespace, after it defines:
//   hero (the runner: x, y, vx, vy, dir, happy, phase), drawHero(), drawSky(), pose(), go() with RUN / JUMP / KO / IDLE,
//   vis.on, art() with the cat's sprite letters, bg / fb, rnd(), T, GROUND,
//   BIRD_TOP (bird's top row: a hop's head passes under it), HEAD_X0 / HEAD_X1 / HEAD_H (head span from hero.x and its
//   top above the paws, for the bird),
//   LABEL_Y (score bubble row: down in the grass, clear of the jumps), SCROLL_Y (first ground row),
//   the wide view: gfb (GN x GN), OX / OY (world x = view x - OX), skyAt(), bg (its ground rows scroll).
// The hero runs at a fixed x, the world scrolls left. Tap = hop (pots, under birds); hold = high jump (fences).
// The firmware feeds the button level (gameInput), restarts after a game over and reads `game.ended`.

enum { POT, FENCE, BIRD };  // BIRD: a pot with a bird flying low over it
// Jump: a tap = SHORT_H (clears a pot, passes under a bird), held = up to LONG_H (clears a fence, hits a bird).
// Letting go caps the rise at SHORT_H (or at once if already higher), so a quick tap is always the same small hop.
const float GJ = 110, SHORT_H = 14, LONG_H = 22, DROP_V = 20;  // gravity px/s², apex px, rise left after a late let-go px/s
const float JUMP_V = sqrtf(2 * GJ * LONG_H);                     // take-off speed px/s
const float SPEED0 = 35, SPEED_UP = 1, SPEED_MAX = 90;   // px/s; +SPEED_UP every second played (top speed after ~1 min)
const int POT_H = 10, FENCE_H = 16;
const float OB_W[] = {5, 7, 5}, OB_H[] = {POT_H, FENCE_H, POT_H};  // hitbox width, height
const float BIRD_LOW = GROUND - BIRD_TOP - 4;  // bird's bottom, px above the ground
// The bird flies BIRD_DX px behind its pot (art and hitbox 7 px wide): a high jump can't sneak a rise or a fall past it
// (both need SPEED_MAX * 0.14 s < ~14 px; ~0.14 s = the time to cross between pot and bird height).
const int BIRD_DX = 4;

// art: hitbox + 1 px each side, OB_H rows tall
const char *const POT_ART[POT_H] = {"..g.g..", ".gGgGg.", "..gGg..", "...G...", "KKKKKKK",
                                    "KCCCCCK", "KCCCCCK", ".KCCCK.", ".KCCCK.", ".KKKKK."};
const char *const FENCE_ART[FENCE_H] = {".K..K..K.", "KWKKWKKWK", "KWKKWKKWK", "KWKKWKKWK", "KWKKWKKWK", "KWKKWKKWK", "KKKKKKKKK",
                                        "KWWWWWWWK", "KKKKKKKKK", "KWKKWKKWK", "KWKKWKKWK",
                                        "KWKKWKKWK", "KKKKKKKKK", "KWWWWWWWK", "KKKKKKKKK", "KWKKWKKWK"};
const char *const BIRD_ART[2][5] = {{"...KK..", "..KYYK.", ".KYYYYK", "CKKYYYK", ".KKKKK."},   // wings up
                                    {".......", ".KKKKK.", "CKKYYYK", ".KYYYYK", "...KYK."}};  // wings down

using pet::Game;
using pet::Ob;
Game &game = pet::runner;

// Spacing, after Chrome's dino (gap = what a jump covers at the current speed, x a random 1..1.5): centre to centre,
// half of each obstacle's jump time plus SLACK to land and press again. Roomy at first, down to a quick re-press
// (~human reaction) as the game speeds up. In seconds, so it holds at any speed.
const float AIR_TAP = 2 * sqrtf(2 * SHORT_H / GJ), AIR_HIGH = 2 * sqrtf(2 * LONG_H / GJ);  // ~1.0 s, ~1.26 s in the air
const float AIR[] = {AIR_TAP, AIR_HIGH, AIR_TAP};
const float SPAN[] = {5, 7, BIRD_DX + 7};  // how far along the ground each one reaches (the bird flies behind its pot)
const float SLACK0 = 0.8f, SLACK1 = 0.3f, LEVEL_S = 60;  // s: at the start, at full difficulty (after LEVEL_S)
float level() { return fminf(game.t / LEVEL_S, 1); }
float gapX(const Ob &a, uint8_t kind, float extra) {  // x of the next obstacle after a; extra 0..1 = up to 1.5x the gap
  float s = (AIR[a.kind] + AIR[kind]) / 2 + SLACK0 + (SLACK1 - SLACK0) * level();
  return a.x + SPAN[a.kind] / 2 + game.speed * s * (1 + 0.5f * extra) - SPAN[kind] / 2;
}
uint8_t pickOb() {
  float r = rnd();
  if (game.score >= 10 && r < 0.25f) return BIRD;
  return game.score >= 3 && r < 0.6f ? FENCE : POT;
}

void gameStart() {
  game = Game();
  game.on = true, game.speed = SPEED0, game.air = -1, game.labelY = LABEL_Y;
  hero.x = -2, hero.dir = 1, hero.happy = 0;  // left in the wide view, clear of the round edge
  pose(RUN, 0);
  vis.on = false;
  const Ob *prev = &(game.ob[0] = {GN - OX + 10.f, POT, false});  // just off the right edge; a few easy pots first
  for (Ob &o : game.ob)
    if (&o != prev) o = {gapX(*prev, POT, rnd()), POT, false}, prev = &o;
}

void finish() {  // stats and the event for the firmware
  game.newBest = pet::gameOver(game.score);
  game.ended = true;
}

void gameStop() {  // left with a long hold, or by find
  if (game.on && !game.dead && game.score > 0) finish();
  game.on = false;
  pose(IDLE, 1);
}

void gameInput(bool held) {  // button level, every frame
  bool press = held && !game.held;
  game.held = held;
  if (press && !game.dead && game.air < 0) game.air = 0, hero.vy = -JUMP_V, go(JUMP, 0);
}

void gameTilt(float) {}  // the runner jumps with the button; tilt steers Pavi's and Nausicaa's games

bool hits(const Ob &o) {
  float h = GROUND - hero.y;
  float bx = o.x + BIRD_DX;
  if (o.kind == BIRD && bx < hero.x + HEAD_X1 && bx + 7 > hero.x + HEAD_X0 && h + HEAD_H > BIRD_LOW) return true;
  return o.x < hero.x + 4 && o.x + OB_W[o.kind] > hero.x - 2 && h < OB_H[o.kind];
}

void gameStep(float dt) {
  T += dt, game.t += dt;
  if (game.dead) {  // tumble down, wait for a press (firmware) or give up after 10 s
    if (hero.y < GROUND || hero.vy < 0) {
      hero.vy += GJ * dt, hero.y += hero.vy * dt;
      if (hero.y >= GROUND) hero.y = GROUND, hero.vy = 0;
    }
    if (game.t > 10) game.on = false, pose(IDLE, 1);
    return;
  }
  if (!game.fixedSpeed) game.speed = fminf(SPEED0 + SPEED_UP * game.t, SPEED_MAX);
  float dx = game.speed * dt;
  game.dist += dx;
  hero.phase += 15 * dt * game.speed / SPEED0;
  if (game.air >= 0) {
    game.air += dt;
    float cap = fmaxf(sqrtf(2 * GJ * fmaxf(SHORT_H - (GROUND - hero.y), 0)), DROP_V);
    if (!game.held && hero.vy < -cap) hero.vy = -cap;  // let go: the rise ends at SHORT_H
    hero.vy += GJ * dt, hero.y += hero.vy * dt, hero.vx = game.speed;  // vx only tilts the jump pose
    if (hero.y >= GROUND) hero.y = GROUND, hero.vy = 0, game.air = -1, go(RUN, 0);
  }
  const Ob *last = game.ob;
  for (Ob &o : game.ob) {
    o.x -= dx;
    if (o.x > last->x) last = &o;
    if (!o.passed && o.x + OB_W[o.kind] < hero.x - 2) o.passed = true, game.score++;
    if (hits(o)) {
      game.dead = true, game.t = 0, game.air = -1;
      hero.vy = -50;
      go(KO, 0);
      finish();
      return;
    }
  }
  for (Ob &o : game.ob)
    if (o.x < -OX - 10) {
      uint8_t kind = pickOb();
      o = {gapX(*last, kind, rnd()), kind, false}, last = &o;
    }
}

void gameDraw() {
  int off = (int)game.dist % N;  // the ground scrolls with the world
  for (int vy = 0; vy < GN; vy++)
    for (int vx = 0, y = vy - OY; vx < GN; vx++) {
      int x = vx - OX;
      gfb[vy * GN + vx] = y < SCROLL_Y ? skyAt(vx, y < 0 ? 0 : y) : y < N ? bg[y * N + (x + off + N) % N] : uint8_t(GRASS2);
    }
  drawSky();
  for (const Ob &o : game.ob) {
    int x = lroundf(o.x);
    if (o.kind == FENCE) art(FENCE_ART, FENCE_H, x - 1, GROUND - FENCE_H);
    else art(POT_ART, POT_H, x - 1, GROUND - POT_H);
    if (o.kind == BIRD) art(BIRD_ART[sinf(T * 14) > 0], 5, x + BIRD_DX, BIRD_TOP);
  }
  drawHero();
}

// Bubble text: score and high score while running, the result after a game over.
void gameLabel(char *out, int cap) {
  int best = pet::best() > game.score ? pet::best() : game.score;
  if (!game.dead) snprintf(out, cap, "%d HI %d", game.score, best);
  else if (game.newBest) snprintf(out, cap, "NEW BEST %d", game.score);
  else snprintf(out, cap, "%d BEST %d", game.score, best);
}
