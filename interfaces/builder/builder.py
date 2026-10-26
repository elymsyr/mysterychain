"""MysteryChain builder: see whether the phone (adb) and the XIAO are plugged in,
build and install the current firmware and Android app, with progress."""

import glob
import os
import queue
import re
import shutil
import subprocess
import tempfile
import threading
import tkinter as tk
from tkinter import ttk

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SKETCH = os.path.join(ROOT, "interfaces", "watch", "firmware")
APP = os.path.join(ROOT, "interfaces", "mobile-app")
APK = os.path.join(APP, "build", "app", "outputs", "flutter-apk", "app-release.apk")
FQBN = "Seeeduino:nrf52:xiaonRF52840Sense"
FW_BUILD = os.path.join(tempfile.gettempdir(), "mysterychain-fw")

BG, FG, DIM, OK, BAD, BOX = "#1e1f22", "#d4d4d4", "#7a7d85", "#7fb685", "#c7826f", "#26282c"
PCT = re.compile(r"(\d{1,3})\s?%")


def phone():
    """Serial of the first adb device in 'device' state, or None."""
    try:
        out = subprocess.run(["adb", "devices"], capture_output=True, text=True, timeout=5).stdout
    except (OSError, subprocess.TimeoutExpired):
        return None
    for line in out.splitlines()[1:]:
        parts = line.split()
        if len(parts) == 2 and parts[1] == "device":
            return parts[0]
    return None


def xiao():
    """Serial port of the XIAO, or None."""
    for path in glob.glob("/dev/serial/by-id/*"):
        if "XIAO" in path or "Seeed" in path:
            return os.path.realpath(path)
    return None


class Builder(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("MysteryChain builder")
        self.configure(bg=BG, padx=16, pady=16)
        self.geometry("640x460")
        self.events = queue.Queue()
        self.busy = False

        style = ttk.Style(self)
        style.theme_use("clam")
        style.configure(".", background=BG, foreground=FG, font=("Sans", 10))
        style.configure("TButton", background=BOX, foreground=FG, borderwidth=0, padding=(12, 6))
        style.map("TButton", background=[("active", "#33363b"), ("disabled", BG)],
                  foreground=[("disabled", DIM)])
        style.configure("Horizontal.TProgressbar", troughcolor=BOX, background=OK, borderwidth=0)

        self.phone_lbl = tk.Label(self, bg=BG, anchor="w", font=("Sans", 11))
        self.xiao_lbl = tk.Label(self, bg=BG, anchor="w", font=("Sans", 11))
        self.phone_lbl.pack(fill="x")
        self.xiao_lbl.pack(fill="x", pady=(2, 12))

        row = tk.Frame(self, bg=BG)
        row.pack(fill="x")
        self.buttons = [
            ttk.Button(row, text="Firmware", command=lambda: self.start([self.fw_steps])),
            ttk.Button(row, text="App", command=lambda: self.start([self.app_steps])),
            ttk.Button(row, text="All", command=lambda: self.start([self.fw_steps, self.app_steps])),
        ]
        for b in self.buttons:
            b.pack(side="left", padx=(0, 8))

        # wireless adb: ip:port from Developer options → Wireless debugging (after `adb pair` once)
        ttk.Button(row, text="Connect", command=self.connect).pack(side="right")
        self.addr = tk.Entry(row, bg=BOX, fg=FG, insertbackground=FG, relief="flat", width=22,
                             highlightthickness=0, font=("Monospace", 10))
        self.addr.pack(side="right", padx=(0, 8), ipady=5)
        self.addr.bind("<Return>", lambda e: self.connect())

        self.step_lbl = tk.Label(self, bg=BG, fg=DIM, anchor="w", text="Ready")
        self.step_lbl.pack(fill="x", pady=(14, 4))
        self.bar = ttk.Progressbar(self, maximum=100)
        self.bar.pack(fill="x")

        self.log = tk.Text(self, bg=BOX, fg=DIM, relief="flat", font=("Monospace", 9),
                           wrap="none", padx=8, pady=6, highlightthickness=0)
        self.log.pack(fill="both", expand=True, pady=(12, 0))

        self.phone = self.xiao = None
        self.poll()
        self.pump()

    # device status, refreshed every 2 s off the UI thread
    def poll(self):
        threading.Thread(target=lambda: self.events.put(("status", phone(), xiao())), daemon=True).start()
        self.after(2000, self.poll)

    def show_status(self, ph, port):
        self.phone, self.xiao = ph, port
        self.phone_lbl.config(text=f"●  Phone   {ph or 'not connected'}", fg=OK if ph else BAD)
        self.xiao_lbl.config(text=f"●  XIAO    {port or 'not connected'}", fg=OK if port else BAD)

    def connect(self):
        addr = self.addr.get().strip()
        if not addr:
            return

        def go():
            try:
                r = subprocess.run(["adb", "connect", addr], capture_output=True, text=True, timeout=10)
                msg = (r.stdout + r.stderr).strip()
            except (OSError, subprocess.TimeoutExpired) as e:
                msg = str(e)
            self.events.put(("log", msg))

        threading.Thread(target=go, daemon=True).start()

    def pump(self):
        while not self.events.empty():
            kind, *args = self.events.get()
            if kind == "status":
                self.show_status(*args)
            elif kind == "log":
                self.log.insert("end", args[0] + "\n")
                self.log.see("end")
            elif kind == "step":
                self.step_lbl.config(text=args[0], fg=FG)
            elif kind == "bar":
                self.bar["value"] = args[0]
            elif kind == "done":
                self.busy = False
                self.step_lbl.config(text=args[0], fg=OK if args[1] else BAD)
                for b in self.buttons:
                    b.state(["!disabled"])
        self.after(100, self.pump)

    # each job returns [(label, cmd, cwd)]; built at start so the current port/serial is used
    def fw_steps(self):
        if not self.xiao:
            raise RuntimeError("XIAO not connected")
        return [
            ("Compiling firmware", ["arduino-cli", "compile", "--fqbn", FQBN, "--build-path", FW_BUILD, SKETCH], ROOT),
            ("Flashing XIAO", ["arduino-cli", "upload", "--fqbn", FQBN, "--input-dir", FW_BUILD,
                               "-p", self.xiao, SKETCH], ROOT),
        ]

    def app_steps(self):
        if not self.phone:
            raise RuntimeError("Phone not connected")
        return [
            ("Building APK", ["flutter", "build", "apk", "--release"], APP),
            ("Installing on phone", ["adb", "-s", self.phone, "install", "-r", APK], APP),
        ]

    def start(self, jobs):
        if self.busy:
            return
        try:
            steps = [s for job in jobs for s in job()]
        except RuntimeError as e:
            self.step_lbl.config(text=str(e), fg=BAD)
            return
        missing = sorted({s[1][0] for s in steps if not shutil.which(s[1][0])})
        if missing:
            self.step_lbl.config(text="Not found in PATH: " + ", ".join(missing), fg=BAD)
            return
        self.busy = True
        for b in self.buttons:
            b.state(["disabled"])
        self.log.delete("1.0", "end")
        threading.Thread(target=self.run, args=(steps,), daemon=True).start()

    def run(self, steps):
        n = len(steps)
        for i, (label, cmd, cwd) in enumerate(steps):
            self.events.put(("step", f"{i + 1}/{n}  {label}…"))
            self.events.put(("bar", 100 * i / n))
            self.events.put(("log", "$ " + " ".join(cmd)))
            # text mode turns the \r of progress bars into line breaks, so percentages stream in
            p = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            for line in p.stdout:
                line = line.rstrip()
                if not line:
                    continue
                self.events.put(("log", line))
                m = PCT.search(line)
                if m and int(m.group(1)) <= 100:
                    self.events.put(("bar", 100 * (i + int(m.group(1)) / 100) / n))
            if p.wait() != 0:
                self.events.put(("done", f"Failed: {label} (exit {p.returncode})", False))
                return
        self.events.put(("bar", 100))
        self.events.put(("done", "Done", True))


if __name__ == "__main__":
    Builder().mainloop()
