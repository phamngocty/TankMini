#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

enum FeedbackSound {
    SOUND_NONE = 0,
    SOUND_IDLE,
    SOUND_DRIVE,
    SOUND_FIRE,
    SOUND_ULTI,
    SOUND_HIT,
    SOUND_RELOAD,
    SOUND_RELOAD_DONE,
    SOUND_EMPTY,
    SOUND_DEATH,
    SOUND_GAME_START,
    SOUND_IGNITION_CRANK,
    SOUND_IGNITION_SUCCESS
};

class Feedback {
public:
    static constexpr uint8_t PIN_RGB = 6;
    static constexpr uint8_t PIN_BUZZER = 7;
    static constexpr uint8_t LEDC_AUDIO_CHANNEL = 4;
    static constexpr uint8_t NUM_LEDS = 1;

    Feedback();

    void begin(uint8_t teamId);
    void update();

    void playSound(FeedbackSound snd);
    void triggerHitEffect();
    void triggerFireEffect();
    void triggerUltiEffect();
    void triggerReloadDoneEffect();
    void triggerEmptyEffect();
    void triggerGameStartEffect();
    void triggerIgnitionCrank();
    void triggerIgnitionSuccess();
    void setEngineRunning(bool run);
    void setThrottle(int16_t throttle);
    void setIFrame(bool active);
    void setDead(bool dead);
    void setTeamColor(uint8_t teamId);
    void stopPwmTone();

private:
    void updateBuzzer(uint32_t now);
    void updateLed(uint32_t now);
    void playPwmTone(uint32_t freq);

    Adafruit_NeoPixel _strip;
    uint8_t _teamId;
    uint32_t _teamColor;

    // Sound state machine
    FeedbackSound _currentSound;
    uint32_t _soundStartTime;
    bool _priorityFxActive;
    bool _engineRunning;
    uint16_t _throttle;
    uint32_t _lastIdlePulse;
    uint32_t _lastDrivePulse;
    uint32_t _currentFreq;

    // Visual state machine
    bool _isHitFlashing;
    uint32_t _hitFlashStartTime;
    bool _isIFrame;
    bool _isDead;
    uint32_t _blinkTimer;
    bool _blinkState;
};
