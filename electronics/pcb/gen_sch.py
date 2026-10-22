#!/usr/bin/env python3
"""Generate mysterychain.kicad_sch (+ .kicad_pro, sym/fp-lib-table) from parts.py.

Every symbol sits at Part.sch with a net label on each connected pin end and a no-connect flag on
the rest, so the drawing is a labelled parts sheet, not a wired diagram. uuids are uuid5, so reruns
are byte-identical.
"""
import json
import os
import sys
import uuid

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from parts import FAB, PARTS  # noqa: E402
from sexp import Q, dumps, find, loads  # noqa: E402

ROOT = str(uuid.uuid5(uuid.NAMESPACE_URL, "mysterychain/sch/root"))
FONT = ["effects", ["font", ["size", "1.27", "1.27"]]]


def uid(*key):
    return Q(str(uuid.uuid5(uuid.NAMESPACE_URL, "mysterychain/sch/" + "/".join(key))))


_libs = {}


def lib_symbol(lib_id):
    """The symbol flattened (extends resolved) and renamed 'Lib:Name', as lib_symbols wants it."""
    lib, name = lib_id.split(":")
    path = os.path.join(HERE, "lib", f"{lib}.kicad_sym") if lib == "mysterychain" else f"/usr/share/kicad/symbols/{lib}.kicad_sym"
    if path not in _libs:
        _libs[path] = {s[1]: s for s in find(loads(open(path).read()), "symbol")}
    syms = _libs[path]
    sym = syms[name]
    ext = find(sym, "extends")
    base = syms[ext[0][1]] if ext else sym
    props = find(sym, "property")
    out = [c for c in base if not (isinstance(c, list) and c[0] in ("property", "symbol", "extends"))]
    out[1] = Q(lib_id)
    out += props
    for unit in find(base, "symbol"):
        out.append(["symbol", Q(name + unit[1][len(base[1]):]), *unit[2:]])
    return out


def pins_of(sym):
    """[(number, x, y, angle)] in lib coordinates (y up), all units."""
    out = []
    for unit in find(sym, "symbol"):
        for p in find(unit, "pin"):
            at = find(p, "at")[0]
            out.append((find(p, "number")[0][1], float(at[1]), float(at[2]), int(float(at[3]))))
    return out


def label(net, x, y, pin_angle, key):
    """Global label on a pin end (global everywhere: board net names get no "/" sheet prefix)."""
    # the label text runs away from the body: opposite the pin's direction into the symbol
    ang = {0: 180, 180: 0, 90: 270, 270: 90}[pin_angle]
    just = ["justify", "left" if ang in (0, 90) else "right"]
    return ["global_label", Q(net), ["shape", "passive"], ["at", f"{x:.2f}", f"{y:.2f}", str(ang)],
            ["fields_autoplaced", "yes"], ["effects", ["font", ["size", "1.27", "1.27"]], just],
            ["uuid", uid(key, "glabel")],
            ["property", Q("Intersheetrefs"), Q("${INTERSHEET_REFS}"), ["at", f"{x:.2f}", f"{y:.2f}", "0"],
             ["effects", ["font", ["size", "1.27", "1.27"]], ["hide", "yes"]]]]


def instance(p, sym):
    X, Y, rot = p.sch
    assert rot == 0, f"{p.ref}: only unrotated schematic symbols are supported"
    fields = {"Reference": p.ref, "Value": p.value, "Footprint": p.footprint}
    props = []
    for prop in find(sym, "property"):
        k = prop[1]
        at = find(prop, "at")[0]
        v = fields.get(k, prop[2])
        props.append(["property", Q(k), Q(v),
                      ["at", f"{X + float(at[1]):.2f}", f"{Y - float(at[2]):.2f}", at[3]],
                      *[c for c in prop[3:] if isinstance(c, list) and c[0] != "at"]])
    if p.mpn:
        props.append(["property", Q("MPN"), Q(p.mpn), ["at", f"{X:.2f}", f"{Y:.2f}", "0"],
                      ["effects", ["font", ["size", "1.27", "1.27"]], ["hide", "yes"]]])
    flags = ["exclude_from_sim", "no"], ["in_bom", "yes" if p.bom else "no"], ["on_board", "yes" if p.footprint else "no"]
    return ["symbol", ["lib_id", Q(p.symbol)], ["at", f"{X:.2f}", f"{Y:.2f}", "0"], ["unit", "1"], *flags,
            ["dnp", "no"], ["uuid", uid(p.ref)], *props,
            *[["pin", Q(n), ["uuid", uid(p.ref, "pin", n)]] for n in sorted({n for n, *_ in pins_of(sym)})],
            ["instances", ["project", Q("mysterychain"), ["path", Q("/" + ROOT), ["reference", Q(p.ref)], ["unit", "1"]]]]]


def schematic():
    used = {}
    for p in PARTS:
        used.setdefault(p.symbol, lib_symbol(p.symbol))
    items, seen = [], set()
    for p in PARTS:
        sym = used[p.symbol]
        X, Y, _ = p.sch
        items.append(instance(p, sym))
        for n, px, py, ang in pins_of(sym):
            x, y = round(X + px, 2), round(Y - py, 2)
            net = p.pins.get(n)
            key = (x, y, net)
            if key in seen:
                continue  # stacked pins (USB-C GND/VBUS, IMU GND)
            seen.add(key)
            if net:
                items.append(label(net, x, y, ang, f"{p.ref}/{n}"))
            else:
                items.append(["no_connect", ["at", f"{x:.2f}", f"{y:.2f}"], ["uuid", uid(p.ref, n, "nc")]])
    sch = ["kicad_sch", ["version", "20250114"], ["generator", Q("eeschema")], ["generator_version", Q("9.0")],
           ["uuid", Q(ROOT)], ["paper", Q("A3")],
           ["title_block", ["title", Q("MysteryChain PCB rev A")], ["date", Q("2026-10-20")], ["rev", Q("A")],
            ["comment", "1", Q("Generated by gen_sch.py from parts.py - do not edit by hand")]],
           ["lib_symbols", *used.values()], *items,
           ["sheet_instances", ["path", Q("/"), ["page", Q("1")]]], ["embedded_fonts", "no"]]
    return dumps(sch) + "\n"


def project():
    rules = {
        "min_clearance": 0.15, "min_track_width": 0.2, "min_via_diameter": 0.6, "min_through_hole_diameter": 0.3,
        "min_via_annular_width": 0.15, "min_copper_edge_clearance": 0.3, "min_hole_to_hole": 0.5,
        "min_hole_clearance": 0.25, "min_silk_clearance": 0.0, "min_text_thickness": 0.15, "min_text_height": 0.8,
    }
    assert rules["min_track_width"] >= FAB["min_track"] and rules["min_clearance"] >= FAB["min_space"]
    assert rules["min_through_hole_diameter"] >= FAB["via_drill"] and rules["min_via_diameter"] >= FAB["via_d"]
    # vias 0.3/0.6 (fab minimum): a 0.8 via does not fit between the E73 pad tips and the board edge / J1
    # 0.20 track / 0.15 space = the fab's published limit: at 0.25/0.20 the 4-layer board leaves ~40 open
    netclass = {"name": "Default", "clearance": 0.15, "track_width": 0.2, "via_diameter": 0.6, "via_drill": 0.3,
                "microvia_diameter": 0.3, "microvia_drill": 0.1, "diff_pair_gap": 0.25, "diff_pair_width": 0.25,
                "diff_pair_via_gap": 0.25, "line_style": 0, "pcb_color": "rgba(0, 0, 0, 0.000)",
                "schematic_color": "rgba(0, 0, 0, 0.000)", "wire_width": 6, "bus_width": 12, "priority": 2147483647}
    pro = {"board": {"design_settings": {"rules": rules}},
           "meta": {"filename": "mysterychain.kicad_pro", "version": 3},
           "net_settings": {"classes": [netclass], "meta": {"version": 4}},
           "sheets": [[ROOT, "Root"]]}
    return json.dumps(pro, indent=2) + "\n"


TABLE = """({kind}_lib_table
\t(version 7)
\t(lib (name "mysterychain")(type "KiCad")(uri "${{KIPRJMOD}}/lib/{file}")(options "")(descr "MysteryChain custom"))
{stock})
"""


def stock_libs(kind):
    """Stock KiCad libs this project uses, so it opens without a global lib table."""
    libs = sorted({getattr(p, kind).split(":")[0] for p in PARTS if getattr(p, kind)} - {"mysterychain"})
    if kind == "symbol":
        return "".join(f'\t(lib (name "{n}")(type "KiCad")(uri "${{KICAD9_SYMBOL_DIR}}/{n}.kicad_sym")(options "")(descr ""))\n'
                       for n in libs)
    return "".join(f'\t(lib (name "{n}")(type "KiCad")(uri "${{KICAD9_FOOTPRINT_DIR}}/{n}.pretty")(options "")(descr ""))\n'
                   for n in libs)


def main():
    files = {
        "mysterychain.kicad_sch": schematic(),
        "mysterychain.kicad_pro": project(),
        "sym-lib-table": TABLE.format(kind="sym", file="mysterychain.kicad_sym", stock=stock_libs("symbol")),
        "fp-lib-table": TABLE.format(kind="fp", file="mysterychain.pretty", stock=stock_libs("footprint")),
        # one thermal spoke is enough: every GND pad also has its own via to the In2.Cu plane
        "mysterychain.kicad_dru": '(version 1)\n(rule "one_spoke"\n\t(constraint min_resolved_spokes 1))\n',
    }
    for name, text in files.items():
        open(os.path.join(HERE, name), "w").write(text)
    print("gen_sch:", ", ".join(files))


if __name__ == "__main__":
    main()
