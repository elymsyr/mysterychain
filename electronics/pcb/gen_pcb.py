#!/usr/bin/env python3
"""Build mysterychain.kicad_pcb: 4 layers, outline, placed footprints, nets, GND pours. Unrouted.

Stackup (spec 2026-10-21): F.Cu signal, In1.Cu signal, In2.Cu solid GND plane (power layer), B.Cu = E73 side.

Run with /usr/bin/python3 -I after gen_sch.py (nets come from the schematic's netlist, so the board
matches it exactly, including KiCad's 'unconnected-(...)' pin nets).
Case coordinates: origin = display centre, +Y toward the lug. KiCad x = X + 100, KiCad y = 100 - Y.
"""
import math
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

R_BOARD = 18.0          # board radius (cover hook arms start at r 18.25)
TOP_Y = 7.0             # battery bay starts at 7.1
USB_FACE_Y = -18.2      # J1 face = board edge line in the tongue
TONGUE_X = 5.6          # J1 courtyard is +/-5.32
NOTCH_R, NOTCH_D = 17.6, 3.0
NOTCHES = [(NOTCH_R * math.cos(math.radians(a)), NOTCH_R * math.sin(math.radians(a))) for a in (205, 335)]
# EG1390 body centre. v7 had it at (-11.25, -7.0); its gull-wing pads reach 1.8 mm past the body and
# would cut into the 205 deg leg notch there, so it sits 0.7 mm toward +X and case v8 moves the slot.
CASE_SW2 = (-10.55, -7.0)
PCB = os.path.join(HERE, "mysterychain.kicad_pcb")
NETLIST = os.path.join(HERE, "build", "mysterychain.net")


# ---------------------------------------------------------------- outline (pure Python: case v8 reuses it)

def outline(step_deg=0.5):
    """Board outline, case coordinates, closed CCW polygon (last point != first)."""
    r = R_BOARD
    a_top = math.degrees(math.asin(TOP_Y / r))                       # right end of the top cut
    a_tl = 180 + math.degrees(math.atan2(math.sqrt(r * r - TONGUE_X ** 2), TONGUE_X))  # (-5.6, -17.1)
    a_tr = 360 - math.degrees(math.atan2(math.sqrt(r * r - TONGUE_X ** 2), TONGUE_X))  # (5.6, -17.1)
    cos_d = (r * r + NOTCH_R ** 2 - (NOTCH_D / 2) ** 2) / (2 * r * NOTCH_R)
    half = math.degrees(math.acos(cos_d))                              # notch half-width on the rim
    feats = []                                                         # (start angle, end angle, points)
    for a in (205, 335):
        cx, cy = NOTCH_R * math.cos(math.radians(a)), NOTCH_R * math.sin(math.radians(a))
        p1 = (r * math.cos(math.radians(a - half)), r * math.sin(math.radians(a - half)))
        p2 = (r * math.cos(math.radians(a + half)), r * math.sin(math.radians(a + half)))
        f1 = math.atan2(p1[1] - cy, p1[0] - cx)
        f2 = math.atan2(p2[1] - cy, p2[0] - cx)
        if f2 > f1:                                                   # concave: walk clockwise around the notch centre
            f2 -= 2 * math.pi
        n = max(2, int(abs(f2 - f1) / math.radians(10)))
        pts = [(cx + NOTCH_D / 2 * math.cos(f1 + (f2 - f1) * i / n), cy + NOTCH_D / 2 * math.sin(f1 + (f2 - f1) * i / n))
               for i in range(n + 1)]
        feats.append((a - half, a + half, pts))
    ty = -math.sqrt(r * r - TONGUE_X ** 2)
    feats.append((a_tl, a_tr, [(-TONGUE_X, ty), (-TONGUE_X, USB_FACE_Y), (TONGUE_X, USB_FACE_Y), (TONGUE_X, ty)]))
    feats.sort()
    pts = [(r * math.cos(math.radians(a_top)), TOP_Y), (-r * math.cos(math.radians(a_top)), TOP_Y)]
    a, end = 180 - a_top, 360 + a_top
    fi = 0
    while a < end:
        if fi < len(feats) and a >= feats[fi][0]:
            pts += feats[fi][2]
            a = feats[fi][1]
            fi += 1
            continue
        pts.append((r * math.cos(math.radians(a)), r * math.sin(math.radians(a))))
        a += step_deg
    out = []
    for p in pts:  # drop repeats closer than 1 um
        if not out or math.dist(p, out[-1]) > 1e-3:
            out.append((round(p[0], 4), round(p[1], 4)))
    return out


# ---------------------------------------------------------------- board

def mm(v):
    import pcbnew
    return pcbnew.FromMM(v)


def kxy(X, Y):
    import pcbnew
    return pcbnew.VECTOR2I(mm(X + 100.0), mm(100.0 - Y))


def antenna_zone_case(board=None):
    """U1's all-copper keep-out on the board, as a case-coordinate box (x0, y0, x1, y1)."""
    import pcbnew
    board = board or pcbnew.LoadBoard(PCB)
    u1 = [f for f in board.GetFootprints() if f.GetReference() == "U1"][0]
    for z in u1.Zones():
        if z.GetDoNotAllowPads():
            bb = z.GetBoundingBox()
            xs = [pcbnew.ToMM(bb.GetLeft()) - 100, pcbnew.ToMM(bb.GetRight()) - 100]
            ys = [100 - pcbnew.ToMM(bb.GetTop()), 100 - pcbnew.ToMM(bb.GetBottom())]
            return min(xs), min(ys), max(xs), max(ys)
    raise AssertionError("U1 has no antenna keep-out")


def schematic_nets():
    """(ref, pin) -> net name from the schematic netlist."""
    from sexp import find, loads
    os.makedirs(os.path.dirname(NETLIST), exist_ok=True)
    subprocess.run(["kicad-cli", "sch", "export", "netlist", "--format", "kicadsexpr", "-o", NETLIST,
                    os.path.join(HERE, "mysterychain.kicad_sch")], check=True, capture_output=True)
    out = {}
    for n in find(find(loads(open(NETLIST).read()), "nets")[0], "net"):
        for node in find(n, "node"):
            out[(find(node, "ref")[0][1], find(node, "pin")[0][1])] = find(n, "name")[0][1]
    return out


def load_fp(lib_id):
    import pcbnew
    lib, name = lib_id.split(":")
    path = os.path.join(HERE, "lib", "mysterychain.pretty") if lib == "mysterychain" else f"/usr/share/kicad/footprints/{lib}.pretty"
    fp = pcbnew.FootprintLoad(path, name)
    fp.SetFPID(pcbnew.LIB_ID(lib, name))
    return fp


def edge_cuts(board):
    import pcbnew
    pts = outline()
    for a, b in zip(pts, pts[1:] + pts[:1]):
        s = pcbnew.PCB_SHAPE(board)
        s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetStart(kxy(*a))
        s.SetEnd(kxy(*b))
        s.SetLayer(pcbnew.Edge_Cuts)
        s.SetWidth(mm(0.05))
        board.Add(s)


def gnd_pour(board, net, layer, solid=False):
    """GND pour over the whole outline. solid: the In2 plane, pads join it directly (no spokes to starve)."""
    import pcbnew
    z = pcbnew.ZONE(board)
    z.SetLayer(layer)
    z.SetNet(net)
    z.SetAssignedPriority(0)
    z.SetLocalClearance(mm(0.25))
    z.SetMinThickness(mm(0.2))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_FULL if solid else pcbnew.ZONE_CONNECTION_THERMAL)
    z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)  # unstitched F/B scraps are not GND
    z.SetThermalReliefGap(mm(0.25))
    z.SetThermalReliefSpokeWidth(mm(0.3))
    o = z.Outline()
    o.NewOutline()
    for p in outline():
        v = kxy(*p)
        o.Append(v.x, v.y)
    board.Add(z)


def fiducial_keepouts(board):
    """No tracks/vias inside a fiducial's pad clearance: Freerouting ignores pad-local clearances."""
    import pcbnew
    for f in board.GetFootprints():
        if not f.GetReference().startswith("FID"):
            continue
        p = list(f.Pads())[0]
        layer = pcbnew.B_Cu if f.IsFlipped() else pcbnew.F_Cu  # GetPrincipalLayer() says F.Cu on flipped pads
        r = p.GetSize(layer).x // 2 + p.GetLocalClearance()
        z = pcbnew.ZONE(board)
        z.SetIsRuleArea(True)
        z.SetLayer(layer)
        z.SetDoNotAllowTracks(True)
        z.SetDoNotAllowVias(True)
        z.SetDoNotAllowPads(False)
        z.SetDoNotAllowCopperPour(False)
        z.SetDoNotAllowFootprints(False)
        o = z.Outline()
        o.NewOutline()
        c = p.GetPosition()
        for i in range(16):
            a = 2 * math.pi * i / 16
            o.Append(c.x + int(r / math.cos(math.pi / 16) * math.cos(a)), c.y + int(r / math.cos(math.pi / 16) * math.sin(a)))
        board.Add(z)


def text(board, s, X, Y, layer, size=0.8, rot=0):
    import pcbnew
    t = pcbnew.PCB_TEXT(board)
    t.SetText(s)
    t.SetPosition(kxy(X, Y))
    t.SetLayer(layer)
    t.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size)))
    t.SetTextThickness(mm(0.15))
    t.SetTextAngleDegrees(rot)
    if layer == pcbnew.B_SilkS:
        t.SetMirrored(True)
    board.Add(t)
    return t


def locked_track(board, a, b, net, width=0.25, layer=None):
    import pcbnew
    t = pcbnew.PCB_TRACK(board)
    t.SetStart(kxy(*a))
    t.SetEnd(kxy(*b))
    t.SetWidth(mm(width))
    t.SetLayer(pcbnew.B_Cu if layer is None else layer)
    t.SetNet(net)
    t.SetLocked(True)
    board.Add(t)


def locked_via(board, at, net):
    import pcbnew
    v = pcbnew.PCB_VIA(board)
    v.SetPosition(kxy(*at))
    v.SetViaType(pcbnew.VIATYPE_THROUGH)
    v.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
    v.SetDrill(mm(0.3))
    v.SetWidth(mm(0.6))
    v.SetNet(net)
    v.SetLocked(True)
    board.Add(v)
    return v


# J1 fan-out on B.Cu, case coordinates, locked: the router cannot get J1's 0.5-pitch pads out (the E73
# bottom via row is full and 0.23 mm above them). Pads at Y -10.85; every signal pad drops to its own via
# under the J1 body, in two rows (Y -12.15 / -12.95, hole-to-hole >= 0.5, >= 0.875 from the NPTH pegs at
# X +/-2.89); the router joins the pairs. Pad order: VBUS -2.4, CC2 -1.75, SBU1, D+ -0.75, D- -0.25,
# D+ 0.25, D- 0.75, CC1 1.25, SBU2, VBUS 2.4.
J1_FANOUT = {
    "USB_DP": [[(-0.75, -10.85), (-0.75, -11.8), (-0.85, -12.15)], [(0.25, -10.85), (0.25, -12.6), (0.41, -12.95)]],
    "USB_DN": [[(-0.25, -10.85), (-0.25, -12.6), (-0.41, -12.95)], [(0.75, -10.85), (0.75, -11.8), (0.85, -12.15)]],
    "CC1": [[(1.25, -10.85), (1.25, -11.5), (1.5, -11.8), (1.5, -12.95)]],
    "CC2": [[(-1.75, -10.85), (-1.75, -12.95)]],
    "VBUS": [[(-2.2, -10.85), (-2.2, -12.3), (-2.6, -12.95)], [(2.2, -10.85), (2.2, -12.3), (2.6, -12.95)]],
}


def j1_fanout(board):
    for name, paths in J1_FANOUT.items():
        net = board.FindNet(name)
        for path in paths:  # pad centre ... via
            for a, b in zip(path, path[1:]):
                locked_track(board, a, b, net, 0.2)
            locked_via(board, path[-1], net)


# Locked vias at pads the router cannot reach from the front, as (ref, pad) -> offset from the pad centre
# (case mm). J2 (display pins): 1.75 below the pad, the back J3/J4 pads sit right behind it. J5 (SWD) and
# TP9/TP14: in the pad, tented E73 body behind (RESET via kept 0.4 below the E73 inner pad row).
PAD_VIAS = {**{("J2", n): (0.0, -1.75) for n in "34567"},
            ("J5", "2"): (-0.6, 0.0), ("J5", "3"): (-0.6, 0.0), ("J5", "4"): (-0.6, -0.34),
            ("TP9", "1"): (0.0, 0.0), ("TP14", "1"): (0.0, 0.3)}


def pad_vias(board):
    import pcbnew
    fps = {f.GetReference(): f for f in board.GetFootprints()}
    for (ref, num), (dx, dy) in PAD_VIAS.items():
        pad = [p for p in fps[ref].Pads() if p.GetNumber() == num][0]
        X, Y = pcbnew.ToMM(pad.GetPosition().x) - 100, 100 - pcbnew.ToMM(pad.GetPosition().y)
        at = (X + dx, Y + dy)
        if (dx, dy) != (0.0, 0.0):
            locked_track(board, (X, Y), at, pad.GetNet(), 0.2, pcbnew.B_Cu if fps[ref].IsFlipped() else pcbnew.F_Cu)
        locked_via(board, at, pad.GetNet())


FANOUT_Y = {"bottom": -9.75, "top": 6.35}   # via rows past the E73 pad tips (J1 copper / board edge beyond)


def fanout(board, u1):
    """Locked back stub + via per top/bottom-row signal pad of U1: the only via spots past the pad tips
    (board edge above, J1 copper below) are too tight to leave to the router. route.py keeps them."""
    import pcbnew
    X0, Y0 = pcbnew.ToMM(u1.GetPosition().x) - 100, 100 - pcbnew.ToMM(u1.GetPosition().y)
    for pad in u1.Pads():
        net = pad.GetNetname()
        if not net or net == "GND" or net.startswith("unconnected-"):
            continue
        X, Y = pcbnew.ToMM(pad.GetPosition().x) - 100, 100 - pcbnew.ToMM(pad.GetPosition().y)
        if abs(Y - (Y0 - 13.0)) < 0.01:
            via = (X, FANOUT_Y["bottom"])
        elif abs(Y - Y0) < 0.01:
            via = (X, FANOUT_Y["top"])
        elif abs(X - X0) < 0.01:
            continue  # left column: the back is open to the left of the module, the router fans these out
        else:
            raise AssertionError(f"U1 pad {pad.GetNumber()} ({net}) is not a castellated edge pad")
        locked_track(board, (X, Y), via, pad.GetNet())
        locked_via(board, via, pad.GetNet())


def pad_names(board, fp, names):
    """Silk name beside each pad of a pad row, on the side facing the board centre."""
    import pcbnew
    pads = sorted(fp.Pads(), key=lambda q: int(q.GetNumber()))
    a, b = pads[0].GetPosition(), pads[1].GetPosition()
    along_x = abs(b.x - a.x) > abs(b.y - a.y)
    layer = pcbnew.B_SilkS if fp.IsFlipped() else pcbnew.F_SilkS
    for pad, name in zip(pads, names):
        c = pad.GetPosition()
        X, Y = pcbnew.ToMM(c.x) - 100, 100 - pcbnew.ToMM(c.y)
        t = text(board, name, X, Y, layer, 0.8, 90 if along_x else 0)
        bb, pb = t.GetBoundingBox(), pad.GetBoundingBox()
        if along_x:  # row along X: name below/above, toward Y = 0 (KiCad y grows downward)
            dy = (pb.GetHeight() + bb.GetHeight()) // 2 + mm(0.25)
            t.Move(pcbnew.VECTOR2I(0, dy if Y > 0 else -dy))
        else:        # row along Y: name left/right, toward X = 0
            dx = (pb.GetWidth() + bb.GetWidth()) // 2 + mm(0.25)
            t.Move(pcbnew.VECTOR2I(-dx if X > 0 else dx, 0))


def build():
    import pcbnew
    from gen_sch import uid
    from parts import PADROW_NAMES, PARTS, TEXTS
    board = pcbnew.NewBoard(PCB)  # picks up mysterychain.kicad_pro (rules, net class) from gen_sch
    board.SetCopperLayerCount(4)
    ds = board.GetDesignSettings()
    ds.SetBoardThickness(mm(1.0))
    ds.m_TentViasFront = ds.m_TentViasBack = True       # vias pass under the E73 body
    ds.SetAuxOrigin(kxy(0, 0))
    ds.SetGridOrigin(kxy(0, 0))
    edge_cuts(board)

    pin_net = schematic_nets()
    nets = {}
    for name in sorted(set(pin_net.values())):
        nets[name] = pcbnew.NETINFO_ITEM(board, name)
        board.Add(nets[name])

    for p in PARTS:
        if not p.footprint:
            continue
        assert p.pcb, f"{p.ref} has no placement"
        X, Y, rot = p.pcb
        fp = load_fp(p.footprint)
        fp.SetReference(p.ref)
        fp.SetValue(p.value)
        fp.SetExcludedFromBOM(not p.bom)
        fp.SetPath(pcbnew.KIID_PATH("/" + str(uid(p.ref))))
        fp.SetSheetname("Root")
        fp.SetSheetfile("mysterychain.kicad_sch")
        board.Add(fp)
        fp.SetPosition(kxy(X, Y))
        if p.side == "B":
            fp.Flip(fp.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
        fp.SetOrientationDegrees(rot)
        for pad in fp.Pads():
            net = pin_net.get((p.ref, pad.GetNumber()))
            if net:
                pad.SetNet(nets[net])
        # too dense for silk refs: they stay on the Fab layers (assembly PDF)
        fp.Reference().SetVisible(False)
        if p.ref in PADROW_NAMES:
            pad_names(board, fp, PADROW_NAMES[p.ref])

    # In2 as a power layer: Freerouting gets it as a plane and never routes on it. Set only after the
    # back parts are flipped: KiCad 9 Flip() with a power layer drops every inner layer from footprint
    # zones (U1's antenna keep-out would leave In1/In2 open to the GND plane).
    board.SetLayerType(pcbnew.In2_Cu, pcbnew.LT_POWER)
    fanout(board, [f for f in board.GetFootprints() if f.GetReference() == "U1"][0])
    j1_fanout(board)
    pad_vias(board)
    fiducial_keepouts(board)
    for s, X, Y, layer, rot in TEXTS:
        text(board, s, X, Y, getattr(pcbnew, layer), 0.8, rot)
    gnd_pour(board, nets["GND"], pcbnew.F_Cu)
    gnd_pour(board, nets["GND"], pcbnew.In2_Cu, solid=True)
    gnd_pour(board, nets["GND"], pcbnew.B_Cu)
    pcbnew.SaveBoard(PCB, board, True)  # keep gen_sch's .kicad_pro as the single source of rules
    return board


if __name__ == "__main__":
    pts = outline()
    assert len(pts) > 100 and all(y <= TOP_Y + 1e-9 for _, y in pts) and min(y for _, y in pts) == USB_FACE_Y
    for cx, cy in NOTCHES:  # each notch bites in to r 16.1 (17.6 - 1.5)
        near = [p for p in pts if math.dist(p, (cx, cy)) <= NOTCH_D / 2 + 1e-6]
        assert near and abs(min(math.hypot(*p) for p in near) - (NOTCH_R - NOTCH_D / 2)) < 0.05, (cx, cy)
    build()
    print("gen_pcb:", PCB)
