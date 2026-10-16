// All three pets in one firmware, the look picked from the phone (#set scene cat|pavi|nausicaa, saved in /pet):
// firmware.ino talks to this `scene` as it would to a single scene header. Stats and high score are shared; each
// character keeps its own name (pet::s.names).
#pragma once
#define SCENE_PICK
#include "cat.h"
#include "pavi.h"
#include "nausicaa.h"

namespace scene {

const int N = 80;
static_assert(kitty::N == N && pavi::N == N && nausicaa::N == N, "all scenes draw the same buffer size");
const char *const NAMES[] = {"cat", "pavi", "nausicaa"};
const int COUNT = 3;
static_assert(COUNT <= pet::LOOKS, "a saved name per look");

uint8_t cur = 0;  // 0 cat, 1 Pavi, 2 Nausicaa
// The current scene's buffer, palette and the bubble colors (pick() rebinds them).
uint8_t *fb = kitty::fb;
uint8_t (*PAL)[3] = kitty::PAL;
uint8_t NCOL = kitty::NCOL, INK = kitty::INK, CREAM = kitty::CREAM;
pet::Game &game = pet::runner;

inline void px(int x, int y, uint8_t c) { cur == 2 ? nausicaa::px(x, y, c) : cur ? pavi::px(x, y, c) : kitty::px(x, y, c); }
void init() { kitty::init(), pavi::init(), nausicaa::init(); }
void step(float dt) { cur == 2 ? nausicaa::step(dt) : cur ? pavi::step(dt) : kitty::step(dt); }
void shake() { cur == 2 ? nausicaa::shake() : cur ? pavi::shake() : kitty::shake(); }
void cheer() { cur == 2 ? nausicaa::cheer() : cur ? pavi::cheer() : kitty::cheer(); }
void tint(int m) { cur == 2 ? nausicaa::tint(m) : cur ? pavi::tint(m) : kitty::tint(m); }
void setMood(pet::Mood m) { kitty::setMood(m), pavi::setMood(m), nausicaa::setMood(m); }
void eat() { cur == 2 ? nausicaa::eat() : cur ? pavi::eat() : kitty::eat(); }
void refuse() { cur == 2 ? nausicaa::refuse() : cur ? pavi::refuse() : kitty::refuse(); }
void purr() { cur == 2 ? nausicaa::purr() : cur ? pavi::purr() : kitty::purr(); }
void yawn() { cur == 2 ? nausicaa::yawn() : cur ? pavi::yawn() : kitty::yawn(); }
void music(bool on) { cur == 2 ? nausicaa::music(on) : cur ? pavi::music(on) : kitty::music(on); }
void gameStart() { cur == 2 ? nausicaa::gameStart() : cur ? pavi::gameStart() : kitty::gameStart(); }
void gameInput(bool held) { cur == 2 ? nausicaa::gameInput(held) : cur ? pavi::gameInput(held) : kitty::gameInput(held); }
void gameStop() { cur == 2 ? nausicaa::gameStop() : cur ? pavi::gameStop() : kitty::gameStop(); }
void gameTilt(float g) { cur == 2 ? nausicaa::gameTilt(g) : cur ? pavi::gameTilt(g) : kitty::gameTilt(g); }
void gameLabel(char *out, int cap) { kitty::gameLabel(out, cap); }  // reads only the shared runner state

int find(const char *name) {  // index of a scene name, -1 = none
  for (int i = 0; i < COUNT; i++)
    if (!strcmp(name, NAMES[i])) return i;
  return -1;
}

// Switch the look. A running game ends; the firmware then retints and rebuilds its palette.
void pick(int i) {
  if (i < 0 || i >= COUNT) return;
  if (i != cur && game.on) gameStop();
  if (i != cur) music(false);
  cur = i;
  if (cur == 2) fb = nausicaa::fb, PAL = nausicaa::PAL, NCOL = nausicaa::NCOL, INK = nausicaa::INK, CREAM = nausicaa::CREAM;
  else if (cur) fb = pavi::fb, PAL = pavi::PAL, NCOL = pavi::NCOL, INK = pavi::INK, CREAM = pavi::CREAM;
  else fb = kitty::fb, PAL = kitty::PAL, NCOL = kitty::NCOL, INK = kitty::INK, CREAM = kitty::CREAM;
}

}  // namespace scene
