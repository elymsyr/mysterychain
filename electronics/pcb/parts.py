"""MysteryChain PCB rev A: the single source of parts, nets, placement and BOM data.

Every generator (gen_lib, gen_sch, gen_pcb, fab) and check.py reads this file.
Coordinates in `pcb` are case coordinates (origin = display centre, +Y toward the lug);
gen_pcb.py converts them to KiCad. Spec: docs/superpowers/specs/2026-10-20-pcb-design.md.

TR prices were checked 2026-10-21 (KDV excluded at e-komponent, included at direnc.net).
Heights are the part's height above the board: KiCad 3D model or datasheet, whichever is known.
"""
from dataclasses import dataclass, field


@dataclass
class Part:
    ref: str
    value: str
    symbol: str                      # "Lib:Name"
    footprint: str                   # "Lib:Name"; "" = schematic only
    pins: dict[str, str]             # symbol pin number -> net; missing pin = no-connect
    side: str                        # "F" (display side) / "B" (midframe side)
    sch: tuple[float, float, float]  # sheet x, y (mm), rotation
    height: float                    # mm above the board
    mpn: str = ""
    tr_source: str = ""
    alt: str = ""
    bom: bool = True
    pcb: tuple[float, float, float] | None = None  # case X, case Y, rotation; Task 4
    extra: dict = field(default_factory=dict)


EK = "e-komponent.com"
DN = "direnc.net"

R0603 = "Resistor_SMD:R_0603_1608Metric"
C0603 = "Capacitor_SMD:C_0603_1608Metric"
SOT23 = "Package_TO_SOT_SMD:SOT-23"

# Schematic blocks: origin, columns, cell size (mm). Parts are dropped into cells in order.
_BLOCKS = {
    "power": ((25.4, 30.48), 7, (30.48, 33.02)),
    "mcu": ((190.5, 175.26), 5, (27.94, 30.48)),
    "imu": ((304.8, 45.72), 4, (27.94, 33.02)),
    "disp": ((304.8, 124.46), 4, (27.94, 33.02)),
    "tp": ((25.4, 228.6), 12, (20.32, 20.32)),
}
_fill: dict[str, int] = {}


def _cell(block):
    (x0, y0), cols, (dx, dy) = _BLOCKS[block]
    i = _fill.get(block, 0)
    _fill[block] = i + 1
    return (round(x0 + (i % cols) * dx, 2), round(y0 + (i // cols) * dy, 2), 0)


def R(ref, value, a, b, block, side="F"):
    return Part(ref, value, "Device:R", R0603, {"1": a, "2": b}, side, _cell(block), 0.55,
                mpn=f"0603 1% thick film, {value} (Yageo RC0603FR-07{value}L class)",
                tr_source=f"{DN} 0603 1% (20K: 0,08 TL, stokta)")


def C(ref, value, a, b, block, side="F", mpn=None, src=None, fp=C0603, height=0.9):
    return Part(ref, value, "Device:C", fp, {"1": a, "2": b}, side, _cell(block), height,
                mpn=mpn or f"0603 {'C0G 50 V' if value.endswith('pF') else 'X5R/X7R 10 V'}, {value}",
                tr_source=src or f"{DN} / {EK} 0603 MLCC")


def TP(ref, net):
    return Part(ref, net, "Connector:TestPoint", "mysterychain:TestPad_D1.0mm", {"1": net},
                "F", _cell("tp"), 0.0, mpn="bare pad", tr_source="-", bom=False)


E73_PINS = {
    "1": "MOTOR", "2": "LCD_EN", "3": "LCD_CS", "4": "LCD_DC", "5": "GND",
    "6": "LCD_SCK", "7": "BTN", "8": "LCD_RST", "9": "VBAT_ADC", "10": "LCD_MOSI",
    "11": "XL1", "13": "XL2", "15": "IMU_SCL", "17": "IMU_SDA", "19": "3V3", "21": "GND",
    "23": "3V3", "24": "GND", "26": "RESET", "27": "VBUS", "29": "USB_DN", "31": "USB_DP",
    "33": "IMU_PWR", "35": "CHG_STAT", "37": "SWDIO", "39": "SWCLK", "41": "NFC1", "43": "P010",
}

PARTS: list[Part] = [
    # ---- USB-C and charger
    Part("J1", "USB_C_16P", "Connector:USB_C_Receptacle_USB2.0_16P",
         "Connector_USB:USB_C_Receptacle_GCT_USB4105-xx-A_16P_TopMnt_Horizontal",
         {**{p: "VBUS" for p in ("A4", "A9", "B4", "B9")},
          **{p: "GND" for p in ("A1", "A12", "B1", "B12", "S1")},
          "A5": "CC1", "B5": "CC2", "A6": "USB_DP", "B6": "USB_DP", "A7": "USB_DN", "B7": "USB_DN"},
         "B", (38.1, 177.8, 0), 3.31, mpn="GCT USB4105-GF-A",
         tr_source=f"{EK} 57,11 TL (stokta)", alt="USB4105-GF-A-060/-120 (same footprint, longer posts)"),
    R("R1", "5.1k", "CC1", "GND", "power"),
    R("R2", "5.1k", "CC2", "GND", "power"),
    C("C1", "4.7uF", "VBUS", "GND", "power"),
    # SOT-23-5 is 1.45-1.55 mm tall (Microchip OT max / KiCad model): too tall for the front.
    Part("U3", "MCP73831-2", "Battery_Management:MCP73831-2-OT", "Package_TO_SOT_SMD:SOT-23-5",
         {"1": "STAT", "2": "GND", "3": "BAT", "4": "VBUS", "5": "PROG"},
         "B", _cell("power"), 1.55, mpn="MCP73831T-2ACI/OT",
         tr_source=f"{EK} 54,26 TL (stokta); {DN} 63,93 TL (tükendi)"),
    R("R3", "20k", "PROG", "GND", "power"),
    C("C2", "4.7uF", "BAT", "GND", "power"),
    R("R4", "10k", "STAT", "CHG_STAT", "power"),
    R("R5", "15k", "CHG_STAT", "GND", "power"),
    # ---- battery switch and power path
    Part("SW2", "EG1390", "Switch:SW_SPDT", "mysterychain:EG1390",
         {"2": "BAT", "1": "BATT_RAW"},
         "B", _cell("power"), 1.5, mpn="E-Switch EG1390",
         tr_source="❓ not listed at e-komponent / direnc.net; owner has 1 (LilyPad switch board)",
         alt=f"{EK} quote request ('teklif iste') for EG1390; body 3.5×7.0×1.5, actuator 2.0 into the slot"),
    # battery and motor pads on the back top edge: the LiPo sits in the back layer above Y 7.1
    Part("J3", "BATT", "Connector_Generic:Conn_01x02", "mysterychain:PadRow_1x02_P2.00mm",
         {"1": "BATT_RAW", "2": "GND"}, "B", _cell("power"), 0.0,
         mpn="wire pads (LiPo 402030 leads)", tr_source="-", bom=False),
    Part("Q1", "AO3401A", "Transistor_FET:AO3401A", SOT23, {"1": "VBUS", "2": "VSYS", "3": "BAT"},
         "F", _cell("power"), 1.2, mpn="AO3401A", tr_source=f"{EK} 37,12 TL (stokta)",
         alt="Si2301 (same SOT-23 G/S/D)"),
    R("R6", "100k", "VBUS", "GND", "power"),
    Part("D1", "B5819W", "Device:D_Schottky", "Diode_SMD:D_SOD-123", {"1": "VSYS", "2": "VBUS"},
         "B", _cell("power"), 1.26, mpn="B5819W", tr_source=f"{DN} 3,49 TL (stokta)",
         alt="SS14 is SMA, not pin-compatible"),
    C("C3", "1uF", "VSYS", "GND", "power"),
    # ---- 3V3
    Part("U4", "MCP1700-3302", "Regulator_Linear:MCP1700x-330xxTT", SOT23,
         {"1": "GND", "2": "3V3", "3": "VSYS"}, "F", _cell("power"), 1.2, mpn="MCP1700T-3302E/TT",
         tr_source=f"❓ {DN} 34,87 TL and {EK} 36,41 TL, both out of stock",
         alt=f"XC6206P332MR-G ({EK} 44,26 TL, stokta): SOT-23 1 VSS / 2 VOUT / 3 VIN, same pads"),
    C("C4", "4.7uF", "3V3", "GND", "power"),
    C("C5", "100nF", "3V3", "GND", "power"),
    C("C6", "100uF", "3V3", "GND", "power", side="B", fp="Capacitor_SMD:C_1206_3216Metric",
      mpn="Murata GRM31CR60J107KEA8L (1206 X5R 6.3 V)", src=f"{EK} 32,84 TL (stokta)", height=1.8),
    # ---- MCU
    Part("U1", "E73-2G4M08S1C", "mysterychain:E73-2G4M08S1C", "mysterychain:E73-2G4M08S1C",
         E73_PINS, "B", (241.3, 101.6, 0), 3.0, mpn="Ebyte E73-2G4M08S1C",
         tr_source="Robiz (robiz.net) 265,50 TL, checked 2026-10-20"),
    # CL 12.5 pF (nRF52840 LFXO allows <= 12.5 pF): caps 2 x (12.5 - 2) = 21 -> 22 pF
    Part("Y1", "32.768kHz", "Device:Crystal", "Crystal:Crystal_SMD_3215-2Pin_3.2x1.5mm",
         {"1": "XL1", "2": "XL2"}, "F", _cell("mcu"), 0.93,
         mpn="RT3215-32.768-12.5-TR (CL 12.5 pF, 20 ppm)", tr_source=f"{DN} 9,76 TL (stokta)",
         alt=f"ECS-.327-7-12R-TR (CL 7 pF, {EK} 47,12 TL) with 10 pF caps"),
    C("C8", "22pF", "XL1", "GND", "mcu"),
    C("C9", "22pF", "XL2", "GND", "mcu"),
    R("R11", "1M", "BAT", "VBAT_ADC", "mcu"),
    R("R12", "1M", "VBAT_ADC", "GND", "mcu"),
    C("C13", "100nF", "VBAT_ADC", "GND", "mcu"),
    Part("SW3", "RESET", "Switch:SW_Push", "Button_Switch_SMD:SW_SPST_PTS810",
         {"1": "RESET", "2": "GND"}, "B", _cell("mcu"), 2.5, mpn="C&K PTS810SJK250SMTRLFS",
         tr_source=f"{EK} 41,41 TL (stokta)", alt=f"KMR221G ({EK} 49,83 TL) needs its own footprint"),
    Part("SW1", "BUTTON", "Switch:SW_Push", "Button_Switch_SMD:SW_SPST_CK_KMS2xxGP",
         {"1": "BTN", "2": "GND"}, "B", _cell("mcu"), 1.42, mpn="C&K KMS221GP LFS (side actuated)",
         tr_source=f"{EK} 54,29 TL (stokta)"),
    Part("J5", "SWD", "Connector_Generic:Conn_01x05", "mysterychain:PadRow_1x05_P2.54mm",
         {"1": "3V3", "2": "SWDIO", "3": "SWCLK", "4": "RESET", "5": "GND"}, "F", _cell("mcu"), 0.0,
         mpn="SWD pads 2.54 mm (pogo clip / held header)", tr_source="-", bom=False),
    # ---- IMU
    Part("U2", "LSM6DS3TR-C", "Sensor_Motion:LSM6DS3", "mysterychain:LGA-14_3x2.5mm_P0.5mm_HandSolder",
         {"1": "GND", "2": "GND", "3": "GND", "4": "IMU_INT1", "5": "IMU_PWR", "6": "GND",
          "7": "GND", "8": "IMU_PWR", "12": "IMU_PWR", "13": "IMU_SCL", "14": "IMU_SDA"},
         "F", _cell("imu"), 0.86, mpn="ST LSM6DS3TR-C",
         tr_source=f"❓ bare chip not stocked in TR ({EK} has only the Adafruit 4503 breakout, 739,95 TL)",
         alt=f"LSM6DSLTR ({EK} 299,85 TL, stokta): same LGA-14 pinout and WHO_AM_I 0x6A; "
             "pedometer/tap registers differ, firmware check first"),
    C("C10", "100nF", "IMU_PWR", "GND", "imu"),
    R("R7", "4.7k", "IMU_SDA", "IMU_PWR", "imu"),
    R("R8", "4.7k", "IMU_SCL", "IMU_PWR", "imu"),
    Part("JP1", "INT1", "Jumper:SolderJumper_2_Open", "Jumper:SolderJumper-2_P1.3mm_Open_Pad1.0x1.5mm",
         {"1": "NFC1", "2": "IMU_INT1"}, "F", _cell("imu"), 0.0, mpn="solder jumper, open",
         tr_source="-", bom=False),
    # ---- display, motor
    Part("J2", "LCD", "Connector_Generic:Conn_01x07", "mysterychain:PadRow_1x07_P2.00mm",
         {"1": "LCD_VCC", "2": "GND", "3": "LCD_SCK", "4": "LCD_MOSI", "5": "LCD_DC", "6": "LCD_CS",
          "7": "LCD_RST"}, "F", _cell("disp"), 0.0,
         mpn="wire pads: VCC GND SCL SDA DC CS RST (GC9A01 module labels)", tr_source="-", bom=False),
    Part("Q2", "AO3401A", "Transistor_FET:AO3401A", SOT23, {"1": "LCD_EN", "2": "3V3", "3": "LCD_VCC"},
         "F", _cell("disp"), 1.2, mpn="AO3401A", tr_source=f"{EK} 37,12 TL (stokta)", alt="Si2301"),
    R("R9", "100k", "LCD_EN", "3V3", "disp"),
    Part("Q3", "AO3400A", "Transistor_FET:AO3400A", SOT23, {"1": "MOTOR", "2": "GND", "3": "MOTOR_SW"},
         "F", _cell("disp"), 1.2, mpn="AO3400A", tr_source=f"{EK} 38,67 TL (stokta)", alt="Si2302"),
    R("R10", "100k", "MOTOR", "GND", "disp"),
    # motor flyback: SOD-323 (1.1 mm) so it fits on the front; the back has no room left
    Part("D2", "1N4148WS", "Device:D", "Diode_SMD:D_SOD-323", {"1": "3V3", "2": "MOTOR_SW"},
         "F", _cell("disp"), 1.11, mpn="1N4148WS", tr_source=f"{DN} 0,41 TL (stokta)",
         alt=f"{EK} 1N4148WS_R1_00001 9,99 TL"),
    C("C12", "100nF", "3V3", "MOTOR_SW", "disp"),
    Part("J4", "MOTOR", "Connector_Generic:Conn_01x02", "mysterychain:PadRow_1x02_P2.00mm",
         {"1": "3V3", "2": "MOTOR_SW"}, "B", _cell("disp"), 0.0, mpn="wire pads (vibration motor)",
         tr_source="-", bom=False),
]

TEST_NETS = ["VBUS", "VSYS", "BAT", "BATT_RAW", "3V3", "LCD_VCC", "IMU_PWR", "GND", "CHG_STAT",
             "MOTOR_SW", "BTN", "IMU_SDA", "IMU_SCL", "P010"]

PARTS += [TP(f"TP{i}", n) for i, n in enumerate(TEST_NETS + ["GND"], 1)]

PARTS += [
    Part(f"FID{i}", "FID", "Mechanical:Fiducial",
         "Fiducial:Fiducial_1mm_Mask2mm" if side == "F" else "Fiducial:Fiducial_0.75mm_Mask1.5mm",  # back is full
         {}, side, _cell("tp"), 0.0, bom=False)
    for i, side in enumerate("FFB", 1)  # a 2nd back fiducial does not fit: series panels carry rail fiducials
]
PARTS += [
    Part(f"H{i}", "NPTH 2.0", "Mechanical:MountingHole", "mysterychain:NPTH_D2.0mm", {}, "F",
         _cell("tp"), 0.0, bom=False)
    for i in (1, 2)
]
# ERC: nets driven only by passive pins need a flag. Schematic only.
PARTS += [
    Part(f"#FLG0{i}", "PWR_FLAG", "power:PWR_FLAG", "", {"1": n}, "F", _cell("tp"), 0.0, bom=False)
    for i, n in enumerate(["VBUS", "VSYS", "GND", "IMU_PWR"], 1)
]

# Chosen fab: Izmir PCB's import line (TR company, customs handled by them, online calculator
# pcbfiyathesaplama.com). Their domestic line does 1.0 mm but only 0.35 mm track/space, too coarse
# for the 0.5 mm pitch USB-C and LGA pads. Second source: Asik Elektronik (0.4-3.2 mm, 0.075 mm).
FAB = {
    "name": "Izmir PCB (import line)",
    "url": "https://www.izmirnumunepcb.com/pcb-teknik-uretim-kabiliyeti-becerileri.html",
    "min_track": 0.15,
    "min_space": 0.15,
    "via_drill": 0.30,
    "via_d": 0.60,         # not published; drill + 2 x 0.15 ring, the usual import-line minimum
    "thickness_ok": True,  # 0.6-2.4 mm
    "price_10": None,      # quote only
    "second": "Asik Elektronik, https://www.asikelektronik.com.tr/baski-devre-teknik-ozellikleri/",
}


def nets() -> dict[str, set[tuple[str, str]]]:
    """Board nets: net -> {(ref, pin)} over parts that have a footprint."""
    out: dict[str, set[tuple[str, str]]] = {}
    for p in PARTS:
        if p.footprint:
            for pin, net in p.pins.items():
                out.setdefault(net, set()).add((p.ref, pin))
    return out


if __name__ == "__main__":
    for net, pins in sorted(nets().items()):
        print(f"{net:10} {sorted(pins)}")

# ---------------------------------------------------------------- placement (case X, Y, KiCad rotation)
# Back parts: KiCad 9 stores a flipped footprint as y-mirrored then rotated, so at rotation 0 a back
# footprint's local (x, y) lands at case (X + x, Y + y). Front = display side, back = midframe side.
PLACE = {
    # ---- locked by the case
    "U1": (-3.9, 4.8, 0),        # body X -3.9..14.1, Y -8.2..4.8: top-row vias fit under Y 7, bottom-row vias
                                 # above J1's copper, body clear of the 335 deg leg notch; pads 1-10 face -Y
    "J1": (0.0, -14.53, 180),    # face (footprint y +3.67) on Y -18.2, opening -Y
    "SW2": (-10.55, -7.0, 90),   # gen_pcb.CASE_SW2
    "SW1": (-16.22, 0.0, 90),    # plunger toward -X, pads >= 0.3 mm inside the rim
    "SW3": (8.6, -11.7, 0),      # right of J1, pads 0.25 below the bottom via row; (no room left of it); case v8 moves the reset hole here
    "H1": (-6.2, -14.8, 0),      # 0.95 mm to the rim, 0.58 mm hole-to-hole to J1's shell slot
    "H2": (6.2, -14.8, 0),
    "J2": (-9.6, 5.5, 180),      # front top edge, pad 1 (VCC) at the right end
    "J3": (-13.5, 5.5, 0),       # back top edge
    "J4": (-9.5, 5.5, 0),        # back top edge
    "J5": (8.9, -1.0, 90),       # SWD column, 0.6 mm from the antenna keep-out
    # ---- placed by a greedy placer (nearest legal spot to a target beside each part's nets), then frozen
    "U3": (-11.6, 0.0, 0),
    "C6": (-8.0, -0.7, 90),
    "D2": (3.9, -0.6, 90),
    "D1": (-9.5, -12.2, 0),
    "FID3": (-15.7, -4.1, 0),
    "U2": (-11.0, -1.4, 0),
    "C10": (-12.0, 1.2, 0),
    "R7": (-13.9, -2.4, 90),
    "R8": (-15.5, -0.1, 90),
    "TP12": (-13.8, -4.8, 0),
    "TP13": (-11.8, -4.0, 0),
    "TP7": (-15.5, -2.4, 0),
    "Y1": (-7.4, -5.6, 90),
    "C8": (-10.1, -7.0, 0),
    "C9": (-7.4, -2.8, 0),
    "C4": (-7.2, -0.5, 90),
    "C5": (-4.4, -0.3, 90),
    "Q2": (-1.0, 2.0, 90),
    "R9": (1.5, 2.6, 90),
    "TP6": (-1.4, 4.8, 0),
    "Q3": (0.9, -1.7, 0),
    "R10": (1.6, -4.2, 0),
    "C12": (3.1, 2.6, 90),
    "TP10": (3.8, 4.9, 0),
    "JP1": (3.4, -6.3, 0),
    "Q1": (-8.1, -10.0, 90),
    "U4": (-4.5, -8.2, 90),
    "C3": (-11.8, -11.3, 90),
    "R6": (-8.3, -13.5, 90),
    "TP2": (-10.2, -12.8, 0),
    "C1": (0.0, -16.0, 0),
    "R1": (1.6, -14.5, 0),       # CC1 pad inward, below the J1 fan-out vias (gen_pcb.J1_FANOUT)
    "R2": (-1.6, -14.5, 180),    # CC2 pad inward
    "TP1": (3.8, -13.4, 0),       # below J1's right shell pad, clear of the J1 fan-out
    "TP15": (-3.7, -13.3, 0),     # below J1's left shell pad, clear of the J1 fan-out
    "C2": (8.2, -11.4, 0),
    "TP3": (8.4, -13.6, 0),
    "TP4": (-15.8, -4.4, 0),
    "R3": (6.2, -8.8, 0),
    "TP5": (-4.0, -5.4, 0),
    "R11": (-0.6, -7.2, 90),
    "R12": (-2.2, -4.7, 90),
    "C13": (-1.9, -1.6, 90),
    "R4": (1.5, 4.9, 0),
    "R5": (3.1, -8.4, 0),
    "TP9": (4.5, -3.0, 0),
    "TP8": (8.5, -8.0, 0),
    "TP11": (5.8, -10.8, 0),
    "TP14": (6.6, -7.0, 0),      # was (16.9, 0.2): right of the antenna keep-out, no way in from P0.10
    "FID1": (-12.9, -6.9, 0),
    "FID2": (11.0, -11.8, 0),
}
for _p in PARTS:
    _p.pcb = PLACE.get(_p.ref)

# silkscreen names beside each wire/SWD pad, pad 1 first
PADROW_NAMES = {
    "J2": ["VCC", "GND", "SCL", "SDA", "DC", "CS", "RST"],
    "J3": ["B+", "B-"],
    "J4": ["M+", "M-"],
    "J5": ["3V3", "DIO", "CLK", "RST", "GND"],
}
# board name on the front over the antenna keep-out (silk only, no copper there)
TEXTS = [("MysteryChain", 14.5, -1.5, "F_SilkS", 90), ("rev A 2026-10", 13.2, -1.5, "F_SilkS", 90)]
