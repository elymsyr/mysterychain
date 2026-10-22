# Short showcase clip: the case spinning slowly on a tilted axis, like a globe, the cat playing on the screen.
# Transparent background.
#   1. Dump the cat's clips: interfaces/watch/test/preview.cpp (see its header), e.g. into /tmp/cat
#   2. python3 showcase.py /tmp/cat <out dir>   -> showcase.webp (soft edges) + showcase.gif (1-bit alpha)
# Needs numpy, scipy, trimesh, pyrender, pillow, and a GPU (EGL) or OSMesa for pyrender.
import os
import sys

os.environ.setdefault("PYOPENGL_PLATFORM", "egl")
import numpy as np
import pyrender
import trimesh
from scipy.spatial import ConvexHull
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
# (clip, seconds): one turn per loop, the screen faces the camera in the middle (message, morning)
CLIPS = [("petting", 3), ("message", 5), ("morning", 4), ("wx_rain", 4)]
FPS = 15
W = H = 440              # output size; rendered at 2x and scaled down
TILT = 20                # spin axis lean, degrees
CASE = [0.22, 0.50, 0.80, 1]  # light blue (linear RGB)
SCREEN_Z = 13.74 - 0.55  # front face - lip and glass gap
SCREEN_DY = -0.335       # glass centre offset toward the USB-C
ACTIVE_D, GLASS_D = 32.05, 35.2
PIVOT = [0, 12, 13.74 / 2]  # spin axis: the case's long axis, mid-thickness


def load_clip(path):  # preview.cpp dump: per frame a 768-byte palette + 240x240 indices
    n = 240
    rec = np.fromfile(path, np.uint8).reshape(-1, 768 + n * n)
    return [r[:768].reshape(256, 3)[r[768:].reshape(n, n)] for r in rec]


def part(name, flip):
    """Print STL back into case coordinates: display centre at the origin, front +Z, lug +Y."""
    m = trimesh.load(f"{HERE}/print/{name}.stl")
    if flip:  # printed front face down (print_parts.py: Rot(180, 0, 0))
        m.apply_transform(trimesh.transformations.rotation_matrix(np.pi, [1, 0, 0]))
    # centre of the round body: circle fit to the lower arc of the back face's outline (the cover's hooks stick out)
    v = m.vertices
    face = v[v[:, 2] < v[:, 2].min() + 0.3]
    hull = face[ConvexHull(face[:, :2]).vertices]
    arc = hull[hull[:, 1] < hull[:, 1].min() + 10]
    a, b, _ = np.linalg.lstsq(np.c_[arc[:, :2], np.ones(len(arc))], -(arc[:, :2] ** 2).sum(1), rcond=None)[0]
    m.apply_translation([a / 2, b / 2, -v[:, 2].min()])
    return m


def screen_mesh(frame):
    n, r = 96, GLASS_D / 2
    a = np.linspace(0, 2 * np.pi, n, endpoint=False)
    xy = np.vstack([[0, 0], np.c_[np.cos(a), np.sin(a)] * r])
    pos = np.c_[xy[:, 0], xy[:, 1] + SCREEN_DY, np.full(n + 1, SCREEN_Z)]
    uv = np.c_[xy[:, 0] / (2 * r) + 0.5, xy[:, 1] / (2 * r) + 0.5]
    faces = np.array([[0, 1 + i, 1 + (i + 1) % n] for i in range(n)])
    # texture: the round active area inside the black glass ring
    t = round(240 * GLASS_D / ACTIVE_D)
    tex = Image.new("RGB", (t, t))
    mask = Image.new("L", (240, 240))
    yy, xx = np.mgrid[:240, :240] + 0.5
    mask.putdata(((xx - 120) ** 2 + (yy - 120) ** 2 <= 120 ** 2).ravel() * 255)
    tex.paste(Image.fromarray(frame), ((t - 240) // 2,) * 2, mask)
    # emissive, so the screen doesn't dim as it turns away from the lights (floats: pyrender drops int factors)
    mat = pyrender.MetallicRoughnessMaterial(baseColorFactor=[0.0, 0.0, 0.0, 1.0], emissiveFactor=[1.0, 1.0, 1.0],
                                             emissiveTexture=pyrender.Texture(source=np.array(tex), source_channels="RGB"),
                                             roughnessFactor=0.2)
    prim = pyrender.Primitive(positions=pos, normals=np.tile([0, 0, 1.0], (n + 1, 1)), texcoord_0=uv,
                              indices=faces, material=mat)
    return pyrender.Mesh([prim])


def rot(deg, axis):
    return trimesh.transformations.rotation_matrix(np.radians(deg), axis)


def main(clip_dir, out):
    os.makedirs(out, exist_ok=True)
    frames = []
    for name, secs in CLIPS:
        clip = load_clip(f"{clip_dir}/{name}.raw")
        frames += [clip[min(int(i * 25 / FPS), len(clip) - 1)] for i in range(secs * FPS)]
    total = len(frames)

    case = pyrender.MetallicRoughnessMaterial(baseColorFactor=CASE, roughnessFactor=0.55, metallicFactor=0)
    parts = [part("front_housing", True), part("midframe", False), part("back_cover", False)]
    parts[1].apply_translation([0, 0, 1.2])  # the midframe deck lies on the 1.2 cover plate

    scene = pyrender.Scene(bg_color=[0, 0, 0, 0], ambient_light=[0.18] * 3)
    body = pyrender.Node()
    scene.add_node(body)
    for m in parts:
        m.apply_translation(np.negative(PIVOT))
        scene.add(pyrender.Mesh.from_trimesh(m, material=case, smooth=False), parent_node=body)

    cam = np.eye(4)
    cam[2, 3] = 190
    scene.add(pyrender.PerspectiveCamera(yfov=np.radians(27), aspectRatio=W / H), pose=rot(-8, [1, 0, 0]) @ cam)
    for d, k in (([1, 1.2, 2], 3.5), ([-1.5, 0.5, 1], 1.5), ([0, -1, -1.5], 1.2)):
        z = np.array(d, float) / np.linalg.norm(d)
        x = np.cross([0, 1, 0], z) if abs(z[1]) < 0.99 else np.array([1.0, 0, 0])
        x /= np.linalg.norm(x)
        pose = np.eye(4)
        pose[:3, :3] = np.c_[x, np.cross(z, x), z]
        scene.add(pyrender.DirectionalLight(intensity=k), pose=pose)

    r = pyrender.OffscreenRenderer(2 * W, 2 * H)
    screen = None
    images = []
    for i, frame in enumerate(frames):
        turn = 360 * (i / total - 0.5)  # constant speed, front to the camera mid-loop
        scene.set_pose(body, rot(-TILT, [0, 0, 1]) @ rot(turn, [0, 1, 0]))
        if screen:
            scene.remove_node(screen)
        screen = scene.add(screen_mesh(frame), pose=trimesh.transformations.translation_matrix(np.negative(PIVOT)),
                           parent_node=body)
        color, _ = r.render(scene, flags=pyrender.RenderFlags.RGBA)
        images.append(Image.fromarray(color, "RGBA").resize((W, H), Image.LANCZOS))
    r.delete()

    ms = 1000 // FPS
    images[0].save(f"{out}/showcase.webp", save_all=True, append_images=images[1:], duration=ms, loop=0,
                   quality=75)
    gif = []
    for im in images:  # GIF alpha is on/off: index 255 = transparent
        p = im.convert("RGB").quantize(255)
        p.paste(255, mask=im.getchannel("A").point(lambda a: 255 if a < 128 else 0))
        gif.append(p)
    gif[0].save(f"{out}/showcase.gif", save_all=True, append_images=gif[1:], duration=ms, loop=0, disposal=2,
                transparency=255)
    for f in ("showcase.webp", "showcase.gif"):
        print(f, os.path.getsize(f"{out}/{f}") // 1024, "KB")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
