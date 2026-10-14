"""Print files: one per part, in print orientation, sitting on the bed at Z=0 (STL + 3MF)."""
import importlib.util
from pathlib import Path
from build123d import Mesher, Pos, Rot, export_stl

HERE = Path(__file__).parent
spec = importlib.util.spec_from_file_location("pc", HERE / "pet_case.step.py")
pc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pc)

parts = {
    "front_housing": Rot(180, 0, 0) * pc.make_front_housing(),   # front face down (window chamfer prints clean)
    "midframe": pc.make_midframe(),                              # battery ribs down
    "back_cover": pc.make_back_cover(),                          # disc down, hooks up
}
out = HERE / "print"
for name, part in parts.items():
    bb = part.bounding_box()
    part = Pos(-bb.center().X, -bb.center().Y, -bb.min.Z) * part
    export_stl(part, str(out / f"{name}.stl"), tolerance=0.01, angular_tolerance=0.1)
    m = Mesher()
    m.add_shape(part, linear_deflection=0.01, angular_deflection=0.1)
    m.write(str(out / f"{name}.3mf"))
    bb = part.bounding_box()
    print(f"{name}: {bb.size.X:.1f} x {bb.size.Y:.1f} x {bb.size.Z:.1f} mm  vol={part.volume:.0f} mm3")
