// Runner mini game (K7), Chrome dino style. Included from the cat scene (cat.h) inside its namespace, after it defines:
//   hero (the runner: x, y, vx, vy, dir, happy, phase), drawHero(), drawSky(), pose(), go() with RUN / JUMP / KO / IDLE,
//   vis.on, art() with the cat's sprite letters, bg / fb, rnd(), T, GROUND,
//   BIRD_TOP (bird's top row: a hop's head passes under it), HEAD_X0 / HEAD_X1 / HEAD_H (head span from hero.x and its
//   top above the paws, for the bird),
//   GAME_UP (the game is drawn this many rows higher: the ground nears the wide middle of the round screen),
//   LABEL_Y (score bubble row: down in the grass, clear of the jumps), SCROLL_Y (first ground row).
// The hero runs at a fixed x, the world scrolls left. Tap = hop (pots, under birds); hold = high jump (fences).
// The firmware feeds the button level (gameInput), restarts after a game over and reads `game.ended`.

enum { POT, FENCE, BIRD };  // BIRD: a pot with a bird flying low over it
// Jump: a tap = SHORT_H (clears a pot, passes under a bird), held = up to LONG_H (clears a fence, hits a bird).
// Letting go caps the rise at SHORT_H (or at once if already higher), so a quick tap is always the same small hop.
const float GJ = 110, SHORT_H = 14, LONG_H = 22, DROP_V = 20;  // gravity px/s², apex px, rise left after a late let-go px/s
const float JUMP_V = sqrtf(2 * GJ * LONG_H);                     // take-off speed px/s
const float SPEED0 = 50, SPEED_UP = 5, SPEED_MAX = 90;   // px/s; +SPEED_UP every 10 points
const float TIGHT = 0.8f;  // pot right after a pot: gap in s of running
const int POT_H = 10, FENCE_H = 17;
const float OB_W[] = {5, 7, 5}, OB_H[] = {POT_H, FENCE_H, POT_H};  // hitbox width, height
const float BIRD_LOW = GROUND - BIRD_TOP - 4;  // bird's bottom, px above the ground
// The bird flies BIRD_DX px behind its pot (art and hitbox 7 px wide): a high jump can't sneak a rise or a fall past it
// (both need SPEED_MAX * 0.14 s < ~14 px; ~0.14 s = the time to cross between pot and bird height).
const int BIRD_DX = 4;

// art: hitbox + 1 px each side, OB_H rows tall
const char *const POT_ART[POT_H] = {"..g.g..", ".gGgGg.", "..gGg..", "...G...", "KKKKKKK",
                                    "KCCCCCK", "KCCCCCK", ".KCCCK.", ".KCCCK.", ".KKKKK."};
const char *const FENCE_ART[FENCE_H] = {".K..K..K.", "KWKKWKKWK", "KWKKWKKWK", "KWKKWKKWK", "KWKKWKKWK", "KWKKWKKWK", "KKKKKKKKK",
                                        "KWWWWWWWK", "KKKKKKKKK", "KWKKWKKWK", "KWKKWKKWK", "KWKKWKKWK",
                                        "KWKKWKKWK", "KKKKKKKKK", "KWWWWWWWK", "KKKKKKKKK", "KWKKWKKWK"};
const char *const BIRD_ART[2][5] = {{"...KK..", "..KYYK.", ".KYYYYK", "CKKYYYK", ".KKKKK."},   // wings up
                                    {".......", ".KKKKK.", "CKKYYYK", ".KYYYYK", "...KYK."}};  // wings down

using pet::Game;
using pet::Ob;
Game &game = pet::runner;

float nextX(float after) { return after + game.speed * (1.1f + 1.2f * rnd()) + 10; }
// Right after a pot: hop, land, hop again (a high jump lands late, unless timed just right).
float tightX(float after) { return after + game.speed * TIGHT; }
uint8_t pickOb() {
  float r = rnd();
  if (game.score >= 10 && r < 0.25f) return BIRD;
  return game.score >= 3 && r < 0.6f ? FENCE : POT;
}

void gameStart() {
  game = Game();
  game.on = true, game.speed = SPEED0, game.air = -1, game.labelY = LABEL_Y;
  hero.x = 9, hero.dir = 1, hero.happy = 0;
  pose(RUN, 0);
  vis.on = false;
  float x = 90;
  for (Ob &o : game.ob) o = {x, POT, false}, x = nextX(x);  // a few easy pots first
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
  if (!game.fixedSpeed) game.speed = fminf(SPEED0 + SPEED_UP * (game.score / 10), SPEED_MAX);
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
    if (o.x < -10) {
      bool tight = last->kind == POT && game.score >= 5 && rnd() < 0.3f;
      uint8_t kind = pickOb();
      o = {tight ? tightX(last->x) : nextX(last->x), tight && kind == FENCE ? (uint8_t)POT : kind, false}, last = &o;
    }
}

void gameDraw() {
  drawSky();
  int off = (int)game.dist % N;  // the ground scrolls with the world
  for (int y = SCROLL_Y; y < N; y++)
    for (int x = 0; x < N; x++) fb[y * N + x] = bg[y * N + (x + off) % N];
  for (const Ob &o : game.ob) {
    int x = lroundf(o.x);
    if (o.kind == FENCE) art(FENCE_ART, FENCE_H, x - 1, GROUND - FENCE_H);
    else art(POT_ART, POT_H, x - 1, GROUND - POT_H);
    if (o.kind == BIRD) art(BIRD_ART[sinf(T * 14) > 0], 5, x + BIRD_DX, BIRD_TOP);
  }
  drawHero();
  memmove(fb, fb + GAME_UP * N, (N - GAME_UP) * N);  // view raised: the ground nears the wide middle
  memcpy(fb + (N - GAME_UP) * N, fb + (N - 2 * GAME_UP) * N, GAME_UP * N);  // bottom rows: the grass again
}

// Bubble text: score and high score while running, the result after a game over.
void gameLabel(char *out, int cap) {
  int best = pet::best() > game.score ? pet::best() : game.score;
  if (!game.dead) snprintf(out, cap, "%d HI %d", game.score, best);
  else if (game.newBest) snprintf(out, cap, "NEW BEST %d", game.score);
  else snprintf(out, cap, "%d BEST %d", game.score, best);
}
