#include "MotorDriver.h"
#include <esp_arduino_version.h>

MotorDriver::MotorDriver()
    : _leftTarget(0),
      _rightTarget(0),
      _leftCurrent(0),
      _rightCurrent(0),
      _lastCmdTime(0),
      _lastSlewTime(0),
      _isAsleep(false) {}

void MotorDriver::begin() {
    pinMode(PIN_NSLEEP, OUTPUT);
    digitalWrite(PIN_NSLEEP, HIGH); // Wake DRV8833

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcAttach(PIN_IN1, PWM_FREQ, PWM_RES);
    ledcAttach(PIN_IN2, PWM_FREQ, PWM_RES);
    ledcAttach(PIN_IN3, PWM_FREQ, PWM_RES);
    ledcAttach(PIN_IN4, PWM_FREQ, PWM_RES);
#else
    ledcSetup(0, PWM_FREQ, PWM_RES);
    ledcSetup(1, PWM_FREQ, PWM_RES);
    ledcSetup(2, PWM_FREQ, PWM_RES);
    ledcSetup(3, PWM_FREQ, PWM_RES);
    ledcAttachPin(PIN_IN1, 0);
    ledcAttachPin(PIN_IN2, 1);
    ledcAttachPin(PIN_IN3, 2);
    ledcAttachPin(PIN_IN4, 3);
#endif

    emergencyBrake();
    _lastCmdTime = millis();
    _lastSlewTime = millis();
}

void MotorDriver::setSpeeds(int16_t left, int16_t right) {
    if (_isAsleep) {
        wake();
    }
    _leftTarget = constrain(left, -MAX_PWM, MAX_PWM);
    _rightTarget = constrain(right, -MAX_PWM, MAX_PWM);
    _lastCmdTime = millis();
}

void MotorDriver::applyPwm(uint8_t pinFwd, uint8_t pinRev, int16_t speed) {
    uint32_t duty = abs(speed);
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    if (speed > 0) {
        ledcWrite(pinFwd, duty);
        ledcWrite(pinRev, 0);
    } else if (speed < 0) {
        ledcWrite(pinFwd, 0);
        ledcWrite(pinRev, duty);
    } else {
        ledcWrite(pinFwd, 0);
        ledcWrite(pinRev, 0);
    }
#else
    // Channel mapping: IN1->0, IN2->1, IN3->2, IN4->3
    uint8_t chFwd = (pinFwd == PIN_IN1) ? 0 : 2;
    uint8_t chRev = (pinRev == PIN_IN2) ? 1 : 3;
    if (speed > 0) {
        ledcWrite(chFwd, duty);
        ledcWrite(chRev, 0);
    } else if (speed < 0) {
        ledcWrite(chFwd, 0);
        ledcWrite(chRev, duty);
    } else {
        ledcWrite(chFwd, 0);
        ledcWrite(chRev, 0);
    }
#endif
}

void MotorDriver::update() {
    uint32_t now = millis();

    // 1. Fail-Safe: Stop if connection lost for > TIMEOUT_MS
    if (now - _lastCmdTime > TIMEOUT_MS) {
        _leftTarget = 0;
        _rightTarget = 0;
    }

    // 2. Slew-Rate Limiting: Run every 10ms
    if (now - _lastSlewTime >= 10) {
        _lastSlewTime = now;

        // Left channel ramp
        if (_leftCurrent < _leftTarget) {
            _leftCurrent = min((int16_t)(_leftCurrent + SLEW_STEP), _leftTarget);
        } else if (_leftCurrent > _leftTarget) {
            _leftCurrent = max((int16_t)(_leftCurrent - SLEW_STEP), _leftTarget);
        }

        // Right channel ramp
        if (_rightCurrent < _rightTarget) {
            _rightCurrent = min((int16_t)(_rightCurrent + SLEW_STEP), _rightTarget);
        } else if (_rightCurrent > _rightTarget) {
            _rightCurrent = max((int16_t)(_rightCurrent - SLEW_STEP), _rightTarget);
        }

        // Apply updated PWM duties
        applyPwm(PIN_IN1, PIN_IN2, _leftCurrent);
        applyPwm(PIN_IN3, PIN_IN4, _rightCurrent);
    }
}

void MotorDriver::stop() {
    _leftTarget = 0;
    _rightTarget = 0;
}

void MotorDriver::emergencyBrake() {
    _leftTarget = 0;
    _rightTarget = 0;
    _leftCurrent = 0;
    _rightCurrent = 0;
    applyPwm(PIN_IN1, PIN_IN2, 0);
    applyPwm(PIN_IN3, PIN_IN4, 0);
}

void MotorDriver::sleep() {
    emergencyBrake();
    digitalWrite(PIN_NSLEEP, LOW);
    _isAsleep = true;
}

void MotorDriver::wake() {
    digitalWrite(PIN_NSLEEP, HIGH);
    _isAsleep = false;
    _lastCmdTime = millis();
}

bool MotorDriver::isMoving() const {
    return (_leftCurrent != 0 || _rightCurrent != 0 || _leftTarget != 0 || _rightTarget != 0);
}
