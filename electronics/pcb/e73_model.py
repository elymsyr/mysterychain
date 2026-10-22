#!/usr/bin/env python3
"""E73-2G4M08S1C 3D model -> lib/3d/E73-2G4M08S1C.step. Run with ~/.venvs/cad/bin/python -I.

Origin = the footprint origin (pad 26 row, pad 11 column): model x = footprint x, model y = -footprint y.
ponytail: envelope only (PCB, shield can, antenna chip), sized from manual §3; enough for case fit.
"""
import os
import re

from build123d import Box, Compound, Location, export_step

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "lib", "3d", "E73-2G4M08S1C.step")

PCB_T = 0.8
H = 3.0  # manual: 3.00 +/- 0.1 overall


def box(x0, y0, z0, x1, y1, z1):
    return Box(x1 - x0, y1 - y0, z1 - z0).locate(Location(((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2)))


def main():
    parts = [
        box(0, 0, 0, 18, 13, PCB_T),               # module PCB
        box(0.6, 0.6, PCB_T, 14.2, 12.4, H),       # shield can over the pad area
        box(15.2, 3.0, PCB_T, 17.4, 10.0, PCB_T + 1.0),  # ceramic antenna at +x
    ]
    export_step(Compound(parts), OUT)
    # fixed timestamp so reruns are byte-identical
    text = re.sub(r"FILE_NAME\('([^']*)','[^']*'", r"FILE_NAME('\1','2026-10-20T00:00:00'", open(OUT).read())
    open(OUT, "w").write(text)
    print("e73_model:", OUT)


if __name__ == "__main__":
    main()
