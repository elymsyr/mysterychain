// Runner mini game (K7), Chrome dino style. Included from cat.h inside namespace kitty: uses the cat's drawing.
// The cat runs at a fixed x, the world scrolls left. Press = jump; keep holding = floats higher (tall fences).
// The firmware feeds the button level (gameInput), restarts after a game over and reads `game.ended`.

enum { POT, FENCE, BIRD };
const float GJ = 200, JUMP_V = 65;              // gravity px/s², take-off speed px/s
const float HOLD_FROM = 0.10f, HOLD_TO = 0.35f;  // while held in this window after take-off: no gravity (higher jump)
const float SPEED0 = 40, SPEED_UP = 5, SPEED_MAX = 90;  // px/s; +SPEED_UP every 10 points
const float OB_W[] = {4, 5, 7}, OB_H[] = {6, 15, 0};  // hitbox width; height of the ground obstacles
const int BIRD_TOP = GROUND - 29;                       // bird rows: just above the running cat's head
const int CAM = 8;                                      // the game is drawn this many rows lower than the scene

const char *const POT_ART[] = {".g..g.", "..gG..", "KKKKKK", "KCCCCK", ".KCCK.", ".KKKK."};
const char *const FENCE_ART[] = {".K.K.K.", "KWKWKWK", "KWKWKWK", "KKKKKKK", "KWWWWWK", "KKKKKKK", "KWKWKWK", "KWKWKWK",
                                 "KWKWKWK", "KKKKKKK", "KWWWWWK", "KKKKKKK", "KWKWKWK", "KWKWKWK", "KWKWKWK"};
const char *const BIRD_ART[2][5] = {{"...KK..", "..KYYK.", ".KYYYYK", "CKKYYYK", ".KKKKK."},   // wings up
                                    {".......", ".KKKKK.", "CKKYYYK", ".KYYYYK", "...KYK."}};  // wings down

struct Ob {
  float x;  // left edge
  uint8_t kind;
  bool passed;
};
struct Game {
  bool on, dead, ended, newBest, held, fixedSpeed;  // ended: game over event for the firmware (it clears it)
  int score;
  float t, speed, dist, air;  // t: since start or game over; air: since take-off, < 0 on the ground
  Ob ob[3];
} game;

float nextX(float after) { return after + game.speed * (1.1f + 1.2f * rnd()) + 10; }
uint8_t pickOb() {
  float r = rnd();
  if (game.score >= 20 && r < 0.25f) return BIRD;
  return game.score >= 3 && r < 0.6f ? FENCE : POT;
}

void gameStart() {
  game = Game();
  game.on = true, game.speed = SPEED0, game.air = -1;
  cat.x = 20, cat.dir = 1, cat.happy = 0;
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
  if (press && !game.dead && game.air < 0) game.air = 0, cat.vy = -JUMP_V, go(JUMP, 0);
}

bool hits(const Ob &o) {
  if (o.kind == BIRD) return o.x < cat.x + 12 && o.x + OB_W[BIRD] > cat.x + 1 && GROUND - cat.y > 1;  // any jump
  return o.x < cat.x + 4 && o.x + OB_W[o.kind] > cat.x - 2 && GROUND - cat.y < OB_H[o.kind];
}

void gameStep(float dt) {
  T += dt, game.t += dt;
  if (game.dead) {  // tumble down, wait for a press (firmware) or give up after 10 s
    if (cat.y < GROUND || cat.vy < 0) {
      cat.vy += GJ * dt, cat.y += cat.vy * dt;
      if (cat.y >= GROUND) cat.y = GROUND, cat.vy = 0;
    }
    if (game.t > 10) game.on = false, pose(IDLE, 1);
    return;
  }
  if (!game.fixedSpeed) game.speed = fminf(SPEED0 + SPEED_UP * (game.score / 10), SPEED_MAX);
  float dx = game.speed * dt;
  game.dist += dx;
  cat.phase += 15 * dt * game.speed / SPEED0;
  if (game.air >= 0) {
    game.air += dt;
    bool hold = game.held && game.air > HOLD_FROM && game.air < HOLD_TO;
    cat.vy += (hold ? 0 : GJ) * dt, cat.y += cat.vy * dt, cat.vx = game.speed;  // vx only tilts the jump pose
    if (cat.y >= GROUND) cat.y = GROUND, cat.vy = 0, game.air = -1, go(RUN, 0);
  }
  float far = 0;
  for (Ob &o : game.ob) {
    o.x -= dx, far = fmaxf(far, o.x);
    if (!o.passed && o.x + OB_W[o.kind] < cat.x - 2) o.passed = true, game.score++;
    if (hits(o)) {
      game.dead = true, game.t = 0, game.air = -1;
      cat.vy = -50;
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
  for (int y = GROUND - 1; y < N; y++)
    for (int x = 0; x < N; x++) fb[y * N + x] = bg[y * N + (x + off) % N];
  for (const Ob &o : game.ob) {
    int x = lroundf(o.x);
    if (o.kind == POT) art(POT_ART, 6, x - 1, GROUND - 6);
    else if (o.kind == FENCE) art(FENCE_ART, 15, x - 1, GROUND - 15);
    else art(BIRD_ART[sinf(T * 14) > 0], 5, x, BIRD_TOP);
  }
  drawCat();
  memmove(fb + CAM * N, fb, (N - CAM) * N);  // view lowered: a high jump stays below the score bubble
  memcpy(fb, bg, CAM * N);
}

// Bubble text: score and high score while running, the result after a game over.
void gameLabel(char *out, int cap) {
  int best = pet::s.best > game.score ? pet::s.best : game.score;
  if (!game.dead) snprintf(out, cap, "%d HI %d", game.score, best);
  else if (game.newBest) snprintf(out, cap, "NEW BEST %d", game.score);
  else snprintf(out, cap, "%d BEST %d", game.score, best);
}
