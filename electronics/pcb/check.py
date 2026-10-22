#!/usr/bin/env python3
"""Every rev A assertion, by build stage.

Usage: check.py parts|lib|sch|pcb [--placed]|fab
Runs the stage's test_* functions in order and exits 1 on the first failure.
"""
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)  # -I drops the script dir from sys.path


# ---------------------------------------------------------------- parts

def test_parts_table():
    from parts import PARTS
    refs = [p.ref for p in PARTS]
    dup = {r for r in refs if refs.count(r) > 1}
    assert not dup, f"duplicate refs {dup}"
    for p in PARTS:
        assert p.side in ("F", "B"), f"{p.ref} side {p.side!r}"
        limit = 1.2 if p.side == "F" else 3.4
        assert p.height <= limit, f"{p.ref} {p.height} mm > {limit} on {p.side}"
        if p.bom:
            assert p.mpn, f"{p.ref} has no MPN"
            assert p.tr_source or p.alt, f"{p.ref} has no TR source or alternate"


def test_nets_named():
    from parts import TEST_NETS, nets
    for net, pins in nets().items():
        if net not in TEST_NETS:
            assert len(pins) >= 2, f"net {net} has one pin {pins}"


# ---------------------------------------------------------------- lib

LIB = os.path.join(HERE, "lib")
VENDOR_E73 = os.path.join(LIB, "vendor", "E73(2G4M08S1C).PcbLib")
CASTELLATED = [*range(1, 12), 13, *range(15, 26, 2), 26, 27, *range(29, 44, 2)]
INNER = [*range(12, 25, 2), *range(28, 43, 2)]


def _fp(name):
    import pcbnew
    lib, fp = name.split(":")
    path = os.path.join(LIB, f"{lib}.pretty") if lib == "mysterychain" else f"/usr/share/kicad/footprints/{lib}.pretty"
    assert os.path.isfile(os.path.join(path, fp + ".kicad_mod")), f"footprint {name} missing"
    out = pcbnew.FootprintLoad(path, fp)
    assert out is not None, f"footprint {name} not found in {path}"
    return out


def _mm(v):
    import pcbnew
    return round(pcbnew.ToMM(v), 4)


def _pads(fp):
    return {p.GetNumber(): p for p in fp.Pads() if p.GetNumber()}


def test_e73_pads():
    import pcbnew
    fp = _fp("mysterychain:E73-2G4M08S1C")
    pads = _pads(fp)
    assert sorted(pads, key=int) == [str(i) for i in range(1, 44)], sorted(pads)
    for n, size in [(CASTELLATED, (2.0, 0.8)), (INNER, (1.2, 0.8))]:
        for i in n:
            s = pads[str(i)].GetSize()
            assert (_mm(s.x), _mm(s.y)) == size, f"pad {i} size {_mm(s.x)}x{_mm(s.y)} != {size}"
    io = pcbnew.PCB_IO_MGR.PluginFind(pcbnew.PCB_IO_MGR.ALTIUM_DESIGNER)
    ref = _pads(io.FootprintLoad(VENDOR_E73, "E73(2G4M08S1C)"))
    for i, p in pads.items():
        a, b = p.GetPosition(), ref[i].GetPosition()
        assert abs(_mm(a.x - b.x)) <= 0.001 and abs(_mm(a.y - b.y)) <= 0.001, f"pad {i} moved"


def test_e73_zones():
    import pcbnew
    from gen_lib import E73_ANTENNA_ZONE
    fp = _fp("mysterychain:E73-2G4M08S1C")
    x0, y0, x1, y1 = E73_ANTENNA_ZONE
    body = [p.GetBoundingBox() for p in fp.Pads()]
    pad_right = max(_mm(b.GetRight()) for b in body)
    assert pad_right < x0 <= pad_right + 0.5, f"antenna zone starts at {x0}, pads end at {pad_right}"
    assert x1 >= 18.0 + 2 and y0 <= -13.0 - 2 and y1 >= 0 + 2, "antenna zone is not end + 2 mm"
    ant = body_area = None
    for z in fp.Zones():
        bb = z.GetBoundingBox()
        box = (_mm(bb.GetLeft()), _mm(bb.GetTop()), _mm(bb.GetRight()), _mm(bb.GetBottom()))
        layers = set(z.GetLayerSet().CuStack())
        if box == (x0, y0, x1, y1):
            ant = z
            assert {pcbnew.F_Cu, pcbnew.B_Cu} <= layers, "antenna keep-out not on all copper"
            assert z.GetIsRuleArea() and z.GetDoNotAllowTracks() and z.GetDoNotAllowVias()
            assert z.GetDoNotAllowPads() and z.GetDoNotAllowCopperPour()
        else:
            body_area = z
            assert layers == {pcbnew.F_Cu}, "body keep-out must be on the module's own side only"
            # spec 2026-10-21 (4 layers): no tracks under the module, tented vias may pass
            assert z.GetIsRuleArea() and z.GetDoNotAllowTracks() and not z.GetDoNotAllowVias()
            assert not z.GetDoNotAllowCopperPour(), "GND must pour under the body"
            assert box[0] <= 1.5 and box[2] >= x0 - 0.5 and box[1] <= -11.5 and box[3] >= -1.5, box
    assert ant and body_area, "missing E73 rule areas"


def test_e73_symbol():
    from sexp import find, loads
    lib = loads(open(os.path.join(LIB, "mysterychain.kicad_sym")).read())
    sym = [s for s in find(lib, "symbol") if s[1] == "E73-2G4M08S1C"][0]
    pins = {}
    for unit in find(sym, "symbol"):
        for p in find(unit, "pin"):
            pins[find(p, "number")[0][1]] = (find(p, "name")[0][1], p[1])
    assert sorted(pins, key=int) == [str(i) for i in range(1, 44)]
    want = {"26": "P0.18/RESET", "27": "VBUS", "29": "D-", "31": "D+", "37": "SWDIO", "39": "SWCLK",
            "25": "DCCH", "41": "P0.09/NFC1", "43": "P0.10/NFC2", "11": "P0.00/XL1", "13": "P0.01/XL2",
            "1": "P1.11", "2": "P1.10", "3": "P0.03", "4": "P0.28/AIN4", "6": "P1.13", "7": "P0.02/AIN0",
            "8": "P0.29/AIN5", "9": "P0.31/AIN7", "10": "P0.30/AIN6", "15": "P0.05/AIN3", "17": "P1.09",
            "19": "VDD", "23": "VDDH", "33": "P0.13", "35": "P0.24", "5": "GND", "21": "GND", "24": "GND"}
    for n, name in want.items():
        assert pins[n][0] == name, f"pin {n} is {pins[n][0]!r}, want {name!r}"
    for n in ("5", "19", "21", "23", "24", "27"):
        assert pins[n][1] == "power_in", f"pin {n} type {pins[n][1]}"
    import subprocess
    import tempfile
    with tempfile.TemporaryDirectory() as d:  # KiCad itself must parse it
        r = subprocess.run(["kicad-cli", "sym", "export", "svg", os.path.join(LIB, "mysterychain.kicad_sym"),
                            "-o", d], capture_output=True, text=True)
        assert r.returncode == 0 and os.listdir(d), r.stdout + r.stderr


def test_custom_footprints():
    import pcbnew
    # EG1390 (E-Switch drawing): 2 rows of 3 pads 1.0 x 1.5 at 2.5 pitch, rows 5.6 apart, 2 x NPTH 0.9
    fp = _fp("mysterychain:EG1390")
    smd = sorted((p.GetNumber(), _mm(p.GetPosition().x), _mm(p.GetPosition().y)) for p in fp.Pads()
                 if p.GetAttribute() == pcbnew.PAD_ATTRIB_SMD)
    assert smd == sorted([(n, x, y) for (n, x), y in
                          [(("1", -2.5), -2.8), (("2", 0.0), -2.8), (("3", 2.5), -2.8),
                           (("1", -2.5), 2.8), (("2", 0.0), 2.8), (("3", 2.5), 2.8)]]), smd
    holes = sorted((_mm(p.GetPosition().x), _mm(p.GetDrillSize().x)) for p in fp.Pads()
                   if p.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH)
    assert holes == [(-1.5, 0.9), (1.5, 0.9)], holes
    for name, n, pitch, w in [("PadRow_1x02_P2.00mm", 2, 2.0, 1.4), ("PadRow_1x07_P2.00mm", 7, 2.0, 1.4),
                              ("PadRow_1x05_P2.54mm", 5, 2.54, 1.6)]:
        pads = _pads(_fp("mysterychain:" + name))
        assert sorted(pads, key=int) == [str(i) for i in range(1, n + 1)], name
        xs = [_mm(pads[str(i)].GetPosition().x) for i in range(1, n + 1)]
        assert all(abs(b - a - pitch) < 1e-3 for a, b in zip(xs, xs[1:])), (name, xs)
        assert all((_mm(p.GetSize().x), _mm(p.GetSize().y)) == (w, 2.4) for p in pads.values()), name
    # LGA-14 hand-solder: stock pads, each 0.25 longer outward, inner edge unchanged
    ours = _pads(_fp("mysterychain:LGA-14_3x2.5mm_P0.5mm_HandSolder"))
    stock = _pads(_fp("Package_LGA:LGA-14_3x2.5mm_P0.5mm_LayoutBorder3x4y"))
    assert sorted(ours) == sorted(stock)
    for n, s in stock.items():
        a, b = ours[n].GetBoundingBox(), s.GetBoundingBox()
        da = [_mm(a.GetLeft() - b.GetLeft()), _mm(a.GetTop() - b.GetTop()),
              _mm(a.GetRight() - b.GetRight()), _mm(a.GetBottom() - b.GetBottom())]
        assert sorted(da) == [-0.25, 0, 0, 0] or sorted(da) == [0, 0, 0, 0.25], (n, da)
        c = s.GetPosition()
        grew_out = (da[0] < 0 and c.x < 0) or (da[2] > 0 and c.x > 0) or (da[1] < 0 and c.y < 0) or (da[3] > 0 and c.y > 0)
        assert grew_out, f"LGA pad {n} grew inward {da}"


def test_footprints_resolve():
    from parts import PARTS
    for p in PARTS:
        if p.footprint:
            _fp(p.footprint)
    assert os.path.exists(os.path.join(LIB, "3d", "E73-2G4M08S1C.step"))


# ---------------------------------------------------------------- sch

SCH = os.path.join(HERE, "mysterychain.kicad_sch")
BUILD = os.path.join(HERE, "build")
README = os.path.join(HERE, "README.md")


def _cli(*args):
    import subprocess
    return subprocess.run(["kicad-cli", *args], capture_output=True, text=True)


def _sch_netlist():
    """kicad-cli netlist -> {net: {(ref, pin)}}, PWR_FLAGs and unconnected-pin nets dropped."""
    from sexp import find, loads
    os.makedirs(BUILD, exist_ok=True)
    out = os.path.join(BUILD, "mysterychain.net")
    r = _cli("sch", "export", "netlist", "--format", "kicadsexpr", "-o", out, SCH)
    assert r.returncode == 0, r.stdout + r.stderr
    nets = {}
    for n in find(find(loads(open(out).read()), "nets")[0], "net"):
        name = find(n, "name")[0][1].lstrip("/")
        nodes = {(find(x, "ref")[0][1], find(x, "pin")[0][1]) for x in find(n, "node")}
        nodes = {x for x in nodes if not x[0].startswith("#")}
        if nodes and not name.startswith("unconnected-"):
            nets[name] = nodes
    return nets


def _pin_net():
    return {node: net for net, nodes in _sch_netlist().items() for node in nodes}


def test_erc_clean():
    import json
    os.makedirs(BUILD, exist_ok=True)
    out = os.path.join(BUILD, "erc.json")
    r = _cli("sch", "erc", "--format", "json", "--severity-all", "-o", out, SCH)
    assert r.returncode == 0, r.stdout + r.stderr
    rep = json.load(open(out))
    vs = [v for s in rep["sheets"] for v in s["violations"]]
    errors = [f"{v['type']}: {v['description']}" for v in vs if v["severity"] == "error"]
    assert not errors, "ERC errors:\n  " + "\n  ".join(errors)
    kept = open(README).read() if os.path.exists(README) else ""
    for v in vs:
        print(f"  ERC warning {v['type']}: {v['description']} {[i['description'] for i in v['items']]}")
        assert f"`{v['type']}`" in kept, f"ERC warning {v['type']} not explained in README.md"


def test_sch_netlist_matches_parts():
    from parts import nets
    got, want = _sch_netlist(), nets()
    diff = {n: (got.get(n), want.get(n)) for n in set(got) | set(want) if got.get(n) != want.get(n)}
    assert not diff, "schematic != parts.py:\n  " + "\n  ".join(f"{n}: sch {a} parts {b}" for n, (a, b) in sorted(diff.items()))


def test_critical_pins():
    from parts import E73_PINS
    pn = _pin_net()
    want = {
        "Q1": ["VBUS", "VSYS", "BAT"], "Q2": ["LCD_EN", "3V3", "LCD_VCC"], "Q3": ["MOTOR", "GND", "MOTOR_SW"],
        "D1": ["VSYS", "VBUS"], "D2": ["3V3", "MOTOR_SW"],  # pin 1 = K, 2 = A
        "U4": ["GND", "3V3", "VSYS"], "U3": ["STAT", "GND", "BAT", "VBUS", "PROG"],
    }
    for ref, ns in want.items():
        for i, n in enumerate(ns, 1):
            assert pn.get((ref, str(i))) == n, f"{ref} pin {i} is {pn.get((ref, str(i)))}, want {n}"
    for pin, n in {"1": "GND", "4": "IMU_INT1", "5": "IMU_PWR", "8": "IMU_PWR", "12": "IMU_PWR",
                   "13": "IMU_SCL", "14": "IMU_SDA"}.items():
        assert pn.get(("U2", pin)) == n, f"U2 pin {pin} is {pn.get(('U2', pin))}, want {n}"
    for pin in map(str, CASTELLATED):
        assert pn.get(("U1", pin)) == E73_PINS.get(pin), f"U1 pad {pin} is {pn.get(('U1', pin))}"
    # spec pin map, verbatim (guards parts.E73_PINS itself)
    spec = {"7": "BTN", "3": "LCD_CS", "4": "LCD_DC", "8": "LCD_RST", "6": "LCD_SCK", "10": "LCD_MOSI",
            "2": "LCD_EN", "1": "MOTOR", "9": "VBAT_ADC", "15": "IMU_SCL", "17": "IMU_SDA", "33": "IMU_PWR",
            "35": "CHG_STAT", "41": "NFC1", "43": "P010", "11": "XL1", "13": "XL2", "26": "RESET",
            "37": "SWDIO", "39": "SWCLK", "27": "VBUS", "29": "USB_DN", "31": "USB_DP", "19": "3V3",
            "23": "3V3", "5": "GND", "21": "GND"}
    for pin, n in spec.items():
        assert pn.get(("U1", pin)) == n, f"U1 pad {pin} is {pn.get(('U1', pin))}, spec says {n}"
    assert ("U1", "25") not in pn, "DCCH must stay unconnected"


def test_no_high_voltage_on_gpio():
    from gen_lib import E73_NAMES
    pn = _pin_net()
    high = {"VBUS", "VSYS", "BAT", "BATT_RAW", "STAT"}
    for pad, name in E73_NAMES.items():
        net = pn.get(("U1", str(pad)))
        if name != "VBUS":
            assert net not in high, f"U1 pad {pad} ({name}) on {net}"
    nets = _sch_netlist()
    bridges = {r for r, _ in nets["STAT"]} & {r for r, _ in nets["CHG_STAT"]}
    assert bridges == {"R4"}, f"STAT reaches CHG_STAT through {bridges}"


# ---------------------------------------------------------------- pcb (placed)

PCB = os.path.join(HERE, "mysterychain.kicad_pcb")
_board = None


def _brd():
    global _board
    if _board is None:
        import pcbnew
        assert os.path.exists(PCB), "no board yet"
        _board = pcbnew.LoadBoard(PCB)
    return _board


def _case(p):
    """KiCad point (nm) -> case coordinates (mm)."""
    import pcbnew
    return pcbnew.ToMM(p.x) - 100.0, 100.0 - pcbnew.ToMM(p.y)


def _inside(pt, poly):
    x, y = pt
    n, c = len(poly), False
    for i in range(n):
        (x1, y1), (x2, y2) = poly[i], poly[(i + 1) % n]
        if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1) + x1:
            c = not c
    return c


def _outline_pts(shape_poly, step=0.25):
    """Points every `step` mm along every outline of a SHAPE_POLY_SET, case coordinates."""
    import pcbnew
    pts = []
    for oi in range(shape_poly.OutlineCount()):
        o = shape_poly.Outline(oi)
        n = o.PointCount()
        for i in range(n):
            a, b = _case(o.CPoint(i)), _case(o.CPoint((i + 1) % n))
            k = max(1, int(math.dist(a, b) / step))
            pts += [(a[0] + (b[0] - a[0]) * t / k, a[1] + (b[1] - a[1]) * t / k) for t in range(k)]
    return pts


def _fps():
    return {f.GetReference(): f for f in _brd().GetFootprints()}


def test_backside_orientation():
    import pcbnew
    from parts import PARTS
    fps = _fps()
    for p in PARTS:
        if p.footprint:
            assert p.ref in fps, f"{p.ref} not on the board"
            assert fps[p.ref].IsFlipped() == (p.side == "B"), f"{p.ref} on the wrong side"
    u1 = _pads(fps["U1"])
    assert _case(u1["1"].GetPosition())[1] < _case(u1["26"].GetPosition())[1], "U1 pads 1-10 must face -Y"
    from gen_pcb import antenna_zone_case
    ax0, _, ax1, _ = antenna_zone_case()
    assert _case(u1["11"].GetPosition())[0] < (ax0 + ax1) / 2, "U1 antenna must be at +X"
    assert ax0 > max(_case(p.GetPosition())[0] for p in u1.values()), "antenna zone overlaps U1 pads"
    j1 = fps["J1"]
    edge = [g for g in j1.GraphicalItems() if g.GetLayer() == pcbnew.Dwgs_User and isinstance(g, pcbnew.PCB_SHAPE)]
    assert edge, "J1 has no PCB-edge line"
    ys = [_case(g.GetStart())[1] for g in edge] + [_case(g.GetEnd())[1] for g in edge]
    assert all(abs(y + 18.2) <= 0.05 for y in ys), f"J1 face at {ys}, want -18.2"
    assert all(_case(p.GetPosition())[1] > -18.2 for p in j1.Pads()), "J1 must open toward -Y"


def test_case_fit():
    import pcbnew
    from gen_pcb import CASE_SW2, NOTCHES, TONGUE_X, outline
    from parts import PARTS
    poly = outline()
    fps = _fps()
    for f in fps.values():
        ref = f.GetReference()
        layer = pcbnew.B_CrtYd if f.IsFlipped() else pcbnew.F_CrtYd
        for x, y in _outline_pts(f.GetCourtyard(layer)):
            if _inside((x, y), poly):
                continue
            own_edge = (ref == "J1" and y < -17.0 and abs(x) <= TONGUE_X) or (ref == "SW1" and x < -16.0 and abs(y) < 3.5)
            assert own_edge, f"{ref} courtyard leaves the board at ({x:.2f}, {y:.2f})"
        for p in f.Pads():
            for x, y in _outline_pts(p.GetEffectivePolygon(pcbnew.F_Cu if p.IsOnLayer(pcbnew.F_Cu) else pcbnew.B_Cu, pcbnew.ERROR_INSIDE)):
                in_tongue = abs(x) <= TONGUE_X and y < -16.5
                assert math.hypot(x, y) <= 17.7 or in_tongue, f"{ref} pad copper at r {math.hypot(x, y):.2f}"
                for cx, cy in NOTCHES:
                    assert math.hypot(x - cx, y - cy) >= 1.5 + 0.3, f"{ref} pad in notch at ({cx:.1f},{cy:.1f})"
        for cx, cy in NOTCHES:
            cyd = f.GetCourtyard(layer)
            assert not _inside((cx, cy), _outline_pts(cyd, 0.1)) if cyd.OutlineCount() else True
            for x, y in _outline_pts(cyd):
                assert math.hypot(x - cx, y - cy) >= 1.5, f"{ref} courtyard in notch at ({cx:.1f},{cy:.1f})"
    sx, sy = _case(fps["SW2"].GetPosition())
    assert math.dist((sx, sy), CASE_SW2) <= 0.1, f"SW2 at ({sx:.2f},{sy:.2f}), case slot wants {CASE_SW2}"
    for p in PARTS:
        if p.footprint:
            assert p.height <= (1.2 if p.side == "F" else 3.4), f"{p.ref} too tall"


def test_testpoints():
    import pcbnew
    from parts import TEST_NETS
    on_f = {p.GetNetname() for f in _brd().GetFootprints() if f.GetReference().startswith("TP")
            for p in f.Pads() if p.IsOnLayer(pcbnew.F_Cu) and not f.IsFlipped()}
    missing = set(TEST_NETS) - on_f
    assert not missing, f"no front test pad for {missing}"
    j5 = _pads(_fps()["J5"])
    assert [j5[str(i)].GetNetname() for i in range(1, 6)] == ["3V3", "SWDIO", "SWCLK", "RESET", "GND"]
    assert not _fps()["J5"].IsFlipped()


def _drc(*extra):
    import json
    os.makedirs(BUILD, exist_ok=True)
    out = os.path.join(BUILD, "drc.json")
    r = _cli("pcb", "drc", "--format", "json", "--schematic-parity", "--severity-all", *extra, "-o", out, PCB)
    assert r.returncode == 0, r.stdout + r.stderr
    return json.load(open(out))


def _npth_artifact(v):
    """KiCad 9 saves a flipped footprint's "F&B.Cu" NPTH pegs (stock J1/SW1) as "*.Cu"; on 4 layers DRC
    then says it no longer matches the library. True when those pegs' inner layers are the only difference."""
    import pcbnew
    from gen_pcb import load_fp
    if v["type"] != "lib_footprint_mismatch":
        return False
    f = pcbnew.FOOTPRINT(_fps()[v["items"][0]["description"].split()[-1]])
    f.SetParent(_brd())
    for p in f.Pads():
        if p.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH:
            s = p.GetLayerSet()
            for l in list(pcbnew.LSET.InternalCuMask().Seq()):
                s.RemoveLayer(l)
            p.SetLayerSet(s)
    return not f.FootprintNeedsUpdate(load_fp(f.GetFPIDAsString()))


def test_drc_placed():
    rep = _drc()
    # unrouted: the E73 fan-out vias have no front track yet
    bad = [f"{v['severity']} {v['type']}: {v['description']} {[i['description'] for i in v['items']]}"
           for v in rep["violations"] if v["type"] != "via_dangling" and not _npth_artifact(v)] + [f"parity {v['type']}: {v['description']} {[i['description'] for i in v['items']]}"
                                          for v in rep["schematic_parity"]]
    assert not bad, f"{len(bad)} DRC problems:\n  " + "\n  ".join(bad[:40])


# ---------------------------------------------------------------- pcb (routed)

def test_drc_clean():
    rep = _drc()  # kicad-cli 9 has no --refill-zones: route.py saves filled zones
    bad = [f"{v['severity']} {v['type']}: {v['description']} {[i['description'] for i in v['items']]}"
           for v in rep["violations"] + rep["schematic_parity"] if not _npth_artifact(v)]
    bad += [f"unconnected: {[i['description'] for i in v['items']]}" for v in rep["unconnected_items"]]
    assert not bad, f"{len(bad)} DRC problems:\n  " + "\n  ".join(bad[:40])


def _u1_zone(all_copper):
    """U1's antenna (all copper) or body (one layer) rule area on the board, as a SHAPE_POLY_SET."""
    u1 = _fps()["U1"]
    zs = [z for z in u1.Zones() if z.GetDoNotAllowPads() == all_copper]
    assert len(zs) == 1
    return zs[0].Outline()


def _hits(poly, shape):
    import pcbnew
    s = pcbnew.SHAPE_POLY_SET(poly)
    s.BooleanIntersection(shape)
    return s.Area() > 0


def test_stackup():
    """4 copper layers; In2.Cu is a power layer holding one GND plane; vias are tented."""
    import pcbnew
    brd = _brd()
    assert brd.GetCopperLayerCount() == 4, f"{brd.GetCopperLayerCount()} copper layers"
    assert brd.GetLayerType(pcbnew.In2_Cu) == pcbnew.LT_POWER, "In2.Cu is not a power layer"
    planes = [z.GetNetname() for z in brd.Zones() if not z.GetIsRuleArea() and z.IsOnLayer(pcbnew.In2_Cu)]
    assert planes == ["GND"], f"In2.Cu zones: {planes}"
    on_in2 = [t for t in brd.GetTracks() if t.GetClass() != "PCB_VIA" and t.GetLayer() == pcbnew.In2_Cu]
    assert not on_in2, f"{len(on_in2)} tracks on the In2.Cu plane"
    ds = brd.GetDesignSettings()
    assert ds.m_TentViasFront and ds.m_TentViasBack, "vias under the E73 must be tented"
    ant = [z for z in _fps()["U1"].Zones() if z.GetDoNotAllowPads()][0]
    cu = [brd.GetLayerName(l) for l in ant.GetLayerSet().CuStack() if brd.IsLayerEnabled(l)]
    assert len(cu) == 4, f"U1 antenna keep-out only on {cu}"


def test_gnd_plane_under_e73():
    """The filled In2.Cu plane covers >= 75 % of the U1 body area (via clearance holes eat the rest)."""
    import pcbnew
    body = _u1_zone(False)
    z = [z for z in _brd().Zones() if not z.GetIsRuleArea() and z.IsOnLayer(pcbnew.In2_Cu)][0]
    s = pcbnew.SHAPE_POLY_SET(body)
    s.BooleanIntersection(z.GetFilledPolysList(pcbnew.In2_Cu))
    assert s.Area() >= 0.75 * body.Area(), f"In2 GND covers {s.Area() / body.Area():.0%} of the E73 body"


def _cu_layers():
    return list(_brd().GetEnabledLayers().CuStack())


def test_antenna_clear():
    import pcbnew
    ant = _u1_zone(True)
    brd = _brd()
    for t in brd.GetTracks():
        for layer in _cu_layers():
            if t.IsOnLayer(layer):
                s = pcbnew.SHAPE_POLY_SET()
                t.TransformShapeToPolygon(s, layer, 0, pcbnew.FromMM(0.005), pcbnew.ERROR_OUTSIDE)
                assert not _hits(ant, s), f"{t.GetClass()} of {t.GetNetname()} in the antenna keep-out"
    for f in brd.GetFootprints():
        for p in f.Pads():
            for layer in _cu_layers():
                if p.IsOnLayer(layer):
                    assert not _hits(ant, p.GetEffectivePolygon(layer, pcbnew.ERROR_OUTSIDE)), \
                        f"pad {f.GetReference()}.{p.GetNumber()} in the antenna keep-out"
    for z in brd.Zones():
        if z.GetIsRuleArea():
            continue
        for layer in _cu_layers():
            if z.IsOnLayer(layer):
                assert not _hits(ant, z.GetFilledPolysList(layer)), f"{z.GetNetname()} pour in the antenna keep-out"


def test_e73_back_gnd():
    """No B.Cu track under the U1 body; tented vias may pass (spec, 4 layers)."""
    import pcbnew
    body = _u1_zone(False)
    for t in _brd().GetTracks():
        if t.GetClass() != "PCB_VIA" and t.IsOnLayer(pcbnew.B_Cu):
            s = pcbnew.SHAPE_POLY_SET()
            t.TransformShapeToPolygon(s, pcbnew.B_Cu, 0, pcbnew.FromMM(0.005), pcbnew.ERROR_OUTSIDE)
            assert not _hits(body, s), f"{t.GetClass()} of {t.GetNetname()} on B.Cu under the E73"


def test_usb_pair():
    import pcbnew
    length, layers = {}, {}
    for t in _brd().GetTracks():
        n = t.GetNetname()
        if n in ("USB_DP", "USB_DN"):
            if t.GetClass() == "PCB_TRACK":
                length[n] = length.get(n, 0) + pcbnew.ToMM(t.GetLength())
                layers.setdefault(n, set()).add(t.GetLayerName())
    assert set(length) == {"USB_DP", "USB_DN"}, "USB pair not routed"
    assert layers["USB_DP"] == layers["USB_DN"], f"USB pair on different layers {layers}"
    d = abs(length["USB_DP"] - length["USB_DN"])
    assert d <= 2.0, f"USB pair length mismatch {d:.2f} mm ({length})"


STAGES = {
    "parts": [test_parts_table, test_nets_named],
    "lib": [test_e73_pads, test_e73_zones, test_e73_symbol, test_custom_footprints, test_footprints_resolve],
    "sch": [test_erc_clean, test_sch_netlist_matches_parts, test_critical_pins, test_no_high_voltage_on_gpio],
    "pcb --placed": [test_backside_orientation, test_case_fit, test_testpoints, test_stackup, test_drc_placed],
    "pcb": [test_backside_orientation, test_case_fit, test_testpoints, test_stackup, test_drc_clean,
            test_antenna_clear, test_gnd_plane_under_e73, test_e73_back_gnd, test_usb_pair],
}


def main():
    stage = " ".join(sys.argv[1:])
    if stage not in STAGES:
        sys.exit(f"usage: check.py {'|'.join(STAGES)}")
    for t in STAGES[stage]:
        try:
            t()
        except Exception as e:  # noqa: BLE001 - report any failure the same way
            print(f"FAIL {t.__name__}: {type(e).__name__}: {e}")
            sys.exit(1)
        print(f"PASS {t.__name__}")


if __name__ == "__main__":
    main()
