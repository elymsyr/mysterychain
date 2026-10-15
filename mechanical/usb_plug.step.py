"""MysteryChain - tethered USB-C plug (TPU 95A), splash protection (IPX4 goal).

One TPU piece, same coordinates as pet_case.step.py (drops straight into the assembly):
  - plug body: press fit in the 13 x 7 USB pocket, stops short of the receptacle face
  - flange: curved over the pocket mouth on the outer wall; this is the actual seal
    (the pocket ceiling is a 13 mm bridge in the housing print, too rough to seal on)
  - strap: runs along the outer wall from the pocket (-Y) to the old switch recess (+X)
  - anchor pad: fills the switch thumb recess, barb through the old switch slot.
    The slide switch is gone since case v6 (power on/off in firmware); the pad also seals that slot.

Print with Z up as modelled: flange + strap bottom on the bed.
"""

from pathlib import Path

from build123d import Cylinder, Polygon, Pos, extrude

from cadgen.sources import load_source_module

pc = load_source_module(Path(__file__).resolve().parent / "pet_case.step.py")
zbox, zcyl, MIN = pc.zbox, pc.zcyl, pc.MIN

FLANGE_T = 0.8        # flange thickness, proud of the wall
FLANGE_M = 1.2        # flange overlap around the pocket mouth
STRAP_T, STRAP_H = 0.6, 3.0
BODY_FIT = 0.1        # press fit per side in the pocket (TPU); tune after the first print
BODY_STOP = 0.4       # gap to the USB-C receptacle face
BARB_OVER = 0.6       # barb width beyond the slot, per side

POCKET_HW, POCKET_HH = pc.USB_POCKET_W / 2, pc.USB_POCKET_H / 2
SLOT_HW, SLOT_HH = (pc.SW_ACT_W + pc.SW_TRAVEL + 0.4) / 2, 0.7    # = the switch slot cut in the housing
RECESS_HW, RECESS_HH = 3.25, 1.8                                   # = the switch thumb recess
RO, RI = pc.RO, pc.RI
Z0 = max(pc.Z_USB - POCKET_HH - FLANGE_M, 0.0)     # part bottom (bed), stops at the case back face


def shell(r_out, z0, z1):
    return zcyl(0, 0, r_out, z0, z1).cut(zcyl(0, 0, RO, z0 - 1, z1 + 1))


def make_usb_plug():
    z1 = pc.Z_USB + POCKET_HH + FLANGE_M
    flange = shell(RO + FLANGE_T, Z0, z1).intersect(
        zbox(-POCKET_HW - FLANGE_M, POCKET_HW + FLANGE_M, -RO - 2, -10, Z0, z1))
    body = zbox(-POCKET_HW - BODY_FIT, POCKET_HW + BODY_FIT, -RO - 1, pc.USB_FACE_Y - BODY_STOP,
                pc.Z_USB - POCKET_HH - BODY_FIT, pc.Z_USB + POCKET_HH + BODY_FIT
                ).intersect(Cylinder(RO, pc.H, align=MIN))
    # strap: quarter turn on the outer wall, bottom on the bed, top overlaps the pad
    s_top = pc.Z_MID - RECESS_HH + 0.6
    strap = shell(RO + STRAP_T, Z0, max(s_top, Z0 + STRAP_H)).intersect(zbox(0, RO + 2, -RO - 2, 0, Z0, pc.H))
    pad = zbox(RO - 0.9, RO + 1, -RECESS_HW, RECESS_HW, pc.Z_MID - RECESS_HH, pc.Z_MID + RECESS_HH
               ).intersect(Cylinder(RO + STRAP_T, pc.H, align=MIN))
    # barb: stem fills the slot, arrow head springs open behind the wall (shoulder clears the curved inner wall)
    shoulder = (RI**2 - (SLOT_HW + BARB_OVER) ** 2) ** 0.5 - 0.05
    stem = zbox(shoulder - 0.01, RO - 0.85, -SLOT_HW, SLOT_HW, pc.Z_MID - SLOT_HH, pc.Z_MID + SLOT_HH)
    head = Pos(0, 0, pc.Z_MID - SLOT_HH + 0.1) * extrude(Polygon(
        (shoulder, -SLOT_HW - BARB_OVER), (shoulder, SLOT_HW + BARB_OVER),
        (shoulder - 0.9, 1.0), (shoulder - 0.9, -1.0), align=None), 2 * SLOT_HH - 0.2)
    return flange.fuse(body, strap, pad, stem, head).clean()


def gen_step():
    part = make_usb_plug()
    part.label = "usb_plug_tpu"
    return part


if __name__ == "__main__":
    p = make_usb_plug()
    bb = p.bounding_box()
    print(f"vol={p.volume:.1f} mm3  bbox {bb.size.X:.2f} x {bb.size.Y:.2f} x {bb.size.Z:.2f}  z {bb.min.Z:.2f}..{bb.max.Z:.2f}")
