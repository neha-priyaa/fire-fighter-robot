# Autonomous Fire Fighter Robot

An embedded-systems prototype built on **Arduino Uno** that detects a
flame, drives toward it, stops at a safe distance, and extinguishes it
with a servo-swept water jet — then returns to monitoring mode.

> ⚠️ **DISCLAIMER:** Educational prototype. This is **not** a certified
> safety product. Never leave it operating unattended near real fires.

## Table of contents

1. [Hardware components](#1-hardware-components)
2. [Pin configuration](#2-pin-configuration)
3. [Circuit connections](#3-circuit-connections)
4. [How it works (algorithm)](#4-how-it-works-algorithm)
5. [Project structure](#5-project-structure)
6. [Build & upload](#6-build--upload)
7. [Tuning](#7-tuning)
8. [Troubleshooting](#8-troubleshooting)

---

## 1. Hardware components

| Component | Qty | Purpose |
|---|---|---|
| Arduino Uno R3 | 1 | Main controller |
| IR flame sensor module (4-pin: VCC/GND/DO/AO) | 3 | Flame detection left/front/right |
| L298N dual H-bridge motor driver | 1 | Drives the two DC motors |
| DC gear motors + wheels | 2 | Locomotion (differential drive) |
| Water pump (5–6 V DC diaphragm) | 1 | Sprays water |
| Relay module (or logic-level MOSFET) | 1 | Switches the pump from D4 |
| SG90 servo motor | 1 | Sweeps the nozzle left/right |
| Active buzzer | 1 | Audible alarm on flame detection |
| Battery pack (2× 18650, 7.4 V) | 1 | Motor + Uno power |
| Chassis, caster wheel, water bottle mount, wires | — | Assembly |

## 2. Pin configuration

| Uno pin | Connected to | Notes |
|---|---|---|
| A0 | Flame sensor LEFT — AO | Analog 0–1023, higher = stronger flame |
| A1 | Flame sensor FRONT — AO | |
| A2 | Flame sensor RIGHT — AO | |
| D2 | Buzzer (+) | Active buzzer, other leg to GND |
| D3 | Servo signal (orange) | Servo library on Timer1 |
| D4 | Relay module IN | HIGH = pump ON |
| D5 | L298N ENA | PWM, left motor speed |
| D6 | L298N ENB | PWM, right motor speed |
| D7 | L298N IN1 | Left motor direction |
| D8 | L298N IN2 | Left motor direction |
| D12 | L298N IN3 | Right motor direction |
| D13 | L298N IN4 | Right motor direction |
| 5V | Sensor VCC ×3, servo VCC, relay VCC | From Uno regulator |
| GND | Common ground | Battery −, Uno, L298N, all modules |

> **Why no PWM on pins 9/10?** The Arduino `Servo` library uses Timer1,
> which disables `analogWrite()` on pins 9 and 10. All PWM pins in this
> project (5, 6) are on Timer0 and unaffected.

## 3. Circuit connections

See `docs/circuit_diagram.png` (wiring) and `docs/block_diagram.png`
(system overview).

**Power**

- Battery pack (7.4–12 V) → L298N `12V` terminal and L298N GND.
- L298N `5V` regulator jumper **on**: its onboard 5V can feed the Uno
  `5V` pin — or power the Uno from its own USB/battery via VIN.
- The water pump connects through the relay's COM/NO contacts to the
  battery, **not** to the Uno 5V rail (pump inrush current would brown
  out the MCU).
- **All grounds must be common** — battery, Uno, L298N, sensors, relay.

**Flame sensors** — set the module potentiometer so the DO LED turns on
near a lighter from ~50 cm; we use AO for graded intensity readings.

**Servo** — SG90 works off Uno 5V for bench testing; on the robot, power
it from a separate 5 V BEC if you see twitching when the pump runs.

## 4. How it works (algorithm)

Full explanation + flowchart: [`docs/working_flow.md`](docs/working_flow.md).

```
PATROL: monitor L/F/R flame sensors
   └─ flame seen? → buzzer, APPROACH
APPROACH: pivot/steer toward strongest sensor
   └─ flame strong (close)? → stop motors (safe distance), ATTACK
ATTACK: pump ON + servo sweeps 55°–125° over the flame
   └─ flame gone ≥ 1.5 s → pump OFF, COOLDOWN
COOLDOWN: re-check after 0.8 s
   ├─ re-ignition → APPROACH
   └─ clear → PATROL (monitoring mode)
```

## 5. Project structure

```
fire-fighter-robot/
├── arduino/
│   ├── fire_fighter_robot.ino   # main sketch: state machine
│   ├── sensors.h                # flame sensor reading + direction logic
│   ├── motors.h                 # L298N motor control
│   └── pump_control.h           # pump relay + nozzle servo + buzzer
├── docs/
│   ├── circuit_diagram.png      # wiring diagram
│   ├── block_diagram.png        # system block diagram
│   ├── working_flow.md          # algorithm + flowchart + traceability
│   └── generate_diagrams.py     # regenerates the PNGs
├── README.md
└── LICENSE
```

## 6. Build & upload

1. Open `arduino/fire_fighter_robot.ino` in the Arduino IDE (1.8+ or 2.x).
2. Select board **Tools → Board → Arduino Uno** and the correct serial port.
3. **Upload.**
4. Open Serial Monitor at **9600 baud** to watch state transitions.

Command-line alternative:

```bash
arduino-cli compile --fqbn arduino:uno:arduino arduino/
arduino-cli upload  --fqbn arduino:uno:arduino -p /dev/cu.usbserial* arduino/
```

The four files sit in one folder, so the IDE picks up the headers
automatically — no library installs needed beyond the built-in `Servo`.

## 7. Tuning

| Constant (file) | Default | Adjust when |
|---|---|---|
| `FLAME_SENSE_THRESHOLD` (sensors.h) | 300 | Robot ignores fires → lower; triggers on lamps/sunlight → raise |
| `FLAME_FIGHT_THRESHOLD` (sensors.h) | 650 | Attacks too far away → raise; gets too close → lower |
| `DRIVE_SPEED` / `TURN_SPEED` (motors.h) | 180/160 | Slow, jerky, or battery-dependent motion |
| `TURN_STEP_MS` (motors.h) | 140 | Overshoots the flame direction → shorten |
| `SERVO_SWEEP_MIN/MAX` (pump_control.h) | 55/125 | Match your nozzle geometry |
| `FLAME_LOST_CONFIRM_MS` (ino) | 1500 | Pump cuts out early → raise |

Calibrate thresholds with the Serial Monitor: `readFlames()` values are
printed on every fire event (≈0 ambient → 1023 at close range).

## 8. Troubleshooting

| Symptom | Likely cause / fix |
|---|---|
| Sensors read ~1023 constantly | Sensor pot misadjusted, or direct sunlight/incandescent light — recalibrate pot, shield sensor, raise `FLAME_SENSE_THRESHOLD` |
| Robot never detects flame | AO wire on the wrong pin; test each sensor with a lighter and watch the values in Serial Monitor |
| Motors don't move | L298N needs ≥ 7 V on its 12 V input; check ENA/ENB jumpers are removed (we drive them with PWM); battery too weak |
| One motor runs backwards | Swap that motor's two wires at the L298N OUT terminals |
| Robot turns the wrong way | LEFT/RIGHT sensors are swapped — swap A0/A2 wiring or the pin constants |
| Servo twitches when pump runs | Pump noise/brown-out — power the servo separately, add a 470 µF cap across servo supply |
| Pump never starts | Relay logic: some relay boards are **active-LOW** — invert `pumpOn()`/`pumpOff()`; also check the relay clicks when D4 toggles |
| Pump runs at power-up | Should be impossible (D4 initialized LOW); verify wiring isn't forcing the relay closed |
| Arduino resets when motors start | Motor EMF brown-out — common ground missing or Uno powered from the L298N 5 V while motors stall; use separate supply paths |
| Robot oscillates around flame | `TURN_STEP_MS` too long or sensors mounted too close together — shorten the step or spread the sensors |
| Attack starts too close / singes things | Lower `FLAME_FIGHT_THRESHOLD` so it stops farther away |

## Safety notes

- Test with a **candle**, in a tray, on a non-flammable surface.
- Only fill the pump reservoir with clean water; never run the pump dry.
- Kill-switch: lifting the robot or removing power is the emergency stop.
