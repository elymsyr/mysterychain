// PC check for pet.h (stats) and game.h (runner physics), with the cat (default), Pavi (-DPAVI) or Nausicaa (-DNAUSICAA) as the runner;
// -DPETS: pets.h (all three, the cat on screen) and its look switch:
//   g++ -std=c++17 -I../firmware pet_test.cpp -o /tmp/pet_test && /tmp/pet_test
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <utility>
#if defined(PETS)
#include "pets.h"
#elif defined(PAVI)
#include "pavi.h"
#elif defined(NAUSICAA)
#include "nausicaa.h"
#else
#include "cat.h"
#endif
#ifdef PETS
namespace run = kitty;  // the runner checks need a hero: the cat's
#else
namespace run = scene;
#endif

static bool near(float a, float b) { return fabsf(a - b) < 0.01f; }

#ifdef PAVI
// Pavi's game: one boulder in the middle lane, the tilt `g` held all along. True = she ran into it.
static bool tiltHits(float g, float speed, float dt) {
  using namespace run;
  gameStart();
  game.speed = speed, game.fixedSpeed = true;
  obs[0] = {dist + ZP + 6, 0, BOULDER};
  obs[1].w = obs[2].w = 1e9f;
  for (int i = 0; i < 1000 && !game.dead && !game.score; i++) gameTilt(g), gameStep(dt);
  assert(game.dead || game.score == 1);
  bool d = game.dead;
  game.on = false;
  return d;
}
#elif defined(NAUSICAA)
// Nausicaa's game with no planes coming on their own: one plane of `kind` at x, the tilt `g` and the trigger held all
// along, until the plane is gone (shot down, crashed, flown past) or `secs`. True = she was shot down / rammed.
static bool fight(int kind, float x, float g, bool fire, float secs) {
  using namespace run;
  gameStart();
  spawnT = 1e9f;
  planes[0] = {x, 0, x, 0, 0, 0.5f, (int8_t)PLANE[kind].hp, (uint8_t)kind, true};
  for (float t = 0; t < secs && !game.dead && planes[0].on; t += 1 / 60.f) gameTilt(g), gameInput(fire), gameStep(1 / 60.f);
  bool d = game.dead;
  game.on = false;
  return d;
}
#else
// Runs one game obstacle at a fixed speed; the button is held for `hold` s at the best moment. True = cleared.
static bool clears(int kind, float speed, float hold) {
  using namespace run;
  for (float lead = -5; lead < 60; lead += 0.5f) {  // press when the obstacle is `lead` px ahead: try them all, as a player would
    gameStart();
    game.speed = speed, game.fixedSpeed = true;
    game.ob[0] = {hero.x + 70, (uint8_t)kind, false};
    for (int i = 1; i < 6; i++) game.ob[i].x = 1e9f;
    float t = 0, press = -1, passed = -1;
    while (!game.dead && (passed < 0 || t - passed < 0.4f)) {  // a while past the pot: its bird is behind it
      if (press < 0 && game.ob[0].x - hero.x <= lead) press = t;
      gameInput(press >= 0 && t - press < hold);
      gameStep(1 / 60.f);
      t += 1 / 60.f;
      if (passed < 0 && game.score) passed = t;
    }
    bool ok = !game.dead;
    game.on = false;
    if (ok) return true;
  }
  return false;
}

// Two obstacles at the closest gap at full difficulty (gapX, no random extra), the second `kind`: a jump fit for each
// (tap / held), the second pressed at every possible moment. Returns how long the window to press it is, in s.
static float pairWindow(int first, int kind, float speed) {
  using namespace run;
  float best = 0;
  for (float lead1 = -5; lead1 < 60; lead1 += 1) {
    int ok = 0;
    for (float lead2 = -5; lead2 < 80; lead2 += 0.5f) {
      gameStart();
      game.speed = speed, game.fixedSpeed = true, game.t = 1e3f;  // full difficulty: the tightest gaps
      game.ob[0] = {hero.x + 70, (uint8_t)first, false};
      game.ob[1] = {gapX(game.ob[0], (uint8_t)kind, 0), (uint8_t)kind, false};
      for (int i = 2; i < 6; i++) game.ob[i].x = 1e9f;
      float h1 = first == FENCE ? 0.4f : 0.1f, h2 = kind == FENCE ? 0.4f : 0.1f, t = 0, p1 = -1, p2 = -1, passed = -1;
      while (!game.dead && (passed < 0 || t - passed < 0.4f)) {
        if (p1 < 0 && game.ob[0].x - hero.x <= lead1) p1 = t;
        if (p2 < 0 && game.score && game.ob[1].x - hero.x <= lead2) p2 = t;
        gameInput((p1 >= 0 && t - p1 < h1) || (p2 >= 0 && t - p2 < h2));
        gameStep(1 / 60.f);
        t += 1 / 60.f;
        if (passed < 0 && game.score == 2) passed = t;
      }
      ok += !game.dead;
      game.on = false;
    }
    best = fmaxf(best, ok * 0.5f / speed);
  }
  return best;
}
#endif

int main() {
  using namespace pet;
  s = Saved();
  tick(3600, false);  // 1 h awake
  assert(near(s.full, 70) && near(s.happy, 80 - 6.25f) && near(s.energy, 80 - 6.25f));
  s = Saved();
  tick(3600, true);  // 1 h asleep: half hunger, no sadness, energy back
  assert(near(s.full, 75) && near(s.happy, 80) && near(s.energy, 92.5f));
  s = Saved();
  s.full = 10;
  tick(3600, false);  // starving: happiness drops twice as fast
  assert(near(s.full, 0) && near(s.happy, 80 - 12.5f));
  s = Saved();
  tick(40 * 3600, false);  // clamped
  assert(s.full == 0 && s.happy == 0 && s.energy == 0);
  s = Saved(), s.energy = 0;
  offline(100 * 3600);  // capped at 48 h, sleep rules
  assert(s.energy == 100 && near(s.full, 0) && s.happy == 80);

  s = Saved(), s.full = 91;
  assert(!feed() && s.full == 91);
  s.full = 80;
  assert(feed() && s.full == 100);

  s = Saved();
  message(), message();  // second one inside the cooldown
  assert(near(s.happy, 88));
  tick(61, false), message();
  assert(s.happy > 95);

  s = Saved();
  assert(night(23) && night(3) && !night(7) && !night(12) && !night(-1));
  s.nightFrom = s.nightTo = 5;
  assert(!night(5));
  s.nightFrom = 1, s.nightTo = 6;
  assert(night(1) && night(5) && !night(6) && !night(0));
  s = Saved();
  assert(mood(12) == HAPPY && mood(0) == SLEEP);
  s.energy = 5;
  assert(mood(12) == SLEEP);
  s.energy = 20, s.happy = 20, s.full = 20;
  assert(mood(12) == HUNGRY);
  s.full = 50;
  assert(mood(12) == SAD);
  s.happy = 50;
  assert(mood(12) == TIRED);
  s = Saved(), batLow = true;
  assert(mood(12) == TIRED && mood(0) == SLEEP);
  batLow = false;

  s = Saved();
  addSteps(250, 10);
  assert(s.steps == 250 && s.stepsDay == 10 && near(s.happy, 82));
  addSteps(60, 0);  // clock unknown: keeps counting
  assert(s.steps == 310 && near(s.happy, 83));
  addSteps(5, 11);  // new day
  assert(s.steps == 5 && s.stepsDay == 11);

  s = Saved();
  assert(canPlay() && gameOver(30) && best() == 30 && near(s.energy, 70) && near(s.happy, 90));
  assert(!gameOver(12) && best() == 30);
  s.energy = 14;
  assert(!canPlay());

#ifdef PAVI
  // Pavi: level = middle lane = into the boulder, a tilt past TILT_IN = a side lane; 20 Hz frames at top speed don't
  // jump over it. The lane holds until the tilt drops under TILT_OUT.
  s = Saved();
  for (float v : {4.f, 12.f})
    for (float dt : {1 / 60.f, 0.05f}) {
      assert(tiltHits(0, v, dt) && tiltHits(0.12f, v, dt));
      assert(!tiltHits(0.25f, v, dt) && !tiltHits(-0.25f, v, dt));
    }
  run::gameStart();
  for (auto [g, l] : {std::pair{0.12f, 0}, {0.2f, 1}, {0.12f, 1}, {0.05f, 0}, {-0.2f, -1}, {-0.12f, -1}, {0.5f, 1}})
    run::gameTilt(g), assert(run::lane == l);
  run::game.on = false;
#elif defined(NAUSICAA)
  // Nausicaa: the tilt sets where she flies; held fire jams the gun in ~2 s, it fires again only once fully cooled;
  // a gunship coming straight at her is shot down, unshot it rams her, a step aside it misses; a corvette's aimed
  // shots get her if she stays put.
  s = Saved();
  {
    using namespace run;
    gameStart();
    spawnT = 1e9f;
    for (auto [g, x] : {std::pair{0.f, 40.f}, {0.5f, 40 + REACH_X}, {-2.f, 40 - REACH_X}, {0.125f, 40 + REACH_X / 2}}) {
      for (int i = 0; i < 90; i++) gameTilt(g), gameStep(1 / 60.f);
      assert(fabsf(mx - x) < 0.1f);  // there within 1.5 s (eased glide)
    }
    float t = 0;
    for (; !jam && t < 5; t += 1 / 60.f) gameInput(true), gameStep(1 / 60.f);
    assert(jam && t > 1.5f && t < 3);
    int fired = 0;
    for (; jam; t += 1 / 60.f) {
      gameInput(true), gameStep(1 / 60.f);
      for (auto &s : shots) fired += jam && s.on && s.y > PY - 8;
    }
    assert(!fired && t > 3 && t < 5);  // jammed for ~1.4 s, then it fires again
    for (int i = 0; i < 30; i++) gameInput(false), gameStep(1 / 60.f);
    gameInput(true), gameStep(1 / 60.f);  // a press fires right away
    for (auto &s : shots) fired += s.on && s.y > PY - 8;
    assert(fired == 1);
    game.on = false;
  }
  using run::GUNSHIP, run::CORVETTE, run::CROW, run::game;
  assert(!fight(GUNSHIP, 40, 0, true, 4) && game.score == 1);
  assert(fight(GUNSHIP, 40, 0, false, 4));
  assert(!fight(GUNSHIP, 40, 0.5f, false, 4) && game.score == 0);
  assert(fight(CORVETTE, 40, 0, false, 8));
  assert(!fight(CROW, 40, 0.15f, true, 10) && game.score == 8);
#else
  // Runner: a tap clears a pot, not a fence; a held jump clears a fence; a pot under a bird needs a tap, a held
  // jump hits the bird; a pot right after a pot: tap-tap works.
  s = Saved();
  using run::POT;
  using run::FENCE;
  using run::BIRD;
  for (float v : {35.f, 60.f, 90.f}) {
    assert(clears(POT, v, 0.1f));
    assert(!clears(FENCE, v, 0.1f));
    assert(clears(FENCE, v, 0.4f));
    assert(clears(BIRD, v, 0.1f) && !clears(BIRD, v, 0.5f) && !clears(BIRD, v, 0));
    for (int a : {POT, FENCE, BIRD})  // any two in a row, closest gap, top difficulty: still a fair window to press
      for (int b : {POT, FENCE, BIRD}) assert(pairWindow(a, b, v) >= 0.15f);  // measured 0.20 s (35 px/s) .. 0.43 s (90 px/s)
  }
#endif
#ifdef SCENE_PICK  // pets.h: switching the look rebinds buffer, palette and bubble colors, and ends a running game
  scene::pick(1);
  assert(scene::fb == pavi::fb && scene::PAL == pavi::PAL && scene::INK == pavi::INK && scene::NCOL == pavi::NCOL);
  scene::gameStart();
  assert(scene::game.on);
  scene::pick(2);
  assert(!scene::game.on && scene::fb == nausicaa::fb && scene::INK == nausicaa::INK && scene::find("nausicaa") == 2);
  assert(!strcmp(s.names[0], "Cat") && !strcmp(s.names[2], "Nausicaa"));
  scene::pick(0);
  assert(!scene::game.on && scene::fb == kitty::fb && scene::CREAM == kitty::CREAM && scene::find("pavi") == 1 && scene::find("dog") < 0);
#endif
  // Travel: the music keeps a trip going and ends it; a trip on its own ends after its time; feeding ends one.
  {
    using namespace run;
    s = Saved();
    setMood(HAPPY);
    music(false), pose(IDLE, 1);
    music(true);
    for (int i = 0; i < 30 * 60; i++) step(1 / 60.f);
    assert(rove);
    music(false);
    assert(!rove);
    travel(2);
    for (int i = 0; i < 3 * 60; i++) step(1 / 60.f);
    assert(!rove);
    travel(10), eat();
    assert(!rove);
  }

  puts("pet_test OK (" PET_NAME ")");
}
