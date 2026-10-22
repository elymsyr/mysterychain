#!/usr/bin/env python3
"""Autoroute mysterychain.kicad_pcb (4 layers) with Freerouting 1.9.0 (DSN -> SES), refill zones, save.

Idempotent: every unlocked track/via is removed first; the locked E73 fan-out from gen_pcb stays.
Single-threaded Freerouting, so reruns give the same board. Run with /usr/bin/python3 -I.
"""
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

PCB = os.path.join(HERE, "mysterychain.kicad_pcb")
BUILD = os.path.join(HERE, "build")
JAR = os.path.expanduser("~/.local/share/freerouting/freerouting-1.9.0.jar")
PASSES = 30
EDGE = 0.3          # min_copper_edge_clearance (gen_sch rules)
VIA_VIA = 0.25      # 0.6 vias -> centres 0.85 apart (Freerouting rounds) -> min_hole_to_hole 0.5


def stitch(board, pitch=1.5):
    """GND vias wherever one fits in the F and B pours and the In2 plane, away from In1 tracks and other
    holes: ties every pour scrap to the plane. Run after a fill; unlocked, so the next route.py run redoes it."""
    import pcbnew
    from gen_pcb import kxy, locked_via
    gnd = board.FindNet("GND")
    r = pcbnew.FromMM(0.3 + 0.05)                              # via radius + margin
    ok = None
    for layer in (pcbnew.F_Cu, pcbnew.In2_Cu, pcbnew.B_Cu):
        s = pcbnew.SHAPE_POLY_SET()
        for z in board.Zones():
            if not z.GetIsRuleArea() and z.GetNetname() == "GND" and z.IsOnLayer(layer):
                s.BooleanAdd(z.GetFilledPolysList(layer))
        s.Deflate(r, pcbnew.CORNER_STRATEGY_ROUND_ALL_CORNERS, pcbnew.FromMM(0.01))
        if ok is None:
            ok = s
        else:
            ok.BooleanIntersection(s)
    # (centre, min distance): via hole r 0.15 + min_hole_to_hole 0.5 + the other hole's radius
    holes = [(t.GetPosition(), pcbnew.FromMM(0.8)) for t in board.GetTracks() if t.GetClass() == "PCB_VIA"]
    holes += [(p.GetPosition(), pcbnew.FromMM(0.65) + max(p.GetDrillSize().x, p.GetDrillSize().y) // 2)
              for f in board.GetFootprints() for p in f.Pads() if p.HasHole()]
    in1 = pcbnew.SHAPE_POLY_SET()
    for t in board.GetTracks():
        if t.GetClass() != "PCB_VIA" and t.GetLayer() == pcbnew.In1_Cu:
            t.TransformShapeToPolygon(in1, pcbnew.In1_Cu, r + pcbnew.FromMM(0.15), pcbnew.FromMM(0.01), pcbnew.ERROR_OUTSIDE)
    added = []
    for i in range(-180, 181, 3):
        for j in range(-185, 71, 3):
            p = kxy(i / 10, j / 10)
            if not ok.Contains(p) or in1.Contains(p):
                continue
            if any((p - h).EuclideanNorm() < d for h, d in holes):
                continue
            if any((p - a).EuclideanNorm() < pcbnew.FromMM(pitch) for a in added):
                continue
            added.append(p)
    for p in added:
        locked_via(board, (pcbnew.ToMM(p.x) - 100, 100 - pcbnew.ToMM(p.y)), gnd).SetLocked(False)
    return len(added)


def edge_band(board):
    """Rule area EDGE wide inside the outline on every copper layer: Freerouting only keeps its default
    clearance from the board edge. Routing aid only, removed before save (DRC checks the real rule)."""
    import pcbnew
    from gen_pcb import kxy, outline
    outer = pcbnew.SHAPE_POLY_SET()
    outer.NewOutline()
    for p in outline():
        v = kxy(*p)
        outer.Append(v.x, v.y)
    inner = pcbnew.SHAPE_POLY_SET(outer)
    inner.Deflate(pcbnew.FromMM(EDGE), pcbnew.CORNER_STRATEGY_ROUND_ALL_CORNERS, pcbnew.FromMM(0.01))
    outer.BooleanSubtract(inner)
    z = pcbnew.ZONE(board)
    z.SetIsRuleArea(True)
    z.SetLayerSet(pcbnew.LSET.AllCuMask())
    z.SetDoNotAllowTracks(True)
    z.SetDoNotAllowVias(True)
    z.SetDoNotAllowPads(False)
    z.SetDoNotAllowCopperPour(False)
    z.SetDoNotAllowFootprints(False)
    z.Outline().BooleanAdd(outer)
    board.Add(z)
    return z


def main():
    import pcbnew
    assert os.path.exists(JAR), f"Freerouting missing: {JAR} (see README)"
    board = pcbnew.LoadBoard(PCB)
    for t in list(board.GetTracks()):
        if not t.IsLocked():
            board.Remove(t)
    os.makedirs(BUILD, exist_ok=True)
    dsn, ses = os.path.join(BUILD, "mysterychain.dsn"), os.path.join(BUILD, "mysterychain.ses")
    if os.path.exists(ses):
        os.remove(ses)
    # F/B GND pours would go out as full-board planes and leave the router no room: route without them.
    # The In2.Cu plane stays: on a power layer Freerouting never routes and joins GND pads to it by vias.
    pours = [z for z in board.Zones() if not z.GetIsRuleArea() and not z.IsOnLayer(pcbnew.In2_Cu)]
    for z in pours:
        board.Remove(z)
    band = edge_band(board)
    assert pcbnew.ExportSpecctraDSN(board, dsn), "DSN export failed"
    board.Remove(band)
    # KiCad's hole-to-hole rule has no DSN form: a via-to-via clearance stands in for it
    text, n = re.subn(r"(\(clearance [\d.]+ \(type smd_smd\)\))", rf"\1\n      (clearance {VIA_VIA * 1000:g} (type via_via))",
                      open(dsn).read(), count=1)
    assert n == 1, "DSN rule block not found"
    open(dsn, "w").write(text)
    log = os.path.join(BUILD, "freerouting.log")
    with open(log, "w") as out:
        # Freerouting 1.9 needs a display even in batch mode: a virtual one keeps it off the desktop
        xvfb = ["xvfb-run", "-a"] if shutil.which("xvfb-run") else []
        subprocess.run([*xvfb, "java", "-jar", JAR, "-de", dsn, "-do", ses, "-mp", str(PASSES), "-mt", "1"],
                       stdout=out, stderr=subprocess.STDOUT, check=True, timeout=3600)
    assert os.path.exists(ses), f"Freerouting wrote no session, see {log}"
    assert pcbnew.ImportSpecctraSES(board, ses), "SES import failed"
    for z in pours:
        board.Add(z)
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    n = stitch(board)
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    print("route: stitching vias", n)
    pcbnew.SaveBoard(PCB, board, True)
    print("route:", PCB, f"{sum(1 for _ in board.GetTracks())} tracks/vias")


if __name__ == "__main__":
    main()
