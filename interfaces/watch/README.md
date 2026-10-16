# Watch (device firmware)

One Arduino sketch for the keychain's round display: [`firmware/firmware.ino`](firmware/firmware.ino). The scene is a header next to it, picked by the `#include` line near the top of the sketch. The default, [`pets.h`](firmware/pets.h), holds all three pets (cat, Pavi, Nausicaa) and lets the phone switch between them (`#set scene`, the app's settings); the choice is saved on the board.

| Scene header | What happens |
| --- | --- |
| [`cat.h`](firmware/cat.h) | Pixel cat on a lawn. Shake: the yarn ball flies, the cat jumps and plays for 6 s. The cat is also a pet, see [Pets](#pets). |
| [`pavi.h`](firmware/pavi.h) | Pavi (Seven Havens) on a wall above the rooftops: clouds and sun by day, aurora, moon and stars at night. Shake: her fairy darts off and she chases it. Pavi is also a pet, see [Pets](#pets). |
| [`nausicaa.h`](firmware/nausicaa.h) | Nausicaa in a windy valley with Teto on her shoulder. Shake: Teto jumps off, she tries to catch him, then kneels and he climbs back. |

Every scene behaves the same way (screens: [docs/mockups/device_screens.png](../../docs/mockups/device_screens.png)):

- 80×80 pixel scene, scaled 3× to the 240×240 display over SPI DMA, ~30 fps. Motion is tied to real time, so speed holds if fps drops.
- **BLE** (Nordic UART, name `MysteryChain`): text from the [mobile app](../mobile-app/) scrolls in a speech bubble at the top, the motor buzzes twice, the character cheers. Messages starting with `#` are commands, see [protocol](#protocol).
- **Clock:** the app sends the phone's time on every connect. Kept until power-off; after that it's back on the next connect.
- **Button** (D0): the character cheers, motor buzzes once, `button N` is sent to the phone. Once the clock is set, a tap also shows the time (`14:32`) in the bubble for 3 s.
- **Find** (`#find`): `FIND ME!` in the bubble, the motor buzzes twice and pauses, the screen colors invert twice a second. A button press stops it and sends `found`; `#find 0` stops it from the app; after `FIND_MS` (60 s) it stops by itself and sends `find timeout`. Shakes are ignored while it runs.
- **Power** (button in firmware; the battery switch on the back stays on): hold the button 3 s to turn off (display cut by Q2 on D7, System OFF, a few µA). Hold ~1 s to turn on; a short tap leaves it off. Plugging in USB also turns it on, so uploads work with the case closed. Waking is a reset: the scene starts over.
- **Screen off:** no movement (`MOVE_G`) and no button for 1 min: the display goes dark (Q2 cut), BLE stays up. A shake (`SHAKE_G`), the button or `#find` brings it back; the waking shake is not sent as `shake N`. Turned off with the button, only the button turns it on (IMU unpowered in System OFF).
- **Shake** (IMU): the scene reacts, `shake N (x.xx g)` is sent to the phone. Shakes are ignored for `MOTOR_MASK_MS` after the motor stops.
- Serial monitor prints fps once a second.

## Pets

With `cat.h` or `pavi.h` the character lives on its own: stats, moods, day and night, a runner game, a music screen. Both have the same set of animations, each in its own way (below: cat / Pavi). Screens: [day and night](../../docs/mockups/pet_day_night.png), [animations](../../docs/mockups/pet_animations.png), [game](../../docs/mockups/pet_game.png); every animation as a clip: [preview](#preview). Design: [spec](../../docs/superpowers/specs/2026-10-15-pet-design.md).

- **Stats** ([`pet.h`](firmware/pet.h)): fullness, happiness, energy, 0–100, kept on the board only to pick the animations (not shown anywhere). Fullness empties in 10 h (half as fast asleep), happiness in 16 h awake (twice as fast when starving), energy in 16 h awake and refills in 8 h of sleep. Messages and reactions +8 (once a minute), petting +5, 100 steps +1, a game up to +15 and −10 energy. The pet never dies.
- **Mood**, first that holds: `sleep` (night window or energy < 10), `hungry` (< 25), `sad` (< 25), `tired` (< 25), `happy`. Asleep: curled up with z's / hugging her knees, the fairy dozing on her head. Hungry: by an empty bowl / plate. Sad: lies down / sits with her head down. Tired: yawns. Happy: now and then a butterfly (day) or a mouse / firefly (night) comes by and gets chased; every 10–25 s an idle trick: grooming or a tail chase / a wave, sitting with her feet tapping, or a twirl.
- **Button:** tap = clock + cheer (after 300 ms); double tap = feed (eats from the bowl / an apple, or shakes the head when fullness > 90); hold until the 1 s buzz and release = runner game; hold 3 s = off.
- **Music screen:** when the phone's track changes (`#music`), the whole background turns into the character's own gradient (cat: peach to plum, Pavi: pink to violet), the title scrolls across the top and the character dances to a 120 bpm beat, notes rising. No buzz. It stays `MUSIC_MS` (12 s) or two scroll loops, up to 15 s; a message, button tap, find or the game ends it, a shake is a hop and back to dancing. Asleep, the pet sleeps on.
- **Runner game:** tap = jump over a flower pot, hold = high jump over a fence, stay down under birds (from 20 points). Score and best in the bubble; a new best buzzes 3 times. After a game over a press restarts, 10 s idle or a 3 s hold leaves (no power-off inside the game). Not with energy < 15.
- **Petting:** double tap on the case (IMU tap engine): hearts and a soft purr buzz. A shake waits 300 ms so a double tap isn't taken as a shake.
- **Steps:** the IMU's hardware pedometer; today's count resets at local midnight. Off = no counting.
- **Day and night:** sky, ground, clouds, sun and moon follow the clock (dawn 06–08, day, sunset 18–20, night; keyframes in `pet.h`, colors per scene in its `LOOK` table). Pavi: the aurora and stars fade in at night, her rooftops fade into the haze by day and their windows light up in the evening. No clock yet: night look.
- **Night window** (default 23–07): the pet sleeps and the motor is quiet, except for find.
- **Saved** in the internal flash (`/pet`, LittleFS) every 10 min and on feed, settings, a new best and power-off. Time spent off counts as sleep (up to 48 h) once the phone sends the time.

## Flash

Arduino IDE 2 with **Seeed nRF52 Boards** (non-mbed), board **Seeed XIAO nRF52840 Sense**, libraries **Adafruit GC9A01A** and **Seeed Arduino LSM6DS3**. Full setup: [electronics/bring-up.md](../../electronics/bring-up.md#phase-0-tools-and-software).

Open `firmware/firmware.ino`, upload (to flash a single scene instead, set it in the `#include` line). Or with arduino-cli:

```bash
arduino-cli compile --fqbn Seeeduino:nrf52:xiaonRF52840Sense --upload -p /dev/ttyACM0 firmware
```

Tunables at the top of `firmware.ino`:

| Constant | Default | Meaning |
| --- | --- | --- |
| `SPI_HZ` | 32 MHz | Display clock. Drop to 16 MHz (~17 fps) if the image glitches. |
| `SHAKE_G` | 0.5 | Shake threshold: deviation of \|a\| from 1 g |
| `SHAKE_GAP_MS` | 400 | Minimum time between two shakes |
| `MOTOR_MASK_MS` | 400 | Shake blackout after the motor stops. Use the value measured in bring-up T8. |
| `ON_HOLD_MS` / `OFF_HOLD_MS` | 1000 / 3000 | Button hold to turn on / off |
| `MOVE_G` | 0.15 | Movement that keeps the screen on. Must sit above the at-rest noise |
| `IDLE_OFF_MS` | 60000 | Screen off after this long without movement or button |
| `CLOCK_MS` | 3000 | How long a button tap shows the time |
| `MUSIC_MS` | 12000 | Pets: the music screen stays at least this long after a track change |
| `FIND_MS` | 60000 | Find stops by itself after this long |
| `GAME_HOLD_MS` | 1000 | Pets: hold and release after this long = game |
| `TAP_WAIT_MS` | 300 | Pets: a tap waits this long for a second one (double = feed) |
| `SHAKE_WAIT_MS` | 300 | Pets: a shake is decided this late; a case double tap inside it is petting |
| `TAP_GUARD_MS` | 500 | Pets: case taps this soon after a button edge are ignored |
| `TAP_THS` | 3 | Pets: IMU tap threshold, × 0.5 g. Check through the printed case |
| `SAVE_S` | 600 | Pets: stats saved this often |

Stat rates and gains are at the top of [`pet.h`](firmware/pet.h), game physics at the top of [`game.h`](firmware/game.h).

## Protocol

Plain UTF-8 text over Nordic UART. A message ends at a line break (the app sends one), or 150 ms after its last byte for senders without one, so a write split into 20-byte packets on a small MTU still arrives as one message. A message the board doesn't recognise as a command shows in the bubble, so plain text, and text that only looks like a command (`#1 fan`), keeps working.

| Phone → keychain | Effect |
| --- | --- |
| any text | Bubble (max 63 characters; Turkish letters folded to ASCII, other non-ASCII shows as `?`), buzz twice, cheer |
| `#time <unix s> <utc offset min>` | Set the clock. The app sends it on every connect. |
| `#find` / `#find 0` | Start / stop find |
| `#heart` | Three pink hearts in the bubble, buzz twice, cheer |
| `#call <caller>` / `#call 0` | A call rings: caller in the bubble, ring buzz (0.4 s every second) / answered or gone: bubble and buzz stop. Stops by itself after `CALL_MS` (45 s). Sent by the app's background link. |
| `#music <title - artist>` | The phone's track changed: pets show the [music screen](#pets), other scenes ♫ and the text in the bubble. No buzz. Sent by the app's background link. |
| `#feed` | Pets: feed (answers `fed` or `full`) |
| `#stats` | Pets: send `stats` now |
| `#set vib 0\|1` | Pets: motor off / on (find always buzzes) |
| `#set shake <g>` | Pets: shake threshold, 0.3–1.0 |
| `#set night <from h> <to h>` | Pets: night window (sleep + quiet); `from == to` = none |
| `#set name <look> <text>` | `pets.h`: that look's own name, up to 12 characters (defaults `Cat`, `Pavi`, `Nausicaa`); a single-scene build takes `#set name <text>` |
| `#set scene cat\|pavi\|nausicaa` | `pets.h`: the pet's look, at once and saved. Stats and high score are shared, each look has its own name; a running game ends |

| Keychain → phone | When |
| --- | --- |
| `button N` | Button pressed (N counts since boot) |
| `shake N (x.xx g)` | Shake detected |
| `found` | Button pressed during find |
| `find timeout` | Find stopped by itself after `FIND_MS` |
| `stats <steps> <mood>` | Pets: after `#time`, on `#stats`, after a feed, and once a minute if a value changed |
| `settings vib=<0\|1> shake=<g> night=<from>-<to> scene=<look> name=<cat>\|<pavi>\|<nausicaa>` | Pets: after `#time` and every `#set`; one name per look in `pets.h` order (a single-scene build sends one) |
| `fed` / `full` | Pets: answer to a feed (button or app) |
| `game <score> best <best>` | Pets: game over |

## Scene interface

Each scene header (`cat.h`, `pavi.h`, `nausicaa.h`) is plain C++ with no Arduino dependency, so it can be compiled on a PC and dumped frame by frame. It lives in its own namespace, ends with `namespace scene = <its namespace>;` (the sketch only talks to `scene::`) and exposes:

| Name | Purpose |
| --- | --- |
| `N`, `fb[N*N]` | Scene size and the color-indexed framebuffer |
| `PAL[NCOL][3]`, `NCOL` | RGB palette |
| `INK`, `CREAM` | Palette indices used by the speech bubble |
| `px(x, y, c)` | Set one pixel (clipped) |
| `init()` | Draw the static background |
| `step(dt)` | Advance by `dt` seconds and redraw `fb` |
| `shake()` | React to a shake |
| `cheer()` | React to a message or button press |

To add a scene, put its header in `firmware/` with the same interface and the `namespace scene = …;` line, and include it from `firmware.ino`. The sketch adds one palette entry after the scene's own (`HEART`, index `NCOL`) for the pink hearts.

A scene that ends with `#define SCENE_PET` (`cat.h`, `pavi.h`) turns on the pet code in the sketch and also exposes: `PAL` writable plus `tint(minuteOfDay)`, `setMood()`, `eat()`, `refuse()`, `purr()`, `yawn()`, `music(on)` (called every frame), and the game (`game`, `gameStart()`, `gameInput(held)`, `gameStop()`, `gameLabel()`). Stats and the time-of-day keyframes live in [`pet.h`](firmware/pet.h); the runner in [`game.h`](firmware/game.h), included by both pet scenes (its header lists what a scene defines for it: the `hero`, its head span, the bird height). PC check for both, with either runner:

```bash
g++ -std=c++17 -I firmware test/pet_test.cpp -o /tmp/pet_test && /tmp/pet_test
g++ -std=c++17 -DPAVI -I firmware test/pet_test.cpp -o /tmp/pet_test && /tmp/pet_test
g++ -std=c++17 -DPETS -I firmware test/pet_test.cpp -o /tmp/pet_test && /tmp/pet_test   # pets.h look switch
```

[`pets.h`](firmware/pets.h) includes the three pet headers (they skip their `namespace scene` alias when `SCENE_PICK` is defined) and is itself the `scene`: `fb`, `PAL`, `NCOL`, `INK`, `CREAM` point at the current look, the calls go to it, `pick(i)` switches. The runner state is one object for all (`pet::runner`).

### Preview

[`test/preview.cpp`](test/preview.cpp) runs a pet scene on the PC and dumps a clip of every animation (times of day, music screen, moods, visitors, idle tricks, the game on autopilot); [`test/preview.py`](test/preview.py) turns them into round-masked GIFs. Look at them before flashing:

```bash
cd test
g++ -std=c++17 -O2 -I../firmware -I ~/Arduino/libraries/Adafruit_GFX_Library preview.cpp -o /tmp/preview_cat   # -DPAVI for Pavi
mkdir -p /tmp/clips && /tmp/preview_cat /tmp/clips && python3 preview.py /tmp/clips /tmp/gifs   # --sheet: contact sheets too
```

## Rules for new animations

1. Existing behaviors stay as they are (cat `IDLE` / `RUN` / `CROUCH` / `JUMP` / `LAND` / `BAT`, Pavi and the fairy, yarn physics, `shake()` and `cheer()` reactions). New animations are added next to them, for both pets.
2. For every animation, write down when it starts, how long it lasts, which state it returns to and which events interrupt it.
3. Animations read the pet stats (hunger, happiness, energy) to pick what to play, and change them (eating lowers hunger, play costs energy). See [roadmap](../../docs/roadmap.md) K1.
4. They follow the time of day and weather backgrounds; characters and props must stay readable on every background, including night colors. At night the default is sleep.
5. Events can interrupt any animation: message/notification bubble, "find" command, shake, petting, button. A sleeping pet wakes up when shaken.
6. Motion uses `dt`, never frame counts.
7. Logic and drawing stay separate: stats and clock keep running while the display is off; drawing only happens when it's on.
8. Limits: 80×80 indexed buffer with the scene palette, ~30 fps target, the top band (y 11–24) is reserved for the speech bubble, corners are invisible on the round screen.
