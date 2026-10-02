#include "TankGameEngine.h"
#include <ArduinoJson.h>

TankGameEngine::TankGameEngine()
    : _motors(nullptr),
      _ir(nullptr),
      _feedback(nullptr),
      _state(STATE_BATTLE),
      _teamId(1),
      _playerId(1),
      _maxHp(DEFAULT_MAX_HP),
      _currentHp(DEFAULT_MAX_HP),
      _maxAmmo(DEFAULT_MAX_AMMO),
      _currentAmmo(DEFAULT_MAX_AMMO),
      _ultiCharge(0),
      _lastFireTime(0),
      _reloadStartTime(0),
      _stunStartTime(0),
      _iFrameStartTime(0),
      _lastUltiTick(0),
      _isIFrame(false),
      _deathStartTime(0),
      _deathSpinFinished(false),
      _recoilStartTime(0),
      _isRecoiling(false),
      _engineStarted(false) {}

void TankGameEngine::begin(MotorDriver* motors, IrTransceiver* ir, Feedback* feedback, uint8_t teamId, uint8_t playerId) {
    _motors = motors;
    _ir = ir;
    _feedback = feedback;
    _teamId = teamId;
    _playerId = playerId;
    _callsign = (_teamId == 1) ? ("Tiger-" + String(_playerId)) : ("Dragon-" + String(_playerId));

    if (_ir) {
        _ir->setOnHitCallback([this](const IrPacket& pkt) {
            this->handleHit(pkt);
        });
    }

    resetGame();
}

void TankGameEngine::setProfile(const String& name, uint8_t teamId, uint8_t playerId) {
    if (name.length() > 0) _callsign = name;
    _teamId = teamId;
    _playerId = playerId;
    if (_feedback) _feedback->setTeamColor(_teamId);
    if (_ir) _ir->setIdentity(_teamId, _playerId);
}

void TankGameEngine::resetGame() {
    _state = STATE_BATTLE;
    _currentHp = _maxHp;
    _currentAmmo = _maxAmmo;
    _ultiCharge = 0;
    _isIFrame = false;
    _deathSpinFinished = false;
    _isRecoiling = false;
    _engineStarted = false; // Sau khi hồi sinh: Động cơ AUTO TẮT, phải nhấn giữ để khởi động lại
    _lastUltiTick = millis();

    if (_ir) _ir->setImmune(false);
    if (_feedback) {
        _feedback->setIFrame(false);
        _feedback->setDead(false);
        _feedback->setTeamColor(_teamId);
        _feedback->setThrottle(0);
        _feedback->setEngineRunning(false);
        _feedback->playSound(SOUND_NONE);
        _feedback->stopPwmTone();
    }
    if (_motors) _motors->stop();
}

void TankGameEngine::setEngineStarted(bool started) {
    if (_state == STATE_DEAD) return;
    _engineStarted = started;
    if (_feedback) {
        _feedback->setEngineRunning(started);
        if (started) {
            _feedback->triggerIgnitionSuccess();
        } else {
            _feedback->playSound(SOUND_NONE);
            _feedback->stopPwmTone();
        }
    }
    if (!started && _motors) {
        _motors->stop();
    }
}

void TankGameEngine::triggerIgnitionCrank() {
    if (_state == STATE_DEAD || _engineStarted) return;
    if (_feedback) _feedback->triggerIgnitionCrank();
}

void TankGameEngine::handleDrive(int16_t left, int16_t right) {
    if (!_engineStarted || _state == STATE_DEAD || _state == STATE_HIT_STUN || _isRecoiling) {
        if (_motors) _motors->stop();
        if (_feedback) _feedback->setThrottle(0);
        return;
    }
    if (_motors) {
        _motors->setSpeeds(left, right);
    }
    if (_feedback) {
        int16_t maxSpeed = max((int16_t)abs(left), (int16_t)abs(right));
        _feedback->setThrottle(maxSpeed);
    }
}

bool TankGameEngine::handleFire() {
    if (!_engineStarted || _state == STATE_DEAD || _state == STATE_HIT_STUN || _state == STATE_RELOADING) {
        return false;
    }

    uint32_t now = millis();
    if (now - _lastFireTime < FIRE_COOLDOWN_MS) {
        return false;
    }

    if (_currentAmmo <= 0) {
        if (_feedback) _feedback->triggerEmptyEffect();
        return false;
    }

    _lastFireTime = now;
    _currentAmmo--;

    // Build Ulti charge on firing
    if (_ultiCharge < 100) {
        _ultiCharge = min((int)_ultiCharge + 10, 100);
    }

    if (_ir) _ir->fire(1); // Standard shot deals 1 damage
    if (_feedback) _feedback->triggerFireEffect();

    // Recoil kickback: brief reverse pulse
    if (_motors) {
        _motors->setSpeeds(-200, -200);
        _isRecoiling = true;
        _recoilStartTime = now;
    }

    if (_currentAmmo == 0) {
        _state = STATE_RELOADING;
        _reloadStartTime = now;
        if (_feedback) _feedback->playSound(SOUND_RELOAD);
    }

    return true;
}

bool TankGameEngine::handleManualReload() {
    if (!_engineStarted || _state != STATE_BATTLE || _currentAmmo >= _maxAmmo) {
        return false;
    }
    _state = STATE_RELOADING;
    _reloadStartTime = millis();
    if (_feedback) _feedback->playSound(SOUND_RELOAD);
    return true;
}

bool TankGameEngine::handleUlti() {
    if (!_engineStarted || _state != STATE_BATTLE || _ultiCharge < 100) {
        return false;
    }

    uint32_t now = millis();
    _ultiCharge = 0; // Consume charge
    _lastFireTime = now;

    // Heavy artillery blast: deals 3 damage!
    if (_ir) _ir->fire(3);
    if (_feedback) _feedback->triggerUltiEffect();

    // Heavy recoil
    if (_motors) {
        _motors->setSpeeds(-255, -255);
        _isRecoiling = true;
        _recoilStartTime = now;
    }

    return true;
}

void TankGameEngine::handleHit(const IrPacket& pkt) {
    if (_state == STATE_DEAD || _isIFrame) {
        return;
    }

    uint32_t now = millis();

    uint8_t dmg = (pkt.damage > 0) ? pkt.damage : 1;
    if (_currentHp > dmg) {
        _currentHp -= dmg;
    } else {
        _currentHp = 0;
    }

    // Build ulti charge on taking damage
    if (_ultiCharge < 100) {
        _ultiCharge = min((int)_ultiCharge + 20, 100);
    }

    if (_feedback) _feedback->triggerHitEffect();

    if (_currentHp == 0) {
        _state = STATE_DEAD;
        _engineStarted = false; // Tắt động cơ khi bị tiêu diệt
        _deathStartTime = now;
        _deathSpinFinished = false;
        if (_feedback) _feedback->setDead(true);
        if (_motors) _motors->setSpeeds(255, -255);
    } else {
        _state = STATE_HIT_STUN;
        _stunStartTime = now;
        _isIFrame = true;
        _iFrameStartTime = now;

        if (_ir) _ir->setImmune(true);
        if (_feedback) _feedback->setIFrame(true);
        if (_motors) _motors->emergencyBrake();
    }
}

void TankGameEngine::update() {
    uint32_t now = millis();

    // Passive ulti charge gain: +2% every 2 seconds in battle
    if (_state == STATE_BATTLE && (now - _lastUltiTick >= 2000)) {
        _lastUltiTick = now;
        if (_ultiCharge < 100) {
            _ultiCharge = min((int)_ultiCharge + 2, 100);
        }
    }

    // 1. Recoil recovery (50ms)
    if (_isRecoiling && (now - _recoilStartTime >= 50)) {
        _isRecoiling = false;
        if (_motors) _motors->stop();
    }

    // 2. Hit-Stun expiry (400ms)
    if (_state == STATE_HIT_STUN && (now - _stunStartTime >= HIT_STUN_TIME_MS)) {
        _state = (_currentAmmo == 0) ? STATE_RELOADING : STATE_BATTLE;
    }

    // 3. I-Frame expiry (1500ms)
    if (_isIFrame && (now - _iFrameStartTime >= IFRAME_TIME_MS)) {
        _isIFrame = false;
        if (_ir) _ir->setImmune(false);
        if (_feedback) _feedback->setIFrame(false);
    }

    // 4. Reloading completion (3500ms)
    if (_state == STATE_RELOADING && (now - _reloadStartTime >= RELOAD_TIME_MS)) {
        _currentAmmo = _maxAmmo;
        _state = STATE_BATTLE;
        if (_feedback) _feedback->triggerReloadDoneEffect();
    }

    // 5. Death spin completion (1200ms)
    if (_state == STATE_DEAD) {
        if (!_deathSpinFinished && (now - _deathStartTime >= DEATH_SPIN_MS)) {
            _deathSpinFinished = true;
            if (_motors) _motors->sleep();
        }
    }
}

String TankGameEngine::getStatusJson() const {
    StaticJsonDocument<384> doc;
    doc["type"] = "hud";
    doc["hp"] = _currentHp;
    doc["maxHp"] = _maxHp;
    doc["ammo"] = _currentAmmo;
    doc["maxAmmo"] = _maxAmmo;
    doc["ulti"] = _ultiCharge;
    doc["isIFrame"] = _isIFrame;
    doc["isDead"] = (_state == STATE_DEAD);
    doc["engine"] = _engineStarted;
    doc["team"] = _teamId;
    doc["playerId"] = _playerId;
    doc["callsign"] = _callsign;

    const char* stateNames[] = {"LOBBY", "BATTLE", "STUN", "RELOAD", "DEAD"};
    doc["state"] = stateNames[_state];

    String output;
    serializeJson(doc, output);
    return output;
}
