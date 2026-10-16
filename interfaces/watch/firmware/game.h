// Runner mini game (K7), Chrome dino style. Included from the cat scene (cat.h) inside its namespace, after it defines:
//   hero (the runner: x, y, vx, vy, dir, happy, phase), drawHero(), drawSky(), pose(), go() with RUN / JUMP / KO / IDLE,
//   vis.on, art() with the cat's sprite letters, bg / fb, rnd(), T, GROUND,
//   BIRD_TOP (bird rows: just above the running hero's head), HEAD_X0 / HEAD_X1 (head span from hero.x, for the bird),
//   GAME_CAM (the game is drawn this many rows lower: a high jump stays below the score bubble), SCROLL_Y (first ground row).
// The hero runs at a fixed x, the world scrolls left. Press = jump; keep holding = higher (tall fences), like Chrome's dino.
// The firmware feeds the button level (gameInput), restarts after a game over and reads `game.ended`.

enum { POT, FENCE, BIRD };
// Chrome dino scaled: its T-rex is 47 px tall, here DINO px. Its jump (60 fps frames): take-off 10 px/frame, gravity
// 0.6 px/frame², no higher than 63 px; letting go after 30 px drops the speed to 5 px/frame (apex 51 px vs 84 px held).
// Cacti: small 17x35, large 25x50 (pot, fence here). Chrome's world is 13 T-rexes wide, this one 6: time runs SLOW times
// slower so a jump still spans the same number of obstacle widths at this game's speed.
const float DINO = 14, SLOW = 2.0f, K = DINO / 47;
const float GJ = 0.6f * 3600 * K / (SLOW * SLOW);                 // gravity px/s²
const float JUMP_V = 10 * 60 * K / SLOW, DROP_V = 5 * 60 * K / SLOW;  // take-off / cut-off speed px/s
const float MIN_H = 30 * K, MAX_H = 63 * K;                          // let go from MIN_H up: cut; at MAX_H: cut anyway
const float SPEED0 = 40, SPEED_UP = 5, SPEED_MAX = 90;  // px/s; +SPEED_UP every 10 points
const int POT_H = 10, FENCE_H = 15;                                  // 35 K, 50 K: the cacti
const float OB_W[] = {5, 7, 7}, OB_H[] = {POT_H, FENCE_H, 0};  // hitbox width (17 K, 25 K); height of the ground obstacles

// art: hitbox + 1 px each side, OB_H rows tall
const char *const POT_ART[POT_H] = {"..g.g..", ".gGgGg.", "..gGg..", "...G...", "KKKKKKK",
                                    "KCCCCCK", "KCCCCCK", ".KCCCK.", ".KCCCK.", ".KKKKK."};
const char *const FENCE_ART[FENCE_H] = {".K..K..K.", "KWKKWKKWK", "KWKKWKKWK", "KWKKWKKWK", "KKKKKKKKK",
                                        "KWWWWWWWK", "KKKKKKKKK", "KWKKWKKWK", "KWKKWKKWK", "KWKKWKKWK",
                                        "KWKKWKKWK", "KKKKKKKKK", "KWWWWWWWK", "KKKKKKKKK", "KWKKWKKWK"};
const char *const BIRD_ART[2][5] = {{"...KK..", "..KYYK.", ".KYYYYK", "CKKYYYK", ".KKKKK."},   // wings up
                                    {".......", ".KKKKK.", "CKKYYYK", ".KYYYYK", "...KYK."}};  // wings down

using pet::Game;
using pet::Ob;
Game &game = pet::runner;

float nextX(float after) { return after + game.speed * (1.1f + 1.2f * rnd()) + 10; }
uint8_t pickOb() {
  float r = rnd();
  if (game.score >= 20 && r < 0.25f) return BIRD;
  return game.score >= 3 && r < 0.6f ? FENCE : POT;
}

void gameStart() {
  game = Game();
  game.on = true, game.speed = SPEED0, game.air = -1;
  hero.x = 20, hero.dir = 1, hero.happy = 0;
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
  if (o.kind == BIRD) return o.x < hero.x + HEAD_X1 && o.x + OB_W[BIRD] > hero.x + HEAD_X0 && GROUND - hero.y > 1;  // any jump
  return o.x < hero.x + 4 && o.x + OB_W[o.kind] > hero.x - 2 && GROUND - hero.y < OB_H[o.kind];
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
    float h = GROUND - hero.y;
    if ((!game.held || h > MAX_H) && h > MIN_H && hero.vy < -DROP_V) hero.vy = -DROP_V;  // let go / top: the rise ends
    hero.vy += GJ * dt, hero.y += hero.vy * dt, hero.vx = game.speed;  // vx only tilts the jump pose
    if (hero.y >= GROUND) hero.y = GROUND, hero.vy = 0, game.air = -1, go(RUN, 0);
  }
  float far = 0;
  for (Ob &o : game.ob) {
    o.x -= dx, far = fmaxf(far, o.x);
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
    if (o.x < -10) o = {nextX(far), pickOb(), false}, far = o.x;
}

void gameDraw() {
  drawSky();
  int off = (int)game.dist % N;  // the ground scrolls with the world
  for (int y = SCROLL_Y; y < N; y++)
    for (int x = 0; x < N; x++) fb[y * N + x] = bg[y * N + (x + off) % N];
  for (const Ob &o : game.ob) {
    int x = lroundf(o.x);
    if (o.kind == POT) art(POT_ART, POT_H, x - 1, GROUND - POT_H);
    else if (o.kind == FENCE) art(FENCE_ART, FENCE_H, x - 1, GROUND - FENCE_H);
    else art(BIRD_ART[sinf(T * 14) > 0], 5, x, BIRD_TOP);
  }
  drawHero();
  memmove(fb + GAME_CAM * N, fb, (N - GAME_CAM) * N);  // view lowered: a high jump stays below the score bubble
  memcpy(fb, bg, GAME_CAM * N);
}

// Bubble text: score and high score while running, the result after a game over.
void gameLabel(char *out, int cap) {
  int best = pet::s.best > game.score ? pet::s.best : game.score;
  if (!game.dead) snprintf(out, cap, "%d HI %d", game.score, best);
  else if (game.newBest) snprintf(out, cap, "NEW BEST %d", game.score);
  else snprintf(out, cap, "%d BEST %d", game.score, best);
}
