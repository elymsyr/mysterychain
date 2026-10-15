"""MysteryChain - case assembly (v6: 0.4 front lip hides the glass edge, slide switch -> tethered TPU USB plug).

Coordinates: origin = display center, back face z=0, front (display) face z=H, +Z toward the viewer.
Looking at the display: +X right, +Y up (display tab + keyring lug at the top).
Depths "d" are measured inward from the front face (mm); z = H - d.

Layers (front to back):
  bezel | display (LCD) | display PCB | rear component clearance + display wires |
  XIAO + motor + button + passives | midframe (deck) | battery | back cover

Assembly order (all from the back): display (tab into the pocket on the lug side) -> midframe (+XIAO, motor) ->
button -> battery -> back cover (tab tip + 4 perimeter hooks). USB plug (TPU, usb_plug.step.py) last, from outside.

Load paths:
  - Display: back cover posts -> midframe -> midframe legs -> display PCB -> housing step (LCD floats).
  - USB insertion: plug pushes the XIAO to +Y -> midframe stop rib.
  - Button press: button body -> midframe backer block.
  - Keyring: slot + bridge at the tab end, one piece with the body (not through the cover / PCB).

Splash (IPX4 goal): round screen protector bonded on the flat front face over the window,
TPU plug in the USB pocket, its barb seals the old switch slot; power on/off in firmware.
"""

import math
from pathlib import Path

from build123d import (
    Align, Box, Compound, Cone, Cylinder, Location, Plane, Polygon, Pos, RectangleRounded, Rot, extrude,
)
from cadgen import srgb
from cadgen.assembly import AssemblyHelper
from cadgen.sources import load_source_module
from cadgen.step_scene import import_step

HERE = Path(__file__).resolve().parent
MIN = (Align.CENTER, Align.CENTER, Align.MIN)

# ---------------------------------------------------------------- dimensions (mm)
# GC9A01 1.28" round module (measured from the Teyleten Robot STEP)
DISP_STEP = HERE / "parts" / "gc9a01_teyleten_nopins.step"   # vendor STEP minus the 7-pin header
DISP_PCB_D = 38.1
MODULE_L = 45.5          # round PCB + rectangular tab, total length
TAB_W = 22.86
DISP_PCB_T = 1.6          # 1.5875 + mounting-hole pad rings (pads sit on the step)
LCD_D = 35.6             # LCD panel (with neck), offset LCD_DY from the PCB center
LCD_DY = -0.335
LCD_STACK = 1.626        # PCB front -> polarizer front (model: no tape)
GLASS_GAP = 0.15         # polarizer - lip gap: layer rounding of the lip/step faces (~±0.08) + tape spread
DISP_REAR = 3.25         # display PCB back -> XIAO = 3.0 net (XIAO STEP bottom protrusion 0.25); rear SOT-23 LDO <=1.45
WIN_D = 34.6             # window: active area Ø32.05 + black ring, lip covers the glass edge (Ø35.2) by 0.3
TAB_REAR = 2.5           # behind the tab pads: header removed, solder/pin stubs <=1 + 30AWG wires bent flat
SOLDER_HALF_X, SOLDER_Y, SOLDER_H = 8.52, (24.36, 26.14), 1.164   # front-side solder joints

# XIAO nRF52840 Sense (measured from the official Seeed STEP)
XIAO_STEP = HERE / "parts" / "xiao_nrf52840_sense.step"
XIAO_PCB_CX, XIAO_PCB_CZ = 1.805, -6.11   # STEP-local PCB center (x, z)
XIAO_USB_FACE_X = 13.81                   # STEP-local USB-C front face
XIAO_TOP_Y = 4.21                         # STEP-local highest point (USB-C)
XIAO_SHIELD_Y = 3.0
XIAO_USB_CY = 2.11                        # USB-C height center
USB_FACE_Y = -18.2                        # USB-C front face in the assembly (world Y)

MOTOR_D, MOTOR_T = 10.0, 3.0
MOTOR_Y = 11.8

# old EG1390 slide switch opening (+X): switch removed, on/off in firmware; slot + recess anchor the USB plug
SW_ACT_W, SW_TRAVEL = 1.5, 1.5
# 3x6 tact button
BTN_BODY = (2.5, 6.0, 3.5)
BTN_PLUNGER = (1.8, 3.0, 1.5)

# LiPo 402030 (~200 mAh) - chosen, not bought yet
BAT_W, BAT_L, BAT_T = 20.0, 30.0, 4.0
BAT_SWELL = 0.5            # swelling allowance (~10% of thickness)
BAT_GAP = 0.5              # side clearance

# case
FIT = 0.25                 # display PCB - case radial clearance
WALL = 1.8
BEZEL_T = 0.4              # front lip (3 layers, printed on the bed)
DECK_T = 1.0
COVER_T = 1.2
KEY_WEB, KEY_SLOT_W, KEY_SLOT_L, KEY_BAR = 1.2, 3.0, 10.0, 2.4   # tab pocket | slot | bridge

# ---------------------------------------------------------------- derived
RI = DISP_PCB_D / 2 + FIT
RO = RI + WALL
D_PCB_FRONT = BEZEL_T + GLASS_GAP + LCD_STACK          # display PCB sits on the case step
D_PCB_BACK = D_PCB_FRONT + DISP_PCB_T
D_XIAO_BOT = D_PCB_BACK + DISP_REAR
D_DECK_FRONT = D_XIAO_BOT + XIAO_TOP_Y + 0.3
D_DECK_BACK = D_DECK_FRONT + DECK_T
D_COVER_FRONT = D_DECK_BACK + BAT_T + BAT_SWELL
H = D_COVER_FRONT + COVER_T


def z(d):
    return H - d


Z_DF, Z_DB = z(D_DECK_FRONT), z(D_DECK_BACK)
Z_MID = z((D_XIAO_BOT + D_DECK_FRONT) / 2)      # button / plug anchor height center
TAB_HALF = TAB_W / 2 + FIT
TAB_TOP = MODULE_L - DISP_PCB_D / 2 + FIT
KEY_SLOT_CY = TAB_TOP + KEY_WEB + KEY_SLOT_W / 2
LUG_HALF, LUG_TOP = TAB_HALF + WALL, TAB_TOP + KEY_WEB + KEY_SLOT_W + KEY_BAR
# tab region only wraps the tab + its wires: its back face is shallower than the body (stepped)
D_LUG_IN = D_PCB_BACK + TAB_REAR
Z_LUG_BACK = z(D_LUG_IN + COVER_T)

# XIAO placement: component side back (-Z), USB-C down (-Y)
XIAO_ORIGIN = (-XIAO_PCB_CZ, USB_FACE_Y + XIAO_USB_FACE_X, z(D_XIAO_BOT))
XIAO_PCB_TOP_Y = XIAO_ORIGIN[1] + 8.67     # antenna end (STEP-local x min = -8.67)
Z_USB = XIAO_ORIGIN[2] - XIAO_USB_CY

# USB-C plug overmold (USB-IF max 12.35 x 6.5) + clearance; pocket floor = receptacle face -> plug seats fully
USB_POCKET_W, USB_POCKET_H = 13.0, 7.0

# snap hooks (back cover <-> front housing)
CATCH_Z0 = COVER_T + 3.7         # catch tip (45° retention ramp, removable)
CATCH_Z1 = CATCH_Z0 + 1.2        # arm tip (lead-in ramp)
GROOVE = (CATCH_Z0 - 0.9, CATCH_Z1 + 0.1)
HOOK_W = 4.0
HOOK_ANGLES = (0, 180, 225, 315)   # 270 hits the USB plug, 240/300 hit battery corners
TAB_HOOK_X, TAB_HOOK_W = 10.0, 2.0   # tab-tip hooks sit outside the pad row (±7.95)
TAB_HOOK_R = TAB_TOP + 1.35          # catch wall beyond the tab tip: arms clear the PCB by 0.3 (0.15 deflected)
TAB_DZ = Z_LUG_BACK                  # tab hooks rise from the stepped cover plate
GROOVE_R = RI + 0.7               # >=0.2 clearance including outer corners


def polar(r, deg):
    a = math.radians(deg)
    return r * math.cos(a), r * math.sin(a)


def sector_box(r0, r1, width, z0, z1, deg):
    """Straight box, radial r0..r1, tangential width, z0..z1, at angle deg."""
    return Rot(0, 0, deg) * Pos((r0 + r1) / 2, 0, (z0 + z1) / 2) * Box(r1 - r0, width, z1 - z0)


def zbox(x0, x1, y0, y1, z0, z1):
    x0, x1 = sorted((x0, x1))
    y0, y1 = sorted((y0, y1))
    return Pos((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2) * Box(x1 - x0, y1 - y0, z1 - z0)


def zcyl(x, y, r, z0, z1):
    return Pos(x, y, z0) * Cylinder(r, z1 - z0, align=MIN)


# ---------------------------------------------------------------- front housing
def make_front_housing():
    body = Cylinder(RO, H, align=MIN)
    lug = Pos(0, LUG_TOP / 2, Z_LUG_BACK) * extrude(
        RectangleRounded(2 * LUG_HALF, LUG_TOP, 3.0), H - Z_LUG_BACK)
    body = body.fuse(lug).clean()

    sw_y = SW_ACT_W + SW_TRAVEL + 0.4
    cuts = [
        zcyl(0, LCD_DY, WIN_D / 2, z(BEZEL_T) - 0.1, H + 1),                  # window
        Pos(0, LCD_DY, H - 0.2) * Cone(WIN_D / 2, WIN_D / 2 + 0.3, 0.3, align=MIN),  # window chamfer
        zcyl(0, LCD_DY, LCD_D / 2 + 0.2, z(D_PCB_FRONT), z(BEZEL_T)),         # LCD recess
        # LCD neck + FPC bend (on the tab, PCB front side)
        extrude(Polygon((-8.85, 14.5), (8.85, 14.5), (8.85, 15.3), (6.95, 19.97), (6.95, 21.2),
                        (-6.95, 21.2), (-6.95, 19.97), (-8.85, 15.3), align=None),
                z(BEZEL_T) - z(D_PCB_FRONT)).moved(Location((0, 0, z(D_PCB_FRONT)))),
        # header front-side solder joints
        zbox(-SOLDER_HALF_X - 0.4, SOLDER_HALF_X + 0.4, SOLDER_Y[0] - 0.4, SOLDER_Y[1] + 0.4,
             z(D_PCB_FRONT) - 0.1, z(D_PCB_FRONT) + SOLDER_H + 0.3),
        zcyl(0, 0, RI, -1, z(D_PCB_FRONT)),                                  # main cavity
        # tab + header + wire pocket, open to the back (display inserts from the back)
        zbox(-TAB_HALF, TAB_HALF, 0, TAB_TOP, -1, z(D_PCB_FRONT)),
        zcyl(0, 0, GROOVE_R, GROOVE[0], GROOVE[1]),                          # hook groove
        # tab-tip hook recesses (arms rise beside the PCB tip, past the shallow tab cavity) + catch grooves
        *[zbox(x - 1.5, x + 1.5, TAB_TOP - 0.1, TAB_HOOK_R, -1, TAB_DZ + CATCH_Z1 + 0.3)
          for x in (-TAB_HOOK_X, TAB_HOOK_X)],
        *[zbox(x - 1.5, x + 1.5, TAB_TOP, TAB_HOOK_R + 0.7, TAB_DZ + GROOVE[0], TAB_DZ + GROOVE[1])
          for x in (-TAB_HOOK_X, TAB_HOOK_X)],
        # USB-C plug overmold pocket, down to the receptacle face
        zbox(-USB_POCKET_W / 2, USB_POCKET_W / 2, -RO - 1, USB_FACE_Y,
             Z_USB - USB_POCKET_H / 2, Z_USB + USB_POCKET_H / 2),
        # USB plug anchor: old switch slot + thumb recess
        zbox(RI - 1, RO + 1, -sw_y / 2, sw_y / 2, Z_MID - 0.7, Z_MID + 0.7),
        zbox(RO - 0.9, RO + 1, -3.25, 3.25, Z_MID - 1.8, Z_MID + 1.8),
        # button hole + finger dimple
        zbox(-RO - 1, -RI + 1, -1.8, 1.8, Z_MID - 1.0, Z_MID + 1.0),
        Pos(-RO - 1, 0, Z_MID) * Rot(0, 90, 0) * Cylinder(3.2, 1.6, align=MIN),
        # key ring / chain slot
        Pos(0, KEY_SLOT_CY, -1) * extrude(
            RectangleRounded(KEY_SLOT_L, KEY_SLOT_W, KEY_SLOT_W / 2 - 0.05), H + 2),
        # cover pry notch
        sector_box(RI - 0.5, RI + 1.0, 5.0, -1, 0.8, 225),
    ]
    # midframe key channels (lock rotation, one-way fit)
    cuts += [sector_box(RI - 0.5, RI + 0.7, 4.0, -1, Z_DF + 0.3, a) for a in (45, 135)]
    return body.cut(*cuts).clean()


# ---------------------------------------------------------------- midframe
def make_midframe():
    deck = zcyl(0, 0, RI - 0.2, Z_DB, Z_DF)
    adds = [sector_box(RI - 0.4, RI + 0.5, 3.4, Z_DB, Z_DF, a) for a in (45, 135)]
    # display clamp legs (press on the PCB edge ring)
    adds += [zcyl(*polar(17.6, a), 1.2, Z_DF - 0.1, z(D_PCB_BACK)) for a in (60, 120, 205, 335)]
    # XIAO: tape pad on the shield + stop rib at the antenna end
    shield_top = XIAO_ORIGIN[2] - XIAO_SHIELD_Y
    adds += [
        zbox(-4, 4, XIAO_ORIGIN[1] - 4.05, XIAO_ORIGIN[1] + 3.95, Z_DF - 0.1, shield_top - 0.5),
        zbox(-6, 6, XIAO_PCB_TOP_Y + 0.15, XIAO_PCB_TOP_Y + 1.3, Z_DF - 0.1, z(D_XIAO_BOT + 0.2)),
    ]
    # motor cup (ring with wire notch)
    ring = zcyl(0, MOTOR_Y, MOTOR_D / 2 + 1.0, Z_DF - 0.1, Z_DF + MOTOR_T + 0.2)
    ring = ring.cut(
        zcyl(0, MOTOR_Y, MOTOR_D / 2 + 0.25, Z_DF, Z_DF + 5),
        zbox(-1.0, 1.0, MOTOR_Y - 7, MOTOR_Y - 4, Z_DF, Z_DF + 5),
    )
    adds.append(ring)
    # button backer block (takes the press load)
    btn_in = RI - (RI - math.sqrt(RI**2 - (BTN_BODY[1] / 2) ** 2)) - 0.05 - BTN_BODY[0]
    adds.append(zbox(-(btn_in - 0.1), -14.6, -3.4, 3.4, Z_DF - 0.1, Z_MID + BTN_BODY[2] / 2))
    # battery bay ribs (back face)
    bx, by = BAT_W / 2 + BAT_GAP, BAT_L / 2 + BAT_GAP
    rib_h = 2.0
    for s in (-1, 1):
        adds.append(zbox(s * bx, s * (bx + 0.8), -8, 8, Z_DB - rib_h, Z_DB + 0.1))
        adds.append(zbox(-7, 7, s * by, s * (by + 0.8), Z_DB - rib_h, Z_DB + 0.1))
    deck = deck.fuse(*adds).clean()
    cuts = [
        zbox(10, 13.5, 9, 12.5, Z_DB - 5, Z_DF + 5),            # battery wire pass-through
        zbox(-USB_POCKET_W / 2 - 0.2, USB_POCKET_W / 2 + 0.2, -RI - 1, USB_FACE_Y + 0.2,
             Z_DB - 1, Z_DF + 1),                                 # USB plug clearance
    ]
    # hook arms pass the midframe edge (keeps arm length)
    cuts += [sector_box(RI - 1.4, RI + 0.5, HOOK_W + 0.8, Z_DB - 1, Z_DF + 1, a) for a in HOOK_ANGLES]
    return deck.cut(*cuts).clean()


# ---------------------------------------------------------------- back cover
def make_back_cover():
    th = TAB_HALF - 0.2
    # stepped cover: disc + fill for the body wall notch + tab back plate (battery bay left open)
    notch = zbox(-th, th, 0, RO + 1, 0, Z_LUG_BACK).intersect(Cylinder(RO, H, align=MIN))
    plate = zbox(-th, th, 0, TAB_TOP - 0.2, Z_LUG_BACK, Z_LUG_BACK + COVER_T).fuse(*[
        zbox(x - 1.3, x + 1.3, TAB_TOP - 0.3, TAB_HOOK_R - 0.2, Z_LUG_BACK, Z_LUG_BACK + COVER_T)
        for x in (-TAB_HOOK_X, TAB_HOOK_X)])   # tongues under the tab-tip hook arms
    cover = Cylinder(RI - 0.2, COVER_T, align=MIN).fuse(
        notch.fuse(plate).cut(zcyl(0, 0, RI, COVER_T, H)))   # flush with the wall: midframe keeps 0.2
    adds = []
    # (wall, angle, x offset, width, base z): 4 perimeter hooks + 2 tab-tip hooks
    hooks = [(RI, a, 0, HOOK_W, 0) for a in HOOK_ANGLES]
    hooks += [(TAB_HOOK_R, 90, x, TAB_HOOK_W, TAB_DZ) for x in (-TAB_HOOK_X, TAB_HOOK_X)]
    for r_wall, a, dx, w, dz in hooks:
        r_out = r_wall - 0.35
        c0, c1 = dz + CATCH_Z0, dz + CATCH_Z1
        adds.append(Pos(dx, 0, 0) * sector_box(r_out - 0.7, r_out, w, dz + COVER_T - 0.1, c1, a))
        profile = Plane.XZ * Polygon(
            (r_out - 0.1, c0 - 0.8), (r_wall + 0.4, c0),
            (r_wall + 0.4, c0 + 0.2), (r_out - 0.1, c1), align=None)
        adds.append(Pos(dx, 0, 0) * Rot(0, 0, a) * extrude(profile, amount=w / 2, both=True))
    # posts pressing on the midframe (clear of battery corners)
    adds += [zcyl(*polar(17.9, a), 1.0, COVER_T - 0.1, Z_DB) for a in (30, 150, 210, 330)]
    return cover.fuse(*adds).clean()


# ---------------------------------------------------------------- components
def make_display():
    # STEP-local: +Y display normal, +Z tab, PCB back face Y=0, PCB center at origin
    disp = Compound(import_step(str(DISP_STEP)).solids())
    return Pos(0, 0, z(D_PCB_BACK)) * Rot(0, 0, 180) * Rot(90, 0, 0) * disp


def make_xiao():
    plane = Plane(origin=XIAO_ORIGIN, x_dir=(0, -1, 0), z_dir=(1, 0, 0))
    # flatten the vendor STEP hierarchy (single labeled body)
    return Compound(import_step(str(XIAO_STEP)).solids()).moved(Location(plane))


def make_button():
    bx, by, bz = BTN_BODY
    px, py, pz = BTN_PLUNGER
    x_out = -(math.sqrt(RI**2 - (by / 2) ** 2) - 0.05)
    body = zbox(x_out, x_out + bx, -by / 2, by / 2, Z_MID - bz / 2, Z_MID + bz / 2)
    plunger = zbox(x_out - px, x_out + 0.1, -py / 2, py / 2, Z_MID - pz / 2, Z_MID + pz / 2)
    return body.fuse(plunger)


def gen_step():
    asm = AssemblyHelper("mysterychain")
    asm.add(make_front_housing(), "front_housing", color=srgb("#EDEAE2", 0.55))
    asm.add(make_midframe(), "midframe", color=srgb("#4F7CAC"))
    asm.add(make_back_cover(), "back_cover", color=srgb("#7A8B99"))
    # loaded here: usb_plug.step.py imports this file
    plug = load_source_module(HERE / "usb_plug.step.py").make_usb_plug()
    asm.add(plug, "usb_plug_tpu", color=srgb("#E07A3A"))

    motor = zcyl(0, MOTOR_Y, MOTOR_D / 2, Z_DF, Z_DF + MOTOR_T)
    battery = zbox(-BAT_W / 2, BAT_W / 2, -BAT_L / 2, BAT_L / 2, Z_DB - BAT_T, Z_DB)
    asm.add_module("electronics", [
        asm.feature(make_display(), "gc9a01_display_module", color=srgb("#1F5FA8")),
        asm.feature(make_xiao(), "xiao_nrf52840_sense"),
        asm.feature(motor, "vibration_motor", color=srgb("#B5B5B5")),
        asm.feature(make_button(), "tact_button", color=srgb("#333333")),
        asm.feature(battery, "lipo_402030", color=srgb("#C9A227")),
    ])
    asm.add_module("keepout", [
        asm.feature(
            zbox(-BAT_W / 2 - BAT_GAP, BAT_W / 2 + BAT_GAP, -BAT_L / 2 - BAT_GAP, BAT_L / 2 + BAT_GAP,
                 Z_DB - BAT_T - BAT_SWELL, Z_DB),
            "battery_safe_zone", color=srgb("#E53935", 0.25)),
        asm.feature(zbox(9.5, 14.0, -12.0, -4.5, Z_DF, z(D_XIAO_BOT + 0.2)),
                    "passives_zone", color=srgb("#FF9800", 0.3)),
        # fully inserted USB-C plug overmold (USB-IF max), front face at the receptacle face
        asm.feature(zbox(-6.175, 6.175, USB_FACE_Y - 0.02, USB_FACE_Y - 12, Z_USB - 3.25, Z_USB + 3.25),
                    "usb_c_plug_overmold", color=srgb("#9C27B0", 0.3)),
    ])
    return asm.build()


if __name__ == "__main__":
    print(f"H={H:.2f}  OD={2 * RO:.2f}  height={RO + LUG_TOP:.2f}  lug_depth={H - Z_LUG_BACK:.2f}  "
          f"tab_cavity_depth={D_LUG_IN:.2f}  cover_front_depth={D_COVER_FRONT:.2f}")
