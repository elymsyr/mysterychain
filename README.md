# MysteryChain

A keychain virtual pet. A 1.28" round display, BLE and a vibration motor in a 42 mm 3D-printed case. Shake it and the pet plays; messages from your phone pop up in a speech bubble.

<p align="center">
  <img src="mechanical/renders/front.png" alt="Case, front view" width="360">
</p>

**Status:** working on the breadboard (four scenes, BLE messaging, Android app). Case v4 is ready for its first print. The LiPo isn't bought yet, so nothing is assembled in the case.

## Hardware

| Part | Role |
| --- | --- |
| Seeed XIAO nRF52840 Sense | MCU, BLE, IMU, LiPo charger |
| GC9A01 1.28" 240×240 round TFT | Display |
| 10×3 mm coin vibration motor + BC337 | Haptics |
| Tact button, slide switch | Input, power |
| LiPo 402030 (~200 mAh) | Battery |

Full parts list, pinout and power notes: [electronics/](electronics/).

## Repository layout

```text
interfaces/
  watch/        Device firmware: one Arduino sketch per scene (cat, Pavi, Nausicaa + Teto, Teto)
  mobile-app/   Flutter companion app (Android), talks to the keychain over BLE
electronics/    Parts, pinout, schematic, bring-up guide and test sketches
mechanical/     Parametric case (build123d), STEP, print files, renders
docs/           Roadmap
```

## Quick start

1. Set up Arduino IDE 2 with the Seeed nRF52 core and two libraries: [bring-up, phase 0](electronics/bring-up.md#phase-0-tools-and-software).
2. Wire the parts on a breadboard: [pinout](electronics/README.md#pinout).
3. Flash a scene, e.g. [`interfaces/watch/cat/cat.ino`](interfaces/watch/cat/cat.ino). Shake the board.
4. Run the [mobile app](interfaces/mobile-app/) (`flutter run`) and send the pet a message.
5. Print the case from [`mechanical/print/`](mechanical/print/) (PETG) and assemble: [mechanical](mechanical/README.md#assembly).

## Docs

| Doc | Content |
| --- | --- |
| [interfaces/watch](interfaces/watch/README.md) | Scenes, flashing, tunables, how to add animations |
| [interfaces/mobile-app](interfaces/mobile-app/README.md) | App build and the BLE protocol |
| [electronics](electronics/README.md) | Parts, pinout, motor driver, power budget |
| [electronics/bring-up](electronics/bring-up.md) | Step-by-step build and tests, troubleshooting |
| [mechanical](mechanical/README.md) | Case design, assembly, printing |
| [docs/roadmap](docs/roadmap.md) | Planned features and open decisions |
