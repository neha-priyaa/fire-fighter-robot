#ifndef PUMP_CONTROL_H
#define PUMP_CONTROL_H

#include <Arduino.h>
#include <Servo.h>

// ---------------------------------------------------------------------------
// Water pump + aiming servo.
//
// The pump is a small 5V/6V DC diaphragm pump switched through a relay
// module (or logic-level MOSFET) on PIN_PUMP. The servo tilts the nozzle
// left-right so the water stream sweeps across the flame base.
//
// RELAY_ACTIVE_LOW: most cheap relay boards are ACTIVE-LOW (IN pin pulled
// LOW energizes the coil). Set this to true if your pump turns on when
// D4 is LOW. The OFF level is written at boot so the pump can never be
// running at power-up, whatever your board's polarity is.
// ---------------------------------------------------------------------------

const uint8_t  PIN_PUMP      = 4;    // relay/MOSFET gate pin
const uint8_t  PIN_SERVO     = 3;    // nozzle aiming servo signal
const uint8_t  PIN_BUZZER    = 2;    // active buzzer

const bool RELAY_ACTIVE_LOW = false;  // true for typical opto relay boards

const uint8_t  SERVO_CENTER  = 90;   // degrees, straight ahead
const uint8_t  SERVO_SWEEP_MIN  = 55;  // sweep left limit
const uint8_t  SERVO_SWEEP_MAX  = 125; // sweep right limit
const uint8_t  SERVO_SWEEP_STEP = 5;   // degrees per attack-loop iteration
const uint16_t SERVO_SWEEP_DELAY_MS = 90;  // ms between sweep steps

Servo nozzleServo;

namespace pump {

// Electrical level that keeps the pump OFF / turns it ON
inline uint8_t pumpOffLevel() { return RELAY_ACTIVE_LOW ? HIGH : LOW; }
inline uint8_t pumpOnLevel()  { return RELAY_ACTIVE_LOW ? LOW  : HIGH; }

inline void begin() {
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);
    // Pump first, at its OFF level - safety at boot for either polarity.
    digitalWrite(PIN_PUMP, pumpOffLevel());
    pinMode(PIN_PUMP, OUTPUT);
    nozzleServo.attach(PIN_SERVO);
    nozzleServo.write(SERVO_CENTER);
}

inline void pumpOn()  { digitalWrite(PIN_PUMP, pumpOnLevel()); }
inline void pumpOff() { digitalWrite(PIN_PUMP, pumpOffLevel()); }
inline bool pumpRunning() { return digitalRead(PIN_PUMP) == pumpOnLevel(); }

inline void aim(uint8_t angle) {
    nozzleServo.write(constrain(angle, SERVO_SWEEP_MIN, SERVO_SWEEP_MAX));
}

// One sweep step across the flame. Returns the new angle so the caller
// can ping-pong between SERVO_SWEEP_MIN and SERVO_SWEEP_MAX.
inline uint8_t sweepStep(uint8_t angle, int8_t& direction) {
    int next = angle + (SERVO_SWEEP_STEP * direction);
    if (next >= SERVO_SWEEP_MAX) { next = SERVO_SWEEP_MAX; direction = -1; }
    if (next <= SERVO_SWEEP_MIN) { next = SERVO_SWEEP_MIN; direction = +1; }
    aim((uint8_t)next);
    delay(SERVO_SWEEP_DELAY_MS);
    return (uint8_t)next;
}

inline void buzzerOn()  { digitalWrite(PIN_BUZZER, HIGH); }
inline void buzzerOff() { digitalWrite(PIN_BUZZER, LOW);  }

// Short confirmation beeps (non-blocking alternatives exist, but a short
// blocking beep at a fire event is acceptable here).
inline void alarmBeeps(uint8_t n) {
    for (uint8_t i = 0; i < n; ++i) {
        buzzerOn();  delay(120);
        buzzerOff(); delay(80);
    }
}

}  // namespace pump

#endif  // PUMP_CONTROL_H
