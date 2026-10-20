// MysteryChain watch firmware: an 80x80 scene scaled 3x and pushed to the 240x240 screen in strips over SPI DMA.
// The scene is picked by the #include below; shake the board and the character plays.
// BLE (Nordic UART, name "MysteryChain"): text from the phone shows in a speech bubble, the motor buzzes twice,
// the character cheers. Messages starting with '#' are commands, see receive(). Button and shake events go to the
// phone as "button N" / "shake N".
// Button tap: the character cheers; once the phone has sent the time, the clock shows in the bubble (pets: with the
// weather icon once the phone has sent the forecast).
// Button held 3 s: off. Hold ~1 s to turn on again; plugging in USB also turns it on.
// pets.h holds the five pets, the cat, Pavi, Nausicaa, Gandalf and Sauron; the phone picks the look (#set scene). The pets (SCENE_PET): stats saved in flash, double tap = feed, triple tap = next look, hold 1-3 s = the look's game,
// double tap on the case = petting, IMU step counter, settings from the app, sky follows the clock, and a music
// screen when the phone's track changes (the character travels, the title scrolls over the sky).
// No motion and no button for IDLE_OFF_MS: the screen goes dark (display cut by Q2); a shake or the button wakes it.
// Motion is tied to real time: speed stays the same if fps drops. Serial monitor prints fps once a second.
#include <Adafruit_TinyUSB.h>  // for Serial
#include <Adafruit_GC9A01A.h>
#include <LSM6DS3.h>
#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include "pets.h"  // the scene: pets.h (all five, picked from the phone), cat.h, pavi.h or nausicaa.h alone

const uint8_t TFT_CS = D1, TFT_DC = D2, TFT_RST = D3, MOTOR = D6, BTN = D0, DISP_PWR = D7;
const uint32_t ON_HOLD_MS = 1000, OFF_HOLD_MS = 3000;  // power button, see powerOff()
const uint32_t SPI_HZ = 32000000;          // if the image glitches: 16000000 (~17 fps)
const int SC = 240 / scene::N, BAND = 8;   // 3x scale; strip of 8 logical rows = 11.5 KB (any view size is stretched to 240)

const float SHAKE_G = 0.5;           // shake when |a| deviates from 1 g by this much (pet scene: set from the app)
const uint32_t SHAKE_GAP_MS = 400;   // don't count one shake several times
const uint32_t MOTOR_MASK_MS = 400;  // ignore shakes this long after the motor stops (use the value measured in T8)
const uint32_t CLOCK_MS = 3000;      // a button tap shows the time this long
const uint32_t FIND_MS = 60000;      // find stops by itself after this long: a keychain out of reach mustn't drain the battery
const float MOVE_G = 0.15;           // movement that keeps the screen on: |a| off 1 g by this much (check the at-rest noise)
const uint32_t IDLE_OFF_MS = 60000;  // screen off after this long without movement or button
const uint32_t CALL_MS = 45000;      // a call rings at most this long if "#call 0" never comes (link lost mid-call)
const uint32_t MUSIC_MS = 12000;     // pet scenes: the music screen stays at least this long after a track change
// Pet scene only:
const uint32_t GAME_HOLD_MS = 1000;  // hold the button this long and release: runner game (a buzz says "release now")
const uint32_t TAP_WAIT_MS = 300;    // a button tap waits this long for another one (double tap = feed, triple = next look)
const uint32_t SHAKE_WAIT_MS = 400;  // a shake is decided this late: a double tap on the case inside it is petting
const uint32_t TAP_GUARD_MS = 500;   // case taps this soon after a button press are the button's click, not petting
const uint32_t WX_MS = 4 * 3600000;  // a forecast (#weather) covers this long: no new one by then = weather unknown again
const uint8_t TAP_THS = 2;           // IMU tap threshold, x 0.5 g at the 16 g range: 3 missed half the taps through the printed case
const uint32_t SAVE_S = 600;         // stats saved to flash this often (and on feed, settings, new best, power off)
const int TILT_AXIS = 1;             // Pavi's and Nausicaa's games: accel axis along the screen's left-right (0 x, 1 y, 2 z; check on the device)
const float TILT_SIGN = -1;          // ... and its sign: tilted right must read + (-1 flips it)
const int TILT2_AXIS = 0;            // Sauron's maze also rolls up-down: the accel axis along the screen's top-bottom (check on the device)
const float TILT2_SIGN = -1;         // ... and its sign: bottom edge tilted down must read +

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);
LSM6DS3 imu(I2C_MODE, 0x6A);
BLEUart bleuart;
bool imuOk;
// Palette entries past the scene's own: scene::NCOL pink, for hearts on any scene; NCOL + 1..3 the weather icon's
// cloud grey, rain blue and sun yellow (pet::WX_ICON letters G, B, Y).
uint16_t pal[256], palInv[256], strip[240 * SC * BAND];  // palInv: find blinks with it

void buildPal() {  // the pet scenes' palettes follow the time of day and the weather: rebuilt once a minute and on a change
  static const uint8_t EXTRA[4][3] = {{255, 120, 150}, {150, 158, 172}, {60, 120, 230}, {250, 180, 30}};
  for (int i = 0; i < scene::NCOL + 4; i++) {
    const uint8_t *rgb = i < scene::NCOL ? scene::PAL[i] : EXTRA[i - scene::NCOL];
    uint16_t c = tft.color565(rgb[0], rgb[1], rgb[2]), ci = tft.color565(255 - rgb[0], 255 - rgb[1], 255 - rgb[2]);
    pal[i] = c << 8 | c >> 8;  // display wants big endian: swap once here, no swap while sending
    palInv[i] = ci << 8 | ci >> 8;
  }
}

// Route Adafruit_GFX text drawing into the scene buffer (by color index); clip window on x.
struct FbGfx : Adafruit_GFX {
  int16_t clipL = 0, clipR = scene::N;
  FbGfx() : Adafruit_GFX(scene::N, scene::N) {}
  void drawPixel(int16_t x, int16_t y, uint16_t c) override {
    if (x >= clipL && x < clipR) scene::px(x, y, c);
  }
} gfx;

// Clock: the phone sends its local time on every connect (#time). Lost on power-off until the next connect.
uint32_t syncLocal, syncMs;  // local time in s at millis() == syncMs; syncLocal 0 = no time yet
uint32_t localNow() { return syncLocal + (millis() - syncMs) / 1000; }
uint32_t wxT;  // pets: when the last #weather came (millis)
bool wxKnown;  // ... and whether it named one
int hourNow() { return syncLocal ? localNow() / 3600 % 24 : -1; }

char msg[64];
uint8_t msgColor;
int8_t msgIcon = -1;  // the clock's weather icon (pet::Wx), -1 = none
bool msgMusic;  // msg is a track title: pet scenes show it on the music screen
uint32_t msgT0, msgDur, buzzT0, buzzN, buzzOn = 120, buzzPer = 240;  // vibration: buzzN pulses, buzzOn ms on every buzzPer ms

void buzz(uint32_t pulses, uint32_t on = 120, uint32_t per = 240) {  // find has its own pattern and always buzzes
#ifdef SCENE_PET
  if (!pet::s.vib || pet::night(hourNow())) pulses = 0;  // vibration off, or quiet hours
#endif
  buzzT0 = millis(), buzzN = pulses, buzzOn = on, buzzPer = per;
}

void show(uint32_t pulses, uint8_t color = scene::INK, bool react = true) {  // show msg in the bubble, buzz, the character cheers
  msgT0 = millis();
  msgDur = constrain(2 * (strlen(msg) * 6 + 20) * 40, 4000, 15000);  // scrolling text loops at least twice
  msgColor = color, msgMusic = false, msgIcon = -1;
  buzz(pulses);
  if (react) scene::cheer();
  Serial.printf("screen: %s\n", msg);
}

void tell(const char *s) {  // to the serial monitor and, if connected, the phone
  Serial.print(s);
  if (Bluefruit.connected()) bleuart.write(s, strlen(s));
}

void showClock() {
  uint32_t t = localNow();
  snprintf(msg, sizeof msg, "%02lu:%02lu", t / 3600 % 24, t / 60 % 60);
  msgT0 = millis(), msgDur = CLOCK_MS, msgColor = scene::INK, msgMusic = false, msgIcon = -1;
#ifdef SCENE_PET
  if (wxKnown) msgIcon = pet::wx;
#endif
}

// Find: buzz-buzz-pause and a blinking screen until the button is pressed, #find 0 arrives or FIND_MS passes.
bool finding;
uint32_t findT0;

void find(bool on) {
  if (on) {
#ifdef SCENE_PET
    if (scene::game.on) scene::gameStop();
#endif
    finding = true, findT0 = millis();
    strcpy(msg, "FIND ME!");
    show(0);
    msgDur = FIND_MS;
  } else if (finding) {
    finding = false, msgDur = 0;
    buzz(0);  // restarts MOTOR_MASK_MS: the last find buzz mustn't count as a shake
  }
}

// UTF-8 -> ASCII: the font has no ş ğ ı; fold Turkish letters to ASCII, anything else (emoji etc.) becomes '?'.
void toAscii(const uint8_t *s, int n, char *out, int cap) {
  static const uint16_t CP[] = {0xE7, 0xC7, 0x11F, 0x11E, 0x131, 0x130, 0xF6, 0xD6, 0x15F, 0x15E, 0xFC, 0xDC};
  static const char AS[] = "cCgGiIoOsSuU";
  int o = 0;
  for (int i = 0; i < n && o < cap - 1; i++) {
    uint8_t b = s[i];
    if (b < 0x80) {
      if (b >= 0x20) out[o++] = b;
      continue;
    }
    int len = b >= 0xF0 ? 4 : b >= 0xE0 ? 3 : 2;
    char c = '?';
    if (len == 2 && i + 1 < n)
      for (int k = 0; k < 12; k++)
        if (CP[k] == (((b & 0x1F) << 6) | (s[i + 1] & 0x3F))) c = AS[k];
    out[o++] = c;
    i += len - 1;
  }
  out[o] = 0;
}

#ifdef SCENE_PET
// ---- Pet: flash storage, stats to the phone, feeding, settings, steps, game ----
using namespace Adafruit_LittleFS_Namespace;
bool petLoaded;     // /pet read (or defaults taken): saving is safe from here on
uint32_t bootSaved;  // savedUnix of the file read at boot: the time spent off is worked out from it
bool caughtUp;      // offline time applied (first #time after boot)

void petLoad() {
  InternalFS.begin();
  File f(InternalFS);
  pet::Saved t;
  if (f.open("/pet", FILE_O_READ)) {
    if (f.read(&t, sizeof t) == sizeof t && t.version == pet::VERSION) pet::s = t;  // else: defaults
    f.close();
  }
  for (auto &n : pet::s.names) n[sizeof n - 1] = 0;
  bootSaved = pet::s.savedUnix;
  petLoaded = true;
}

void petSave() {
  if (!petLoaded) return;  // power-off before the load (a tap that goes back off) must not write defaults
  // Clock unknown this session: save as if no time had passed while off, so the next sync still counts the gaps.
  pet::s.savedUnix = syncLocal ? localNow() : bootSaved ? bootSaved + millis() / 1000 : 0;
  File f(InternalFS);
  if (f.open("/pet", FILE_O_WRITE)) {
    f.seek(0);  // FILE_O_WRITE appends; the record has a fixed size
    f.write((const uint8_t *)&pet::s, sizeof pet::s);
    f.close();
  }
}

void sendStats(bool force) {  // force: on connect and #stats; otherwise only when steps or mood changed
  static char last[64];
  char line[64];  // the 0-100 stats stay on the board: they only pick the animations
  snprintf(line, sizeof line, "stats %lu %s\n", (unsigned long)pet::s.steps, pet::MOOD[pet::mood(hourNow())]);
  if (!force && !strcmp(line, last)) return;
  strcpy(last, line);
  tell(line);
}

void sendSettings() {
  char line[128];
#ifdef SCENE_PICK
  const char *look = scene::NAMES[scene::cur];
  const int looks = scene::COUNT;
#else
  const char *look = PET_NAME;
  const int looks = 1;
#endif
  int o = snprintf(line, sizeof line, "settings vib=%d shake=%.2f night=%d-%d scene=%s name=", pet::s.vib, pet::s.shakeG,
                   pet::s.nightFrom, pet::s.nightTo, look);
  for (int i = 0; i < looks; i++) o += snprintf(line + o, sizeof line - o, i ? "|%s" : "%s", pet::s.names[i]);  // per look
  snprintf(line + o, sizeof line - o, "\n");
  tell(line);
  o = snprintf(line, sizeof line, "best ");  // each look's game high score, same order
  for (int i = 0; i < looks; i++) o += snprintf(line + o, sizeof line - o, i ? "|%u" : "%u", pet::s.best[i]);
  snprintf(line + o, sizeof line - o, "\n");
  tell(line);
}

void retint();
#ifdef SCENE_PICK
void pickScene() {  // the saved look on screen: scene, sky colors, palette, mood, a cheer to say hello
  scene::pick(pet::s.scene);
  retint();
  scene::setMood(pet::mood(hourNow()));
  scene::cheer();
}
#endif

bool setting(const char *a) {  // "#set <a>"; false: not a setting, shows as text
  int x, y;
  if (sscanf(a, "vib %d", &x) == 1) pet::s.vib = x != 0;
  else if (!strncmp(a, "shake ", 6)) pet::s.shakeG = constrain(atof(a + 6), 0.3, 1.0);  // atof: no float scanf in newlib-nano
  else if (sscanf(a, "night %d %d", &x, &y) == 2 && x >= 0 && x < 24 && y >= 0 && y < 24) pet::s.nightFrom = x, pet::s.nightTo = y;
  else if (!strncmp(a, "name ", 5) && a[5]) {
    const char *v = a + 5;
    int i = 0;
#ifdef SCENE_PICK
    const char *sp = strchr(v, ' ');  // "name <look> <text>": each look has its own name
    char look[12] = {};
    if (sp && sp - v < (int)sizeof look) memcpy(look, v, sp - v);
    if ((i = scene::find(look)) < 0 || !sp[1]) return false;
    v = sp + 1;
#endif
    toAscii((const uint8_t *)v, strlen(v), pet::s.names[i], sizeof pet::s.names[i]);
    for (char *c = pet::s.names[i]; *c; c++)
      if (*c == '|') *c = '/';  // '|' separates the names in `settings`
  }
#ifdef SCENE_PICK
  else if (!strncmp(a, "scene ", 6) && scene::find(a + 6) >= 0) pet::s.scene = scene::find(a + 6), pickScene();
  else if (!strncmp(a, "best ", 5) && scene::find(a + 5) >= 0) pet::s.best[scene::find(a + 5)] = 0;  // reset one look's high score
#endif
  else if (!strcmp(a, "best all")) memset(pet::s.best, 0, sizeof pet::s.best);
  else return false;
  return true;
}

void feed() {  // double tap on the button or #feed
  if (pet::feed()) scene::eat(), tell("fed\n"), petSave();
  else scene::refuse(), tell("full\n");
  sendStats(true);
}

#ifdef SCENE_PICK
const uint8_t MAX_TAPS = 3;
#else
const uint8_t MAX_TAPS = 2;  // no look to switch: a double tap feeds at once
#endif
void tapAction(uint8_t n) {  // a run of short button presses, once it's over
  if (n == 1) {
    if (syncLocal) showClock();
    scene::cheer();
  } else if (n == 2) feed();
#ifdef SCENE_PICK
  else {  // triple tap: the next look, its name in the bubble
    pet::s.scene = (pet::s.scene + 1) % scene::COUNT;
    pickScene();
    snprintf(msg, sizeof msg, "%s", pet::s.names[pet::s.scene]);
    show(1, scene::INK, false);
    petSave();
    sendSettings();
  }
#endif
}

void retint() {  // sky colors for the current minute (night look without a clock) and the weather
  scene::tint(syncLocal ? localNow() / 60 % 1440 : -1);
  buildPal();
}

// Weather (A5): "#weather clear|cloudy|wind|rain|snow|storm", the phone's forecast for the next hours; anything else
// (#weather 0) = unknown, which looks clear but shows no icon. Not saved: after a power-off the phone sends it again.
void weather(const char *kind) {
  int k = 0, n = sizeof pet::WX / sizeof *pet::WX;
  while (k < n && strcmp(kind, pet::WX[k])) k++;
  wxKnown = k < n, wxT = millis(), pet::wx = wxKnown ? k : pet::CLEAR;
  retint();
}

void startGame() {  // hold-and-release, or a press after a game over
  if (scene::game.on) scene::gameStop();
  if (!pet::canPlay()) {
    strcpy(msg, "tired");
    show(0, scene::INK, false);
    scene::yawn();
    return;
  }
  scene::gameStart();
  msgDur = 0;  // the score takes the bubble
}

void readSteps() {  // the IMU's step counter, 16 bit, restarts when the IMU loses power (off)
  static int32_t last = -1;
  uint8_t lo, hi;
  if (!imuOk || imu.readRegister(&lo, LSM6DS3_ACC_GYRO_STEP_COUNTER_L) || imu.readRegister(&hi, LSM6DS3_ACC_GYRO_STEP_COUNTER_H)) return;
  uint16_t raw = hi << 8 | lo;
  if (last >= 0) pet::addSteps(uint16_t(raw - last), syncLocal ? localNow() / 86400 : 0);
  last = raw;
}

void imuPetSetup() {  // LSM6DS3TR-C embedded functions: pedometer and double tap
  uint8_t v = 0;
  imu.readRegister(&v, LSM6DS3_ACC_GYRO_CTRL10_C);
  imu.writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, v | 0x14);  // FUNC_EN + PEDO_EN
  imu.writeRegister(LSM6DS3_ACC_GYRO_TAP_CFG1, 0x8F);       // interrupts on, tap on x/y/z, latched until TAP_SRC is read
  imu.writeRegister(LSM6DS3_ACC_GYRO_TAP_THS_6D, TAP_THS);
  imu.writeRegister(LSM6DS3_ACC_GYRO_INT_DUR2, 0x4F);       // at 416 Hz: shock 58 ms, quiet 29 ms, single-tap shape
  // single tap only: the chip's own double tap never fired through the case (measured), doubleTap() pairs singles
}

bool doubleTap() {  // reading TAP_SRC clears the latch, so it is read every loop
  static uint32_t lastTap = 0;
  uint8_t v = 0;
  if (!imuOk || imu.readRegister(&v, LSM6DS3_ACC_GYRO_TAP_SRC) || !(v & 0x20)) return false;
  uint32_t t = millis();
  Serial.printf("TAPDBG src=%02X ms=%lu\n", v, t);  // TAPDBG temp
  if (lastTap && t - lastTap < SHAKE_WAIT_MS) return lastTap = 0, true;  // second single tap in time: double
  lastTap = t;
  return false;
}

void petTick(uint32_t now) {  // once a second: stats, mood; steps, sky, save on their own periods
  static uint32_t t0 = now, secs = 0;
  if (now - t0 < 1000) return;
  t0 += 1000, secs++;
  pet::Mood m = pet::mood(hourNow());
  pet::tick(1, m == pet::SLEEP);
  scene::setMood(m);
  if (secs % 2 == 0) readSteps();
  if (secs % 60 == 0) retint(), sendStats(false);
  if (wxKnown && now - wxT > WX_MS) weather("0");  // the forecast ran out
  if (secs % SAVE_S == 0) petSave();
}
#endif

float shakeLimit() {
#ifdef SCENE_PET
  return pet::s.shakeG;
#else
  return SHAKE_G;
#endif
}

// One message from the phone (rx has room for the terminating 0). Commands:
//   #time <unix s> <utc offset min>   set the clock (the app sends it on every connect)
//   #find  /  #find 0                 start / stop find
//   #heart                            hearts in the bubble
//   #call <caller>  /  #call 0        a call rings: name in the bubble + ring buzz / answered or gone
//   #music <title - artist>           the phone's track changed: quiet; pet scenes: music screen, else the bubble
//   pet scene: #feed, #stats, #weather <kind>, #set vib|shake|night|name <look>|scene ...  (interfaces/watch/README.md#protocol)
// Plain text, and anything else starting with '#', shows in the bubble.
void receive(uint8_t *rx, int n) {
  rx[n] = 0;
  const char *s = (const char *)rx;
  unsigned long unix;
  long ofs;
  if (sscanf(s, "#time %lu %ld", &unix, &ofs) == 2) {
    syncLocal = unix + ofs * 60, syncMs = millis();
    Serial.println("clock synced");
#ifdef SCENE_PET
    if (!caughtUp && bootSaved) {  // time spent off counts as sleep (capped in pet::offline)
      long off = (long)syncLocal - (long)bootSaved - (long)(syncMs / 1000);
      if (off > 0) pet::offline(off);
    }
    caughtUp = true;
    retint();
    sendSettings();
    sendStats(true);
#endif
  } else if (!strcmp(s, "#find")) {
    find(true);
  } else if (!strcmp(s, "#find 0")) {
    find(false);
  } else if (!strcmp(s, "#heart")) {
    strcpy(msg, "\x03 \x03 \x03");  // font glyph 3 is a heart
    show(2, scene::NCOL);  // the heart entry
#ifdef SCENE_PET
    pet::message();
#endif
  } else if (!strcmp(s, "#call 0")) {
    msgDur = 0;
    buzz(0);
  } else if (!strncmp(s, "#call ", 6)) {
    toAscii(rx + 6, n - 6, msg, sizeof msg);
    show(0);
    msgDur = CALL_MS;
    buzz(CALL_MS / 1000, 400, 1000);  // ring: 0.4 s every second
  } else if (!strncmp(s, "#music ", 7)) {
    strcpy(msg, "\x0E ");  // font glyph 14 is a music note (13 would be eaten as '\r')
    toAscii(rx + 7, n - 7, msg + 2, sizeof msg - 2);
    show(0, scene::INK, false);
#ifdef SCENE_PET
    msgMusic = true, msgDur = max(msgDur, MUSIC_MS);
#endif
#ifdef SCENE_PET
  } else if (!strcmp(s, "#feed")) {
    feed();
  } else if (!strcmp(s, "#stats")) {
    sendStats(true);
  } else if (!strncmp(s, "#weather ", 9)) {
    weather(s + 9);
  } else if (!strncmp(s, "#set ", 5) && setting(s + 5)) {
    petSave();
    sendSettings();
#endif
  } else {
    toAscii(rx, n, msg, sizeof msg);
    show(2);
#ifdef SCENE_PET
    pet::message();
#endif
  }
}

void bubble(const char *s, uint8_t color, uint32_t t0, uint32_t now, int Y = 11, int icon = -1) {
  const int L = 12, R = 68, H = 13;  // Y = 11 fits the top band of the round screen
  gfx.fillRoundRect(L, Y, R - L, H, 4, scene::INK);
  gfx.fillRoundRect(L + 1, Y + 1, R - L - 2, H - 2, 3, scene::CREAM);
  int w = strlen(s) * 6, inner = R - L - 6;
  gfx.setTextColor(color);
  gfx.setTextWrap(false);
  gfx.clipL = L + 2, gfx.clipR = R - 2;
#ifdef SCENE_PET
  if (icon >= 0) {  // the clock: weather icon, 2 px gap, the time, centered together (test/preview.cpp draws it the same)
    static const char K[] = "KGBY";
    int x0 = (L + R - (w + 11)) / 2 + 1;
    for (int r = 0; r < pet::WX_H; r++)
      for (int c = 0; c < 9; c++)
        if (const char *k = strchr(K, pet::WX_ICON[icon][r][c])) gfx.drawPixel(x0 + c, Y + 2 + r, k == K ? scene::INK : scene::NCOL + (k - K));
    gfx.setCursor(x0 + 11, Y + 3);
    gfx.print(s);
  } else
#endif
  if (w <= inner) {
    gfx.setCursor((L + R - w) / 2 + 1, Y + 3);
    gfx.print(s);
  } else {  // long text: scrolling, 25 px/s
    int x = L + 3 - (int)((now - t0) / 40 % (w + 20));
    for (int k = 0; k < 2; k++, x += w + 20) gfx.setCursor(x, Y + 3), gfx.print(s);
  }
  gfx.clipL = 0, gfx.clipR = scene::N;
}

// Music screen: no bubble; the title always scrolls in from the right over the character's trip, 25 px/s.
// test/preview.cpp redraws this the same way.
void marquee(const char *s, uint32_t t0, uint32_t now) {
  const int L = 8, R = 72, Y = 14, GAP = 24;  // L, R: where the round screen is this narrow
  int w = strlen(s) * 6, x = R - (int)((now - t0) / 40);
  while (x + w + GAP <= L) x += w + GAP;
  gfx.setTextWrap(false);
  gfx.clipL = L, gfx.clipR = R;
  for (; x < R; x += w + GAP) {  // outlined: readable over any sky
    gfx.setTextColor(scene::INK);
    for (int k = 0; k < 4; k++) gfx.setCursor(x + (k == 0) - (k == 1), Y + (k == 2) - (k == 3)), gfx.print(s);
    gfx.setTextColor(scene::CREAM);
    gfx.setCursor(x, Y), gfx.print(s);
  }
  gfx.clipL = 0, gfx.clipR = scene::N;
}

bool musicScreen(uint32_t now) {
#ifdef SCENE_PET
  return msgMusic && msg[0] && now - msgT0 <= msgDur && !finding && !scene::game.on;
#else
  return false;
#endif
}

void drawBubble(uint32_t now) {
  if (musicScreen(now)) return marquee(msg, msgT0, now);
#ifdef SCENE_PET
  int y = scene::game.on ? scene::game.labelY : 11;
#else
  int y = 11;
#endif
  if (msg[0] && now - msgT0 <= msgDur) return bubble(msg, msgColor, msgT0, now, y, msgIcon);
#ifdef SCENE_PET
  if (scene::game.on) {  // a message covers the score for its time, the game goes on
    char g[24];
    scene::gameLabel(g, sizeof g);
    if (g[0]) bubble(g, scene::INK, 0, now, y);  // empty: no bubble (Sauron's maze needs the screen)  // scroll clock: plain millis, so a score change doesn't restart the scroll
  }
#endif
}

// Power: daily on/off is the button (the battery switch on the back stays on). Hold the button OFF_HOLD_MS to turn off, about ON_HOLD_MS to turn on
// (electronics/tests/t10_power). Off = display cut by Q2 + System OFF, a few uA; waking is a reset.
bool dispOn;

void displayOff() {  // display pins LOW first so they don't feed the display, then cut its power
  if (!dispOn) return;
  tft.fillScreen(0);  // goes dark even without Q2
  SPI.end();
  const uint8_t pins[] = {TFT_CS, TFT_DC, TFT_RST, PIN_SPI_SCK, PIN_SPI_MOSI};
  for (uint8_t p : pins) pinMode(p, OUTPUT), digitalWrite(p, LOW);
  digitalWrite(DISP_PWR, HIGH);  // Q2 off
  dispOn = false;
}

// Adafruit_GC9A01A's init sequence without its final DISPON: the module has no backlight pin, so the backlight is lit
// from power-on and DISPON over the random GRAM flashes noise. The panel stays off (black) until the GRAM is cleared.
// clang-format off
const uint8_t PROGMEM GC_INIT[] = {
  GC9A01A_INREGEN2, 0,
  0xEB, 1, 0x14, // ?
  GC9A01A_INREGEN1, 0,
  GC9A01A_INREGEN2, 0,
  0xEB, 1, 0x14, // ?
  0x84, 1, 0x40, // ?
  0x85, 1, 0xFF, // ?
  0x86, 1, 0xFF, // ?
  0x87, 1, 0xFF, // ?
  0x88, 1, 0x0A, // ?
  0x89, 1, 0x21, // ?
  0x8A, 1, 0x00, // ?
  0x8B, 1, 0x80, // ?
  0x8C, 1, 0x01, // ?
  0x8D, 1, 0x01, // ?
  0x8E, 1, 0xFF, // ?
  0x8F, 1, 0xFF, // ?
  0xB6, 2, 0x00, 0x00, // ?
  GC9A01A_MADCTL, 1, 0x48, // MX | BGR
  GC9A01A_COLMOD, 1, 0x05,
  0x90, 4, 0x08, 0x08, 0x08, 0x08, // ?
  0xBD, 1, 0x06, // ?
  0xBC, 1, 0x00, // ?
  0xFF, 3, 0x60, 0x01, 0x04, // ?
  GC9A01A1_POWER2, 1, 0x13,
  GC9A01A1_POWER3, 1, 0x13,
  GC9A01A1_POWER4, 1, 0x22,
  0xBE, 1, 0x11, // ?
  0xE1, 2, 0x10, 0x0E, // ?
  0xDF, 3, 0x21, 0x0c, 0x02, // ?
  GC9A01A_GAMMA1, 6, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,
  GC9A01A_GAMMA2, 6, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,
  GC9A01A_GAMMA3, 6, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,
  GC9A01A_GAMMA4, 6, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,
  0xED, 2, 0x1B, 0x0B, // ?
  0xAE, 1, 0x77, // ?
  0xCD, 1, 0x63, // ?
  // Unsure what this line (from manufacturer's boilerplate code) is
  // meant to do, but users reported issues, seems to work OK without:
  //0x70, 9, 0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03, // ?
  GC9A01A_FRAMERATE, 1, 0x34,
  0x62, 12, 0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, // ?
            0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70,
  0x63, 12, 0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, // ?
            0x18, 0x13, 0x71, 0xF3, 0x70, 0x70,
  0x64, 7, 0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07, // ?
  0x66, 10, 0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00, // ?
  0x67, 10, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98, // ?
  0x74, 7, 0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00, // ?
  0x98, 2, 0x3e, 0x07, // ?
  GC9A01A_TEON, 0,
  GC9A01A_INVON, 0,
  GC9A01A_SLPOUT, 0x80, // Exit sleep
  0x00
};
// clang-format on

void displayOn() {
  pinMode(DISP_PWR, OUTPUT);
  digitalWrite(DISP_PWR, LOW);  // display power on through Q2
  delay(50);
  tft.initSPI(SPI_HZ);  // also pulses the hardware reset
  for (const uint8_t *a = GC_INIT; uint8_t cmd = pgm_read_byte(a++);) {
    uint8_t x = pgm_read_byte(a++), n = x & 0x7F;
    tft.sendCommand(cmd, a, n);
    a += n;
    if (x & 0x80) delay(150);
  }
  tft.fillScreen(0);
  tft.sendCommand(GC9A01A_DISPON);
  dispOn = true;
}

void flashSleep() {  // on-board QSPI flash into deep power-down (0xB9): ~25 uA less while off
  const uint8_t pins[] = {PIN_QSPI_CS, PIN_QSPI_SCK, PIN_QSPI_IO0, PIN_QSPI_IO2, PIN_QSPI_IO3};
  for (uint8_t p : pins) pinMode(p, OUTPUT);  // the core preloads outputs HIGH: CS, WP#, HOLD# idle
  digitalWrite(PIN_QSPI_SCK, LOW);
  digitalWrite(PIN_QSPI_CS, LOW);
  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_QSPI_IO0, (0xB9 >> i) & 1);
    digitalWrite(PIN_QSPI_SCK, HIGH);
    digitalWrite(PIN_QSPI_SCK, LOW);
  }
  digitalWrite(PIN_QSPI_CS, HIGH);
}

void powerOff() {
  digitalWrite(MOTOR, LOW);
#ifdef SCENE_PET
  petSave();
#endif
  displayOff();
  pinMode(DISP_PWR, OUTPUT);
  digitalWrite(DISP_PWR, HIGH);  // Q2 off
  const uint8_t loads[] = {PIN_LSM6DS3TR_C_POWER, PIN_PDM_PWR};  // IMU and mic supplies
  for (uint8_t p : loads) pinMode(p, OUTPUT), digitalWrite(p, LOW);
  flashSleep();
  while (digitalRead(BTN) == LOW) delay(10);  // System OFF with the button held = instant wake
  delay(50);
  systemOff(BTN, LOW);
}

void setup() {
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);  // the core drives it HIGH at boot; keep P0.31 safe once a battery is attached
  pinMode(MOTOR, OUTPUT);
  digitalWrite(MOTOR, LOW);  // outputs start HIGH in this core: the motor would run through setup
  pinMode(BTN, INPUT_PULLUP);
  if (readResetReason() & POWER_RESETREAS_OFF_Msk) {  // woken by the button: on only if held, a tap goes back off
    while (digitalRead(BTN) == LOW && millis() < ON_HOLD_MS) delay(10);
    if (millis() < ON_HOLD_MS) powerOff();
  }
  Serial.begin(115200);
  displayOn();
  buildPal();
  scene::init();
  imuOk = imu.begin() == 0;
  if (!imuOk) Serial.println("NO IMU: shake won't work");
#ifdef SCENE_PET
  petLoad();
  if (imuOk) imuPetSetup();
#ifdef SCENE_PICK
  pickScene();
#else
  retint();
#endif
#endif

  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);  // large MTU: long messages arrive in one write
  Bluefruit.begin();
  Bluefruit.setName("MysteryChain");
  bleuart.begin();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();           // name in the main packet so every scanner sees it (20/31 bytes)
  Bluefruit.ScanResponse.addService(bleuart);  // 128-bit UART UUID doesn't fit the main packet
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.start(0);
  while (digitalRead(BTN) == LOW) delay(10);  // the power-on hold is not a press
}

void loop() {
  static uint32_t last = millis(), n = 0, t0 = last;
  uint32_t now = millis();
#ifdef SCENE_PET
  scene::music(musicScreen(now));
#endif
  scene::step(min(now - last, 50UL) / 1000.0f);
  last = now;
  char line[32];

  // BLE: a message is complete at a line break, or RX_GAP_MS after its last byte (senders without '\n').
  // A gap, not "no byte this loop": a write split into 20-byte packets (small MTU) must not become several messages,
  // or "#music Title - Artist" splits and its tail shows as plain text with a buzz.
  const uint32_t RX_GAP_MS = 150;
  static uint8_t rx[201];  // 200 bytes + the 0 receive() adds
  static int rxN = 0;
  static uint32_t rxT = 0;
  bool got = false;
  while (bleuart.available()) {
    int c = bleuart.read();
    got = true, rxT = now;
    if (c != '\n' && c != '\r' && rxN < (int)sizeof rx - 1) rx[rxN++] = c;
    else if (rxN) receive(rx, rxN), rxN = 0;
  }
  if (!got && rxN && now - rxT >= RX_GAP_MS) receive(rx, rxN), rxN = 0;

  static bool wasConn = false;
  if (bool(Bluefruit.connected()) != wasConn) {
    wasConn = !wasConn;
    strcpy(msg, wasConn ? "connected" : "disconnected");
    show(1);
  }

  static uint32_t presses = 0;
  static bool btnPrev = HIGH;
  bool btn = digitalRead(BTN);  // loop is ~45 ms: debounces itself
  bool changed = btn != btnPrev, pressed = changed && btn == LOW;
  btnPrev = btn;
#ifdef SCENE_PET
  bool released = changed && btn == HIGH;
  static uint32_t btnT = 0;  // last button edge, for TAP_GUARD_MS
  if (pressed || released) btnT = now;
  // Tap: clock after TAP_WAIT_MS; double tap: feed; triple tap (pets.h): next look; hold 1-3 s and release: game; hold 3 s: off.
  // In the game: the button is the jump (held = higher; Pavi steers by tilt instead, Nausicaa steers by tilt and fires), a press after a game over restarts, hold 3 s leaves.
  static uint32_t downT = 0, tapT = 0;
  static uint8_t taps = 0;  // short presses in this run, each within TAP_WAIT_MS of the last
  static bool tapPending = false, consumed = false, longBuzzed = false;  // consumed: this press is used up
  if (pressed) {
    downT = now, longBuzzed = false, consumed = false;
    if (finding) {  // the press that ends a find is not a normal press
      find(false);
      tell("found\n");
      scene::cheer();
      consumed = true;
    } else if (scene::game.on) {
      if (scene::game.dead && scene::game.t > 1) startGame(), consumed = true;
    } else {
      snprintf(line, sizeof line, "button %lu\n", ++presses);
      tell(line);
      buzz(1);
      if (tapPending) tapPending = false;  // the run goes on
      else taps = 0;
    }
  }
  if (scene::game.on) scene::gameInput(btn == LOW && !consumed);
  if (scene::game.on && imuOk) {  // sideways tilt steers Pavi's and Nausicaa's games, both axes Sauron's maze (the runner ignores it)
    static float tilt = 0;
    float a = TILT_AXIS == 0 ? imu.readFloatAccelX() : TILT_AXIS == 1 ? imu.readFloatAccelY() : imu.readFloatAccelZ();
    tilt += (TILT_SIGN * a - tilt) * 0.6f;  // low-pass: a hand jolt or the motor doesn't flip the lane
    scene::gameTilt(tilt);
    if (scene::cur == 4) {
      static float tilt2 = 0;
      float b = TILT2_AXIS == 0 ? imu.readFloatAccelX() : TILT2_AXIS == 1 ? imu.readFloatAccelY() : imu.readFloatAccelZ();
      tilt2 += (TILT2_SIGN * b - tilt2) * 0.6f;
      scene::gameTilt2(tilt, tilt2);
    }
  }
  if (btn == LOW && !consumed) {
    if (!scene::game.on && !longBuzzed && now - downT >= GAME_HOLD_MS) longBuzzed = true, buzz(1);  // "release now"
    if (now - downT >= OFF_HOLD_MS) {
      if (scene::game.on) scene::gameStop(), consumed = true;  // leave the game; no power-off until released
      else powerOff();
    }
  }
  if (released) {
    if (!consumed && !scene::game.on) {
      if (now - downT >= GAME_HOLD_MS) taps = 0, startGame();
      else if (++taps >= MAX_TAPS) tapAction(taps), taps = 0;  // nothing longer to wait for
      else tapPending = true, tapT = now;
    }
    consumed = false;
  }
  if (tapPending && now - tapT >= TAP_WAIT_MS) tapPending = false, tapAction(taps), taps = 0;
  if (scene::game.ended) {
    scene::game.ended = false;
    snprintf(line, sizeof line, "game %d best %u\n", scene::game.score, pet::best());
    tell(line);
    if (scene::game.newBest) buzz(3), petSave();
  }
  if (scene::game.buzz) buzz(scene::game.buzz), scene::game.buzz = 0;
  if (pet::palDirty) pet::palDirty = false, buildPal();
  petTick(now);
#else
  if (pressed) {
    if (finding) {  // the press that ends a find is not a normal press
      find(false);
      tell("found\n");
    } else {
      snprintf(line, sizeof line, "button %lu\n", ++presses);
      tell(line);
      buzz(1);
      if (syncLocal) showClock();
    }
    scene::cheer();
  }
  static uint32_t btnDown = 0;  // held OFF_HOLD_MS: power off
  if (btn == HIGH) btnDown = 0;
  else if (!btnDown) btnDown = now;
  else if (now - btnDown >= OFF_HOLD_MS) powerOff();
#endif
  if (finding && millis() - findT0 >= FIND_MS) {  // millis(), not now: findT0 may be set after now was read
    find(false);
    tell("find timeout\n");
  }

  uint32_t e = now - buzzT0, f = (now - findT0) % 1000;
  digitalWrite(MOTOR, finding ? f < 360 && f % 240 < 120 : e < buzzN * buzzPer && e % buzzPer < buzzOn);  // find: buzz-buzz, pause
  bool still = e >= buzzN * buzzPer + MOTOR_MASK_MS;  // motor vibration must not count as a shake or a tap

  static uint32_t activeT = 0;  // last movement or button edge; finding keeps the screen on
  if (changed || finding) activeT = now;
  static uint32_t shakes = 0, lastShake = 0, shakeT = 0;
  static float shakeDev = 0;
  static bool shakeWait = false;  // a shake detected at shakeT, counted SHAKE_DELAY later
#ifdef SCENE_PET
  const uint32_t SHAKE_DELAY = SHAKE_WAIT_MS;
  if (doubleTap() && still && !finding && now - btnT > TAP_GUARD_MS && !scene::game.on) {  // petting
    shakeWait = false;  // the "shake" was the tap
    scene::purr();
    pet::joy(pet::PET_JOY);
    buzz(5, 40, 160);  // purr: short soft pulses
  }
#else
  const uint32_t SHAKE_DELAY = 0;  // no petting: a shake counts at once
#endif
  if (imuOk && !finding && still) {
    float ax = imu.readFloatAccelX(), ay = imu.readFloatAccelY(), az = imu.readFloatAccelZ();
    float dev = fabsf(sqrtf(ax * ax + ay * ay + az * az) - 1.0f);
    if (dev > (dispOn ? MOVE_G : shakeLimit())) activeT = now;  // dark: only a shake wakes it
    if (!dispOn) lastShake = now;  // the waking shake is not a shake event
    else if (dev > shakeLimit() && now - lastShake > SHAKE_GAP_MS) {
      lastShake = now;
      if (!shakeWait) shakeWait = true, shakeT = now, shakeDev = dev;
    }
  }
  if (shakeWait && now - shakeT >= SHAKE_DELAY) {
    shakeWait = false;
    scene::shake();
    snprintf(line, sizeof line, "shake %lu (%.2f g)\n", ++shakes, shakeDev);
    tell(line);
  }

  if (dispOn && now - activeT >= IDLE_OFF_MS) displayOff();
  else if (!dispOn && now - activeT < IDLE_OFF_MS) displayOn();
  if (!dispOn) {
    delay(20);  // nothing to draw: let the CPU sleep, IMU still read ~40x a second
    t0 = now, n = 0;  // no fps catch-up spam on wake
    return;
  }
  drawBubble(now);
  const uint16_t *P = finding && (now - findT0) / 250 % 2 ? palInv : pal;  // find: inverted colors 2x a second
  tft.startWrite();
  tft.setAddrWindow(0, 0, 240, 240);
  const uint8_t *fbv = scene::view();
  const int vn = scene::viewN(), rows = SC * BAND;  // screen rows per strip
  static uint8_t col[240];  // screen x -> view x (nearest; the cat's game view isn't a whole multiple)
  static int colN = 0;
  if (colN != vn) {
    for (int x = 0; x < 240; x++) col[x] = x * vn / 240;
    colN = vn;
  }
  for (int y0 = 0; y0 < 240; y0 += rows) {
    uint16_t *p = strip;
    for (int y = y0, prev = -1; y < y0 + rows; y++, p += 240) {
      int sy = y * vn / 240;
      if (sy == prev) {
        memcpy(p, p - 240, 240 * 2);
        continue;
      }
      const uint8_t *s = fbv + sy * vn;
      for (int x = 0; x < 240; x++) p[x] = P[s[col[x]]];
      prev = sy;
    }
    tft.writePixels(strip, 240 * rows, true, true);
  }
  tft.endWrite();

  n++;
  if (now - t0 >= 1000) {
    Serial.printf("%lu fps\n", n);
    n = 0;
    t0 += 1000;
  }
}
