#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Flame sensor module (IR, 4-pin: VCC, GND, DO, AO)
// The AO pin outputs a voltage inversely proportional to IR intensity from
// the flame: HIGH analog value = strong flame. Raw ADC range 0..1023.
// ---------------------------------------------------------------------------

// Analog pins for the three flame sensors
const uint8_t PIN_FLAME_LEFT  = A0;
const uint8_t PIN_FLAME_FRONT = A1;
const uint8_t PIN_FLAME_RIGHT = A2;

// Calibration thresholds (0..1023). Tune with the pots on the sensor boards.
const uint16_t FLAME_SENSE_THRESHOLD = 300;  // any flame seen -> start approach
const uint16_t FLAME_FIGHT_THRESHOLD = 650;  // strong/close flame -> extinguish

namespace sensors {

// Direction enum used by the main state machine
enum FlameDir : uint8_t {
    FLAME_NONE = 0,
    FLAME_LEFT,
    FLAME_FRONT,
    FLAME_RIGHT,
    FLAME_MULTIPLE,   // several sensors see flame equally; approach slowly
};

struct FlameReading {
    uint16_t left;
    uint16_t front;
    uint16_t right;
};

inline uint16_t readRaw(uint8_t pin) {
    // analogRead already returns 0..1023 on the Uno; small oversampling
    // reduces noise from the sensor boards.
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 4; ++i) {
        sum += analogRead(pin);
    }
    return (uint16_t)(sum / 4);
}

inline FlameReading readFlames() {
    FlameReading r;
    r.left  = readRaw(PIN_FLAME_LEFT);
    r.front = readRaw(PIN_FLAME_FRONT);
    r.right = readRaw(PIN_FLAME_RIGHT);
    return r;
}

inline bool anyFlame(const FlameReading& r) {
    return (r.left  >= FLAME_SENSE_THRESHOLD ||
            r.front >= FLAME_SENSE_THRESHOLD ||
            r.right >= FLAME_SENSE_THRESHOLD);
}

inline bool flameStrong(const FlameReading& r) {
    return (r.left  >= FLAME_FIGHT_THRESHOLD ||
            r.front >= FLAME_FIGHT_THRESHOLD ||
            r.right >= FLAME_FIGHT_THRESHOLD);
}

// Decide which direction the strongest flame is coming from.
inline FlameDir flameDirection(const FlameReading& r) {
    if (!anyFlame(r)) {
        return FLAME_NONE;
    }
    const uint16_t tol = 40;  // sensors within this band count as "equal"
    uint16_t best = max(r.left, max(r.front, r.right));

    uint8_t count = 0;
    if (best - r.left  <= tol) count++;
    if (best - r.front <= tol) count++;
    if (best - r.right <= tol) count++;

    if (count >= 2) {
        return FLAME_MULTIPLE;  // wide flame or ambiguous heading
    }
    if (best == r.front) return FLAME_FRONT;
    if (best == r.left)  return FLAME_LEFT;
    return FLAME_RIGHT;
}

}  // namespace sensors

#endif  // SENSORS_H
