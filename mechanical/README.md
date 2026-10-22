# Mechanical

Round 3D-printed case, Ø42.2 × 13.7 mm, with a keyring lug. Three printed parts that snap together, no screws, plus a tethered TPU USB-C plug. Everything assembles from the back.

**Status:** case v7.2 (battery beside the XIAO, reaching into a full-depth lug; flat back cover). First PLA test print 2026-10-16; its fit fixes are in (v7.1, see version history), reprint pending. v6 (battery stacked behind the XIAO, 18.24 thick) is in `archive/pet_case_v6.py`. Power on/off is the button in firmware (hold 3 s / ~1 s), with Q2 cutting the display; see [electronics](../electronics/README.md#power).

| Front | Back | Internals (transparent) |
| --- | --- | --- |
| ![Front](renders/front.png) | ![Back](renders/back.png) | ![Internals](renders/internals.png) |

All dimensions below come from [`pet_case.step.py`](pet_case.step.py) (build123d). Parameters are at the top of that file.

## Files

| Path | Content |
| --- | --- |
| `pet_case.step.py` | Parametric source (build123d) |
| `pet_case.step` | Assembly STEP: printed parts + electronics + keep-out zones. Import into Onshape or any CAD. |
| `usb_plug.step.py` / `usb_plug.step` | Tethered TPU USB-C plug (splash protection). Strap anchors in the old switch recess + slot (the wall slide switch is gone since v6: daily on/off is the button in firmware, the battery switch SW2 sits under the back cover since v7). |
| `print_parts.py` | Exports the print files |
| `pet_case_drawing.pdf` | 2D technical drawings, A3 2:1, first-angle: assembly + parts list, front housing, midframe, back cover. Current (v7.2). |
| `pet_case.dxf.py` / `pet_case.dxf` | Drawing source (hidden-line projections of the model, ezdxf) and its DXF export with editable dimensions |
| `print/` | `.3mf` and `.stl` per part, already in print orientation |
| `parts/` | Vendor STEP models: Seeed XIAO nRF52840 Sense, Teyleten GC9A01 (`_nopins` = same model with the 7-pin header removed; the case uses this one) |
| `archive/pet_case_v2.py`, `_v4.py`, `_v5.py`, `_v6.py` | Earlier designs, kept for reference (v4 = header pins kept, 15.23 deep lug; v5 = 1.2 bezel, slide switch; v6 = battery stacked behind the XIAO) |
| `renders/` | Images |

## Overall dimensions

| | |
| --- | --- |
| Body diameter | **Ø42.2** (inside Ø38.6, wall 1.8) |
| Thickness (body and lug) | **13.74** |
| Total length (body + keyring lug) | **65.8** |
| Lug width | 26.96, full depth (holds the battery's top 22 mm) |
| Display window | Ø34.6 + 0.3 chamfer, offset 0.335 toward USB (centred on the glass) |
| Front lip | 0.4, covers the glass edge by 0.3 |
| Printed parts weight (PETG, 100% infill) | ~13 g max |

**Coordinates:** origin at the display center, z = 0 back face, z = 13.74 front face. Looking at the display: **+Y up** (keyring lug and display tab), **−Y down** (USB-C), **+X right** (USB plug anchor), **−X left** (button).

## Layer stack (front to back)

| Depth from front (mm) | Thickness | Layer |
| --- | --- | --- |
| 0 – 0.40 | 0.40 | Front lip, Ø34.6 window |
| 0.40 – 0.55 | 0.15 | Glass gap (print layer rounding + display tape spread) |
| 0.55 – 2.18 | 1.63 | LCD panel |
| 2.18 – 3.78 | 1.60 | Display PCB, sits on a step in the housing |
| 3.78 – 7.03 | 3.25 | Behind the display: **3.0 net** to the XIAO. The display's rear LDO is ≤ ~1.45. |
| 7.03 – 11.54 | 4.51 | XIAO: PCB, components and USB-C (4.21) + 0.3. **Beside it (y > 7.6): battery 7.54 – 11.54** |
| 11.54 – 12.54 | 1.00 | Midframe plate |
| 12.54 – 13.74 | 1.20 | Back cover |

The battery lies on the midframe beside the XIAO, not behind it. Its 0.5 swelling allowance points forward (to 7.04); behind the tab pads the wire joints need 2.5 (to 6.28), so 0.76 is spare. The thickness is set by the XIAO + the USB-C plug overmold (6.5 high, it reaches 0.15 short of the cover plane), not by the battery: a battery up to ~4.7 thick fits without changing anything.

## Printed parts

**Front housing** (42.2 × 65.8 × 13.7)

- Body and keyring lug in one piece, same thickness: keyring pull goes into the body, not into the PCB or the cover.
- **Front lip** 0.4 with a Ø34.6 window: shows the active area (Ø32.05) and ~1.3 of the black ring, and hides the glass edge (Ø35.2), the gap around it and the LCD neck/FPC. The flat front face carries the screen protector.
- Display seat: PCB step Ø38.6, LCD recess Ø36.0, clearance for the LCD neck/FPC bend (1.0 on every side, `NECK_GROW`) and the header's front solder joints (0.71 wall above them).
- **Lug cavity** 23.36 wide, open to the back, up to y = 38.08: display tab, battery and motor. Its front skin is 2.18 thick.
- **Motor ring** on the lug front skin past the tab tip: Ø10.5 inside, 1.0 high, 5 mm wire gap toward −Y.
- **USB-C pocket** 13 × 7, down to the receptacle face, so the plug overmold (USB-IF max 12.35 × 6.5) seats fully. 1.1 of wall under it at the back face.
- **USB plug anchor (+X):** the old switch opening, a 3.4 × 1.4 slot with a 6.5 × 3.6 × 0.9 recess outside. The plug's pad fills the recess and its barb snaps behind the wall.
- **Button (−X):** 3.6 × 2.0 hole and a Ø6.4 finger dimple; the plunger sticks out ~0.3 from the dimple floor but stays ~0.3 below the outer surface. It's also the power button (hold ~1 s = on), so a flat surface in a pocket can't hold it down.
- **Keyring slot** 10 × 3, oval, through the lug end. Bridge 2.4, web 1.2.
- Hook groove on the inner wall (R20.0, z = 4.6–6.8; its back edge sits 0.08 into the retention ramp, so the hooks pull the cover in), plus a 0.7 groove on each lug side wall (y = 31.8–36.4).
- Pry notch at 230° on the back edge, 5 mm wide.

**Midframe** (38.2 × 55.9 × 8.8)

- 1.0 plate: Ø38.2 disc + lug plate 22.96 wide, 0.2 clearance to the walls. The lug plate fixes the orientation (no key tabs).
- **Display clamp legs:** Ø2.4 at r = 17.6, at 40°/140°/205°/335° (clear of the battery). They press on the display PCB edge; the LCD floats.
- **XIAO:** two **pin rails** (x = ±6.75 … ±9.3, under both pin rows) with 7 + 7 Ø0.9 holes through rail and plate. The XIAO's header pins (spacer removed, cut 4.3 behind the PCB) press into them and carry the USB-C plug/unplug force. Rail tops stay 0.5 below the PCB for the pins' solder fillets. Plus the 8 × 8 tape pad (0.5 gap to the shield, 0.5 mm tape) and a 12 mm stop rib at the antenna end.
- **Battery side ribs** on the front face: 0.8 thick, 2 high, at x = ±10.5, y = 9.1–30.6. End stops: the XIAO stop rib side (1.5 lead bend space) and the lug end wall.
- **Backer block** behind the button takes the press force.
- **Battery switch** (EG1390) lies flat on the front face beside the XIAO's D0–D6 edge, x = −13 … −9.5, y = −10.5 … −3.5, between two 0.8 end stops that take the slide force. Its actuator goes back through a 3.4 × 1.4 slot in the midframe and the cover.
- **Reset hole** Ø2.0 over the XIAO reset button, 3.8 from the midframe back face to the button. Cover off, a SIM pin presses it: bootloader (double press), or a reset while USB is in.
- USB plug clearance on the −Y edge; notches where the cover hooks pass.

**USB plug** (TPU 95A, 29.4 × 25.2 × 9.3, [`usb_plug.step.py`](usb_plug.step.py))

- Body press-fits the 13 × 7 pocket (0.1 per side) and stops 0.4 short of the receptacle. The 0.8 flange over the pocket mouth seals on the outer wall (1.1 below the pocket: the flange stops at the back face).
- Strap runs a quarter turn along the outer wall to the +X recess. The barb through the slot holds it, so the plug can't get lost.

**Back cover** (38.4 × 57.0 × 6.1)

- Flat 1.2 plate, same outline as the midframe (which lies directly on it). 0.4 relief under the USB pocket for the plug body.
- **Battery switch slot** 3.4 × 1.4 (lower right, seen from the back). The actuator tip is 0.2 below the cover face: a toothpick or SIM pin moves it, a piece of tape over the slot can't. Keep 3–4 mm of flat plate around it for the tape.
- **Perimeter hooks:** 4 at **20° / 160° / 230° / 310°**, 4 mm wide. 0°/180° are at the plug slot / button height now, 270° hits the USB plug, 315° the passives.
- **Lug hooks:** 2 on the lug side walls at y = 34.1, 4 mm wide, 0.63 beside the battery (0.23 while flexing).
- Hook arm 0.7 thick, ~3.7 free length, 0.4 radial catch, 45° retention ramp (the cover can be removed).

## Component placement

| Component | Position | Held by |
| --- | --- | --- |
| Display module | Front center, tab toward +Y (lug) | Housing step (front) + midframe legs (back). No load on the wires. |
| XIAO | 3 mm behind the display, components facing back, USB-C on the −Y edge; x = 0, y = −18.2 … +4.3 | 14 header pins pressed into the midframe pin rails + tape pad + stop rib at the antenna end |
| Battery | Beside the XIAO, long side along Y: x = 0, y = 7.6 … 37.6, on the midframe front face; leads at the XIAO end | Side ribs + stop rib + lug end wall, thin foam tape; 0.5 side and 0.5 swelling clearance |
| Motor | Lug front skin past the tab tip, x = 0, y = 32.4, in front of the battery | Ring + the motor's own adhesive |
| USB plug (TPU) | −Y USB pocket, strap to the +X wall | Press fit + barb in the +X slot |
| Button | −X wall, same depth | Wall hole + backer block |
| Battery switch SW2 (EG1390) | Midframe front face, −X beside the XIAO: x = −13 … −9.5, y = −10.5 … −3.5; actuator −Z | Thin double-sided tape + 2 end stops |
| Motor driver (BC337, R, D, C) | Beside the XIAO on +X, at the GND/3V3 pads: x = 9.5–14, y = −12 … −4.5, height ≤ ~4.3 | Soldered in free air, heat-shrink + Kapton. BC337 lying on its side; dry-fit the group, it's tight. |
| Q2 (SOT-23) + 100 kΩ | Beside the XIAO on +X, at the D7/D8 pads: x = 9.5–13, y = −3 … +3, height ≤ ~4.3 | Same as above, on its own (`q2_zone`). Gate straight to D7. |
| Screen protector | Front face, centred on the window | Its own adhesive on the flat front face |

## Clearances

| Where | Gap (mm) |
| --- | --- |
| Display PCB ↔ housing (radial) | 0.25 |
| Display tab ↔ pocket | 0.25 |
| Polarizer ↔ front lip | 0.15 (glass edge 0.28 in the model) |
| Front lip over the glass edge | 0.3 radial |
| Display PCB ↔ XIAO | 3.00 net |
| XIAO components ↔ midframe | 0.30 |
| Midframe and cover ↔ wall | 0.20 |
| Battery (sides / swelling) | 0.5 / 0.5, then 0.76 to the tab wire zone |
| Motor ↔ display tab tip | ~0.9 |
| USB pocket ↔ plug overmold | 0.33 sides, 0.25 top/bottom |
| Button hole | ~0.2–0.3 |

If the hooks are too tight or too loose, move the groove in 0.1 mm steps (`CATCH_Z0`, `GROOVE`). XIAO pins too tight / loose in the rails: `PIN_HOLE_D` in 0.05 steps.

## Assembly

1. **XIAO pins:** insert a 2 × 7 male header (2.54) from the component side, solder it on the front, pull the black spacer off, cut the pins flush at the front (they face the display) and to **4.3 mm behind the PCB** (component side). Wires go onto the same pads from the front.
2. **Solder everything else** (see [electronics bring-up](../electronics/bring-up.md)). Remove the display's 7-pin header first: pry the black plastic spacer off, cut the pins to ≤ 1 mm, solder the wires to the stubs and bend them flat along the PCB back toward the body (≤ 2.5 behind the PCB).
3. **Display:** from the back. Tab into the lug pocket first, then seat the PCB on the step.
4. **Motor:** into the ring on the lug front skin, adhesive side down, wires out through the gap toward the XIAO.
5. **Button:** into its wall hole from inside.
6. **Midframe:** XIAO pins pressed into the rails (straight down, until the shield sits on the tape pad), battery between the side ribs (thin foam tape), leads bent at the XIAO end. Battery switch taped between its end stops, actuator into the slot; slide it once to check. Insert lug plate first; the backer block locks the button.
7. **Back cover:** press it flat. 4 perimeter + 2 lug hooks click in.
8. **USB plug:** push the barb through the +X slot from outside (tweezers help), seat the pad in the recess, then press the body into the USB pocket.
9. **Screen protector:** clean the front face with IPA, centre a Ø38–40 round protector on the window, press the rim down all around.

To open: lift at the 230° notch with a fingernail or a small screwdriver.

> The XIAO's reset button and LEDs face the midframe and can't be reached with the case closed. For a reset or a bootloader double-reset, open the cover and press through the midframe's reset hole with a SIM pin. Charge status can be read from P0.17 and shown on screen.

## Printing

| Part | Face on the bed | Supports |
| --- | --- | --- |
| `front_housing` | Front face (clean window chamfer) | Probably none (USB and side holes are short bridges) |
| `midframe` | Plate, legs and ribs up | None |
| `back_cover` | Plate, hooks up | None |
| `usb_plug` (TPU 95A) | As modelled, flange + strap edge down | Small: under the plug body (1.0 above the bed) and the barb. Tune `BODY_FIT` / `BARB_OVER` after the first print. |

PETG (hook arms are 0.7 mm, PLA may snap), 0.4 mm nozzle, 0.12–0.16 mm layers, 3–4 walls, 30% infill. Open the 3MF in Bambu/Orca/Prusa/Cura; send the STL to a print service.

On the first print check: hook fit, display seating on its step, USB-C plug going all the way in.

## Splash protection (IPX4 goal, not tested)

| Opening | Sealed by |
| --- | --- |
| Display window | Screen protector bonded on the flat front face (2–3 mm adhesive band around the window) |
| USB-C pocket | TPU plug flange |
| Old switch slot | TPU plug pad + barb |
| Button | **Open:** needs a cover sticker or TPU cap |
| Battery switch slot | Piece of tape on the flat back cover (peel to switch) |
| Back cover seam | **Open:** snap fit only, no gasket. Since v7 the seam is flat (one plane), so a gasket is possible. |

## To measure (assumptions in the CAD)

| Item | CAD value | Source |
| --- | --- | --- |
| Display PCB diameter / thickness | Ø38.1 / 1.59 | Teyleten STEP, may vary by vendor |
| Display tab width / module length | 22.86 / 45.5 | STEP |
| Wire joints behind the tab pads | ≤ 2.5 | Assumed (header removed, stubs ≤ 1). `TAB_REAR` |
| Components behind the display | ≤ 1.45 (SOT-23 LDO) | Not measured |
| Battery | 20 × 30 × 4 incl. PCM, leads at one 20 mm end | Name "402030"; measure when bought (`BAT_L` sets the case length 1:1) |
| Display stack, PCB front → glass front | 1.626 | Vendor STEP. Measure T (PCB back → glass front) and t (PCB): set `LCD_STACK = T − t`. The lip gap stays 0.15. |
| Button | 2.5 × 6 × 3.5 body + 1.8 plunger | Assumed 3 × 6 × 4.3 |
| Motor | Ø10 × 3 | Product name |

## Regenerate

Uses the [text-to-cad](https://github.com/earthtojake/text-to-cad) `cad` skill scripts and a Python env with build123d. Run from this folder:

```bash
SKILLS=/path/to/text-to-cad/skills   # your text-to-cad checkout
C=$SKILLS/cad/scripts
~/.venvs/cad/bin/python $C/gen pet_case.step.py usb_plug.step.py --write   # STEP
~/.venvs/cad/bin/python print_parts.py                     # STL + 3MF (all four parts)
~/.venvs/cad/bin/python $SKILLS/dxf/scripts/gen pet_case.dxf.py --write   # DXF
~/.venvs/cad/bin/python pet_case.dxf.py                    # drawing PDF
~/.venvs/cad/bin/python $C/inspect interfere pet_case.step --tolerance 0.0001 --format text
```

Expected interference: battery inside its safe zone, TPU plug inside the USB overmold keep-out (they never coexist), TPU plug press fit in the housing (~6.6 mm³), display tab resting on the lug floor (~0 mm³), hook catches preloading the housing groove edge (~0.1 mm³).

## Version history

| Version | Change |
| --- | --- |
| v1 | Three-part case: midframe, snap-on cover, battery safe zone |
| v2 | Real display STEP. Tab pocket opened to the back. USB pocket fixed so the plug seats fully. |
| v3 | Ear removed. Shallow stepped tab. Keyring slot made one piece with the body. |
| v4 | Battery 402030. Display–XIAO gap 3.0 mm. Hook angles 0/180/225/315. Hook notches in the midframe. |
| v5 | Display header removed (`gc9a01_teyleten_nopins.step`). Lug 15.23 → 8.43 deep. Tab-tip hooks moved beyond the tab tip. Cover wall fill flush with the inner wall. |
| v6 | Bezel 1.2 → 0.4 front lip, window Ø33 → Ø34.6: only the black glass shows, glass edge and neck hidden. Body 19.19 → 18.24. Slide switch removed: its opening anchors the new tethered TPU USB plug, midframe switch backer dropped. Screen protector on the front face. |
| v7 | Battery moved beside the XIAO, long side into the lug: thickness 18.24 → 13.74, length 54.4 → 65.8, same 402030. Lug full depth; flat back cover and midframe (lug plate keys them, key tabs dropped); cover posts dropped. Motor moved to the lug front skin. Hooks 0/180/225/315 → 20/160/230/310 + 2 lug side hooks (tab-tip hooks dropped). Midframe and cover print without supports. Reset hole in the midframe; Q2 gets its own spot beside D7. Battery switch back (EG1390, actuator through a slot in midframe + cover, under tape). |
| v7.1 | PLA test-fit fixes: hook groove 0.6 toward the front (z 4.0–6.2 → 4.6–6.8) so the hooks preload the cover (it sat 0.52 loose); LCD neck/FPC pocket 1.0 larger all round (was +0.35). Drawings ported to v7. |
| v7.2 | XIAO held by its header pins: two pin rails on the midframe, 14 Ø0.9 holes (tape alone would creep under repeated USB-C plugging). Tape pad kept for Z and vibration. |
