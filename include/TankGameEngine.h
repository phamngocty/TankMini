#pragma once

#include <Arduino.h>
#include "MotorDriver.h"
#include "IrTransceiver.h"
#include "Feedback.h"

enum TankState {
    STATE_LOBBY = 0,
    STATE_BATTLE,
    STATE_HIT_STUN,
    STATE_RELOADING,
    STATE_DEAD
};

class TankGameEngine {
public:
    static constexpr uint8_t DEFAULT_MAX_HP = 5;
    static constexpr uint8_t DEFAULT_MAX_AMMO = 10;
    static constexpr uint32_t FIRE_COOLDOWN_MS = 800;
    static constexpr uint32_t RELOAD_TIME_MS = 3500;
    static constexpr uint32_t HIT_STUN_TIME_MS = 400;
    static constexpr uint32_t IFRAME_TIME_MS = 1500;
    static constexpr uint32_t DEATH_SPIN_MS = 1200;

    TankGameEngine();

    void begin(MotorDriver* motors, IrTransceiver* ir, Feedback* feedback, uint8_t teamId, uint8_t playerId);
    void update();

    void handleDrive(int16_t left, int16_t right);
    bool handleFire();
    bool handleManualReload();
    bool handleUlti();
    void handleHit(const IrPacket& pkt);
    void resetGame();

    TankState getState() const { return _state; }
    uint8_t getHp() const { return _currentHp; }
    uint8_t getMaxHp() const { return _maxHp; }
    uint8_t getAmmo() const { return _currentAmmo; }
    uint8_t getMaxAmmo() const { return _maxAmmo; }
    uint8_t getUltiCharge() const { return _ultiCharge; }
    bool isIFrame() const { return _isIFrame; }
    bool isDead() const { return _state == STATE_DEAD; }
    bool isEngineStarted() const { return _engineStarted; }
    void setEngineStarted(bool started);
    void triggerIgnitionCrank();

    uint8_t getTeamId() const { return _teamId; }
    uint8_t getPlayerId() const { return _playerId; }
    String getCallsign() const { return _callsign; }
    void setProfile(const String& name, uint8_t teamId, uint8_t playerId);

    // Status serialization helper for Web/WebSocket HUD
    String getStatusJson() const;

private:
    MotorDriver* _motors;
    IrTransceiver* _ir;
    Feedback* _feedback;

    TankState _state;
    bool _engineStarted;
    uint8_t _teamId;
    uint8_t _playerId;
    String _callsign;

    uint8_t _maxHp;
    uint8_t _currentHp;
    uint8_t _maxAmmo;
    uint8_t _currentAmmo;
    uint8_t _ultiCharge; // 0 - 100%

    uint32_t _lastFireTime;
    uint32_t _reloadStartTime;
    uint32_t _stunStartTime;
    uint32_t _iFrameStartTime;
    uint32_t _lastUltiTick;
    bool _isIFrame;

    // Death sequence
    uint32_t _deathStartTime;
    bool _deathSpinFinished;

    // Recoil
    uint32_t _recoilStartTime;
    bool _isRecoiling;
};
