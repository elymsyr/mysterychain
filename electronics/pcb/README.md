# MysteryChain PCB rev A

KiCad 9 project for the custom board that replaces the XIAO nRF52840 Sense build.
Design: [2026-10-20-pcb-design.md](../../docs/superpowers/specs/2026-10-20-pcb-design.md).

## Case v8 inputs

Case coordinates: origin = display centre, +X right, +Y toward the lug, looking at the display.
Front = display side, back = midframe side. The board is 1.0 mm thick.

| Item | Value |
| --- | --- |
| Outline | `gen_pcb.outline()`: circle r 18.0, cut at Y +7.0, J1 tongue X ±5.6 down to Y −18.2, Ø3.0 notches at r 17.6 / 205° and 335° |
| J1 (USB-C, back) | face on Y −18.2, X 0, opening −Y; GCT USB4105, 3.31 mm above the back face |
| H1, H2 | NPTH Ø2.0 at (−6.2, −14.8) and (6.2, −14.8): case v8 locating pins take the USB plug force |
| SW1 (button, back) | C&K KMS221GP side-actuated, body centre (−16.22, 0), plunger tip at X −18.37, axis Y 0, about 0.7 mm above the back face (half the 1.42 mm body) |
| SW2 (battery switch, back) | EG1390 body centre (−10.55, −7.0), slide along Y; v7 had (−11.25, −7.0): its pads would cut the 205° notch, so the slot moves 0.7 mm +X |
| SW3 (reset, back) | C&K PTS810 centre (8.6, −11.7), 2.5 mm tall; the midframe reset hole moves here (no room left of J1) |
| E73 (back) | body X −3.9…14.1, Y −8.2…4.8, 3.0 mm tall, antenna end at +X |
| J3 battery / J4 motor pads | back, top edge: J3 at (−13.5, 5.5) B+ / B−, J4 at (−9.5, 5.5) M+ / M− |
| J2 display pads | front, top edge, X −15.6…−3.6 at Y 5.5, pad 1 (VCC) at X −3.6 |
| U2 (IMU, front) | LSM6DS3TR-C at (−11.0, −1.4), rotation 0, on the front: its Z axis points at the display, opposite the XIAO's |

## ERC warnings kept

`check.py sch` fails on any ERC error and on any warning type not listed here.

| Warning | Where | Why it stays |
| --- | --- | --- |
| `pin_to_pin` | U1 pad 33 (P0.13, bidirectional) + PWR_FLAG on IMU_PWR | The IMU is powered from a GPIO (spec: 0 µA when off). The flag tells ERC the rail has a source. |
| `pin_to_pin` | U2 pins 1/2 (SA0, SDx, bidirectional) + PWR_FLAG on GND | SA0 and SDx are tied to GND on purpose (address 0x6A, aux bus unused). |
