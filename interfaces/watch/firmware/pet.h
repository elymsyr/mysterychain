// Pet stats (hunger, happiness, energy), mood, feeding and the saved state. Plain C++: also compiles on a PC
// (interfaces/watch/test/pet_test.cpp). The firmware stores `s` in flash; the cat scene reads and changes it.
#pragma once
#include <math.h>
#include <stdint.h>

namespace pet {

// Tunables: hours for a stat to go 100 -> 0 (sleep: 0 -> 100), and the gains.
const float FULL_H = 10, HAPPY_H = 16, ENERGY_H = 16, SLEEP_H = 8;
const float FEED = 35, FULL_REFUSE = 90, MSG_JOY = 8, PET_JOY = 5, MSG_COOL_S = 60, STEP_JOY = 1, GAME_COST = 10, GAME_MIN = 15;
const uint32_t MAX_OFFLINE_S = 48 * 3600;  // time off counts as sleep, up to this much
const uint8_t VERSION = 1;                 // file layout version: a mismatch = start fresh

struct Saved {
  uint8_t version = VERSION;
  float full = 80, happy = 80, energy = 80;  // 0..100, higher is better
  uint32_t steps = 0, stepsDay = 0;          // today's steps, and the local day number they belong to
  uint16_t best = 0;                         // game high score
  uint32_t savedUnix = 0;                    // local time of the save in s, 0 = unknown
  uint8_t vib = 1, nightFrom = 23, nightTo = 7;  // motor on, night window (sleep + quiet), from == to: none
  float shakeG = 0.5f;
  char name[13] = "Kitty";
};
Saved s;

enum Mood : uint8_t { HAPPY, TIRED, SAD, HUNGRY, SLEEP };
const char *const MOOD[] = {"happy", "tired", "sad", "hungry", "sleep"};

float msgCool = 0, stepAcc = 0;  // message joy cooldown (s), steps not yet turned into joy

float clamp100(float v) { return fminf(fmaxf(v, 0), 100); }
void joy(float v) { s.happy = clamp100(s.happy + v); }

bool night(int hour) {  // inside the night window; hour -1 = clock unknown = never
  if (hour < 0 || s.nightFrom == s.nightTo) return false;
  if (s.nightFrom < s.nightTo) return hour >= s.nightFrom && hour < s.nightTo;
  return hour >= s.nightFrom || hour < s.nightTo;
}

Mood mood(int hour) {  // first that holds wins
  if (night(hour) || s.energy < 10) return SLEEP;
  if (s.full < 25) return HUNGRY;
  if (s.happy < 25) return SAD;
  if (s.energy < 25) return TIRED;
  return HAPPY;
}

void tick(float sec, bool asleep) {
  float h = sec / 3600;
  if (!asleep) s.happy = clamp100(s.happy - 100 / HAPPY_H * h * (s.full < 20 ? 2 : 1));  // computed before full drops
  s.full = clamp100(s.full - 100 / FULL_H * h * (asleep ? 0.5f : 1));
  s.energy = clamp100(s.energy + (asleep ? 100 / SLEEP_H : -100 / ENERGY_H) * h);
  msgCool -= sec;
}

void offline(uint32_t sec) { tick(sec < MAX_OFFLINE_S ? sec : MAX_OFFLINE_S, true); }

bool feed() {  // false: full, refused
  if (s.full > FULL_REFUSE) return false;
  s.full = clamp100(s.full + FEED);
  return true;
}

void message() {  // a message or reaction from the phone
  if (msgCool > 0) return;
  joy(MSG_JOY), msgCool = MSG_COOL_S;
}

void addSteps(uint32_t n, uint32_t day) {  // day: local day number, 0 = clock unknown (no reset)
  if (day && day != s.stepsDay) s.steps = 0, s.stepsDay = day, stepAcc = 0;
  s.steps += n, stepAcc += n;
  for (; stepAcc >= 100; stepAcc -= 100) joy(STEP_JOY);
}

bool canPlay() { return s.energy >= GAME_MIN; }

bool gameOver(int score) {  // true: new high score
  s.energy = clamp100(s.energy - GAME_COST);
  joy(fminf(15, score / 3));
  if (score <= s.best) return false;
  s.best = score;
  return true;
}

}  // namespace pet
