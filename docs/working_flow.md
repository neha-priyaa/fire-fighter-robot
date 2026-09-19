# Working Flow — Autonomous Fire Fighter Robot

## 1. Algorithm explanation

The firmware is a four-state machine running in `loop()`:

### State 1 — PATROL (monitoring mode)
- All three flame sensors (LEFT / FRONT / RIGHT) are read continuously.
  Each analog reading is oversampled 4× to reject noise.
- A reading ≥ `FLAME_SENSE_THRESHOLD` (default 300/1023) counts as "flame
  seen". The IR flame sensor output is **inversely proportional to
  distance**: closer/stronger flame → higher ADC value.
- If no flame is seen, the robot stays still and keeps monitoring.

### State 2 — APPROACH (move toward the flame)
- `flameDirection()` picks the sensor with the highest reading. If two or
  more sensors are within a 40-ADC tolerance of the maximum, the flame is
  treated as wide/ambiguous and the robot drives forward slowly.
- LEFT → pivot left in place; RIGHT → pivot right in place; FRONT →
  drive forward. Each correction step is short (`TURN_STEP_MS`) and the
  sensors are re-read between steps, so the robot homes in on the flame.
- **Safe-distance stop:** when the strongest reading reaches
  `FLAME_FIGHT_THRESHOLD` (default 650/1023), the flame is close enough
  to extinguish effectively but far enough to keep the chassis safe. The
  motors stop immediately.

### State 3 — ATTACK (extinguish)
- The water pump is switched ON through the relay.
- The nozzle servo sweeps the water jet between 55° and 125°, stepping
  5° every 90 ms and ping-ponging at the limits, so the stream covers the
  base of the flame (where extinguishing actually works).
- Sensors keep being read during the sweep. The moment no sensor sees a
  flame, a **1.5 s confirmation timer** starts — if any sensor re-triggers
  the timer resets. Only after the flame stays absent for the full window
  does the pump switch OFF (requirement 7: "stop when the flame is no
  longer detected", with debounce so flicker doesn't cut the pump early).

### State 4 — COOLDOWN (verify + return)
- The robot waits 0.8 s and re-reads all sensors.
- If a flame reappears (re-ignition), it goes straight back to APPROACH.
- Otherwise the pump stays off, the servo re-centers and the robot
  returns to PATROL (requirement 8).

### Fail-safety notes
- The pump pin is initialized LOW at boot — the robot never sprays on
  power-up.
- If the flame disappears during APPROACH, everything stops and the robot
  returns to PATROL (false-alarm recovery).
- The buzzer beeps at detection and at attack start as an audible alert.

## 2. Flowchart

```
                ┌──────────────┐
                │   POWER ON   │
                │  setup():    │
                │  pump = OFF  │
                │  servo = 90° │
                └──────┬───────┘
                       ▼
        ┌─────────────────────────────┐
   ┌───►│         PATROL              │
   │    │ read L / F / R flame sensors│
   │    └──────────┬──────────────────┘
   │               ▼
   │        ┌─────────────┐   NO (keep monitoring)
   │        │ flame seen? ├──────────────────► (stay in PATROL)
   │        └──────┬──────┘
   │               | YES: buzzer alert
   │               ▼
   │    ┌─────────────────────────────┐
   │    │        APPROACH             │
   │    │ strongest sensor = direction│
   │    └──────────┬──────────────────┘
   │               ▼
   │        ┌─────────────────┐  YES: STOP MOTORS
   │        │ flame strong?   ├───────────────┐
   │        └──────┬──────────┘               │
   │               | NO                        ▼
   │     ┌─────────┴─────────┐      ┌─────────────────────┐
   │     │ LEFT  → pivot L   │      │       ATTACK        │
   │     │ RIGHT → pivot R   │      │ pump ON             │
   │     │ FRONT → forward   │      │ servo sweeps 55-125°│
   │     └─────────┬─────────┘      │ re-read sensors     │
   │               │                └──────────┬──────────┘
   │               ▼                           ▼
   │        (loop APPROACH)            ┌─────────────────┐
   │                                   │ flame gone for  │
   │                                   │ ≥ 1.5 s?        │
   │                                   └───┬─────────┬───┘
   │                                  NO   │         │ YES
   │                            (keep      ▼         ▼
   │                             sweeping) pump OFF  ┌──────────────┐
   │                                               │   COOLDOWN   │
   │                                               │ wait 0.8 s   │
   │                                               │ re-check     │
   │                                               └───┬──────┬───┘
   │                                     re-ignition   │      │ clear
   │                                     ┌─────────────┘      ▼
   └─────────────────────────────────────┘        return to PATROL
```

## 3. Requirements traceability

| # | Requirement | Where implemented |
|---|-------------|-------------------|
| 1 | Continuously monitor flame sensors | `PATROL` + `sensors::readFlames()` every `loop()` |
| 2 | Detect flame direction | `sensors::flameDirection()` |
| 3 | Move toward the flame | `STATE_APPROACH` pivot/forward logic |
| 4 | Stop at a safe distance | `sensors::flameStrong()` → motors stop before ATTACK |
| 5 | Activate the water pump | `pump::pumpOn()` entering ATTACK |
| 6 | Servo sweeps the water stream | `pump::sweepStep()` in ATTACK |
| 7 | Stop pump when flame is gone | 1.5 s flame-lost debounce in ATTACK |
| 8 | Return to monitoring mode | `STATE_COOLDOWN` → `enterPatrol()` |
