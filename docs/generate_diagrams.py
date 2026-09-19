"""Generate block_diagram.png and circuit_diagram.png for the docs folder.

Run: python docs/generate_diagrams.py
"""
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

OUT = __file__.rsplit("/", 1)[0]


def box(ax, xy, w, h, text, fc="#eaf2fb", ec="#2b5d8c", fs=10):
    x, y = xy
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.02",
                                facecolor=fc, edgecolor=ec, linewidth=1.6))
    ax.text(x + w / 2, y + h / 2, text, ha="center", va="center",
            fontsize=fs, wrap=True)


def arrow(ax, p1, p2, label=None, fs=8):
    ax.add_patch(FancyArrowPatch(p1, p2, arrowstyle="-|>",
                                 mutation_scale=14, color="#333", lw=1.4))
    if label:
        mx, my = (p1[0] + p2[0]) / 2, (p1[1] + p2[1]) / 2
        ax.text(mx, my + 0.15, label, ha="center", fontsize=fs, color="#333")


# --------------------------------------------------------------------------
# Block diagram
# --------------------------------------------------------------------------
fig, ax = plt.subplots(figsize=(11, 6))
ax.set_xlim(0, 12)
ax.set_ylim(0, 7)
ax.axis("off")
ax.set_title("Autonomous Fire Fighter Robot — Block Diagram",
             fontsize=13, fontweight="bold")

# Inputs column
box(ax, (0.3, 5.4), 2.6, 1.0, "Flame Sensor LEFT  (A0)", fc="#fdecec", ec="#b03a2e")
box(ax, (0.3, 4.1), 2.6, 1.0, "Flame Sensor FRONT (A1)", fc="#fdecec", ec="#b03a2e")
box(ax, (0.3, 2.8), 2.6, 1.0, "Flame Sensor RIGHT (A2)", fc="#fdecec", ec="#b03a2e")

# Controller
box(ax, (4.2, 3.4), 3.2, 2.2, "Arduino Uno\n\nState machine\nPATROL → APPROACH\n→ ATTACK → COOLDOWN",
    fc="#eaf7ea", ec="#1e7a34", fs=9)

# Outputs column
box(ax, (8.6, 5.4), 3.0, 1.0, "L298N Motor Driver\n→ 2× DC Motors", fc="#eef0f5", ec="#444")
box(ax, (8.6, 4.1), 3.0, 1.0, "Relay / MOSFET\n→ Water Pump", fc="#eef0f5", ec="#444")
box(ax, (8.6, 2.8), 3.0, 1.0, "Nozzle Servo (sweep)", fc="#eef0f5", ec="#444")
box(ax, (8.6, 1.5), 3.0, 1.0, "Buzzer (alarm)", fc="#eef0f5", ec="#444")

# Power
box(ax, (4.2, 0.5), 3.2, 0.9, "Power: 2× 18650 / battery pack\nUno 5V · L298N 12V rail",
    fc="#fff8e1", ec="#b8860b", fs=9)

# Arrows
for y in (5.9, 4.6, 3.3):
    arrow(ax, (2.9, y - 0.5 + 0.5), (4.2, 4.5))
arrow(ax, (7.4, 5.0), (8.6, 5.9), "IN1–4, ENA/ENB")
arrow(ax, (7.4, 4.5), (8.6, 4.6), "D4")
arrow(ax, (7.4, 4.0), (8.6, 3.3), "D3")
arrow(ax, (7.4, 3.5), (8.6, 2.0), "D2")
arrow(ax, (5.8, 1.4), (5.8, 3.4), "", fs=8)
arrow(ax, (2.9, 1.0), (4.2, 1.0))

plt.tight_layout()
plt.savefig(f"{OUT}/block_diagram.png", dpi=150)
plt.close(fig)

# --------------------------------------------------------------------------
# Circuit / wiring diagram
# --------------------------------------------------------------------------
fig, ax = plt.subplots(figsize=(12, 7))
ax.set_xlim(0, 14)
ax.set_ylim(0, 8)
ax.axis("off")
ax.set_title("Wiring Diagram — Arduino Uno ↔ Sensors / Drivers / Actuators",
             fontsize=13, fontweight="bold")

box(ax, (5.6, 2.2), 2.8, 3.6,
    "Arduino\nUno\n\nD2  buzzer\nD3  servo\nD4  pump relay\nD5  ENA\nD6  ENB\nD7  IN1\nD8  IN2\nD12 IN3\nD13 IN4\nA0  flame L\nA1  flame F\nA2  flame R\n5V/GND power",
    fc="#eaf7ea", ec="#1e7a34", fs=8.5)

left_items = [
    (6.3, "Flame Sensor LEFT",  "AO → A0, VCC → 5V, GND → GND"),
    (4.7, "Flame Sensor FRONT", "AO → A1, VCC → 5V, GND → GND"),
    (3.1, "Flame Sensor RIGHT", "AO → A2, VCC → 5V, GND → GND"),
]
for y, title, wire in left_items:
    box(ax, (0.3, y), 3.4, 1.1, f"{title}\n{wire}", fc="#fdecec",
        ec="#b03a2e", fs=8)
    arrow(ax, (3.7, y + 0.55), (5.6, y + 0.55))

right_items = [
    (6.0, "L298N Motor Driver", "ENA←D5 IN1←D7 IN2←D8\nIN3←D12 IN4←D13 ENB←D6\nOUT1/2 → LEFT motor\nOUT3/4 → RIGHT motor"),
    (4.2, "Water Pump (via relay)", "relay IN ← D4\nrelay VCC ← 5V\npump + ← battery via relay\npump − ← GND"),
    (2.7, "Nozzle Servo (SG90)", "signal ← D3\nVCC ← 5V\nGND → GND"),
    (1.3, "Buzzer", "+ ← D2\n− → GND"),
]
for y, title, wire in right_items:
    h = 1.5 if "\n" in wire and wire.count("\n") >= 2 else 1.1
    box(ax, (9.6, y - (h - 1.1)), 4.1, h, f"{title}\n{wire}", fc="#eef0f5",
        ec="#444", fs=8)
    arrow(ax, (8.4, 4.0), (9.6, y + 0.2))

box(ax, (5.6, 0.2), 2.8, 1.0,
    "Battery pack 7.4–12 V\n→ L298N 12V IN + Uno VIN", fc="#fff8e1",
    ec="#b8860b", fs=8.5)
arrow(ax, (6.6, 1.2), (6.6, 2.2))
arrow(ax, (8.4, 0.7), (9.6, 3.9), "", fs=8)

# Common-ground note
ax.text(7.0, 7.6, "ALL grounds must be common: battery −, Uno GND, "
        "L298N GND, sensor GNDs, relay GND", ha="center", fontsize=9,
        color="#b03a2e", fontweight="bold")

plt.tight_layout()
plt.savefig(f"{OUT}/circuit_diagram.png", dpi=150)
plt.close(fig)
print("diagrams written")
