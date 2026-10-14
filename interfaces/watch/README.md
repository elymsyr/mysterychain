# Watch (device firmware)

Arduino sketches for the keychain's round display. Each folder is one scene, a complete firmware you flash on its own.

| Scene | What happens |
| --- | --- |
| [`cat/`](cat/) | Pixel cat on a night lawn. Shake: the yarn ball flies, the cat jumps and plays for 6 s. |
| [`seven_havens/`](seven_havens/) | Pavi (Seven Havens) on a wall above the rooftops, under an aurora. Shake: her fairy darts off and she chases it. |
| [`nausicaa_teto/`](nausicaa_teto/) | Nausicaa in a windy valley with Teto on her shoulder. Shake: Teto jumps off, she tries to catch him, then kneels and he climbs back. |
| [`teto/`](teto/) | Teto in the Sea of Corruption playing with a glowing spore. Shake: the spore is flung, Teto chases it. |

All scenes share the same behavior:

- 80×80 pixel scene, scaled 3× to the 240×240 display over SPI DMA, ~30 fps. Motion is tied to real time, so speed holds if fps drops.
- **BLE** (Nordic UART, name `MysteryChain`): text from the [mobile app](../mobile-app/) scrolls in a speech bubble at the top, the motor buzzes twice, the character cheers.
- **Button** (D0): the character cheers, motor buzzes once, `button N` is sent to the phone.
- **Shake** (IMU): the scene reacts, `shake N (x.xx g)` is sent to the phone. Shakes are ignored for `MOTOR_MASK_MS` after the motor stops.
- Serial monitor prints fps once a second.

## Flash

Arduino IDE 2 with **Seeed nRF52 Boards** (non-mbed), board **Seeed XIAO nRF52840 Sense**, libraries **Adafruit GC9A01A** and **Seeed Arduino LSM6DS3**. Full setup: [electronics/bring-up.md](../../electronics/bring-up.md#phase-0-tools-and-software).

Open e.g. `cat/cat.ino` and upload. Or with arduino-cli:

```bash
arduino-cli compile --fqbn Seeeduino:nrf52:xiaonRF52840Sense --upload -p /dev/ttyACM0 cat
```

Tunables at the top of each `.ino`:

| Constant | Default | Meaning |
| --- | --- | --- |
| `SPI_HZ` | 32 MHz | Display clock. Drop to 16 MHz (~17 fps) if the image glitches. |
| `SHAKE_G` | 0.5 | Shake threshold: deviation of \|a\| from 1 g |
| `SHAKE_GAP_MS` | 400 | Minimum time between two shakes |
| `MOTOR_MASK_MS` | 400 | Shake blackout after the motor stops. Use the value measured in bring-up T8. |

## Scene interface

Each scene header (`cat.h`, `pavi.h`, `nausicaa.h`, `teto.h`) is plain C++ with no Arduino dependency, so it can be compiled on a PC and dumped frame by frame. It lives in its own namespace and exposes:

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

The `.ino` files are identical apart from the namespace they use; to add a scene, copy one and point it at your header.

## Rules for new animations

1. Existing behaviors stay as they are (cat `IDLE` / `RUN` / `CROUCH` / `JUMP` / `LAND` / `BAT`, Pavi and the fairy, yarn physics, `shake()` and `cheer()` reactions). New animations are added next to them.
2. For every animation, write down when it starts, how long it lasts, which state it returns to and which events interrupt it.
3. Animations read the pet stats (hunger, happiness, energy) to pick what to play, and change them (eating lowers hunger, play costs energy). See [roadmap](../../docs/roadmap.md) K1.
4. They follow the time of day and weather backgrounds; characters and props must stay readable on every background, including night colors. At night the default is sleep.
5. Events can interrupt any animation: message/notification bubble, "find" command, shake, petting, button. A sleeping pet wakes up when shaken.
6. Motion uses `dt`, never frame counts.
7. Logic and drawing stay separate: stats and clock keep running while the display is off; drawing only happens when it's on.
8. Limits: 80×80 indexed buffer with the scene palette, ~30 fps target, the top band (y 11–24) is reserved for the speech bubble, corners are invisible on the round screen.
