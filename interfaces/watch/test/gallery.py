# Every character's every animation on one page, to watch on the PC before flashing: builds preview.cpp /
# lotr_preview.cpp per character, dumps the clips, makes the GIFs (preview.py) and opens index.html.
#   python3 gallery.py [character ...]   (default: all; only the named ones are rebuilt, the page lists every one)
#   OUT=/some/dir python3 gallery.py     (default /tmp/mysterychain_gallery)
import html
import os
import shutil
import subprocess
import sys
import webbrowser
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.environ.get("OUT", "/tmp/mysterychain_gallery")
GFX = os.path.expanduser("~/Arduino/libraries/Adafruit_GFX_Library")
PETS = {  # name: (source, defines)
    "cat": ("preview.cpp", []),
    "pavi": ("preview.cpp", ["-DPAVI"]),
    "nausicaa": ("preview.cpp", ["-DNAUSICAA"]),
    "gandalf": ("lotr_preview.cpp", []),
    "sauron": ("lotr_preview.cpp", ["-DSAURON"]),
}


def make(name):
    src, defs = PETS[name]
    exe, raw, gifs = f"{OUT}/bin/{name}", f"{OUT}/raw/{name}", f"{OUT}/{name}"
    for d in (os.path.dirname(exe), raw):
        os.makedirs(d, exist_ok=True)
    shutil.rmtree(gifs, ignore_errors=True)
    subprocess.run(["g++", "-std=c++17", "-O2", *defs, "-I", f"{HERE}/../firmware", "-I", GFX, f"{HERE}/{src}", "-o", exe],
                   check=True)
    subprocess.run([exe, raw], check=True, stdout=subprocess.DEVNULL)
    subprocess.run([sys.executable, f"{HERE}/preview.py", raw, gifs], check=True, stdout=subprocess.DEVNULL)
    shutil.rmtree(raw)  # ~58 KB a frame, hundreds of MB in all
    return name


def page():
    pets = [p for p in PETS if os.path.isdir(f"{OUT}/{p}")]
    nav = " ".join(f'<a href="#{p}">{p}</a>' for p in pets)
    body = ""
    for p in pets:
        clips = sorted(f[:-4] for f in os.listdir(f"{OUT}/{p}") if f.endswith(".gif"))
        cards = "".join(f'<figure><a href="{p}/{c}.gif"><img src="{p}/{c}.gif" loading="lazy"></a>'
                        f'<figcaption>{html.escape(c)}</figcaption></figure>' for c in clips)
        body += f'<h2 id="{p}">{p} <small>{len(clips)}</small></h2><div class="grid">{cards}</div>'
    with open(f"{OUT}/index.html", "w") as f:
        f.write(f"""<!doctype html><meta charset="utf-8"><title>MysteryChain animations</title>
<style>
body{{background:#111;color:#ddd;font:14px system-ui;margin:0 16px}}
nav{{position:sticky;top:0;background:#111;padding:10px 0;z-index:1}} nav a{{color:#8cf;margin-right:14px;font-size:16px}}
h2{{margin:24px 0 8px}} small{{color:#777;font-weight:normal}}
.grid{{display:grid;grid-template-columns:repeat(auto-fill,minmax(var(--s),1fr));gap:12px}}
figure{{margin:0;text-align:center}} img{{width:var(--s);height:var(--s);image-rendering:pixelated}}
figcaption{{color:#aaa;margin-top:4px}}
</style>
<body style="--s:240px"><nav>{nav} &nbsp; size <input type="range" min="120" max="480" value="240"
 oninput="document.body.style.setProperty('--s',this.value+'px')"></nav>{body}</body>""")
    return f"{OUT}/index.html"


if __name__ == "__main__":
    names = sys.argv[1:] or list(PETS)
    if bad := [n for n in names if n not in PETS]:
        sys.exit(f"unknown: {bad}, pick from {list(PETS)}")
    with ThreadPoolExecutor() as ex:
        for n in ex.map(make, names):
            print("done", n)
    index = page()
    print(index)
    webbrowser.open("file://" + index)
