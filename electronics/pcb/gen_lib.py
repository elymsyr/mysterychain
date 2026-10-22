#!/usr/bin/env python3
"""Generate the custom library: lib/mysterychain.pretty footprints + lib/mysterychain.kicad_sym.

Run with /usr/bin/python3 -I (needs KiCad 9 pcbnew). Output is deterministic (uuid5).
"""
import os
import re
import sys
import uuid

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

LIB = os.path.join(HERE, "lib")
PRETTY = os.path.join(LIB, "mysterychain.pretty")
VENDOR_E73 = os.path.join(LIB, "vendor", "E73(2G4M08S1C).PcbLib")

# Footprint-local (x0, y0, x1, y1) mm. The antenna is the pad-free end (+x, manual §3 drawing:
# 3.97 mm from pads 1/43 to the edge); keep-out = that end + 2 mm around it, on every copper layer.
E73_ANTENNA_ZONE = (14.6, -15.0, 20.0, 2.0)
# Under the body, on the module's own side (B.Cu once flipped): no tracks, GND may pour, tented vias
# may pass (4 layers, spec 2026-10-21: the In2.Cu GND plane is the shield under the module).
# Inset 1.25 mm from the castellated edges so pad stubs leaving outward stay outside it.
E73_BODY_ZONE = (1.25, -11.75, 14.6, -1.25)


def _pcbnew():
    import pcbnew
    return pcbnew


def mm(v):
    return _pcbnew().FromMM(v)


def xy(x, y):
    return _pcbnew().VECTOR2I(mm(x), mm(y))


def new_fp(name, desc, ref_y, val_y):
    pcbnew = _pcbnew()
    fp = pcbnew.FOOTPRINT(None)
    fp.SetFPID(pcbnew.LIB_ID("mysterychain", name))
    fp.SetLibDescription(desc)
    fp.Reference().SetText("REF**")
    fp.Reference().SetPosition(xy(0, ref_y))
    fp.Value().SetText(name)
    fp.Value().SetPosition(xy(0, val_y))
    fp.Value().SetLayer(pcbnew.F_Fab)
    return fp


def smd_pad(fp, num, x, y, w, h):
    pcbnew = _pcbnew()
    p = pcbnew.PAD(fp)
    p.SetNumber(num)
    p.SetAttribute(pcbnew.PAD_ATTRIB_SMD)
    p.SetShape(pcbnew.PAD_SHAPE_ROUNDRECT)
    p.SetRoundRectRadiusRatio(0.25)
    p.SetSize(xy(w, h))
    p.SetLayerSet(p.SMDMask())
    p.SetPosition(xy(x, y))
    fp.Add(p)
    return p


def npth(fp, x, y, d):
    pcbnew = _pcbnew()
    p = pcbnew.PAD(fp)
    p.SetAttribute(pcbnew.PAD_ATTRIB_NPTH)
    p.SetShape(pcbnew.PAD_SHAPE_CIRCLE)
    p.SetSize(xy(d, d))
    p.SetDrillSize(xy(d, d))
    p.SetLayerSet(p.UnplatedHoleMask())
    p.SetPosition(xy(x, y))
    fp.Add(p)


def rect(fp, layer, x0, y0, x1, y1, width):
    pcbnew = _pcbnew()
    s = pcbnew.PCB_SHAPE(fp)
    s.SetShape(pcbnew.SHAPE_T_RECT)
    s.SetStart(xy(x0, y0))
    s.SetEnd(xy(x1, y1))
    s.SetLayer(layer)
    s.SetWidth(mm(width))
    fp.Add(s)


def dot(fp, layer, x, y, r=0.15):
    pcbnew = _pcbnew()
    s = pcbnew.PCB_SHAPE(fp)
    s.SetShape(pcbnew.SHAPE_T_CIRCLE)
    s.SetCenter(xy(x, y))
    s.SetEnd(xy(x + r, y))
    s.SetFilled(True)
    s.SetLayer(layer)
    s.SetWidth(mm(0.1))
    fp.Add(s)


def rule_area(fp, layers, box, pour_ok):
    """pour_ok: GND may pour and vias may pass (body area); otherwise no copper at all (antenna)."""
    pcbnew = _pcbnew()
    z = pcbnew.ZONE(fp)
    z.SetIsRuleArea(True)
    z.SetLayerSet(layers)
    z.SetDoNotAllowTracks(True)
    z.SetDoNotAllowVias(not pour_ok)
    z.SetDoNotAllowPads(not pour_ok)
    z.SetDoNotAllowCopperPour(not pour_ok)
    z.SetDoNotAllowFootprints(False)
    x0, y0, x1, y1 = box
    o = z.Outline()
    o.NewOutline()
    for x, y in [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]:
        o.Append(mm(x), mm(y))
    fp.Add(z)


def courtyard_around(fp, extra=()):
    """Replace F.CrtYd with a rectangle 0.25 mm around the pads and the given boxes."""
    pcbnew = _pcbnew()
    for g in list(fp.GraphicalItems()):
        if g.GetLayer() == pcbnew.F_CrtYd:
            fp.Remove(g)
    boxes = [p.GetBoundingBox() for p in fp.Pads()]
    xs = [pcbnew.ToMM(b.GetLeft()) for b in boxes] + [pcbnew.ToMM(b.GetRight()) for b in boxes]
    ys = [pcbnew.ToMM(b.GetTop()) for b in boxes] + [pcbnew.ToMM(b.GetBottom()) for b in boxes]
    for x0, y0, x1, y1 in extra:
        xs += [x0, x1]
        ys += [y0, y1]
    g = lambda v: round(v * 100) / 100  # noqa: E731 - 0.01 grid
    rect(fp, pcbnew.F_CrtYd, g(min(xs) - 0.25), g(min(ys) - 0.25), g(max(xs) + 0.25), g(max(ys) + 0.25), 0.05)


def save(fp):
    pcbnew = _pcbnew()
    name = fp.GetFPID().GetLibItemName().wx_str()
    pcbnew.PCB_IO_MGR.PluginFind(pcbnew.PCB_IO_MGR.KICAD_SEXP).FootprintSave(PRETTY, fp)
    path = os.path.join(PRETTY, name + ".kicad_mod")
    n = iter(range(10**6))
    text = re.sub(r'\(uuid "[0-9a-f-]+"\)',
                  lambda m: f'(uuid "{uuid.uuid5(uuid.NAMESPACE_URL, f"mysterychain/{name}/{next(n)}")}")',
                  open(path).read())
    open(path, "w").write(text)


# ---------------------------------------------------------------- footprints

def e73():
    pcbnew = _pcbnew()
    io = pcbnew.PCB_IO_MGR.PluginFind(pcbnew.PCB_IO_MGR.ALTIUM_DESIGNER)
    fp = io.FootprintLoad(VENDOR_E73, "E73(2G4M08S1C)")
    fp.SetFPID(pcbnew.LIB_ID("mysterychain", "E73-2G4M08S1C"))
    fp.SetLibDescription("Ebyte E73-2G4M08S1C nRF52840 module, 13x18x3.0 mm, 1.27 mm castellated; "
                         "pads from Ebyte's Altium PcbLib. Antenna at +x.")
    fp.SetKeywords("nRF52840 BLE module castellated")
    for g in list(fp.GraphicalItems()):  # vendor silk runs across the castellated pads
        fp.Remove(g)
    fp.Reference().SetText("REF**")
    fp.Reference().SetPosition(xy(9, -6.5))
    fp.Reference().SetLayer(pcbnew.F_Fab)
    fp.Value().SetText("E73-2G4M08S1C")
    fp.Value().SetPosition(xy(9, -4.5))
    fp.Value().SetLayer(pcbnew.F_Fab)
    rect(fp, pcbnew.F_Fab, 0, -13, 18, 0, 0.1)
    rect(fp, pcbnew.F_Fab, E73_ANTENNA_ZONE[0], -12.5, 17.5, -0.5, 0.1)  # antenna end
    dot(fp, pcbnew.F_SilkS, 14.9, -13.0)  # pin 1, between pad 1 and the antenna end
    courtyard_around(fp, [(0, -13, 18, 0)])
    rule_area(fp, pcbnew.LSET.AllCuMask(), E73_ANTENNA_ZONE, pour_ok=False)
    only_f = pcbnew.LSET()
    only_f.AddLayer(pcbnew.F_Cu)
    rule_area(fp, only_f, E73_BODY_ZONE, pour_ok=True)
    m = pcbnew.FP_3DMODEL()
    m.m_Filename = "${KIPRJMOD}/lib/3d/E73-2G4M08S1C.step"
    fp.Models().push_back(m)
    save(fp)


def eg1390():
    """E-Switch EG1390 (DPDT slide, top actuator): both poles in parallel, pads 4/5/6 = 1/2/3."""
    pcbnew = _pcbnew()
    fp = new_fp("EG1390", "E-Switch EG1390 SMD slide switch, 7.2x3.5x1.5 mm body, slide along x; "
                "poles paralleled", -4.5, 4.5)
    for row_y in (-2.8, 2.8):
        for n, x in (("1", -2.5), ("2", 0.0), ("3", 2.5)):
            smd_pad(fp, n, x, row_y, 1.0, 1.5)
    for x in (-1.5, 1.5):
        npth(fp, x, 0, 0.9)
    rect(fp, pcbnew.F_Fab, -3.6, -1.75, 3.6, 1.75, 0.1)
    rect(fp, pcbnew.F_Fab, -0.75, -0.5, 0.75, 0.5, 0.1)  # actuator
    dot(fp, pcbnew.F_SilkS, -3.4, -2.8)
    courtyard_around(fp, [(-3.6, -1.75, 3.6, 1.75)])
    save(fp)


def padrow(n, pitch, w):
    pcbnew = _pcbnew()
    name = f"PadRow_1x{n:02d}_P{pitch:.2f}mm"
    fp = new_fp(name, f"{n} SMD wire/test pads {w}x2.4 mm at {pitch} mm; names on the board silkscreen",
                -2.2, 2.2)
    x0 = -(n - 1) * pitch / 2
    for i in range(n):
        smd_pad(fp, str(i + 1), round(x0 + i * pitch, 3), 0, w, 2.4)
    dot(fp, pcbnew.F_SilkS, round(x0 - w / 2 - 0.35, 3), -1.0)
    rect(fp, pcbnew.F_Fab, x0 - w / 2, -1.2, -x0 + w / 2, 1.2, 0.1)
    courtyard_around(fp)
    save(fp)


def test_pad():
    """1.0 mm round test pad; courtyard = pad + 0.25 (probe spacing is checked as >= 2.0 c-c instead)."""
    pcbnew = _pcbnew()
    fp = new_fp("TestPad_D1.0mm", "1.0 mm round SMD test pad for probes / pogo pins", -1.3, 1.3)
    p = smd_pad(fp, "1", 0, 0, 1.0, 1.0)
    p.SetShape(pcbnew.PAD_SHAPE_CIRCLE)
    s = pcbnew.PCB_SHAPE(fp)
    s.SetShape(pcbnew.SHAPE_T_CIRCLE)
    s.SetCenter(xy(0, 0))
    s.SetEnd(xy(0.75, 0))
    s.SetLayer(pcbnew.F_CrtYd)
    s.SetWidth(mm(0.05))
    fp.Add(s)
    fp.SetAttributes(pcbnew.FP_SMD | pcbnew.FP_EXCLUDE_FROM_BOM | pcbnew.FP_EXCLUDE_FROM_POS_FILES)
    save(fp)


def npth_locator():
    """Bare 2.0 mm NPTH for a case v8 locating pin (no screw head, so a tight courtyard)."""
    pcbnew = _pcbnew()
    fp = new_fp("NPTH_D2.0mm", "2.0 mm unplated locating hole for a case pin", -1.8, 1.8)
    npth(fp, 0, 0, 2.0)
    s = pcbnew.PCB_SHAPE(fp)
    s.SetShape(pcbnew.SHAPE_T_CIRCLE)
    s.SetCenter(xy(0, 0))
    s.SetEnd(xy(1.25, 0))
    s.SetLayer(pcbnew.F_CrtYd)
    s.SetWidth(mm(0.05))
    fp.Add(s)
    save(fp)


def lga_handsolder():
    """KiCad's LSM6DS3TR-C land pattern with every pad 0.25 mm longer outward (spec Risks row)."""
    pcbnew = _pcbnew()
    fp = pcbnew.FootprintLoad("/usr/share/kicad/footprints/Package_LGA.pretty",
                              "LGA-14_3x2.5mm_P0.5mm_LayoutBorder3x4y")
    fp.SetFPID(pcbnew.LIB_ID("mysterychain", "LGA-14_3x2.5mm_P0.5mm_HandSolder"))
    fp.SetLibDescription("LGA-14 3x2.5 mm P0.5 (LSM6DS3TR-C), pads +0.25 mm outward for hot-air hand soldering")
    fp.Value().SetText("LGA-14_3x2.5mm_P0.5mm_HandSolder")
    for p in fp.Pads():
        pos, size = p.GetPosition(), p.GetSize()
        sx, sy = pcbnew.ToMM(size.x), pcbnew.ToMM(size.y)
        x, y = pcbnew.ToMM(pos.x), pcbnew.ToMM(pos.y)
        if sx > sy:  # pad long in x: a left/right pad
            p.SetSize(xy(sx + 0.25, sy))
            p.SetPosition(xy(x + (0.125 if x > 0 else -0.125), y))
        else:
            p.SetSize(xy(sx, sy + 0.25))
            p.SetPosition(xy(x, y + (0.125 if y > 0 else -0.125)))
    for gi in list(fp.GraphicalItems()):  # stock pin-1 silk now touches the longer pads
        if gi.GetLayer() == pcbnew.F_SilkS:
            fp.Remove(gi)
    dot(fp, pcbnew.F_SilkS, -1.75, -1.45)
    courtyard_around(fp, [(-1.5, -1.25, 1.5, 1.25)])
    fp.SetLocalClearance(mm(0.15))  # 0.5 pitch leaves 0.15 between pads (fab minimum); board default is 0.2
    save(fp)


# ---------------------------------------------------------------- symbol

E73_NAMES = {
    1: "P1.11", 2: "P1.10", 3: "P0.03", 4: "P0.28/AIN4", 5: "GND", 6: "P1.13", 7: "P0.02/AIN0",
    8: "P0.29/AIN5", 9: "P0.31/AIN7", 10: "P0.30/AIN6", 11: "P0.00/XL1", 12: "P0.26", 13: "P0.01/XL2",
    14: "P0.06", 15: "P0.05/AIN3", 16: "P0.08", 17: "P1.09", 18: "P0.04/AIN2", 19: "VDD", 20: "P0.12",
    21: "GND", 22: "P0.07", 23: "VDDH", 24: "GND", 25: "DCCH", 26: "P0.18/RESET", 27: "VBUS",
    28: "P0.15", 29: "D-", 30: "P0.17", 31: "D+", 32: "P0.20", 33: "P0.13", 34: "P0.22", 35: "P0.24",
    36: "P1.00", 37: "SWDIO", 38: "P1.02", 39: "SWCLK", 40: "P1.04", 41: "P0.09/NFC1", 42: "P1.06",
    43: "P0.10/NFC2",
}


def _pin_type(name):
    if name in ("GND", "VDD", "VDDH", "VBUS"):
        return "power_in"
    if name == "DCCH":
        return "power_out"
    if name == "SWCLK":
        return "input"
    return "bidirectional"


def e73_symbol():
    from sexp import Q, dumps
    font = ["effects", ["font", ["size", "1.27", "1.27"]]]
    hidden = ["effects", ["font", ["size", "1.27", "1.27"]], ["hide", "yes"]]
    pins = []
    for n, name in E73_NAMES.items():
        left = n <= 22
        row = n - 1 if left else n - 23
        x, ang = ("-17.78", "0") if left else ("17.78", "180")
        y = f"{27.94 - row * 2.54:.2f}"
        pins.append(["pin", _pin_type(name), "line", ["at", x, y, ang], ["length", "5.08"],
                     ["name", Q(name), font], ["number", Q(str(n)), font]])

    def prop(k, v, y, hide=False):
        return ["property", Q(k), Q(v), ["at", "0", y, "0"], hidden if hide else font]

    sym = ["symbol", Q("E73-2G4M08S1C"), ["exclude_from_sim", "no"], ["in_bom", "yes"], ["on_board", "yes"],
           prop("Reference", "U", "31.75"), prop("Value", "E73-2G4M08S1C", "-29.21"),
           prop("Footprint", "mysterychain:E73-2G4M08S1C", "-31.75", True),
           prop("Datasheet", "https://www.cdebyte.com/products/E73-2G4M08S1C", "-34.29", True),
           prop("Description", "Ebyte nRF52840 BLE module, ceramic antenna, castellated 1.27 mm", "-36.83", True),
           ["symbol", Q("E73-2G4M08S1C_0_1"),
            ["rectangle", ["start", "-12.7", "30.48"], ["end", "12.7", "-27.94"],
             ["stroke", ["width", "0.254"], ["type", "default"]], ["fill", ["type", "background"]]]],
           ["symbol", Q("E73-2G4M08S1C_1_1"), *pins],
           ["embedded_fonts", "no"]]
    lib = ["kicad_symbol_lib", ["version", "20241209"], ["generator", Q("mysterychain_gen_lib")],
           ["generator_version", Q("9.0")], sym]
    open(os.path.join(LIB, "mysterychain.kicad_sym"), "w").write(dumps(lib) + "\n")


def main():
    pcbnew = _pcbnew()
    os.makedirs(PRETTY, exist_ok=True)
    e73()
    eg1390()
    padrow(2, 2.0, 1.4)
    padrow(7, 2.0, 1.4)
    padrow(5, 2.54, 1.6)
    lga_handsolder()
    npth_locator()
    test_pad()
    e73_symbol()
    print("gen_lib:", sorted(os.listdir(PRETTY)), pcbnew.Version())


if __name__ == "__main__":
    main()
