"""MysteryChain schematic (schemdraw).

To generate (the system python's numpy is too old for matplotlib, use a venv):
    python3 -m venv .venv && .venv/bin/pip install schemdraw matplotlib && .venv/bin/python schematic.py
Tested with: schemdraw 0.23, matplotlib 3.10.9
Output (this folder):
    schematic.svg/.png                transparent background
    schematic_white.svg/.png          white background
    schematic_pnp_white.svg/.png      bench variant, USB power (no battery), Q2 = BC327 PNP (`schematic.py pnp`; writes only these)
Source of truth: ../README.md (pinout, motor driver, power)
"""
import sys
from pathlib import Path

import schemdraw
import schemdraw.elements as elm

schemdraw.use('matplotlib')
P = elm.IcPin
OUT = Path(__file__).with_suffix('')
S = 1.5  # pin pitch


def text(xy, s, size=10, **kw):
    elm.Label().at(xy).label(s, fontsize=size, **kw)


def unused(xy, side, s='unused'):
    text((xy[0] + (0.3 if side == 'right' else -0.3), xy[1]), s, 9, color='gray',
         halign='left' if side == 'right' else 'right')


def draw(pnp=False):
    d = schemdraw.Drawing(show=False)
    with d:
        d.config(fontsize=11, unit=2.5)

        # ---- U1: XIAO (physical pin order, seen from the component side, USB-C up) ----
        left = ['D0', 'D1', 'D2', 'D3', 'D4', 'D5', 'D6']
        right = [('5V', '14', 'V5'), ('GND', '13', 'GND'), ('3V3', '12', 'V33'), ('D10', '11', 'D10'),
                 ('D9', '10', 'D9'), ('D8', '9', 'D8'), ('D7', '8', 'D7')]
        # 8 slots, bottom one empty (room for BAT labels); slot pitch = (H - 1) / 7 = S
        x = elm.Ic(size=(7, 1 + 7 * S), pins=[P(n, pin=str(i + 1), side='left', slot=f'{8 - i}/8') for i, n in enumerate(left)]
                   + [P(n, pin=p, side='right', slot=f'{8 - i}/8', anchorname=a) for i, (n, p, a) in enumerate(right)]
                   + [P('BAT+', side='bottom', slot='1/2', anchorname='BATP'), P('BAT−', side='bottom', slot='2/2', anchorname='BATN')])
        text((3.5, 10.8), 'USB-C\n(charging + programming)', 9)
        text((3.5, 7.0), 'U1\nXIAO nRF52840\nSense', 13)
        text((3.5, 3.6), 'on board:\nBQ25101 charger (~50 mA)\n3V3 LDO\nLSM6DS3TR-C IMU (Wire1)\nVBAT divider 1M/510k\n(P0.31 sense, P0.14 LOW)', 8, color='dimgray')
        for n in ('D4', 'D5'):
            unused(getattr(x, n), 'left')
        unused(x.V5, 'right', 'unused (USB 5 V)')
        unused(x.D9, 'right', 'unused (SPI MISO, leave open)')

        # ---- U1 right: power and SPI ----
        elm.Line().at(x.GND).right(1.2)
        elm.Ground()
        elm.Line().at(x.V33).right(2.4)
        elm.Vdd().label('3V3', loc='right')

        dx = 17.0  # x of the display pin ends
        elm.Line().at(x.D10).to((dx, x.D10.y)).label('SPI MOSI', fontsize=9)
        elm.Line().at(x.D8).to((dx, x.D8.y)).label('SPI SCK', fontsize=9)
        elm.Line().at(x.D7).right(1.5)
        elm.Tag().label('LCD_EN', fontsize=9)

        # ---- U2: GC9A01 display (pin no. = physical header order) ----
        # slot 2 empty: SDA lines up with U1 D10, SCL with D8
        disp = [('VCC', '1', 8), ('GND', '2', 7), ('RST', '7', 6), ('CS', '6', 5), ('DC', '5', 4), ('SDA', '4', 3), ('SCL', '3', 1)]
        u2 = elm.Ic(size=(5, 1 + 7 * S), pins=[P(n, pin=p, side='left', slot=f'{k}/8', anchorname=n) for n, p, k in disp]).anchor('SCL').at((dx, x.D8.y))
        text((u2.SCL.x + 3.0, u2.DC.y + 0.8), 'U2\nGC9A01 1.28"\n240×240\nround TFT', 13)
        text((u2.SCL.x + 3.0, u2.SCL.y + 1.0), '7 pins, no BL pin\n(backlight on with VCC)\n3.3 V LDO on module', 8, color='dimgray')
        for n, net in (('RST', 'LCD_RST'), ('CS', 'LCD_CS'), ('DC', 'LCD_DC')):
            elm.Line().at(getattr(u2, n)).left(1.2)
            elm.Tag().left().label(net, fontsize=9)
        elm.Line().at(u2.GND).left(1.0)
        elm.Ground()

        # ---- Q2: display power switch, D7 LOW = display on (required: no slide switch, the backlight has no pin) ----
        qx = dx - 4.0
        elm.Line().at(u2.VCC).left(4.0)
        # PFet's natural orientation is vertical: source up, drain down; reverse() puts the gate on the left
        if pnp:  # bench: PNP until the AO3401A arrives; emitter = source, collector = drain, base = gate (+ R4)
            q2 = elm.BjtPnp(circle=True).right().anchor('collector').at((qx, u2.VCC.y + 0.6))
            text((qx + 1.1, q2.base.y), 'Q2\nBC327', 11, halign='left')
            hi, lo, ctl = q2.emitter, q2.collector, q2.base
        else:
            q2 = elm.PFet(bulk=False).right().reverse().anchor('drain').at((qx, u2.VCC.y + 0.6))
            text((qx + 1.1, q2.gate.y), 'Q2\nAO3401A', 11, halign='left')
            hi, lo, ctl = q2.source, q2.drain, q2.gate
        elm.Line().at((qx, u2.VCC.y)).to(lo)
        elm.Dot().at((qx, u2.VCC.y))
        top = (qx, hi.y + 1.4)
        elm.Line().at(hi).to(top)
        elm.Dot().at(top)
        elm.Vdd().at(top).label('3V3')
        gx = ctl.x - 1.0
        elm.Line().at(ctl).to((gx, ctl.y))
        gdot = elm.Dot().at((gx, ctl.y))
        elm.Resistor().at((gx, ctl.y)).to((gx, top[1])).label('R3\n100 kΩ', loc='bottom')
        elm.Line().at((gx, top[1])).to(top)
        if pnp:
            elm.Resistor().at(gdot.center).left(2.5).label('R4\n1 kΩ', loc='bottom')
        else:
            elm.Line().at(gdot.center).left(1.0)
        elm.Tag().left().label('LCD_EN', fontsize=9)

        # ---- U1 left: button and display control lines ----
        elm.Line().at(x.D0).left(0.8)
        elm.Button().left().label('SW1\npower / tact button', loc='top')
        elm.Line().left(0.6)
        elm.Ground()
        text((x.D0.x - 0.3, x.D0.y - 0.7), 'internal pull-up', 8, color='gray', halign='right')
        for n, net in (('D1', 'LCD_CS'), ('D2', 'LCD_DC'), ('D3', 'LCD_RST')):
            elm.Line().at(getattr(x, n)).left(1.0)
            elm.Tag().left().label(net, fontsize=9)

        # ---- Motor driver: D6 -> R1 -> Q1 (BC337), M1 from BAT+ (bench: 3V3) to the collector ----
        elm.Line().at(x.D6).left(0.8)
        r1 = elm.Resistor().left().label('R1\n1 kΩ', loc='top')
        bnode = elm.Dot()
        q1 = elm.BjtNpn(circle=True).right().reverse().anchor('base').at((r1.end.x - 1.2, r1.end.y))
        text((q1.base.x - 2.0, q1.base.y), 'Q1\nBC337', 11, halign='right')
        elm.Line().at(bnode.center).to(q1.base)
        elm.Resistor().at(bnode.center).down(3.0).label('R2\n10 kΩ', loc='bottom')
        elm.Ground()
        elm.Line().at(q1.emitter).toy(r1.end.y - 3.0)
        elm.Ground()
        cnode = elm.Dot().at((q1.collector.x, q1.collector.y + 0.6))
        elm.Line().at(q1.collector).to(cnode.center)
        rail = cnode.center.y + 4.0
        elm.Motor().at(cnode.center).up(4.0).label('M1\n10×3\nvibration motor', loc='bottom', ofst=(0, -0.3))
        mtop = elm.Dot()
        elm.Vdd().label('3V3' if pnp else 'BAT+')  # off the LDO: its current sagged 3V3 and dimmed the display
        # D1: cathode (band) to the motor supply
        dxx = cnode.center.x - 2.2
        elm.Line().at(cnode.center).to((dxx, cnode.center.y))
        elm.Diode().at((dxx, cnode.center.y)).up(4.0).label('D1\n1N4148', loc='bottom')
        elm.Line().to(mtop.center)
        # C1, C2: between 3V3 and GND, hold 3V3 up for the display
        c2 = elm.Dot().at((dxx - 3.0, rail))
        elm.Vdd().at(c2.center).label('3V3')
        elm.Capacitor(polar=True).at(c2.center).down(3.0).label('C2\n100 µF', loc='bottom')
        elm.Ground()
        elm.Line().at(c2.center).left(2.4)
        elm.Capacitor().down(3.0).label('C1\n100 nF', loc='bottom')
        elm.Ground()

        # ---- Battery, through the battery switch SW2 to the BAT pads on the underside of the XIAO ----
        if pnp:  # bench: USB only, no battery
            text(((x.BATP.x + x.BATN.x) / 2, x.BATP.y - 0.8), 'BAT+ / BAT− unused: bench runs on USB only, no battery', 9, color='gray')
        else:
            elm.Dot().at(x.BATP)
            elm.Line().at(x.BATP).left(1.2)
            elm.Tag().left().label('BAT+', fontsize=9)  # motor supply
            elm.Switch().at(x.BATP).down(1.6).label('SW2\nbattery switch\nEG1390', loc='bottom', ofst=(0, 0.2))
            b1 = elm.BatteryCell().down(3.0).label('B1\nLiPo 402030\n3.7 V ~200 mAh\nprotected (PCM)', loc='bottom', ofst=(0, 0.3))
            text((b1.center.x - 0.45, b1.center.y + 0.35), '+', 12)  # long plate = positive
            bb = elm.Line().down(0.6)
            elm.Line().at(bb.end).to((x.BATN.x, bb.end.y))
            elm.Line().toy(x.BATN.y)
            elm.Dot().at((x.BATN.x, bb.end.y + 2.0))
            elm.Line().at((x.BATN.x, bb.end.y + 2.0)).right(1.2)
            elm.Ground()
            text((x.BATN.x + 0.3, x.BATN.y - 1.0), 'BAT− = GND', 8, color='gray', halign='left')

        # ---- title and notes ----
        title_y = 20.0
        title = 'MysteryChain — schematic, bench variant: USB power, Q2 = PNP  (v4, 23 Oct 2026)' if pnp else 'MysteryChain — schematic  (v4, 23 Oct 2026)'
        text((-13.5, title_y), title, 16, halign='left')
        text((-13.5, title_y - 0.9), 'Pinout and notes: electronics/README.md · Build and tests: electronics/bring-up.md', 9,
             color='dimgray', halign='left')
        notes = [
            'Labels with the same name (LCD_CS, LCD_DC, LCD_RST, LCD_EN) are connected. All 3V3, BAT+ and GND symbols are one net each.',
            'Q1 BC337 legs: flat face toward you, legs down, left to right C–B–E (verify on your part with a diode test).',
            'M1 + and the D1 band (cathode) go to BAT+, not 3V3: on 3V3 the motor current sagged the LDO and dimmed the display. '
            'Firmware scales the PWM duty with VBAT so the 3 V motor sees ~3.1 V. Reversed, D1 shorts BAT+ to GND when Q1 turns on.',
            'Solder D1 close to the motor, C1 and C2 close to the 3V3 node. Use a ceramic C2: the battery stays connected while off, '
            'and a tantalum may leak up to ~6 µA (0.01·C·V).',
            'Daily on/off is SW1 in firmware: display pins LOW, Q2 off, System OFF; hold SW1 ~1 s to wake, 3 s to turn off; '
            'USB plug-in also wakes it. '
            + ('Bench: no battery and no SW2; unplug USB for a full power cycle.' if pnp else
               'SW2 (reached from the back, under tape) disconnects the battery for storage, '
               'water or a hang. It charges only with SW2 on (the charger is on the XIAO side of SW2).'),
            'Q2 cuts the display: its backlight has no pin and draws ~15–25 mA whenever VCC is present. '
            'R3 keeps it off while D7 is not driven (reset, boot, System OFF with D7 HIGH). '
            + ('Bench variant below.' if pnp else 'AO3401A SOT-23: 1 G, 2 S, 3 D.'),
            'XIAO pins 1–14 are the edge pads, drawn as seen from the component side (USB-C up). '
            'In the case the component side faces the back, so seen from the front the left and right rows swap. BAT+ and BAT− are underside pads.',
            'Leave D9 unconnected: the display library claims it as MISO when it starts SPI. '
            '5V is USB 5 V; never connect the battery there.',
            'Display header pin order varies by vendor; wire by the labels printed on the board.',
        ]
        if pnp:
            notes.append('Bench variant, Q2 = PNP until the AO3401A arrives: BC327, or 2N2907 / S8550 / 2N3906 (never an NPN). '
                         'Firmware unchanged, D7 LOW = on. M1 stays on 3V3: BAT+ is only the charger output without a battery.\n'
                         '    R4 1 kΩ: ~2.5 mA base current saturates it for the display\'s 25–40 mA, ~0.1 V drop. '
                         'R3 100 kΩ base–emitter keeps it off.\n'
                         '    Legs: BC327 C–B–E (like BC337); S8550, 2N2907, 2N3906 E–B–C. Verify with a diode test, black probe on B.\n'
                         '    TO-92 + R4 do not fit the Q2 spot in the case: bench only.\n'
                         '    No battery: VBAT reads the charger output and the charge LED may flicker; ignore both. '
                         'The low-battery cut-off stays off on USB.')
        text((-13.5, -7.0), 'Notes\n' + '\n'.join(f'{i}. {n}' for i, n in enumerate(notes, 1)), 9, halign='left', valign='top')
    return d


U1_PINS = ['D0', 'D1', 'D2', 'D3', 'D4', 'D5', 'D6', 'V5', 'GND', 'V33', 'D10', 'D9', 'D8', 'D7', 'BATP', 'BATN']
U2_PINS = ['VCC', 'GND', 'RST', 'CS', 'DC', 'SDA', 'SCL']
# Expected connections (electronics/README.md). On two-terminal parts .1 = the end the drawing starts from:
# D1.1 anode, D1.2 cathode; B1.1 positive; C2.1 positive; M1.1 collector side.
EXPECTED = [
    {'GND', 'U1.GND', 'U1.BATN', 'U2.GND', 'SW1.2', 'R2.2', 'Q1.emitter', 'C1.2', 'C2.2', 'B1.2'},
    {'3V3', 'U1.V33', 'C1.1', 'C2.1', 'Q2.source', 'R3.2'},
    {'U2.VCC', 'Q2.drain'},
    {'LCD_EN', 'U1.D7', 'Q2.gate', 'R3.1'},
    {'LCD_CS', 'U1.D1', 'U2.CS'},
    {'LCD_DC', 'U1.D2', 'U2.DC'},
    {'LCD_RST', 'U1.D3', 'U2.RST'},
    {'U1.D10', 'U2.SDA'},
    {'U1.D8', 'U2.SCL'},
    {'U1.D0', 'SW1.1'},
    {'U1.D6', 'R1.1'},
    {'R1.2', 'R2.1', 'Q1.base'},
    {'Q1.collector', 'M1.1', 'D1.1'},
    {'BAT+', 'U1.BATP', 'SW2.1', 'M1.2', 'D1.2'},
    {'SW2.2', 'B1.1'},
]
EXPECTED_PNP = [n for n in EXPECTED if not n & {'Q2.source', 'Q2.drain', 'Q2.gate', 'SW2.1', 'SW2.2', 'GND'}] + [
    {'GND', 'U1.GND', 'U2.GND', 'SW1.2', 'R2.2', 'Q1.emitter', 'C1.2', 'C2.2'},
    {'3V3', 'U1.V33', 'M1.2', 'D1.2', 'C1.1', 'C2.1', 'Q2.emitter', 'R3.2'},
    {'U2.VCC', 'Q2.collector'},
    {'LCD_EN', 'U1.D7', 'R4.2'},
    {'R4.1', 'Q2.base', 'R3.1'},
]


def netlist(d):
    """Netlist from the drawing geometry: wire ends, T-junctions, pin ends (dotted lines are not wires)."""
    parent = {}

    def key(p):
        return (round(p[0], 3), round(p[1], 3))

    def find(k):
        parent.setdefault(k, k)
        while parent[k] != k:
            k = parent[k]
        return k

    def union(a, b):
        parent[find(key(a))] = find(key(b))

    names, segs, dots, ics = {}, [], [], []
    for e in d.elements:
        n, a, lab = type(e).__name__, e.absanchors, [l.label for l in e._userlabels]
        if n == 'Line':
            if e._userparams.get('ls') != ':':
                union(a['start'], a['end'])
                segs.append((a['start'], a['end']))
        elif n == 'Dot':
            dots.append(a['start'])
        elif n in ('Ground', 'Vdd', 'Tag'):
            names.setdefault(key(a['start']), set()).add('GND' if n == 'Ground' else lab[0])
        elif n == 'Ic':
            ics.append(e)
            for pin in (U1_PINS if len(ics) == 1 else U2_PINS):
                names.setdefault(key(a[pin]), set()).add(f'U{len(ics)}.{pin}')
        elif n in ('BjtNpn', 'BjtPnp', 'PFet'):
            for t in ('source', 'drain', 'gate') if n == 'PFet' else ('base', 'collector', 'emitter'):
                names.setdefault(key(a[t]), set()).add(f'{"Q1" if n == "BjtNpn" else "Q2"}.{t}')
        elif n != 'Label' and n != 'EncircleBox':  # two-terminal parts: name = first line of the label
            ref = lab[0].split('\n')[0]
            ends = ('2', '1') if e._userparams.get('reverse') else ('1', '2')  # reverse() flips the symbol, not the ends
            names.setdefault(key(a['start']), set()).add(f'{ref}.{ends[0]}')
            names.setdefault(key(a['end']), set()).add(f'{ref}.{ends[1]}')

    def on_seg(p, s, t):
        cross = (t[0] - s[0]) * (p[1] - s[1]) - (t[1] - s[1]) * (p[0] - s[0])
        inside = min(s[0], t[0]) - 1e-6 <= p[0] <= max(s[0], t[0]) + 1e-6 and min(s[1], t[1]) - 1e-6 <= p[1] <= max(s[1], t[1]) + 1e-6
        return abs(cross) < 1e-6 and inside and key(p) not in (key(s), key(t))

    # degree: wire ends + part ends meeting at a point; a wire passing through counts 2. >= 3 needs a dot.
    dotset = {key(p) for p in dots}
    deg = {p: len(ns) for p, ns in names.items()}
    for s, t in segs:
        for q in (key(s), key(t)):
            deg[q] = deg.get(q, 0) + 1
    problems = []
    for p in set(deg) | dotset:
        for s in [s for s in segs if on_seg(p, *s)]:
            union(p, s[0])
            deg[p] = deg.get(p, 0) + 2
        if deg.get(p, 0) >= 3 and p not in dotset:
            problems.append(f'junction without a dot: {p}')
    for p in dots:
        if not any(key(p) in (key(s), key(t)) or on_seg(p, s, t) for s, t in segs):
            problems.append(f'floating dot: {key(p)}')

    by_label = {}  # GND/3V3 symbols and tags with the same name are one net
    for p, ns in names.items():
        for lbl in (n for n in ns if '.' not in n):
            by_label.setdefault(lbl, []).append(p)
    for pts in by_label.values():
        for p in pts[1:]:
            union(p, pts[0])

    nets = {}
    for p, ns in names.items():
        nets.setdefault(find(p), set()).update(ns)
    return [n for n in nets.values() if len(n) > 1], problems


def check(d, pnp=False):
    expected = [set(n) for n in (EXPECTED_PNP if pnp else EXPECTED)]
    nets, problems = netlist(d)
    got = {frozenset(n) for n in nets}
    want = {frozenset(n) for n in expected}
    for n in got - want:
        problems.append(f'unexpected net: {sorted(n)}')
    for n in want - got:
        problems.append(f'missing net: {sorted(n)}')
    assert not problems, '\n'.join(problems)
    return len(got)


if __name__ == '__main__' and sys.argv[1:] == ['pnp']:
    d = draw(pnp=True)
    print(check(d, pnp=True), 'nets, match expected (PNP bench variant)')
    d.save(f'{OUT}_pnp_white.svg', transparent=False)
    d.save(f'{OUT}_pnp_white.png', transparent=False, dpi=150)
elif __name__ == '__main__':
    d = draw()
    print(check(d), 'nets, match expected')
    d.save(f'{OUT}.svg')
    d.save(f'{OUT}.png', dpi=150)
    d.save(f'{OUT}_white.svg', transparent=False)  # for dark-theme viewers and printing
    d.save(f'{OUT}_white.png', transparent=False, dpi=150)
