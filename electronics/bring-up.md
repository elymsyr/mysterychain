# Bring-up

How to build the electronics step by step and test each part before it goes into the case. Wiring and pinout: [README.md](README.md). Test sketches: [`tests/`](tests/) (all compile for "Seeed XIAO nRF52840 Sense").

Each phase ends with a **pass condition**. Don't move on until it holds. When something breaks, suspect the last thing you added: it's the only thing that changed.

```mermaid
flowchart TD
    F0["Phase 0<br/>Tools and software"] --> F1["Phase 1<br/>Incoming part check"]
    F1 --> F2["Phase 2<br/>XIAO alone<br/>T1 T2 T3"]
    F2 --> F3["Phase 3<br/>Bench: display, button, motor<br/>T4 T5 T6"]
    F3 --> F4["Phase 4<br/>Battery, display switch Q2,<br/>power button<br/>T7 T10"]
    F4 --> F5["Phase 5<br/>Bench integration<br/>T8 T9"]
    F5 --> F6["Phase 6<br/>Permanent wiring"]
    F6 --> F7["Phase 7<br/>Case assembly<br/>T8 after every step"]
    F7 --> F8["Phase 8<br/>Acceptance and endurance"]
```

## General rules

- **Add one thing at a time.** Add, test, then the next.
- **After every solder job, check for shorts before powering up:** multimeter on Ω, across 3V3–GND and BAT+–BAT−. Below a few Ω = short; don't plug in USB or the battery. With capacitors fitted the reading starts low and climbs; that's normal.
- **Never solder with the battery attached.** On the bench the battery + lead is a plug-in link (phase 4); in phase 6 the battery switch SW2 goes in the + lead, and after that turn SW2 off before any rework.
- **Write every measurement into the [log](#measurement-log).** Later phases use these values.

## Phase 0: Tools and software

**Tools:** fine-tip iron (~320–350 °C; a fat tip bridges the small XIAO pads), flux, 0.5 mm solder, multimeter (Ω, diode, DC V, DC mA), breadboard, 1×10 male header strip, tweezers/cutters/stripper, calipers, heat source for heat-shrink, optional 2×AA holder (3 V to test the motor alone).

**Software:**

1. Install **Arduino IDE 2**.
2. *File → Preferences → Additional boards manager URLs*: `https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json`
3. *Boards Manager*: install **Seeed nRF52 Boards** (tested 1.1.13). **Not** the "mbed-enabled" one: `VBAT_ENABLE`, `PIN_VBAT`, `systemOff()` and the Bluefruit BLE library come from the non-mbed core.
4. Board: **Seeed XIAO nRF52840 Sense**. The non-Sense board has no IMU.
5. *Library Manager*: **Adafruit GC9A01A** (tested 1.1.1) and **Seeed Arduino LSM6DS3** (tested 2.0.7). Accept their dependencies (Adafruit GFX, BusIO).
6. **Linux only:**

   ```bash
   pip3 install --user adafruit-nrfutil   # not shipped for Linux; even compiling calls it
   sudo apt install python-is-python3     # the core builds UF2 via 'python'
   sudo usermod -aG dialout $USER         # serial port access; log out and back in
   ```

**Upload stuck?** Double-tap the XIAO reset button. It enters the bootloader, shows up as a drive, and the serial port may change. Pick the new port and upload again.

## Phase 1: Incoming part check

Measure each part on its own before soldering anything.

| Part | How | Pass |
| --- | --- | --- |
| XIAO | Plug into USB, measure 3V3–GND | Computer sees it, 3V3 = 3.25–3.35 V |
| Display | Read pin labels; Ω across VCC–GND | Labels VCC GND SCL SDA DC CS RST; VCC–GND not shorted |
| Motor | Ω across leads, then 3 V from 2×AA for **1 s** | Neither open (OL) nor ~0 Ω; vibrates |
| BC337 | Diode mode, red on the middle leg (B), black on each outer leg; then both ways across the outer legs | B→E and B→C: 0.6–0.75 V; C↔E: OL both ways. Different readings = different pinout, check the datasheet. hFE 100–630 if your meter has a socket. |
| 1N4148 | Diode mode, red on the unbanded end (anode) | 0.5–0.7 V forward, OL reverse. **Band = cathode.** |
| B5819W (if used) | Same | 0.15–0.35 V forward |
| Resistors | Ω | 1 kΩ: 0.95–1.05 k; 10 kΩ: 9.5–10.5 k; 100 kΩ: 95–105 k |
| Capacitors | Capacitance mode if available | Roughly right; ceramic 100 µF may read tens of % low. **On tantalum the band is +**, the opposite of electrolytics. |
| AO3401A (Q2) | Diode mode, red on pin 3 (D), black on pin 2 (S); then reversed. SOT-23, pin 1 = G. | 0.5–0.8 V (body diode), OL reversed. Different: different pinout, check the datasheet. |
| Button | Ω across the legs | Pressed ~0 Ω, released OL |
| LiPo | DC V at the leads. **Don't let them touch.** | 3.6–4.2 V, + on red; protection PCB present; size ≤ 31 × 20.5 × 4.3 |

Don't use a battery below 3.0 V. Also finish the caliper list in [mechanical/README.md](../mechanical/README.md#to-measure-assumptions-in-the-cad).

**Pass:** every row passes and you know your BC337 pinout.

## Phase 2: XIAO alone

USB cable only, nothing soldered.

- **T1 Upload:** *File → Examples → 01.Basics → Blink*. **Pass:** red LED blinks once a second; uploads work every time.
- **T2 IMU** ([`tests/t2_imu`](tests/t2_imu/t2_imu.ino)). The IMU is on the internal `Wire1` bus; the library powers it. **Pass:** flat on the table `a` is 0.97–1.03 and `az` ≈ ±1; gyro below 5 at rest; turning 90° moves 1 g to another axis; a light shake gives `shake:1`, never at rest.
- **T3 BLE** ([`tests/t3_ble`](tests/t3_ble/t3_ble.ino)). **Pass:** nRF Connect lists "MysteryChain" and connects. Log the RSSI at 1 m; phase 7 compares against it.

## Phase 3: Bench prototype

**Don't solder headers to the XIAO.** The case has 4.51 mm for it, headers don't fit, and removing a 14-pin header later risks lifting pads. Instead, **solder the permanent wires to the pads now, but long** (~12 cm), with one male header pin on each free end (heat-shrunk) for the breadboard. In phase 6 you shorten them; the XIAO joints never change. Flag each wire with tape naming its pin.

Plug the display straight into the breadboard (pins down) and run the XIAO wires to its rows. Solder header pins to the motor leads.

Pad soldering: tin the pad, strip and tin 2 mm of wire, join, touch ~1 s. Check for bridges with a magnifier.

| XIAO pad | Bench | Permanent (phase 6) |
| --- | --- | --- |
| D0 | Button leg 1 (leg 2 → GND rail) | Button |
| D1 / D2 / D3 | Display CS / DC / RST | same |
| D6 | 1 kΩ → BC337 B | same |
| D8 / D10 | Display SCL / SDA | same |
| 3V3 (wire a) | Display VCC (through Q2 from phase 4) | Q2 source |
| 3V3 (wire b) | + rail | Motor driver + node |
| GND (wire a) | Display GND | same |
| GND (wire b) | − rail | Motor driver − node and button |
| D7 | Not connected until phase 4 | Q2 gate |

Two wires each on 3V3 and GND. D4, D5, D9 stay free. Nothing on 5V. Every display sketch drives D7 LOW (Q2 on); with nothing on D7 yet that's harmless.

- **T4 Display** ([`tests/t4_display`](tests/t4_display/t4_display.ino)), only the 7 display wires. **Pass:** colors come in the order printed on serial; edge circle equally thick and unbroken; counter runs **30 min** without snow, shift or freeze. Log the "full screen" time (~120–150 ms at 8 MHz).
- **T5 Button** ([`tests/t5_button`](tests/t5_button/t5_button.ino)), D0 ↔ GND. **Pass:** 20 presses count exactly 20; under 600 ms = SHORT, longer = LONG. "raw edges" above 2 is bounce the debounce already filters; replace the button if it's always above 10.
- **T6 Motor driver** ([`tests/t6_motor`](tests/t6_motor/t6_motor.ino)). Build it on the breadboard with your phase 1 pinout: D6 → 1 kΩ → B, 10 kΩ B–E, E → GND, motor + → 3V3, motor − → C, diode across the motor (**band to 3V3**), 100 nF + 100 µF across 3V3–GND.

  > **Don't reverse the diode.** Reversed, it shorts 3V3 to GND through the transistor as soon as it turns on. Check it again before powering up.

  During `h` (3 s on) measure: B–GND 0.7–0.85 V, C–E < 0.3 V, 3V3 ≥ 3.2 V. **Pass:** clear vibration on 1/2/3; no new "BOOT" line during `s`; BC337 no more than warm after the stress; readings in range.

## Phase 4: Battery, display switch and power button

> **LiPo safety:** never let the leads touch. Cut leads **one at a time** and insulate each right away. If the battery gets hot, swells or smells, disconnect and don't use it again.

On the bench the battery + lead plugs into the breadboard; pulling it out is the switch until SW2 goes in (phase 6).

1. Find the battery's + lead with the meter (don't trust the color) and flag it.
2. Solder a wire to the XIAO BAT− pad (underside) and join it to the battery − lead.
3. Solder a wire to the XIAO BAT+ pad and plug its free end into an empty breadboard row (the "BAT+ row").
4. Battery + lead: solder a male header pin on, heat-shrink up to the pin. **Not plugged in yet.** Whenever it's unplugged, cover the pin with tape.
5. Tape the battery wires to the XIAO with Kapton; a tug can rip the small BAT pads off.
6. No USB. Plug the battery pin into the BAT+ row and measure BAT+–BAT− at the XIAO: **positive battery voltage**. Negative? Pull the pin at once and fix it.

**Q2 on the bench.** Battery out, USB out. Put the AO3401A on a SOT-23 adapter (or solder three thin wires to it), using the pinout you checked in phase 1. Source → 3V3 rail, drain → display VCC (move the display VCC wire there from 3V3), gate → the D7 wire, 100 kΩ between gate and source. Short check, then USB and T4: it still runs (T4 drives D7 LOW).

**T7 Battery** ([`tests/t7_battery`](tests/t7_battery/t7_battery.ino)). The sketch drives P0.14 LOW first thing (see [Power](README.md#power)); the divider then draws only ~3 µA.

Calibrate: battery in, USB in, meter on BAT+/BAT−, compare with serial. `VBAT_CAL = meter / reading`; write it into T7, T8 and T10.

**Pass:** after calibration within 30 mV of the meter; "CHARGING" with USB in; "not charging" when full, VBAT 4.15–4.20 V (200 mAh at 50 mA charges from empty in ~4–5 h, battery no more than warm); unplug USB and T4 runs 30 min on battery.

**T10 Power button** ([`tests/t10_power`](tests/t10_power/t10_power.ino)). Set `VBAT_CAL`. Run on battery, no USB, unless the row says otherwise. At start-up the screen shows why it woke (`button`, `USB`, `reset`, `power on`).

| Test | Pass |
| --- | --- |
| Hold 3 s | "release" appears; let go: screen and backlight **fully dark** (check in a dark room: no glow) |
| Off, short tap | Stays dark |
| Off, hold ~1 s | Comes on, shows `button` |
| Hold 3 s and keep holding 10 s | Stays dark while held, doesn't come back on after release |
| Off, plug in USB | Comes on, shows `USB`. Unplug: stays on |
| On USB, hold 3 s | Turns off. Unplug and plug back in: comes on again |
| 20 × on/off | Comes on every time; never shows `power on` (that would be a supply dip when Q2 switches the display on) |
| Low battery: set `VBAT_OFF_MV = 4300` for one run | On battery: "LOW BAT", then off; a 1 s hold shows it again and turns off. On USB: stays on. Set it back to 3400. |

**Pass:** every row passes. Log the wake reasons you saw; if a button wake shows `0x0`, your bootloader clears the reset reason: a tap in the pocket then turns it on, and the wake check needs a flag of its own (e.g. in `NRF_POWER->GPREGRET2`, kept through System OFF).

## Phase 5: Bench integration

**T8 Integration** ([`tests/t8_integration`](tests/t8_integration/t8_integration.ino)). Everything at once, all status on screen so it also works with the case closed (phase 7). Shows reset reason and IMU status for 2 s at boot. If the uptime restarts, the board reset (likely a supply dip on a motor pulse while on battery). Run on USB first, then battery only.

| Test | Pass |
| --- | --- |
| 20 short presses | 20 motor pulses, `btn 20`, uptime never restarts |
| 3 long presses (30 pulses), **on battery** | Uptime never restarts, no screen glitches |
| Motor → IMU tail (USB, serial open) | Repeat short presses on the table and in hand. Largest "IMU tail" + 100 ms → `MOTOR_MASK_MS`. Pressing the button must not count shakes. |
| Shake | `shake` counter goes up, face toggles happy/sad |
| 30 min on battery | No freeze or reset; log the VBAT drop |

On the bench the motor sits loose; in the case it couples more vibration into the IMU, so `MOTOR_MASK_MS` is measured again in phase 7.

**T9 Current** ([`tests/t9_current`](tests/t9_current/t9_current.ino)). Meter on **DC mA** between the battery + pin and the BAT+ row (the meter replaces the link). **No USB.** Changing the range opens the circuit and resets the board: **clip a wire across the meter leads first**, change the range, then remove the clip. Each button press steps through the states:

| State | Estimate (not measured) | Measured |
| --- | --- | --- |
| 1. White screen | ~20–30 mA | |
| 2. Black screen | close to 1 (same backlight) | |
| 3. Display asleep | ~15–25 mA (backlight stays on) | |
| 4. Asleep + BLE advertising | slightly above 3 | |
| 5. Off (same as T10: Q2 off, flash asleep, System OFF) | **~3–15 µA**: switch to the µA range for this one | |

Battery life (h) ≈ 200 mAh × 0.8 / measured mA.

Off time (days) ≈ 200 mAh × 0.8 / state 5 µA / 24 × 1000. Before pressing the button again, clip the leads and go back to the mA range: a woken board on the µA range drops so much voltage across the meter that it resets.

Then flash a scene, turn it off with a 3 s hold and read the µA range again: T9 never starts the IMU, the scene does, so this catches anything the IMU bus leaves behind.

**Pass:** all 5 states measured, state 5 **below ~20 µA**, the button wakes it from state 5 (starts white). State 5 too high: ~25 µA more = flash not asleep; mA = Q2 still on (D7?), display fed through its pins, or the motor pin HIGH; a few µA more = tantalum C2 or flux leakage.

## Phase 6: Permanent wiring

The printed case must be ready. **Measure wire lengths in the printed case**, don't guess.

1. **Dry fit:** display, XIAO (no tape) and midframe in the case. Measure each wire route, add 5 mm. Routes: motor driver on +X beside GND/3V3, Q2 on +X beside D7/D8, button on −X, battery leads at the XIAO's antenna end, motor wires from the lug along the tab back.
2. **Battery out** (pin unplugged and taped) for all of this phase until step 9.
3. **Motor driver in free air (dead-bug):** shorten the BC337 legs and lay it on its side; 10 kΩ across B–E, 1 kΩ to B; diode band to the + node (3V3), other end to C; 100 nF and 100 µF across + and −. Heat-shrink every joint, then wrap the group in Kapton. **Total height ≤ 4.3 mm.**
4. **Q2 on its own spot** beside the XIAO's D7/D8 pads (+X, toward the antenna end; the motor group is already tight): gate straight to D7, 100 kΩ gate to source, source to 3V3 (the wire that used to feed the display), drain to the display VCC wire. SOT-23 is 2.9 × 1.3 × 1.1: heat-shrink the legs, Kapton over it.
5. Short check, then USB and T6 and T4 (Q2 switches the display).
6. **Remove the display header, shorten the display wires.** Pry the black plastic spacer off the header, cut the pins to ≤ 1 mm (don't desolder: pads can lift), solder the wires to the stubs and bend them flat along the PCB back. The case leaves 2.5 mm behind the tab. Short check, then T4. With short wires also try `SPI_HZ = 0` (library default); if clean, use it in T8 too.
7. **Button.** Short check, then T5.
8. **USB only:** T10 rows that work on USB (hold 3 s = off, plug in = on).
9. **Battery switch SW2, last joints:** find the EG1390 common pin with the meter (it connects to either side pin). Cut one side pin off. Dry-fit it on the midframe between its end stops, actuator through the slot, and measure the leads. Solder BAT+ pad wire → common first, then **slide SW2 off**, short check across BAT+–BAT−, and only then the battery + lead (header pin cut off) → side pin. Heat-shrink, Kapton. From here on: SW2 off before any rework.
10. **Full regression:** T8 on battery, every phase 5 test, then T10.

**Pass:** the T8 table passes end to end on battery with permanent wiring. No joint without heat-shrink.

## Phase 7: Case assembly

Same order as [mechanical assembly](../mechanical/README.md#assembly), with T8 running after every step. USB upload works with the case closed (if it's off, plugging in USB turns it on); the reset button faces the midframe: cover off, then a SIM pin through the Ø2 hole in the midframe (bootloader double-reset, frozen firmware). T8 has no off: flash T10 or a scene before you put it away.

| Step | Assembly | Then |
| --- | --- | --- |
| 1 | Display from the back: tab first, then PCB onto the step | T8 boots, text not clipped at the edge |
| 2 | Button into its wall hole | By hand: button clicks |
| 3 | Motor into the ring on the lug front skin | T8 short and long presses |
| 4 | XIAO taped to the pad, battery between the side ribs, SW2 taped down between its end stops (actuator through the slot), midframe in | T7: VBAT correct, no pinched wires. **Re-measure the IMU tail**, update `MOTOR_MASK_MS`. |
| 5 | Back cover, pressed flat | Closed-case tests below |

**Closed-case tests:** SW2 through the cover slot with a toothpick: off = dead (no button, no charging), on = boots; tape over the slot; the button works from outside and `btn` counts; with T10: 3 s hold = off, ~1 s hold = on; USB plug goes all the way in and "CHG" shows; 4 perimeter + 2 lug hooks seated, cover opens at the 230° notch; cover off, a SIM pin through the midframe hole resets it (T8 uptime restarts); T3 RSSI at 1 m vs. phase 2 (more than 10 dB worse: rethink antenna clearance to motor and display) and the distance where the link drops; T8 for 30 min then charge, case no more than warm.

**Pass:** all closed-case tests pass; final `MOTOR_MASK_MS` logged.

## Phase 8: Acceptance and endurance

| Test | How | Pass |
| --- | --- | --- |
| Full discharge | Charge fully, run T8 (or final firmware) without USB until the screen dies | Runtime within ±30% of the T9 estimate; last VBAT logged |
| Full charge | Empty battery, **off** (button, SW2 on), USB in for 5 h, then turn on (T10) | VBAT 4.15–4.20 V, battery not even warm. While on, "CHG" may never go out: the load keeps the charger from terminating. |
| Off drain | Full battery, off for 7 days, then on (T10) | VBAT a few tens of mV lower at most |
| 24 h run | T8 on USB | No freeze, uptime past 86400 |
| Shake | Shake hard for 1 min, then T8 | Nothing changed; no loose-wire symptoms (flicker, reset) |
| Drop | 5 × from 50 cm onto carpet, then T8 | Case stays closed, T8 intact |
| Keyring pull | Pull hard on the ring | Lug and bridge don't flex, T8 unaffected |
| Battery check | Open the cover after the tests | No swelling |

The XIAO has no hardware low-voltage cutoff. The firmware turns itself off below `VBAT_OFF_MV` (~3.4 V, T10) and then draws only µA; the battery's own PCM is the backstop.

**Carry into the final firmware:** `VBAT_CAL`, `MOTOR_MASK_MS`, `SPI_HZ`, pulse length, `VBAT_OFF_MV`, P0.14 always LOW, T10's `powerOff()` and wake check.

## Troubleshooting

| Symptom | Likely cause → fix |
| --- | --- |
| `adafruit-nrfutil` not found when compiling | Linux: `pip3 install --user adafruit-nrfutil`, restart the IDE |
| `exec: "python": executable file not found in $PATH` | `sudo apt install python-is-python3`, restart the IDE |
| `undefined reference to Serial` | Add `#include <Adafruit_TinyUSB.h>` (the test sketches have it) |
| No port, upload hangs | `dialout` group, a data-capable USB cable, double reset |
| Display all white or black | VCC/GND, swapped CS/DC, RST; try `SPI_HZ = 4000000` |
| Colors inverted | Add `tft.invertDisplay(false)` (or `true`) after `tft.begin()`; module-dependent |
| "NO IMU" | Board set to "Sense"? Non-mbed core installed? |
| Motor doesn't run | BC337 pinout; B–GND 0.7 V during `h`? Broken motor lead? |
| Motor always on | Missing 10 kΩ, or C and E swapped |
| Board resets on motor pulses (uptime restarts) | 100 µF close to the motor and 3V3 node? Battery cold or empty? Shorten the pulse, or soft-start with PWM in firmware. |
| Nonsense VBAT (0 or > 5000 mV) | P0.14 LOW? Battery attached? (On USB without a battery it reads the charger output.) |
| BLE not visible | Phone location/Bluetooth permission; try `setTxPower(4)` in T3 |
| Display dark after adding Q2 | D7 driven LOW before `tft.begin()`? Q2 source/drain swapped (phase 1 diode test)? 100 kΩ on the gate, not in series? |
| Turns itself back on right after "off" | Button still pressed when System OFF starts (the code waits for release: check the debounce); a bouncing button |
| No serial port while off | It's off: plug USB in again or hold the button ~1 s |
| Frozen, can't turn off | USB out, SW2 off and on. Or cover off, SIM pin through the midframe hole onto the XIAO reset button |
| Doesn't charge, doesn't turn on | SW2 off? |

## Measurement log

| Test | Measurement | Value | Date |
| --- | --- | --- | --- |
| Phase 1 | XIAO 3V3 (USB) | | |
| Phase 1 | BC337 pinout / hFE | | |
| Phase 1 | Battery voltage as received, measured size | | |
| T2 | `a` at rest / gyro offset | | |
| T3 | RSSI at 1 m (bare board) | | |
| T4 | Full-screen fill time (8 MHz / default) | | |
| T6 | V_BE / V_CE(sat) / 3V3 with motor on | | |
| T7 | `VBAT_CAL` | | |
| T7 | Charge time (empty to full) | | |
| T8 | `MOTOR_MASK_MS` (bench / case) | | |
| T8 | VBAT drop over 30 min | | |
| T9 | Current: state 1 / 2 / 3 / 4 / 5 | | |
| T10 | Wake reasons seen (button / USB) | | |
| Phase 7 | RSSI at 1 m (closed case) / drop-out distance | | |
| Phase 8 | Full discharge time / VBAT at shutdown | | |
