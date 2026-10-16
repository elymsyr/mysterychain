// Pet stats (hunger, happiness, energy), mood, feeding, the saved state and the time-of-day keyframes. Plain C++: also
// compiles on a PC (interfaces/watch/test/pet_test.cpp). The firmware stores `s` in flash; the pet scenes (cat.h, pavi.h)
// read and change it.
#pragma once
#include <math.h>
#include <stdint.h>
#ifndef PET_NAME
#define PET_NAME "Cat"  // the scene sets its own before including this
#endif

namespace pet {

// Tunables: hours for a stat to go 100 -> 0 (sleep: 0 -> 100), and the gains.
const float FULL_H = 7, HAPPY_H = 16, ENERGY_H = 16, SLEEP_H = 8;
const float FEED = 35, FULL_REFUSE = 90, MSG_JOY = 8, PET_JOY = 5, MSG_COOL_S = 60, STEP_JOY = 1, GAME_COST = 10, GAME_MIN = 0;
const uint32_t MAX_OFFLINE_S = 48 * 3600;  // time off counts as sleep, up to this much
const uint8_t VERSION = 4;                 // file layout version: a mismatch = start fresh

const int LOOKS = 3;

struct Saved {
  uint8_t version = VERSION;
  float full = 80, happy = 80, energy = 80;  // 0..100, higher is better
  uint32_t steps = 0, stepsDay = 0;          // today's steps, and the local day number they belong to
  uint16_t best[LOOKS] = {};                 // game high score per look (each has its own game)
  uint32_t savedUnix = 0;                    // local time of the save in s, 0 = unknown
  uint8_t vib = 1, nightFrom = 23, nightTo = 7;  // motor on, night window (sleep + quiet), from == to: none
  float shakeG = 0.5f;
  char names[LOOKS][13] = {PET_NAME, "Pavi", "Nausicaa"};  // one per look (pets.h order); a single scene uses [0]
  uint8_t scene = 0;  // pets.h: which look the pet has (0 cat, 1 Pavi, 2 Nausicaa)
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

// Runner state (game.h). One for all pet scenes: only one is on screen, and the pet is the same.
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
} runner;

// Time of day (K6): each scene keeps a palette look per keyframe; between keyframes the looks blend linearly.
enum Look : uint8_t { NIGHT, DAWN, DAY, DUSK };
const struct { int16_t min; uint8_t look; } KEY[] = {  // minute of day, look
    {0, NIGHT}, {330, NIGHT}, {390, DAWN}, {450, DAWN}, {510, DAY}, {1050, DAY},
    {1110, DUSK}, {1170, DUSK}, {1230, NIGHT}, {1440, NIGHT}};
const int SUNRISE = 360, SUNSET = 1140;  // the sun is up 06:00-19:00, the moon the rest

float look(int minuteOfDay, int &a, int &b) {  // looks around this minute, returns the weight of b; -1 = clock unknown = night
  a = b = NIGHT;
  if (minuteOfDay < 0) return 0;
  int m = minuteOfDay % 1440, i = 0;
  while (KEY[i + 1].min <= m) i++;
  a = KEY[i].look, b = KEY[i + 1].look;
  return float(m - KEY[i].min) / (KEY[i + 1].min - KEY[i].min);
}

uint16_t &best() { return s.best[s.scene]; }  // the current look's high score

bool canPlay() { return s.energy >= GAME_MIN; }

bool gameOver(int score) {  // true: new high score
  s.energy = clamp100(s.energy - GAME_COST);
  joy(fminf(15, score / 3));
  if (score <= best()) return false;
  best() = score;
  return true;
}

}  // namespace pet
