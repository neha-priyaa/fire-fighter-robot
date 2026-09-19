#ifndef MOTORS_H
#define MOTORS_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// L298N dual H-bridge driving two DC gear motors (left + right).
// ENA/ENB are PWM pins so speed can be controlled with analogWrite().
//
//   L298N pin   Uno pin
//   ---------   -------
//   ENA         5  (PWM)
//   IN1         7
//   IN2         8
//   IN3         12
//   IN4         13
//   ENB         6  (PWM)
//
// NOTE: the Servo library uses Timer1, which disables analogWrite() (PWM)
// on pins 9 and 10. All PWM pins chosen here (5, 6) are Timer0 and safe.
// ---------------------------------------------------------------------------

const uint8_t PIN_ENA = 5;   // left motor speed (PWM)
const uint8_t PIN_IN1 = 7;   // left motor direction
const uint8_t PIN_IN2 = 8;
const uint8_t PIN_IN3 = 12;  // right motor direction
const uint8_t PIN_IN4 = 13;
const uint8_t PIN_ENB = 6;   // right motor speed (PWM)

const uint8_t DRIVE_SPEED = 180;  // 0..255 cruise speed
const uint8_t TURN_SPEED  = 160;  // 0..255 pivot speed
const uint16_t TURN_STEP_MS = 140;  // ms per pivot step while scanning

namespace motors {

// forward declarations (so begin() can call stop())
inline void stop();

inline void begin() {
    pinMode(PIN_ENA, OUTPUT);
    pinMode(PIN_IN1, OUTPUT);
    pinMode(PIN_IN2, OUTPUT);
    pinMode(PIN_IN3, OUTPUT);
    pinMode(PIN_IN4, OUTPUT);
    pinMode(PIN_ENB, OUTPUT);
    stop();
}

inline void leftWheel(int16_t speed) {  // -255..255, sign = direction
    if (speed >= 0) {
        digitalWrite(PIN_IN1, HIGH);
        digitalWrite(PIN_IN2, LOW);
    } else {
        digitalWrite(PIN_IN1, LOW);
        digitalWrite(PIN_IN2, HIGH);
        speed = -speed;
    }
    analogWrite(PIN_ENA, (uint8_t)constrain(speed, 0, 255));
}

inline void rightWheel(int16_t speed) {
    if (speed >= 0) {
        digitalWrite(PIN_IN3, HIGH);
        digitalWrite(PIN_IN4, LOW);
    } else {
        digitalWrite(PIN_IN3, LOW);
        digitalWrite(PIN_IN4, HIGH);
        speed = -speed;
    }
    analogWrite(PIN_ENB, (uint8_t)constrain(speed, 0, 255));
}

inline void forward() {
    leftWheel(DRIVE_SPEED);
    rightWheel(DRIVE_SPEED);
}

inline void backward() {
    leftWheel(-DRIVE_SPEED);
    rightWheel(-DRIVE_SPEED);
}

inline void pivotLeft() {   // rotate counter-clockwise in place
    leftWheel(-TURN_SPEED);
    rightWheel(TURN_SPEED);
}

inline void pivotRight() {  // rotate clockwise in place
    leftWheel(TURN_SPEED);
    rightWheel(-TURN_SPEED);
}

inline void stop() {
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, LOW);
    digitalWrite(PIN_IN3, LOW);
    digitalWrite(PIN_IN4, LOW);
    analogWrite(PIN_ENA, 0);
    analogWrite(PIN_ENB, 0);
}

}  // namespace motors

#endif  // MOTORS_H
