"""MysteryChain case - 2D technical drawing set (A3 landscape, 2:1, first-angle projection).

Every view is a hidden-line projection of the solids built by pet_case.step.py, so the
drawing follows the model. Sheets sit side by side in modelspace: geometry at 1:1, sheet
frames drawn at A3 / 2, so the 2:1 plot needs no viewport.

    scripts/gen pet_case.dxf.py --write     -> pet_case.dxf (DXF, editable dimensions)
    python pet_case.dxf.py                  -> pet_case_drawing.pdf (one A3 page per sheet)
"""

import math
import textwrap
from pathlib import Path

import ezdxf
from build123d import Compound, Cylinder, GeomType, Keep, Plane, PositionMode, Rot, Vector
from cadgen.sources import load_source_module
from ezdxf.enums import TextEntityAlignment

HERE = Path(__file__).resolve().parent
pc = load_source_module(HERE / "pet_case.step.py")
up = load_source_module(HERE / "usb_plug.step.py")

SCALE = 2.0
SW, SH = 420 / SCALE, 297 / SCALE      # A3 landscape in model units
MM = 1 / SCALE                         # 1 mm on paper, in model units
SHEET_GAP = 20
REV, DATE, DRAWN = "v7", "2026-10-16", "O. Eren"
FONT = "LiberationSans"

H, RI, RO = pc.H, pc.RI, pc.RO


# ---------------------------------------------------------------- document
def new_doc():
    doc = ezdxf.new("R2018", setup=True, units=ezdxf.units.MM)
    doc.header["$LTSCALE"] = 0.125      # DASHED -> 3.2 / 1.6 mm dashes on paper
    for name, lw, lt, color in (
        ("VISIBLE", 50, "Continuous", 7),
        ("HIDDEN", 25, "DASHED", 7),
        ("CENTER", 18, "CENTER", 7),
        ("PHANTOM", 18, "PHANTOM", 8),
        ("SECTION_HATCH", 13, "Continuous", 8),
        ("DETAIL_BOUNDARY", 25, "Continuous", 7),
        ("DIM", 18, "Continuous", 7),
        ("TEXT", 25, "Continuous", 7),
        ("FRAME", 70, "Continuous", 7),
        ("TITLE", 35, "Continuous", 7),
    ):
        doc.layers.add(name, color=color, linetype=lt, lineweight=lw)
    doc.dimstyles.new("ISO", dxfattribs={
        "dimscale": MM, "dimtxt": 2.5, "dimasz": 2.5, "dimexe": 1.25, "dimexo": 0.625,
        "dimgap": 0.625, "dimtad": 1, "dimdec": 2, "dimzin": 8, "dimdsep": ord("."),
        "dimtih": 0, "dimtoh": 0, "dimtxsty": FONT, "dimblk": "", "dimlfac": 1.0,
        "dimtofl": 1, "dimatfit": 3, "dimtmove": 0, "dimcen": 0,
    })
    return doc


class View:
    """Orthographic (or scaled detail) view: world point -> sheet point."""

    def __init__(self, cam, up, origin, s=1.0, pivot=(0, 0, 0)):
        self.Z = Vector(cam).normalized()
        self.Y = Vector(up).normalized()
        self.X = self.Y.cross(self.Z)
        self.o, self.s = origin, s
        pv = Vector(pivot)
        self.pu, self.pv = pv.dot(self.X), pv.dot(self.Y)

    def uv(self, u, v):          # camera-plane coords (as returned by HLR) -> sheet
        return (self.o[0] + self.s * (u - self.pu), self.o[1] + self.s * (v - self.pv))

    def p(self, x, y, z):        # world -> sheet
        w = Vector(x, y, z)
        return self.uv(w.dot(self.X), w.dot(self.Y))


FRONT, REAR = ((0, 0, 1), (0, 1, 0)), ((0, 0, -1), (0, 1, 0))
LEFT = ((-1, 0, 0), (0, 1, 0))        # first angle: drawn right of the front view
TOP = ((0, 1, 0), (0, 0, -1))         # first angle: drawn below the front view
BOTTOM = ((0, -1, 0), (0, 0, 1))      # first angle: drawn above the front view
ISO = ((1, -1, 1.25), (0, 1, 0))


class Sheet:
    def __init__(self, doc, index):
        self.doc, self.msp = doc, doc.modelspace()
        self.x0, self.y0 = index * (SW + SHEET_GAP), 0.0
        self._seen = set()

    def at(self, x, y):
        return (self.x0 + x, self.y0 + y)

    def view(self, dirs, x, y, s=1.0, pivot=(0, 0, 0)):
        return View(*dirs, self.at(x, y), s, pivot)

    # ---- geometry
    def line(self, a, b, layer):
        self.msp.add_line(a, b, dxfattribs={"layer": layer})

    def edges(self, edges, view, layer, boundary=None):
        for e in edges:
            if e.length < 1e-4 or (boundary and _on_circle(e, view, *boundary)):
                continue
            pts = [view.uv(p.X, p.Y) for p in (e @ 0, e @ 0.5, e @ 1)]
            if e.geom_type == GeomType.CIRCLE and e.is_closed:     # seam position varies between copies
                c0 = e.arc_center
                pts = [view.uv(c0.X, c0.Y), (round(e.radius, 4), 0)]
            key = (layer, e.geom_type.name, tuple(sorted((round(x, 4), round(y, 4)) for x, y in pts)))
            if key in self._seen:
                continue
            self._seen.add(key)
            att = {"layer": layer}
            if e.geom_type == GeomType.LINE:
                self.msp.add_line(pts[0], pts[2], dxfattribs=att)
            elif e.geom_type == GeomType.CIRCLE:
                c0 = e.arc_center
                c = view.uv(c0.X, c0.Y)
                r = e.radius * view.s
                if e.is_closed:
                    self.msp.add_circle(c, r, dxfattribs=att)
                else:
                    a0, am, a1 = (math.degrees(math.atan2(y - c[1], x - c[0])) % 360 for x, y in pts)
                    if (am - a0) % 360 > (a1 - a0) % 360:
                        a0, a1 = a1, a0
                    self.msp.add_arc(c, r, a0, a1, dxfattribs=att)
            else:
                n = max(8, int(e.length / 0.1))
                self.msp.add_lwpolyline(
                    [view.uv(p.X, p.Y) for p in e.positions([i / n for i in range(n + 1)])],
                    dxfattribs=att)

    def project(self, shape, view, hidden=False, boundary=None, layer="VISIBLE"):
        vis, hid = shape.project_to_viewport(view.Z * 1000, view.Y, look_at=(0, 0, 0))
        self.edges(vis, view, layer, boundary)
        if hidden:
            self.edges(hid, view, "HIDDEN")

    def section(self, parts, view, offset=0.0, crop=None):
        """Cut at the plane `offset` along the view axis, keep the far side, hatch the cut faces.
        parts: a shape, or [(shape, fill)] with fill = (ANSI31 angle, scale) or "solid".
        crop = (world centre, radius): circular detail around that point."""
        if not isinstance(parts, list):
            parts = [(parts, (0, 0.125))]
        halves = []
        for shape, fill in parts:
            half = shape.split(Plane(view.Z * offset, z_dir=-view.Z), keep=Keep.TOP)
            half = Compound(half) if isinstance(half, list) else half
            if half is None:          # part lies entirely on the removed side
                continue
            if crop:
                half = half.intersect(Plane(Vector(crop[0]), z_dir=view.Z) * Cylinder(crop[1], 200))
            halves.append((half, fill))
        boundary = None
        if crop:
            boundary = (*view.p(*crop[0]), crop[1] * view.s)
            self.msp.add_circle(boundary[:2], boundary[2], dxfattribs={"layer": "DETAIL_BOUNDARY"})
        self.project(Compound([h for h, _ in halves]), view, boundary=boundary)
        for half, fill in halves:
            for f in half.faces():
                if abs(f.normal_at().dot(view.Z) - 1) < 1e-6 and abs(f.center().dot(view.Z) - offset) < 1e-6:
                    self.hatch(f, view, fill)

    def hatch(self, face, view, fill):
        h = self.msp.add_hatch(color=9 if fill == "solid" else 256, dxfattribs={"layer": "SECTION_HATCH"})
        if fill == "solid":
            h.set_solid_fill(color=9)
        else:
            h.set_pattern_fill("ANSI31", scale=fill[1], angle=fill[0])
        for i, w in enumerate([face.outer_wire(), *face.inner_wires()]):
            n = max(16, int(w.length / 0.05))
            pts = w.positions([w.length * k / n for k in range(n)], PositionMode.LENGTH)
            h.paths.add_polyline_path(
                [view.p(p.X, p.Y, p.Z) for p in pts], is_closed=True,
                flags=ezdxf.const.BOUNDARY_PATH_EXTERNAL if i == 0 else ezdxf.const.BOUNDARY_PATH_DEFAULT)

    # ---- annotation
    def text(self, p, s, h=2.5, align="MIDDLE_CENTER", layer="TEXT", rot=0.0):
        t = self.msp.add_text(s, height=h * MM, rotation=rot, dxfattribs={"layer": layer, "style": FONT})
        t.set_placement(p, align=TextEntityAlignment[align])

    def mtext(self, p, s, h=2.5, width=None):
        m = self.msp.add_mtext(s, dxfattribs={"layer": "TEXT", "style": FONT, "char_height": h * MM})
        if width:
            m.dxf.width = width
        m.set_location(p, attachment_point=ezdxf.enums.MTextEntityAlignment.TOP_LEFT)

    def dim(self, p1, p2, base, vertical=False, text="<>", location=None, **ov):
        if location:
            ov.setdefault("dimtmove", 2)
        d = self.msp.add_linear_dim(base=base, p1=p1, p2=p2, angle=90 if vertical else 0, text=text,
                                    location=location,
                                    dimstyle="ISO", override=ov or None, dxfattribs={"layer": "DIM"})
        d.render()

    def aligned(self, p1, p2, distance, text="<>", **ov):
        d = self.msp.add_aligned_dim(p1=p1, p2=p2, distance=distance, text=text, dimstyle="ISO",
                                     override=ov or None, dxfattribs={"layer": "DIM"})
        d.render()

    def dia(self, c, r, angle, text="<>", location=None, **ov):
        d = self.msp.add_diameter_dim(center=c, radius=r, angle=angle, text=text, location=location,
                                      dimstyle="ISO", override=ov or None, dxfattribs={"layer": "DIM"})
        d.render()

    def rad(self, c, r, angle, text="<>", location=None, **ov):
        d = self.msp.add_radius_dim(center=c, radius=r, angle=angle, text=text, location=location,
                                    dimstyle="ISO", override=ov or None, dxfattribs={"layer": "DIM"})
        d.render()

    def leader(self, target, at, s):
        """Leader with a filled arrow at `target`; text sits on a landing at `at`, away from the target."""
        ax, ay = target
        dx, dy = at[0] - ax, at[1] - ay
        L = math.hypot(dx, dy)
        ux, uy = dx / L, dy / L
        a = 2.5 * MM
        self.msp.add_solid([(ax, ay), (ax + a * ux - a / 6 * uy, ay + a * uy + a / 6 * ux),
                            (ax + a * ux + a / 6 * uy, ay + a * uy - a / 6 * ux)], dxfattribs={"layer": "DIM"})
        self.line(target, at, "DIM")
        side = 1 if at[0] >= ax else -1
        land = len(s) * 2.5 * 0.55 * MM
        self.line(at, (at[0] + side * land, at[1]), "DIM")
        self.text((at[0], at[1] + 0.6 * MM), s, align="BOTTOM_LEFT" if side > 0 else "BOTTOM_RIGHT")

    def balloon(self, target, at, n):
        r = 3.5 * MM
        dx, dy = target[0] - at[0], target[1] - at[1]
        L = math.hypot(dx, dy)
        self.line((at[0] + dx / L * r, at[1] + dy / L * r), target, "DIM")
        self.msp.add_circle(target, 0.5 * MM, dxfattribs={"layer": "DIM"})
        self.msp.add_circle(at, r, dxfattribs={"layer": "DIM"})
        self.text(at, str(n), h=3.5)

    def parts_list(self, rows, x, y, widths=(12, 62, 12, 94), rh=6):
        """Table grows upward from (x, y) (sheet-relative), header row on top."""
        rows = [("ITEM", "PART", "QTY", "MATERIAL / NOTE"), *rows]
        W = sum(widths) * MM
        x0, y0 = self.at(x, y)
        for i, row in enumerate(reversed(rows)):
            yb = y0 + i * rh * MM
            self.line((x0, yb), (x0 + W, yb), "TITLE")
            cx = x0
            for w, cell in zip(widths, row):
                self.text((cx + 1.5 * MM, yb + rh * MM / 2), str(cell), h=2.5 if i < len(rows) - 1 else 1.8,
                          align="MIDDLE_LEFT")
                cx += w * MM
        top = y0 + len(rows) * rh * MM
        self.line((x0, top), (x0 + W, top), "TITLE")
        cx = x0
        for w in (0, *widths):
            cx += w * MM
            self.line((cx, y0), (cx, top), "TITLE")

    def centerlines(self, c, rx, ry=None):
        ry = rx if ry is None else ry
        self.line((c[0] - rx, c[1]), (c[0] + rx, c[1]), "CENTER")
        if ry:
            self.line((c[0], c[1] - ry), (c[0], c[1] + ry), "CENTER")

    def cut_line(self, a, b, letter, look):
        """Cutting plane a-b (sheet points), thick ends only (ISO 128); `look` = viewing direction."""
        ux, uy = b[0] - a[0], b[1] - a[1]
        L = math.hypot(ux, uy)
        ux, uy = ux / L, uy / L
        for (px, py), sgn in ((a, 1), (b, -1)):
            self.msp.add_lwpolyline([(px, py), (px + sgn * ux * 5 * MM, py + sgn * uy * 5 * MM)],
                                    dxfattribs={"layer": "VISIBLE", "const_width": 0.7 * MM})
            tip = (px + look[0] * 6 * MM, py + look[1] * 6 * MM)
            self.line((px, py), tip, "DIM")
            a_ = 2.5 * MM
            self.msp.add_solid([tip, (tip[0] - look[0] * a_ - look[1] * a_ / 4, tip[1] - look[1] * a_ + look[0] * a_ / 4),
                                (tip[0] - look[0] * a_ + look[1] * a_ / 4, tip[1] - look[1] * a_ - look[0] * a_ / 4)],
                               dxfattribs={"layer": "DIM"})
            self.text((tip[0] + look[0] * 2.5 * MM - sgn * ux * 2.5 * MM, tip[1] + look[1] * 2.5 * MM - sgn * uy * 2.5 * MM),
                      letter, h=5)

    def label(self, p, title, scale=None):
        self.text(p, title, h=5)
        if scale:
            self.text((p[0], p[1] - 6 * MM), f"({scale})", h=3.5)

    def arrow(self, tip, d, letter, length=8):
        """View arrow pointing along d (unit sheet vector) to `tip`, letter at its tail."""
        k = length * MM
        tail = (tip[0] - d[0] * k, tip[1] - d[1] * k)
        self.line(tail, tip, "DIM")
        a = 3 * MM
        self.msp.add_solid([tip, (tip[0] - d[0] * a - d[1] * a / 3, tip[1] - d[1] * a + d[0] * a / 3),
                            (tip[0] - d[0] * a + d[1] * a / 3, tip[1] - d[1] * a - d[0] * a / 3)],
                           dxfattribs={"layer": "DIM"})
        self.text((tail[0] - d[0] * 3 * MM, tail[1] - d[1] * 3 * MM), letter, h=5)

    def callout(self, c, r, letter):
        """Detail callout circle on a parent view."""
        self.msp.add_circle(c, r, dxfattribs={"layer": "DETAIL_BOUNDARY"})
        self.text((c[0] + r * 0.8 + 2 * MM, c[1] + r * 0.8 + 2 * MM), letter, h=5)

    # ---- frame + title block (ISO 7200 style)
    def frame(self, title, part, dwg_no, sheet, sheets, material="PETG", notes="", notes_at=(107, 60), notes_w=48):
        m = 10 * MM
        X0, Y0, X1, Y1 = self.x0, self.y0, self.x0 + SW, self.y0 + SH
        self.msp.add_lwpolyline([(X0, Y0), (X1, Y0), (X1, Y1), (X0, Y1)], close=True,
                                dxfattribs={"layer": "TITLE"})
        self.msp.add_lwpolyline([(X0 + m, Y0 + m), (X1 - m, Y0 + m), (X1 - m, Y1 - m), (X0 + m, Y1 - m)],
                                close=True, dxfattribs={"layer": "FRAME"})
        # title block 180 x 50 (paper) at bottom right inside the border
        w, h = 180 * MM, 50 * MM
        bx, by = X1 - m - w, Y0 + m
        self.msp.add_lwpolyline([(bx, by), (bx + w, by), (bx + w, by + h), (bx, by + h)], close=True,
                                dxfattribs={"layer": "FRAME"})
        for r in (10, 20):
            self.line((bx, by + r * MM), (bx + w, by + r * MM), "TITLE")
        self.line((bx, by + 35 * MM), (bx + 135 * MM, by + 35 * MM), "TITLE")
        for c in (45, 90, 135):
            self.line((bx + c * MM, by), (bx + c * MM, by + 20 * MM), "TITLE")
        self.line((bx + 135 * MM, by + 20 * MM), (bx + 135 * MM, by + 50 * MM), "TITLE")

        def cell(cx, cy, k, v, vh=3.5):
            self.text((bx + (cx + 1.5) * MM, by + (cy + 7.5) * MM), k, h=1.8, align="LEFT")
            self.text((bx + (cx + 1.5) * MM, by + (cy + 2.0) * MM), v, h=vh, align="LEFT")

        cell(0, 0, "MATERIAL", material)
        cell(45, 0, "SCALE", f"{SCALE:g}:1")
        cell(90, 0, "UNITS / GEN. TOL.", "mm / ±0.2", vh=3)
        cell(135, 0, "SHEET", f"{sheet} / {sheets}")
        cell(0, 10, "DRAWN", DRAWN)
        cell(45, 10, "DATE", DATE)
        cell(90, 10, "REVISION", REV)
        cell(135, 10, "DRAWING NO.", dwg_no)
        self.text((bx + 1.5 * MM, by + 32.5 * MM), "TITLE", h=1.8, align="LEFT")
        self.text((bx + 3 * MM, by + 25 * MM), part, h=5, align="LEFT")
        self.text((bx + 1.5 * MM, by + 46.5 * MM), "PROJECT", h=1.8, align="LEFT")
        self.text((bx + 3 * MM, by + 40 * MM), title, h=5, align="LEFT")
        self.text((bx + 136.5 * MM, by + 46.5 * MM), "PROJECTION", h=1.8, align="LEFT")
        _first_angle_symbol(self, (bx + 157.5 * MM, by + 35 * MM))
        self.text((bx + 157.5 * MM, by + 23 * MM), "FIRST ANGLE (ISO E)", h=1.8)
        if notes:
            cols = int(notes_w / (0.68 * 2.5 * MM))   # measured LiberationSans average advance
            lines = [ln for par in notes.split("\\P") for ln in (textwrap.wrap(par, cols) or [""])]
            self.mtext(self.at(*notes_at), "\\P".join(lines).rstrip("\\P"), h=2.5)


def _first_angle_symbol(sh, c):
    k = MM
    # side view of a truncated cone (left) + end view circles (right): first-angle arrangement
    x, y = c
    sh.msp.add_lwpolyline([(x - 16 * k, y - 4 * k), (x - 16 * k, y + 4 * k), (x - 4 * k, y + 7 * k),
                           (x - 4 * k, y - 7 * k)], close=True, dxfattribs={"layer": "TITLE"})
    sh.msp.add_circle((x + 8 * k, y), 7 * k, dxfattribs={"layer": "TITLE"})
    sh.msp.add_circle((x + 8 * k, y), 4 * k, dxfattribs={"layer": "TITLE"})
    sh.line((x - 18 * k, y), (x + 17 * k, y), "CENTER")


def _on_circle(e, view, cx, cy, r):
    for t in (0, 0.33, 0.66, 1):
        p = e @ t
        x, y = view.uv(p.X, p.Y)
        if abs(math.hypot(x - cx, y - cy) - r) > 1e-3:
            return False
    return True


# ---------------------------------------------------------------- sheets
PROJECT = "MysteryChain case"
NOTES_COMMON = (
    "NOTES\\P"
    "1. The 3D model pet_case.step (source pet_case.step.py) is the master; "
    "undimensioned features per model.\\P"
    "2. FDM, PETG, 0.4 nozzle, 0.12-0.16 layers, 3-4 walls, 30 % infill.\\P"
    "3. Angles are CCW from +X, seen from the front (display side).\\P"
)


def sheet_front_housing(doc, idx, n):
    sh = Sheet(doc, idx)
    part = pc.make_front_housing()
    LT, LH, TH, LI = pc.LUG_TOP, pc.LUG_HALF, pc.TAB_HALF, pc.LUG_IN_TOP
    z_usb0, z_usb1 = pc.Z_USB - pc.USB_POCKET_H / 2, pc.Z_USB + pc.USB_POCKET_H / 2
    z_bez, z_step = pc.z(pc.BEZEL_T), pc.z(pc.D_PCB_FRONT)
    g0, g1 = pc.GROOVE
    gy0, gy1 = pc.LUG_HOOK_Y - pc.HOOK_W / 2 - 0.3, pc.LUG_HOOK_Y + pc.HOOK_W / 2 + 0.3

    # front view
    F = sh.view(FRONT, 38, 84)
    sh.project(part, F)
    sh.centerlines(F.p(0, 0, H), RO + 1.5, 0)
    sh.line(F.p(0, -RO - 1.5, H), F.p(0, LT + 1.5, H), "CENTER")
    sh.dia(F.p(0, 0, H), RO, 225, location=F.p(-RO - 5, -RO - 2, H))
    sh.dia(F.p(0, pc.LCD_DY, H), pc.WIN_D / 2, 150, location=F.p(-7, 4, H))
    sh.dim(F.p(-RO, -RO, H), F.p(-LH, LT, H), F.p(-RO - 8, 0, H), vertical=True)
    sh.dim(F.p(0, 0, H), F.p(-LH, LT, H), F.p(-RO - 3.5, 0, H), vertical=True)
    sh.dim(F.p(-LH, LT, H), F.p(LH, LT, H), F.p(0, LT + 4, H))
    sh.rad(F.p(LH - 3, LT - 3, H), 3, 45, location=F.p(LH + 3, LT + 3, H))
    sh.leader(F.p(5, pc.KEY_SLOT_CY, H), F.p(LH + 1, LT - 8, H), "SLOT 10 x 3 THRU")
    sh.label(F.p(0, -RO - 9, H), "FRONT")
    sh.cut_line(F.p(0, LT + 1.5, H), F.p(0, -RO - 1.5, H), "A", (1, 0))
    sh.cut_line(F.p(-RO - 2.5, 0, H), F.p(RO + 2.5, 0, H), "B", (0, -1))
    sh.arrow(F.p(-15, -15.2, H), (0, 1), "D")

    # section A-A (x = 0, seen from -X): front face on the right
    A = sh.view(LEFT, 70, 84)
    sh.section(part, A)
    sh.line(A.p(0, 0, -1.5), A.p(0, 0, H + 1.5), "CENTER")
    sh.dim(A.p(0, -RO, 0), A.p(0, -RO, H), A.p(0, -RO - 8, 0))
    sh.dim(A.p(0, -RO, 0), A.p(0, -RO, z_usb0), A.p(0, -RO - 3.5, 0))
    sh.dim(A.p(0, -RO, z_usb0), A.p(0, -RO, z_usb1), A.p(0, -RO - 3.5, 0))
    sh.dim(A.p(0, -RO, 0), A.p(0, pc.USB_FACE_Y, z_usb0), A.p(0, 0, -3.5), vertical=True)
    sh.dim(A.p(0, LI - 2, z_step), A.p(0, LI - 2, H), A.p(0, LT + 3.5, 0))     # lug front skin
    sh.label(A.p(0, -RO - 13.5, H / 2), "SECTION A-A")

    # rear view (mirrored): lug cavity open to the back
    R = sh.view(REAR, 128, 84)
    sh.project(part, R)
    sh.centerlines(R.p(0, 0, 0), RO + 1.5, 0)
    sh.line(R.p(0, -RO - 1.5, 0), R.p(0, LT + 1.5, 0), "CENTER")
    sh.dim(R.p(TH, LI, 0), R.p(-TH, LI, 0), R.p(0, LT + 3.5, 0))
    sh.dim(R.p(0, 0, 0), R.p(-TH, LI, 0), R.p(-RO - 3.5, 0, 0), vertical=True)
    sh.dim(R.p(0, 0, 0), R.p(-TH - 0.7, gy0, 0), R.p(-RO - 7.5, 0, 0), vertical=True)
    sh.dim(R.p(-TH - 0.7, gy0, 0), R.p(-TH - 0.7, gy1, 0), R.p(-RO - 7.5, 0, 0), vertical=True)
    sh.dia(R.p(0, 0, 0), RI, 300, location=R.p(RO + 3, -RO - 1, 0))
    sh.label(R.p(0, -RO - 5, 0), "REAR")

    # view D (from below): USB-C pocket
    D = sh.view(BOTTOM, 180, 75)
    sh.project(part, D)
    sh.line(D.p(0, 0, -1.5), D.p(0, 0, H + 1.5), "CENTER")
    sh.dim(D.p(-pc.USB_POCKET_W / 2, -RO, z_usb0), D.p(pc.USB_POCKET_W / 2, -RO, z_usb0), D.p(0, 0, -3))
    sh.label(D.p(0, 0, -9), "VIEW D")

    # section B-B (y = 0, seen from +Y): front face at the bottom
    B = sh.view(TOP, 38, 46)
    sh.section(part, B)
    sh.line(B.p(0, 0, -1.5), B.p(0, 0, H + 1.5), "CENTER")
    sh.dim(B.p(-RO, 0, H), B.p(RO, 0, H), B.p(0, 0, H + 3.5), text="%%c<>")
    sh.dim(B.p(-RI, 0, 0), B.p(RI, 0, 0), B.p(0, 0, -3.5), text="%%c<>")
    r36 = pc.LCD_D / 2 + 0.2
    sh.dim(B.p(-r36, 0, z_step), B.p(r36, 0, z_step), B.p(0, 0, z_step - 2.5), text="%%c36")
    sh.dim(B.p(-RO, 0, H), B.p(-r36, 0, z_bez), B.p(-RO - 3.5, 0, 0), vertical=True)
    sh.dim(B.p(-RO, 0, H), B.p(-RI, 0, z_step), B.p(-RO - 7.5, 0, 0), vertical=True)
    dc, dr = (RI + 0.6, 0, 5.4), 5.6
    sh.callout(B.p(*dc), dr, "C")
    sh.label(B.p(0, 0, H + 9), "SECTION B-B")

    # detail C (5:1): hook groove + USB plug anchor slot, +X wall
    s = 5 / SCALE
    C = sh.view(TOP, 86, 30, s=s, pivot=dc)
    sh.section(part, C, crop=(dc, dr))
    lf = {"dimlfac": 1 / s}
    sw0, sw1, zm = pc.Z_MID - 0.7, pc.Z_MID + 0.7, pc.Z_MID
    x_in = RI - 1.4
    sh.dim(C.p(RI, 0, 0), C.p(RI, 0, g0), C.p(x_in, 0, 0), vertical=True, **lf)
    sh.dim(C.p(pc.GROOVE_R, 0, g0), C.p(pc.GROOVE_R, 0, g1), C.p(x_in, 0, 0), vertical=True, **lf)
    sh.dim(C.p(RI, 0, sw0), C.p(RI, 0, sw1), C.p(x_in - 1.1, 0, 0), vertical=True, **lf)
    sh.dim(C.p(RI, 0, g1), C.p(pc.GROOVE_R, 0, g1), C.p(0, 0, g1 + 0.8),
           location=C.p(RI - 0.6, 0, g1 + 1.15), **lf)
    sh.dim(C.p(RO, 0, zm - 1.8), C.p(RO, 0, zm + 1.8), C.p(RO + 1.3, 0, 0), vertical=True, **lf)
    sh.dim(C.p(RO, 0, 0), C.p(RO, 0, zm), C.p(RO + 2.6, 0, 0), vertical=True, **lf)
    sh.dim(C.p(RO - 0.9, 0, zm - 1.8), C.p(RO, 0, zm - 1.8), C.p(0, 0, zm - 2.5),
           location=C.p(RO + 0.6, 0, zm - 2.15), **lf)
    sh.line(C.p(RI - 0.6, 0, zm), C.p(RO + 0.6, 0, zm), "CENTER")
    cx, cy = C.p(*dc)
    sh.label((cx, cy - dr * s - 8 * MM), "DETAIL C", "5:1")

    # isometric
    I = sh.view(ISO, 183, 112, s=0.5)
    sh.project(part, I)
    sh.text(sh.at(183, 94), "ISOMETRIC (1:1)", h=3.5)

    sh.frame(PROJECT, "FRONT HOUSING", "MC-M-002", idx + 1, n, notes=NOTES_COMMON + (
        "4. Window %%c34.6 (0.4 lip over the glass edge), centred 0.335 below the body axis (toward USB-C).\\P"
        f"5. Hook groove R{pc.GROOVE_R:.1f} x z {g0:.1f}-{g1:.1f} (detail C); lug side grooves 0.7 deep, "
        f"y {gy0:.1f}-{gy1:.1f}. Pry notch 5 x 0.8 at 230 deg.\\P"
        "6. USB plug anchor slot 3.4 x 1.4 + recess (+X); button hole 3.6 x 2.0, %%c6.4 dimple (-X).\\P"
        f"7. LCD neck / FPC pocket: neck outline + {pc.NECK_GROW:.1f} all round. Motor ring %%c10.5 x 1.0 "
        "on the lug skin."), notes_at=(107, 54), notes_w=95)
    return sh


def sheet_midframe(doc, idx, n):
    sh = Sheet(doc, idx)
    part = pc.make_midframe()
    RD = RI - 0.2
    th, LIT = pc.TAB_HALF - 0.2, pc.LUG_IN_TOP - 0.2
    zf, zb = pc.Z_DF, pc.Z_DB
    z_top = pc.z(pc.D_PCB_BACK)
    pad_y0 = pc.XIAO_ORIGIN[1] - 4.05
    rib_y0 = pc.XIAO_PCB_TOP_Y + 0.15
    rib_top = pc.z(pc.D_XIAO_BOT + 0.2)
    bx = pc.BAT_W / 2 + pc.BAT_GAP
    by0, by1 = pc.BAT_Y0 + 1.5, pc.LUG_HOOK_Y - pc.HOOK_W / 2 - 1.5
    rx, ry = pc.XIAO_RST_X, pc.USB_FACE_Y + pc.XIAO_RST_DY
    uw = pc.USB_POCKET_W / 2 + 0.2

    # front view (display side)
    F = sh.view(FRONT, 38, 84)
    sh.project(part, F)
    sh.centerlines(F.p(0, 0, zf), RD + 1.5, 0)
    sh.line(F.p(0, -RD - 1.5, zf), F.p(0, LIT + 1.5, zf), "CENTER")
    sh.msp.add_circle(F.p(0, 0, zf), 17.6, dxfattribs={"layer": "CENTER"})
    sh.dia(F.p(0, 0, zf), RD, 240, location=F.p(-RD - 4, -RD - 4, zf))
    sh.leader(F.p(*pc.polar(17.6 + 1.2, 140), z_top), F.p(-RD + 2, RD + 12, z_top), "4x %%c2.4 ON %%c35.2")
    sh.dim(F.p(-th, LIT, zf), F.p(th, LIT, zf), F.p(0, LIT + 3.5, zf))
    sh.dim(F.p(0, 0, zf), F.p(th, LIT, zf), F.p(RD + 3.5, 0, zf), vertical=True)
    sh.dim(F.p(bx + 0.8, by0, zf), F.p(bx + 0.8, by1, zf), F.p(RD + 7.5, 0, zf), vertical=True)
    sh.dim(F.p(-bx, 26, zf), F.p(bx, 26, zf), F.p(0, 26, zf))
    sh.dim(F.p(-6, rib_y0 + 1.15, zf), F.p(6, rib_y0 + 1.15, zf), F.p(0, 2.2, zf))
    sh.dim(F.p(-4, pad_y0, zf), F.p(4, pad_y0, zf), F.p(0, -RD - 3, zf))
    sh.label(F.p(0, -RD - 10, zf), "FRONT")
    sh.cut_line(F.p(0, LIT + 1.5, zf), F.p(0, -RD - 1.5, zf), "A", (1, 0))
    sh.cut_line(F.p(-RD - 2.5, 0, zf), F.p(RD + 2.5, 0, zf), "B", (0, -1))

    # section A-A (x = 0)
    A = sh.view(LEFT, 70, 84)
    sh.section(part, A)
    sh.dim(A.p(0, -RD, zb), A.p(0, -RD, z_top), A.p(0, -RD - 7, 0))
    sh.dim(A.p(0, -RD, zb), A.p(0, -RD, zf), A.p(0, -RD - 3, 0), location=A.p(0, -RD - 3, zb - 3.5))
    sh.dim(A.p(0, rib_y0, zf), A.p(0, rib_y0, rib_top), A.p(0, LIT + 2.5, 0))
    sh.dim(A.p(0, 20, zf), A.p(0, 20, zf + 2.0), A.p(0, LIT + 6, 0), location=A.p(0, LIT + 6, zf + 4.5))
    sh.label(A.p(0, -RD - 12, (zb + z_top) / 2), "SECTION A-A")

    # rear view (mirrored): reset hole, battery switch slot, USB clearance
    R = sh.view(REAR, 128, 84)
    sh.project(part, R)
    sh.centerlines(R.p(0, 0, 0), RD + 1.5, 0)
    sh.line(R.p(0, -RD - 1.5, 0), R.p(0, LIT + 1.5, 0), "CENTER")
    sh.dia(R.p(rx, ry, zb), 1.0, 135, location=R.p(rx + 6, ry + 8, zb))
    sh.dim(R.p(0, ry, zb), R.p(rx, ry, zb), R.p(0, -RD - 3, 0))
    sh.dim(R.p(rx, 0, zb), R.p(rx, ry, zb), R.p(-RD - 3.5, 0, 0), vertical=True)
    sh.dim(R.p(uw, -RD, zb), R.p(-uw, -RD, zb), R.p(0, -RD - 6.5, 0))
    sh.leader(R.p(pc.SW_X, pc.SW_Y, zb), R.p(-RD - 1, 6, zb), "SLOT 1.4 x 3.4")
    sh.label(R.p(0, -RD - 11.5, 0), "REAR")

    # section B-B (y = 0): button backer block
    B = sh.view(TOP, 38, 44)
    sh.section(part, B)
    sh.line(B.p(0, 0, zb - 3), B.p(0, 0, z_top + 1), "CENTER")
    sh.dim(B.p(-RD, 0, zf), B.p(RD, 0, zf), B.p(0, 0, zb - 3.5), text="%%c<>")
    sh.dim(B.p(-14.6, 0, zf), B.p(-14.6, 0, pc.Z_MID + pc.BTN_BODY[2] / 2), B.p(-RD - 3, 0, 0), vertical=True)
    sh.label(B.p(0, 0, z_top + 6), "SECTION B-B")

    I = sh.view(ISO, 183, 112, s=0.6)
    sh.project(part, I)
    sh.text(sh.at(183, 92), "ISOMETRIC (1.2:1)", h=3.5)

    sh.frame(PROJECT, "MIDFRAME", "MC-M-003", idx + 1, n, notes=NOTES_COMMON + (
        "4. Display clamp legs 4x %%c2.4 at 40/140/205/335 deg press on the display PCB edge.\\P"
        "5. The lug plate sets the orientation (no key tabs). Hook notches: 4 on the rim at 20/160/230/310 deg, "
        "2 on the lug sides.\\P"
        "6. XIAO tape pad 8 x 8 for 0.5 mm double-sided tape; stop rib 12 wide at the antenna end takes the "
        "USB insertion force.\\P"
        "7. Battery side ribs 0.8 thick x 2 high. Battery switch EG1390 between two 0.8 end stops beside the "
        "XIAO, actuator through the 1.4 x 3.4 slot.\\P"
        "8. Reset hole %%c2 (pin, cover off). Print plate down, no supports."), notes_at=(154, 72), notes_w=50)
    return sh


def sheet_back_cover(doc, idx, n):
    sh = Sheet(doc, idx)
    part = pc.make_back_cover()
    RD = RI - 0.2
    th, LIT = pc.TAB_HALF - 0.2, pc.LUG_IN_TOP - 0.2
    T, c0, c1 = pc.COVER_T, pc.CATCH_Z0, pc.CATCH_Z1
    ly = pc.LUG_HOOK_Y
    a = pc.HOOK_ANGLES[0]                  # section B-B / detail E go through this hook
    rp = Rot(0, 0, -a) * part              # ... turned onto +X so both views stay axis-aligned

    # inside view (from the front)
    F = sh.view(FRONT, 38, 84)
    sh.project(part, F)
    sh.centerlines(F.p(0, 0, T), RD + 1.5, 0)
    sh.line(F.p(0, -RD - 1.5, T), F.p(0, LIT + 1.5, T), "CENTER")
    sh.dia(F.p(0, 0, T), RD, 190, location=F.p(-RD - 6, -4, T))
    sh.dim(F.p(-th, LIT, T), F.p(th, LIT, T), F.p(0, LIT + 3.5, T))
    sh.dim(F.p(0, 0, T), F.p(-th, LIT, T), F.p(-RD - 3.5, 0, T), vertical=True)
    sh.dim(F.p(0, 0, T), F.p(-pc.TAB_HALF - 0.4, ly, c0), F.p(-RD - 7.5, 0, T), vertical=True)
    sh.leader(F.p(*pc.polar(RI + 0.2, 310), c0), F.p(13, -RD - 5, T), "6x HOOK, DETAIL E")
    sh.leader(F.p(pc.SW_X, pc.SW_Y, T), F.p(-RD + 2, -RD - 5, T), "SLOT 1.4 x 3.4")
    sh.label(F.p(0, -RD - 10, T), "INSIDE (FROM FRONT)")
    sh.cut_line(F.p(0, LIT + 1.5, T), F.p(0, -RD - 1.5, T), "A", (1, 0))
    ra = math.radians(a)
    sh.cut_line(F.p(*pc.polar(-(RD + 2.5), a), T), F.p(*pc.polar(RD + 2.5, a), T), "B",
                (math.sin(ra), -math.cos(ra)))

    # section A-A (x = 0)
    A = sh.view(LEFT, 74, 84)
    sh.section(part, A)
    sh.dim(A.p(0, -10, 0), A.p(0, -10, T), A.p(0, -RD - 3, 0), location=A.p(0, -RD - 3, T + 3))
    sh.dim(A.p(0, -10, 0), A.p(0, -10, c1), A.p(0, -RD - 7, 0))
    sh.label(A.p(0, -RD - 12, 3), "SECTION A-A")

    # section B-B (through the hook at angle a)
    B = sh.view(TOP, 38, 44)
    sh.section(rp, B)
    sh.line(B.p(0, 0, -1.5), B.p(0, 0, c1 + 1.5), "CENTER")
    sh.dim(B.p(-RD, 0, 0), B.p(RD, 0, 0), B.p(0, 0, -3.5), text="%%c<>")
    sh.dim(B.p(0, 0, c0), B.p(RI + 0.4, 0, c0), B.p(0, 0, c1 + 3), text="R<>")
    dc, dr = (RI - 0.4, 0, 3.4), 4.0
    sh.callout(B.p(*dc), dr, "E")
    sh.label(B.p(0, 0, c1 + 10), f"SECTION B-B ({a} deg HOOK)")

    # detail E (10:1): snap hook profile
    s = 10 / SCALE
    E = sh.view(TOP, 124, 92, s=s, pivot=dc)
    sh.section(rp, E, crop=(dc, dr))
    lf = {"dimlfac": 1 / s}
    ro = RI - 0.35
    sh.dim(E.p(ro - 0.7, 0, c1), E.p(ro, 0, c1), E.p(0, 0, c1 + 0.7), **lf)
    sh.dim(E.p(ro, 0, c1), E.p(RI + 0.4, 0, c0), E.p(0, 0, c1 + 0.7), **lf)
    sh.dim(E.p(ro - 0.7, 0, 0), E.p(ro - 0.7, 0, c1), E.p(ro - 1.6, 0, 0), vertical=True, **lf)
    sh.dim(E.p(RI + 0.4, 0, 0), E.p(RI + 0.4, 0, c0), E.p(RI + 1.1, 0, 0), vertical=True, **lf)
    sh.dim(E.p(ro - 0.1, 0, c0 - 0.8), E.p(RI + 0.4, 0, c0), E.p(RI + 2.0, 0, 0), vertical=True, **lf)
    cx, cy = E.p(*dc)
    sh.label((cx, cy - dr * s - 8 * MM), "DETAIL E", "10:1")

    I = sh.view(ISO, 183, 112, s=0.5)
    sh.project(part, I)
    sh.text(sh.at(183, 94), "ISOMETRIC (1:1)", h=3.5)

    sh.frame(PROJECT, "BACK COVER", "MC-M-004", idx + 1, n, notes=NOTES_COMMON + (
        f"4. Snap hooks 6x 4 wide: 4 on the rim at {'/'.join(str(h) for h in pc.HOOK_ANGLES)} deg, 2 on the lug "
        f"edges at y = {ly:.1f}; profile per detail E.\\P"
        "5. Retention ramp 45 deg: the cover is removable (pry at the housing notch, 230 deg). The housing groove "
        "edge sits 0.08 into the ramp, so the hooks pull the cover in.\\P"
        "6. Battery switch slot 1.4 x 3.4 thru (tape over it); USB plug relief at 270 deg.\\P"
        "7. Print plate down, hooks up, no supports."), notes_at=(150, 75), notes_w=55)
    return sh


BOM = [
    (1, "Front housing", 1, "PETG, drawing MC-M-002"),
    (2, "Midframe", 1, "PETG, drawing MC-M-003"),
    (3, "Back cover", 1, "PETG, drawing MC-M-004"),
    (4, "GC9A01 1.28in round TFT", 1, "Teyleten Robot module, 7-pin header removed"),
    (5, "XIAO nRF52840 Sense", 1, "Seeed Studio; taped to the midframe pad"),
    (6, "Vibration motor 10 x 3", 1, "Coin type, adhesive, in the lug skin ring"),
    (7, "USB-C plug, tethered", 1, "TPU 95A, usb_plug.step"),
    (8, "Tact switch 3 x 6", 1, "4.3 high; power + menu button"),
    (9, "LiPo 402030", 1, "3.7 V ~200 mAh, 20 x 30 x 4, beside the XIAO"),
    (10, "Round screen protector", 1, "%%c38-40, bonded on the front face"),
    (11, "Slide switch EG1390", 1, "Battery switch, on the midframe (MC-M-003)"),
]


def sheet_assembly(doc, idx, n):
    sh = Sheet(doc, idx)
    housing, mid, cover = pc.make_front_housing(), pc.make_midframe(), pc.make_back_cover()
    zs_ = pc.z(pc.D_PCB_FRONT)
    motor = pc.zcyl(0, pc.MOTOR_Y, pc.MOTOR_D / 2, zs_ - pc.MOTOR_T, zs_)
    battery = pc.zbox(-pc.BAT_W / 2, pc.BAT_W / 2, pc.BAT_Y0, pc.BAT_Y0 + pc.BAT_L, pc.Z_DF, pc.Z_DF + pc.BAT_T)
    parts = [(housing, (0, 0.125)), (mid, (90, 0.125)), (cover, (0, 0.07)),
             (pc.make_display(), "solid"), (pc.make_xiao(), "solid"), (motor, "solid"),
             (up.make_usb_plug(), (45, 0.1)), (pc.make_button(), "solid"), (battery, "solid"),
             (pc.make_battery_switch(), "solid")]
    LT, LH = pc.LUG_TOP, pc.LUG_HALF

    F = sh.view(FRONT, 36, 94)
    sh.project(Compound([p for p, _ in parts]), F)
    sh.centerlines(F.p(0, 0, H), RO + 1.5, 0)
    sh.line(F.p(0, -RO - 1.5, H), F.p(0, LT + 1.5, H), "CENTER")
    sh.dim(F.p(-RO, -RO, H), F.p(-LH, LT, H), F.p(-RO - 4, 0, H), vertical=True)
    sh.dia(F.p(0, 0, H), RO, 225, location=F.p(-RO - 4, -RO - 1, H))
    sh.cut_line(F.p(0, LT + 1.5, H), F.p(0, -RO - 1.5, H), "A", (1, 0))
    sh.cut_line(F.p(-RO - 2.5, 0, H), F.p(RO + 2.5, 0, H), "B", (0, -1))
    sh.label(F.p(0, -RO - 6, H), "FRONT")

    A = sh.view(LEFT, 70, 94)
    sh.section(parts, A)
    sh.dim(A.p(0, -RO, 0), A.p(0, -RO, H), A.p(0, -RO - 4, 0))
    sh.balloon(A.p(0, pc.MOTOR_Y + 2, zs_ - 1.5), A.p(0, LT - 2, H + 6), 6)
    sh.balloon(A.p(0, pc.BAT_Y0 + 12, pc.Z_DF + 2), A.p(0, pc.BAT_Y0 + 12, -6), 9)
    sh.label(A.p(0, -RO - 9, H / 2), "SECTION A-A")

    s = 4 / SCALE
    B = sh.view(TOP, 54, 54, s=s)
    sh.section(parts, B)
    for i, zz in enumerate((pc.z(pc.D_PCB_FRONT), pc.Z_DF, pc.Z_DB)):     # Z_DB = cover front (flat cover)
        sh.dim(B.p(RO, 0, H), B.p(RI, 0, zz), B.p(RO + 1.5 + 1.75 * i, 0, 0),
               vertical=True, dimlfac=1 / s)
    for target, at, k in (
        ((-RO + 0.4, 0, 11), (-RO - 1.8, 0, 11), 1), ((-12, 0, pc.Z_DB + 0.5), (-RO - 1.8, 0, 4.5), 2),
        ((-8, 0, 0.6), (-RO - 1.8, 0, 1), 3), ((-12, 0, pc.z(pc.D_PCB_BACK) + 0.8), (-12, 0, H + 3), 4),
        ((2, 0, pc.XIAO_ORIGIN[2] - 1.5), (4, 0, H + 3), 5), ((RI - 0.6, 0, pc.Z_MID), (12, 0, H + 3), 7),
        ((-RI + 1.2, 0, pc.Z_MID), (-RO - 1.8, 0, 8), 8),
    ):
        sh.balloon(B.p(*target), B.p(*at), k)
    sh.label(B.p(0, 0, H + 8), "SECTION B-B", "4:1")

    I = sh.view(ISO, 140, 114, s=0.7)
    sh.project(Compound([housing, cover]), I)
    sh.text(sh.at(140, 88), "ISOMETRIC (1.4:1)", h=3.5)

    sh.parts_list(BOM, 115, 32.5 - 2.5)
    sh.frame(PROJECT, "ASSEMBLY", "MC-M-001", idx + 1, n, notes=(
        "ASSEMBLY (all from the back)\\P"
        "1. Display (4), tab first into the lug pocket, PCB on the housing step.\\P"
        "2. Motor (6) into the ring on the lug skin; button (8) into its wall hole.\\P"
        "3. Midframe (2) with XIAO (5), battery (9) and battery switch (11).\\P"
        "4. Back cover (3): 4 rim + 2 lug hooks snap into the housing grooves.\\P"
        "5. USB plug (7): barb through the +X slot, then into the USB-C pocket.\\P"
        "6. Screen protector (10) on the clean, dry front face.\\P"
        f"Overall %%c{2 * RO:.1f} x {RO + pc.LUG_TOP:.1f} x {H:.2f}. Section fill: grey = purchased parts."),
        notes_at=(172, 140), notes_w=33)
    return sh


def gen_dxf():
    doc = new_doc()
    sheets = [sheet_assembly, sheet_front_housing, sheet_midframe, sheet_back_cover]
    for i, fn in enumerate(sheets):
        fn(doc, i, len(sheets))
    return {"document": doc}


# ---------------------------------------------------------------- PDF
def export_pdf(doc, path, png_dir=None):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from ezdxf.addons.drawing import Frontend, RenderContext
    from ezdxf.addons.drawing.config import BackgroundPolicy, ColorPolicy, Configuration, LineweightPolicy
    from ezdxf.addons.drawing.matplotlib import MatplotlibBackend
    from matplotlib.backends.backend_pdf import PdfPages

    msp = doc.modelspace()
    n = round((max(e.dxf.end.x for e in msp.query("LINE")) + SHEET_GAP) / (SW + SHEET_GAP))
    fig = plt.figure(figsize=(420 / 25.4, 297 / 25.4))
    ax = fig.add_axes((0, 0, 1, 1))
    cfg = Configuration(lineweight_policy=LineweightPolicy.ABSOLUTE, color_policy=ColorPolicy.COLOR,
                        background_policy=BackgroundPolicy.WHITE)
    Frontend(RenderContext(doc), MatplotlibBackend(ax), config=cfg).draw_layout(msp, finalize=False)
    ax.set_axis_off()
    ax.set_aspect("equal")
    with PdfPages(path) as pdf:
        for i in range(n):
            x0 = i * (SW + SHEET_GAP)
            ax.set_xlim(x0, x0 + SW)
            ax.set_ylim(0, SH)
            pdf.savefig(fig)
            if png_dir:
                fig.savefig(Path(png_dir) / f"sheet{i + 1}.png", dpi=150)
    plt.close(fig)


if __name__ == "__main__":
    import sys
    export_pdf(gen_dxf()["document"], HERE / "pet_case_drawing.pdf", sys.argv[1] if len(sys.argv) > 1 else None)
