#pragma once

#include <Arduino.h>

class MotorDriver {
public:
    static constexpr uint8_t PIN_IN1 = 0;      // Left Motor Forward
    static constexpr uint8_t PIN_IN2 = 1;      // Left Motor Backward
    static constexpr uint8_t PIN_IN3 = 2;      // Right Motor Forward
    static constexpr uint8_t PIN_IN4 = 3;      // Right Motor Backward
    static constexpr uint8_t PIN_NSLEEP = 8;   // DRV8833 Sleep Control (Active HIGH)

    static constexpr uint32_t PWM_FREQ = 2000;  // 2kHz tối ưu mô-men xoắn (torque) cho động cơ N20
    static constexpr uint8_t PWM_RES = 8;       // 8-bit resolution (0 - 255)
    static constexpr int16_t MAX_PWM = 255;
    static constexpr int16_t SLEW_STEP = 20;    // Max PWM delta per 10ms (Brownout protection)
    static constexpr uint32_t TIMEOUT_MS = 300; // Fail-safe auto-stop timeout

    MotorDriver();

    void begin();
    void setSpeeds(int16_t left, int16_t right); // Speed: -255 to +255
    void update();                               // Call periodically in loop()
    void stop();                                 // Graceful stop
    void emergencyBrake();                       // Instant hardware brake
    void sleep();                                // Put DRV8833 into low-power sleep
    void wake();                                 // Wake DRV8833

    bool isMoving() const;
    int16_t getLeftCurrent() const { return _leftCurrent; }
    int16_t getRightCurrent() const { return _rightCurrent; }

private:
    void applyPwm(uint8_t pinFwd, uint8_t pinRev, int16_t speed);

    int16_t _leftTarget;
    int16_t _rightTarget;
    int16_t _leftCurrent;
    int16_t _rightCurrent;

    uint32_t _lastCmdTime;
    uint32_t _lastSlewTime;
    bool _isAsleep;
};
