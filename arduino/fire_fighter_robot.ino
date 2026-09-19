// ===========================================================================
// Autonomous Fire Fighter Robot - main sketch (Arduino Uno)
//
// Behavior:
//   PATROL  - sit still and monitor the three flame sensors
//   APPROACH- drive toward the strongest flame reading
//   ATTACK  - stop at the flame, pump ON, sweep nozzle until flame out
//   COOLDOWN- confirm the flame is really gone, then back to PATROL
//
// Requirements covered:
//   1. continuously monitor flame sensors       -> PATROL loop
//   2. detect flame direction                   -> sensors::flameDirection()
//   3. move toward the flame                    -> APPROACH state
//   4. stop at a safe distance                  -> strong flame => ATTACK
//   5. activate water pump                      -> pump::pumpOn()
//   6. sweep water stream with servo            -> pump::sweepStep()
//   7. stop pump when flame no longer detected  -> flame lost debounce
//   8. return to monitoring mode                -> COOLDOWN -> PATROL
//
// DISCLAIMER: educational prototype. Never leave it running unattended
// near real flames; it is not a safety product.
// ===========================================================================

#include "sensors.h"
#include "motors.h"
#include "pump_control.h"

// --- Attack tuning ----------------------------------------------------------
const uint16_t FLAME_LOST_CONFIRM_MS = 1500; // flame must be gone this long
const uint16_t COOLDOWN_MS           = 800;  // pause after extinguishing
const uint16_t SERIAL_BAUD           = 9600;

enum RobotState : uint8_t { STATE_PATROL, STATE_APPROACH, STATE_ATTACK,
                            STATE_COOLDOWN };

RobotState state = STATE_PATROL;
uint8_t  sweepAngle    = 90;  // current nozzle angle
int8_t   sweepDir      = +1;  // +1 right, -1 left
uint32_t flameLostAt   = 0;   // millis() when flame disappeared in ATTACK
bool     flameWasLost  = false;

void enterPatrol(const __FlashStringHelper* reason) {
    motors::stop();
    pump::pumpOff();
    pump::buzzerOff();
    pump::aim(SERVO_CENTER);
    sweepAngle = SERVO_CENTER;
    sweepDir   = +1;
    flameWasLost = false;
    state = STATE_PATROL;
    Serial.print(F("[-> PATROL] "));
    Serial.println(reason);
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    motors::begin();
    pump::begin();
    Serial.println(F("Fire Fighter Robot ready - monitoring."));
}

void loop() {
    // (1) Continuously monitor the flame sensors every cycle.
    sensors::FlameReading flame = sensors::readFlames();
    sensors::FlameDir dir = sensors::flameDirection(flame);

    switch (state) {

        case STATE_PATROL: {
            if (dir == sensors::FLAME_NONE) {
                break;  // keep monitoring, nothing to do
            }
            // (2) Direction known -> alert and start moving toward flame.
            pump::alarmBeeps(2);
            Serial.print(F("[FIRE] left=")); Serial.print(flame.left);
            Serial.print(F(" front="));     Serial.print(flame.front);
            Serial.print(F(" right="));     Serial.println(flame.right);
            state = STATE_APPROACH;
            break;
        }

        case STATE_APPROACH: {
            if (dir == sensors::FLAME_NONE) {
                // Flame vanished while approaching (or was a false alarm).
                enterPatrol(F("flame lost during approach"));
                break;
            }

            // (4) Stop at a safe distance: flame strong => close enough.
            if (sensors::flameStrong(flame)) {
                motors::stop();
                pump::alarmBeeps(1);
                sweepAngle = SERVO_CENTER;
                sweepDir   = +1;
                flameWasLost = false;
                pump::pumpOn();                    // (5) activate pump
                Serial.println(F("[-> ATTACK] pump ON, sweeping nozzle"));
                state = STATE_ATTACK;
                break;
            }

            // (3) Move toward the detected flame direction.
            switch (dir) {
                case sensors::FLAME_FRONT:
                    motors::forward();
                    break;
                case sensors::FLAME_LEFT:
                    motors::pivotLeft();
                    delay(TURN_STEP_MS);
                    motors::stop();
                    break;
                case sensors::FLAME_RIGHT:
                    motors::pivotRight();
                    delay(TURN_STEP_MS);
                    motors::stop();
                    break;
                case sensors::FLAME_MULTIPLE:
                default:
                    motors::forward();  // wide flame ahead; close slowly
                    delay(TURN_STEP_MS);
                    motors::stop();
                    break;
            }
            delay(60);  // let sensors settle between moves
            break;
        }

        case STATE_ATTACK: {
            // (6) Sweep the water stream across the flame base.
            sweepAngle = pump::sweepStep(sweepAngle, sweepDir);

            // Re-read the sensors while fighting.
            flame = sensors::readFlames();

            if (sensors::anyFlame(flame)) {
                flameWasLost = false;               // still burning
            } else if (!flameWasLost) {
                flameWasLost = true;                // (7) maybe out...
                flameLostAt = millis();
            } else if (millis() - flameLostAt >= FLAME_LOST_CONFIRM_MS) {
                pump::pumpOff();                    // confirmed out: stop pump
                pump::buzzerOff();
                Serial.println(F("[-> COOLDOWN] flame extinguished"));
                state = STATE_COOLDOWN;
            }
            break;
        }

        case STATE_COOLDOWN: {
            // Double-check there is no re-ignition, then (8) go back to
            // monitoring mode.
            delay(COOLDOWN_MS);
            flame = sensors::readFlames();
            if (sensors::anyFlame(flame)) {
                Serial.println(F("[RE-IGNITION] attacking again"));
                state = STATE_APPROACH;
            } else {
                enterPatrol(F("fire out - resuming monitoring"));
            }
            break;
        }
    }
}
