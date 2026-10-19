# Turns preview.cpp's clip dumps into GIFs (240x240 like the screen, round mask)
# and, with --sheet, one PNG per clip with every 5th frame for a quick look.
#   python3 preview.py <dump dir> <gif dir> [--sheet]
import os
import sys

from PIL import Image

N = 240
FRAME = 256 * 3 + N * N
src, dst = sys.argv[1], sys.argv[2]
os.makedirs(dst, exist_ok=True)
mask = Image.new("L", (N, N), 0)
for y in range(N):
    for x in range(N):
        if (x + 0.5 - N / 2) ** 2 + (y + 0.5 - N / 2) ** 2 <= (N / 2) ** 2:
            mask.putpixel((x, y), 255)

for name in sorted(f[:-4] for f in os.listdir(src) if f.endswith(".raw")):
    data = open(os.path.join(src, name + ".raw"), "rb").read()
    frames = []
    for i in range(len(data) // FRAME):
        rec = data[i * FRAME:(i + 1) * FRAME]
        im = Image.frombytes("P", (N, N), rec[768:])
        im.putpalette(rec[:768])
        rgb = Image.new("RGB", (N, N), (0, 0, 0))
        rgb.paste(im.convert("RGB"), mask=mask)
        frames.append(rgb)
    frames[0].save(os.path.join(dst, name + ".gif"), save_all=True, append_images=frames[1:], duration=40, loop=0,
                   optimize=False)
    if "--sheet" in sys.argv:
        picks = frames[::5]
        cols = 10
        sheet = Image.new("RGB", (cols * N, ((len(picks) + cols - 1) // cols) * N))
        for k, f in enumerate(picks):
            sheet.paste(f, ((k % cols) * N, (k // cols) * N))
        sheet.save(os.path.join(dst, name + "_sheet.png"))
    print(name, len(frames), "frames", os.path.getsize(os.path.join(dst, name + ".gif")) // 1024, "KB")
