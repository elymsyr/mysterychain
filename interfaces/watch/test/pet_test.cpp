// PC check for pet.h (stats) and game.h (runner physics):
//   g++ -std=c++17 -I../firmware pet_test.cpp -o /tmp/pet_test && /tmp/pet_test
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include "cat.h"

static bool near(float a, float b) { return fabsf(a - b) < 0.01f; }

// Runs one game obstacle at a fixed speed; the button is held for `hold` s at the best moment. True = cleared.
static bool clears(int kind, float speed, float hold) {
  using namespace kitty;
  for (float lead = -5; lead < 60; lead += 0.5f) {  // press when the obstacle is `lead` px ahead: try them all, as a player would
    gameStart();
    game.speed = speed, game.fixedSpeed = true;
    game.ob[0] = {cat.x + 70, (uint8_t)kind, false};
    game.ob[1].x = game.ob[2].x = 1e9f;
    float t = 0, press = -1;
    while (!game.ob[0].passed && !game.dead) {  // passed = fully behind the cat
      if (press < 0 && game.ob[0].x - cat.x <= lead) press = t;
      gameInput(press >= 0 && t - press < hold);
      gameStep(1 / 60.f);
      t += 1 / 60.f;
    }
    bool ok = !game.dead;
    game.on = false;
    if (ok) return true;
  }
  return false;
}

// Bird ahead of the head: standing passes under it, a jump into it ends the game.
static bool birdHits(float speed, bool jump) {
  using namespace kitty;
  gameStart();
  game.speed = speed, game.fixedSpeed = true;
  game.ob[0] = {cat.x + 14, BIRD, false};
  game.ob[1].x = game.ob[2].x = 1e9f;
  for (int i = 0; i < 120 && !game.dead; i++) gameInput(jump && i < 6), gameStep(1 / 60.f);
  bool d = game.dead;
  game.on = false;
  return d;
}

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

  s = Saved();
  addSteps(250, 10);
  assert(s.steps == 250 && s.stepsDay == 10 && near(s.happy, 82));
  addSteps(60, 0);  // clock unknown: keeps counting
  assert(s.steps == 310 && near(s.happy, 83));
  addSteps(5, 11);  // new day
  assert(s.steps == 5 && s.stepsDay == 11);

  s = Saved();
  assert(canPlay() && gameOver(30) && s.best == 30 && near(s.energy, 70) && near(s.happy, 90));
  assert(!gameOver(12) && s.best == 30);
  s.energy = 14;
  assert(!canPlay());

  // Runner: a tap clears a pot, not a fence; a held jump clears a fence; any jump hits a bird.
  s = Saved();
  using kitty::POT;
  using kitty::FENCE;
  for (float v : {40.f, 65.f, 90.f}) {
    assert(clears(POT, v, 0.1f));
    assert(!clears(FENCE, v, 0.1f));
    assert(clears(FENCE, v, 0.4f));
    assert(!birdHits(v, false) && birdHits(v, true));
  }
  puts("pet_test OK");
}
