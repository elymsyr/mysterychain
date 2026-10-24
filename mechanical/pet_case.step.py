"""MysteryChain - case assembly (v7: battery beside the XIAO, reaching into the lug; flat back cover).

Coordinates: origin = display center, back face z=0, front (display) face z=H, +Z toward the viewer.
Looking at the display: +X right, +Y up (display tab + keyring lug at the top).
Depths "d" are measured inward from the front face (mm); z = H - d.

Layers (front to back):
  bezel | display (LCD) | display PCB | rear component clearance + display wires (+ motor in the lug) |
  XIAO + button + passives  |  battery (beside the XIAO, under the tab, into the lug) | midframe (deck) | back cover

Assembly order (all from the back): display (tab into the lug pocket) -> motor (adhesive, ring on the lug skin) ->
button -> midframe (+XIAO, battery) -> back cover (4 perimeter + 2 lug side hooks).

Load paths:
  - Display: back cover -> midframe deck -> midframe legs -> display PCB -> housing step (LCD floats).
  - USB insertion: the cable plug pushes the XIAO to +Y -> header pins + midframe stop rib.
  - Button press: button body -> midframe backer block.
  - Keyring: slot + bridge at the lug end, one piece with the body (not through the cover / PCB).

Splash: round screen protector bonded on the flat front face over the window. The USB-C port sits in a
port-shaped hole in the wall (open); power on/off in firmware.
Battery switch (EG1390) lies on the midframe, its actuator reached from the back through a slot in the
midframe + cover (tip 0.2 below the cover face); a piece of tape over the slot seals it.
"""

import math
from pathlib import Path

from build123d import (
    Align, Box, Circle, Compound, Cone, Cylinder, Kind, Location, Plane, Polygon, Pos, Rectangle, RectangleRounded,
    Rot, SlotOverall, extrude, offset,
)
from cadgen import srgb
from cadgen.assembly import AssemblyHelper
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
NECK_GROW = 1.0          # extra on every side of the LCD neck/FPC pocket (PLA test print: 0.35 was too tight)
LCD_TOP_GROW = 0.5       # extra LCD recess radius on the tab half (PLA test print: header in the solder pocket pulls the glass toward the tab, 0.2 too tight)
LCD_CORNER_R = 1.0       # rounds the inner corners where the LCD recess meets the neck pocket / the tab-half step

# XIAO nRF52840 Sense (measured from the official Seeed STEP)
XIAO_STEP = HERE / "parts" / "xiao_nrf52840_sense.step"
XIAO_PCB_CX, XIAO_PCB_CZ = 1.805, -6.11   # STEP-local PCB center (x, z)
XIAO_USB_FACE_X = 13.81                   # STEP-local USB-C front face
XIAO_TOP_Y = 4.21                         # STEP-local highest point (USB-C)
XIAO_SHIELD_Y = 3.0
XIAO_PCB_T = 1.0                          # STEP: PCB back z = origin - 1.0
XIAO_USB_CY = 2.605                       # USB-C shell height center below the origin (shell: PCB back -3.21..0)
XIAO_PCB_R = 1.905                        # PCB corner radius
# USB-C front face in the assembly (world Y): 0.9 behind the outer wall at x=0; the PCB corners reach into
# the hook groove radius (0.78 of wall left outside their pocket)
USB_FACE_Y = -20.2
USB_FIT = 0.2            # port-shaped hole around the USB-C shell (8.94 x 3.21), per side
XIAO_FIT = 0.25          # wall pocket around the XIAO's USB end: the PCB corners must not be squeezed
XIAO_RST_X, XIAO_RST_DY = -5.7, 3.4       # reset button (Seeed STEP): world X, distance from the USB-C face
# header pins (spacer removed, cut short) stuck into the midframe pin rails: they carry the USB-C plug force
XIAO_PIN_X, XIAO_PIN_Y0, XIAO_PIN_N = 7.62, USB_FACE_Y + 4.45, 7   # world, rows at +-X, pitch 2.54 toward +Y
PIN_HOLE_D = 1.2         # ~0.55 square pin (diag 0.78). PLA test print: Ø0.9 printed too small, pins didn't go in at all.
PIN_RAIL_X = (6.75, 9.3)  # inner edge clears the reset hole + a part near the USB end; outer < passives (9.5)
PIN_RAIL_GAP = 0.5       # rail top <-> XIAO PCB back: room for the pins' solder fillets

MOTOR_D, MOTOR_T = 10.0, 3.0
MOTOR_WIRE_SIDE = 1      # ring wire gap: +1 = +X (right, seen from the front), -1 = -X

SW_ACT_W, SW_TRAVEL = 1.5, 1.5
# battery switch: EG1390 (desoldered from the LilyPad board), lying flat on the midframe, actuator -Z
SW_BODY = (3.5, 7.0, 1.5)             # x, y (slide direction), z; 7.0 measured
SW_ACT_L, SW_ACT_T = 2.0, 1.0         # actuator: protrusion, thickness (its width along Y = SW_ACT_W)
SW_X, SW_Y = -11.25, -7.0             # body center: beside the XIAO's D0-D6 edge, clear of leg 205 / hook 230
# 3x6 tact button
BTN_BODY = (2.5, 6.0, 3.5)
BTN_PLUNGER = (1.8, 3.0, 1.5)

# LiPo 402030 (~200 mAh) - chosen, not bought yet
BAT_W, BAT_L, BAT_T = 20.0, 30.0, 4.0
BAT_SWELL = 0.5            # swelling allowance (~10% of thickness)
BAT_GAP = 0.5              # side clearance
BAT_WIRE = 1.5             # lead bend at the XIAO end

# case
FIT = 0.25                 # display PCB - case radial clearance
WALL = 1.8
BEZEL_T = 0.4              # front lip (3 layers, printed on the bed)
DECK_T = 1.0
COVER_T = 1.2
KEY_WEB, KEY_SLOT_W, KEY_SLOT_L, KEY_BAR = 1.2, 3.0, 10.0, 2.4   # tab pocket | slot | bridge
LEG_SHORT = 1.5            # display clamp legs end this far behind the PCB (PLA test print: too long). Fill with tape.

# ---------------------------------------------------------------- derived
RI = DISP_PCB_D / 2 + FIT
RO = RI + WALL
D_PCB_FRONT = BEZEL_T + GLASS_GAP + LCD_STACK          # display PCB sits on the case step
D_PCB_BACK = D_PCB_FRONT + DISP_PCB_T
D_XIAO_BOT = D_PCB_BACK + DISP_REAR
D_DECK_FRONT = D_XIAO_BOT + XIAO_TOP_Y + 0.3
D_DECK_BACK = D_DECK_FRONT + DECK_T     # deck depth is set by the XIAO + USB-C, not the battery
D_COVER_FRONT = D_DECK_BACK
H = D_COVER_FRONT + COVER_T
# battery lies on the deck; its swell must stay clear of the tab pads + wires
assert D_DECK_FRONT - BAT_T - BAT_SWELL >= D_PCB_BACK + TAB_REAR


def z(d):
    return H - d


Z_DF, Z_DB = z(D_DECK_FRONT), z(D_DECK_BACK)
Z_MID = z((D_XIAO_BOT + D_DECK_FRONT) / 2)      # button height center
TAB_HALF = TAB_W / 2 + FIT
TAB_TOP = MODULE_L - DISP_PCB_D / 2 + FIT

# XIAO placement: component side back (-Z), USB-C down (-Y)
XIAO_ORIGIN = (-XIAO_PCB_CZ, USB_FACE_Y + XIAO_USB_FACE_X, z(D_XIAO_BOT))
XIAO_PCB_TOP_Y = XIAO_ORIGIN[1] + 8.67     # antenna end (STEP-local x min = -8.67)
Z_USB = XIAO_ORIGIN[2] - XIAO_USB_CY

# battery: past the XIAO stop rib, long side up into the lug; the lug is full depth and ends behind it
# (fixed at the v7.3 spot: the XIAO moved 2.0 toward the USB wall in v7.4, battery + lug stay; the lead bend gains 2.0)
BAT_Y0 = 7.58
assert BAT_Y0 >= XIAO_PCB_TOP_Y + 1.3 + BAT_GAP + BAT_WIRE
LUG_IN_TOP = BAT_Y0 + BAT_L + BAT_GAP
KEY_SLOT_CY = LUG_IN_TOP + KEY_WEB + KEY_SLOT_W / 2
LUG_HALF, LUG_TOP = TAB_HALF + WALL, LUG_IN_TOP + KEY_WEB + KEY_SLOT_W + KEY_BAR
MOTOR_Y = (TAB_TOP + LUG_IN_TOP) / 2       # on the lug front skin, past the tab tip, in front of the battery

# snap hooks (back cover <-> front housing)
# PLA test print: 0.7 x 4 arms with 3.7 free length were too fragile -> thicker, wider, longer (same snap force)
CATCH_Z0 = COVER_T + 5.2         # catch tip (45° retention ramp, removable)
CATCH_Z1 = CATCH_Z0 + 1.2        # arm tip (lead-in ramp)
# groove edge sits 0.08 above where the retention ramp crosses the wall (r=RI, cover seated): the hooks
# preload the cover inward. Old (CATCH_Z0 - 0.9) left 0.52 of axial play (PLA test print: cover loose, sat low).
GROOVE = (CATCH_Z0 - 0.3, CATCH_Z1 + 0.7)
HOOK_W = 5.0
HOOK_T = 1.0                      # arm thickness (outer face 0.25 off the wall)
HOOK_ANGLES = (20, 160, 230, 310)   # 180 = button height, 270 = USB-C, 315 = passives, 53..127 = lug
LUG_HOOK_Y = LUG_IN_TOP - 4.0       # 2 more on the lug side walls, beside the battery
HOOKS = [(RI, a, 0.0) for a in HOOK_ANGLES] + [(TAB_HALF, a, LUG_HOOK_Y) for a in (0, 180)]  # (wall, angle, y)
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
def lcd_pocket_profile():
    """LCD recess (wider on the tab half) + LCD neck/FPC bend pocket, inner corners rounded."""
    r = LCD_D / 2 + 0.2
    prof = (Pos(0, LCD_DY) * Circle(r)
            + (Pos(0, LCD_DY) * Circle(r + LCD_TOP_GROW) & Pos(0, LCD_DY + RO / 2) * Rectangle(2 * RO, RO))
            + offset(Polygon((-8.85, 14.5), (8.85, 14.5), (8.85, 15.3), (6.95, 19.97), (6.95, 21.2),
                             (-6.95, 21.2), (-6.95, 19.97), (-8.85, 15.3), align=None),
                     NECK_GROW, kind=Kind.INTERSECTION))
    return offset(offset(prof, LCD_CORNER_R), -LCD_CORNER_R)   # closing: fills only the inward corners


def make_front_housing():
    body = Cylinder(RO, H, align=MIN).fuse(
        Pos(0, LUG_TOP / 2, 0) * extrude(RectangleRounded(2 * LUG_HALF, LUG_TOP, 3.0), H)).clean()

    cuts = [
        zcyl(0, LCD_DY, WIN_D / 2, z(BEZEL_T) - 0.1, H + 1),                  # window
        Pos(0, LCD_DY, H - 0.2) * Cone(WIN_D / 2, WIN_D / 2 + 0.3, 0.3, align=MIN),  # window chamfer
        extrude(lcd_pocket_profile(), z(BEZEL_T) - z(D_PCB_FRONT)).moved(Location((0, 0, z(D_PCB_FRONT)))),
        # header front-side solder joints
        zbox(-SOLDER_HALF_X - 0.4, SOLDER_HALF_X + 0.4, SOLDER_Y[0] - 0.4, SOLDER_Y[1] + 0.4,
             z(D_PCB_FRONT) - 0.1, z(D_PCB_FRONT) + SOLDER_H + 0.3),
        zcyl(0, 0, RI, -1, z(D_PCB_FRONT)),                                  # main cavity
        # lug cavity: display tab + battery + motor, open to the back (display inserts from the back)
        zbox(-TAB_HALF, TAB_HALF, 0, LUG_IN_TOP, -1, z(D_PCB_FRONT)),
        zcyl(0, 0, GROOVE_R, GROOVE[0], GROOVE[1]),                          # hook groove
        # lug side hook grooves
        *[zbox(s * (TAB_HALF - 0.1), s * (TAB_HALF + 0.7), LUG_HOOK_Y - HOOK_W / 2 - 0.3,
               LUG_HOOK_Y + HOOK_W / 2 + 0.3, GROOVE[0], GROOVE[1]) for s in (-1, 1)],
        # USB-C: port-shaped hole through the wall, the shell slides in from inside
        Pos(0, USB_FACE_Y + 3, Z_USB) * extrude(Plane.XZ * SlotOverall(8.94 + 2 * USB_FIT, 3.21 + 2 * USB_FIT), RO),
        # XIAO USB end sunk into the wall: PCB outline + its edge parts, XIAO_FIT all round
        Pos(0, USB_FACE_Y + 1.53 - XIAO_FIT + 5, XIAO_ORIGIN[2] - 2.0) * extrude(
            RectangleRounded(17.78 + 2 * XIAO_FIT, 10, XIAO_PCB_R + XIAO_FIT), 2.5),
        # button hole + finger dimple
        zbox(-RO - 1, -RI + 1, -1.8, 1.8, Z_MID - 1.0, Z_MID + 1.0),
        Pos(-RO - 1, 0, Z_MID) * Rot(0, 90, 0) * Cylinder(3.2, 1.6, align=MIN),
        # key ring / chain slot
        Pos(0, KEY_SLOT_CY, -1) * extrude(
            RectangleRounded(KEY_SLOT_L, KEY_SLOT_W, KEY_SLOT_W / 2 - 0.05), H + 2),
        # cover pry notch
        sector_box(RI - 0.5, RI + 1.0, 5.0, -1, 0.8, 230),
    ]
    # motor locating ring on the lug front skin, wire gap to the side; flattened where it would hit the tab tip
    zf = z(D_PCB_FRONT)
    ring = zcyl(0, MOTOR_Y, MOTOR_D / 2 + 1.05, zf - 1.0, zf + 0.1).cut(
        zcyl(0, MOTOR_Y, MOTOR_D / 2 + 0.25, zf - 2, zf + 1),
        zbox(0, MOTOR_WIRE_SIDE * 7, MOTOR_Y - 2.5, MOTOR_Y + 2.5, zf - 2, zf + 1),
        zbox(-7, 7, MOTOR_Y - 7, TAB_TOP, zf - 2, zf + 1))
    return body.cut(*cuts).fuse(ring).clean()


# ---------------------------------------------------------------- midframe
def make_midframe():
    th = TAB_HALF - 0.2
    deck = zcyl(0, 0, RI - 0.2, Z_DB, Z_DF).fuse(zbox(-th, th, 0, LUG_IN_TOP - 0.2, Z_DB, Z_DF))   # lug keys rotation
    # display clamp legs (press on the PCB edge ring, clear of the battery)
    adds = [zcyl(*polar(17.6, a), 1.2, Z_DF - 0.1, z(D_PCB_BACK) - LEG_SHORT) for a in (40, 140, 205, 335)]
    # XIAO: tape pad on the shield + stop rib at the antenna end
    shield_top = XIAO_ORIGIN[2] - XIAO_SHIELD_Y
    adds += [
        zbox(-4, 4, XIAO_ORIGIN[1] - 4.05, XIAO_ORIGIN[1] + 3.95, Z_DF - 0.1, shield_top - 0.5),
        zbox(-6, 6, XIAO_PCB_TOP_Y + 0.15, XIAO_PCB_TOP_Y + 1.3, Z_DF - 0.1, z(D_XIAO_BOT + 0.2)),
    ]
    # pin rails: the XIAO's header pins press into them (USB-C plug / unplug force, X and Y)
    pin_ys = [XIAO_PIN_Y0 + 2.54 * i for i in range(XIAO_PIN_N)]
    rail_top = XIAO_ORIGIN[2] - XIAO_PCB_T - PIN_RAIL_GAP
    adds += [zbox(s * PIN_RAIL_X[0], s * PIN_RAIL_X[1], pin_ys[0] - 1.0, pin_ys[-1] + 1.0, Z_DF - 0.1, rail_top)
             for s in (-1, 1)]
    # button backer block (takes the press load)
    btn_in = RI - (RI - math.sqrt(RI**2 - (BTN_BODY[1] / 2) ** 2)) - 0.05 - BTN_BODY[0]
    adds.append(zbox(-(btn_in - 0.1), -14.6, -3.4, 3.4, Z_DF - 0.1, Z_MID + BTN_BODY[2] / 2))
    # battery switch end stops (take the slide force; the body is taped down)
    sx, sy = SW_BODY[0] / 2, SW_BODY[1] / 2 + 0.1
    adds += [zbox(SW_X - sx, SW_X + sx, SW_Y + s * sy, SW_Y + s * (sy + 0.8), Z_DF - 0.1, Z_DF + 1.0)
             for s in (-1, 1)]
    # battery side ribs (front face; end stops = stop rib + lug end wall), clear of the lug hooks
    bx = BAT_W / 2 + BAT_GAP
    adds += [zbox(s * bx, s * (bx + 0.8), BAT_Y0 + 1.5, LUG_HOOK_Y - HOOK_W / 2 - 1.5, Z_DF - 0.1, Z_DF + 2.0)
             for s in (-1, 1)]
    deck = deck.fuse(*adds).clean()
    # reset access: no power switch, so a pin through here (cover off) is the way out of a hung firmware
    cuts = [zcyl(XIAO_RST_X, USB_FACE_Y + XIAO_RST_DY, 1.0, Z_DB - 1, Z_DF + 1)]
    cuts.append(battery_switch_slot(Z_DB - 1, Z_DF + 1))
    # pin holes through rail + deck (open at the back: pin tips sit on/above the cover)
    cuts += [zcyl(s * XIAO_PIN_X, XIAO_PIN_Y0 + 2.54 * i, PIN_HOLE_D / 2, Z_DB - 1, Z_DF + 10)
             for s in (-1, 1) for i in range(XIAO_PIN_N)]
    # hook arms pass the midframe edge (keeps arm length)
    cuts += [Pos(0, dy, 0) * sector_box(r - 1.4, r + 0.5, HOOK_W + 0.8, Z_DB - 1, Z_DF + 1, a) for r, a, dy in HOOKS]
    return deck.cut(*cuts).clean()


# ---------------------------------------------------------------- back cover
def make_back_cover():
    th = TAB_HALF - 0.2
    # flat cover, same outline as the deck (which lies directly on it)
    cover = Cylinder(RI - 0.2, COVER_T, align=MIN).fuse(zbox(-th, th, 0, LUG_IN_TOP - 0.2, 0, COVER_T))
    adds = []
    for r_wall, a, dy in HOOKS:
        r_out = r_wall - 0.25
        adds.append(Pos(0, dy, 0) * sector_box(r_out - HOOK_T, r_out, HOOK_W, COVER_T - 0.1, CATCH_Z1, a))
        profile = Plane.XZ * Polygon(
            (r_wall - 0.45, CATCH_Z0 - 0.8), (r_wall + 0.4, CATCH_Z0),
            (r_wall + 0.4, CATCH_Z0 + 0.2), (r_wall - 0.45, CATCH_Z1), align=None)
        adds.append(Pos(0, dy, 0) * Rot(0, 0, a) * extrude(profile, amount=HOOK_W / 2, both=True))
    return cover.fuse(*adds).cut(battery_switch_slot(-1, COVER_T + 1)).clean()


def battery_switch_slot(z0, z1):
    """Actuator slot through midframe + cover: actuator width + travel + 0.4 along Y, thickness + 0.4 along X."""
    hy, hx = (SW_ACT_W + SW_TRAVEL + 0.4) / 2, (SW_ACT_T + 0.4) / 2
    return zbox(SW_X - hx, SW_X + hx, SW_Y - hy, SW_Y + hy, z0, z1)


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


def make_battery_switch():
    bx, by, bz = SW_BODY
    body = zbox(SW_X - bx / 2, SW_X + bx / 2, SW_Y - by / 2, SW_Y + by / 2, Z_DF, Z_DF + bz)
    ay = SW_Y + SW_TRAVEL / 2     # actuator at one end of its travel
    act = zbox(SW_X - SW_ACT_T / 2, SW_X + SW_ACT_T / 2, ay - SW_ACT_W / 2, ay + SW_ACT_W / 2,
               Z_DF - SW_ACT_L, Z_DF + 0.1)
    return body.fuse(act)


def gen_step():
    asm = AssemblyHelper("mysterychain")
    asm.add(make_front_housing(), "front_housing", color=srgb("#EDEAE2", 0.55))
    asm.add(make_midframe(), "midframe", color=srgb("#4F7CAC"))
    asm.add(make_back_cover(), "back_cover", color=srgb("#7A8B99"))

    motor = zcyl(0, MOTOR_Y, MOTOR_D / 2, z(D_PCB_FRONT) - MOTOR_T, z(D_PCB_FRONT))
    battery = zbox(-BAT_W / 2, BAT_W / 2, BAT_Y0, BAT_Y0 + BAT_L, Z_DF, Z_DF + BAT_T)
    asm.add_module("electronics", [
        asm.feature(make_display(), "gc9a01_display_module", color=srgb("#1F5FA8")),
        asm.feature(make_xiao(), "xiao_nrf52840_sense"),
        # header pins: flush at the PCB front, cut 0.2 short of the cover
        asm.feature(Compound([zbox(s * XIAO_PIN_X - 0.32, s * XIAO_PIN_X + 0.32, y - 0.32, y + 0.32,
                                   Z_DB + 0.2, XIAO_ORIGIN[2])
                              for s in (-1, 1) for y in (XIAO_PIN_Y0 + 2.54 * i for i in range(XIAO_PIN_N))]),
                    "xiao_header_pins", color=srgb("#D4AF37")),
        asm.feature(motor, "vibration_motor", color=srgb("#B5B5B5")),
        asm.feature(make_button(), "tact_button", color=srgb("#333333")),
        asm.feature(battery, "lipo_402030", color=srgb("#C9A227")),
        asm.feature(make_battery_switch(), "battery_switch_eg1390", color=srgb("#222222")),
    ])
    asm.add_module("keepout", [
        asm.feature(
            zbox(-BAT_W / 2 - BAT_GAP, BAT_W / 2 + BAT_GAP, BAT_Y0 - BAT_GAP, BAT_Y0 + BAT_L + BAT_GAP,
                 Z_DF, Z_DF + BAT_T + BAT_SWELL),
            "battery_safe_zone", color=srgb("#E53935", 0.25)),
        asm.feature(zbox(9.5, 14.0, -12.0, -4.5, Z_DF, z(D_XIAO_BOT + 0.2)),
                    "passives_zone", color=srgb("#FF9800", 0.3)),
        # Q2 (SOT-23) + 100k on their own, beside the XIAO's D7/D8 pads
        asm.feature(zbox(9.5, 13.0, -3.0, 3.0, Z_DF, z(D_XIAO_BOT + 0.2)),
                    "q2_zone", color=srgb("#FF9800", 0.3)),
    ])
    return asm.build()


if __name__ == "__main__":
    print(f"H={H:.2f}  OD={2 * RO:.2f}  height={RO + LUG_TOP:.2f}  lug_width={2 * LUG_HALF:.2f}  "
          f"battery_y={BAT_Y0:.2f}..{BAT_Y0 + BAT_L:.2f}  motor_y={MOTOR_Y:.2f}  cover_front_depth={D_COVER_FRONT:.2f}")
