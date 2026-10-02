#include "Feedback.h"
#include <esp_arduino_version.h>

Feedback::Feedback()
    : _strip(NUM_LEDS, PIN_RGB, NEO_GRB + NEO_KHZ800),
      _teamId(1),
      _teamColor(0),
      _currentSound(SOUND_IDLE),
      _soundStartTime(0),
      _priorityFxActive(false),
      _engineRunning(true),
      _throttle(0),
      _lastIdlePulse(0),
      _lastDrivePulse(0),
      _currentFreq(0),
      _isHitFlashing(false),
      _hitFlashStartTime(0),
      _isIFrame(false),
      _isDead(false),
      _blinkTimer(0),
      _blinkState(false) {}

void Feedback::begin(uint8_t teamId)
{
    _strip.begin();
    _strip.setBrightness(120);
    setTeamColor(teamId);

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcAttach(PIN_BUZZER, 1000, 8);
    ledcWrite(PIN_BUZZER, 0);
#else
    ledcSetup(LEDC_AUDIO_CHANNEL, 1000, 8);
    ledcAttachPin(PIN_BUZZER, LEDC_AUDIO_CHANNEL);
    ledcWrite(LEDC_AUDIO_CHANNEL, 0);
#endif

    _currentFreq = 0;
    _engineRunning = true;
    _priorityFxActive = false;
    _currentSound = SOUND_IDLE;
    _lastIdlePulse = millis();
    _lastDrivePulse = millis();
}

void Feedback::setTeamColor(uint8_t teamId)
{
    _teamId = teamId;
    if (_teamId == 1)
    {
        _teamColor = _strip.Color(0, 50, 255); // Blue team
    }
    else
    {
        _teamColor = _strip.Color(255, 20, 0); // Red team
    }
    if (!_isHitFlashing && !_isIFrame && !_isDead)
    {
        _strip.setPixelColor(0, _teamColor);
        _strip.show();
    }
}

void Feedback::playPwmTone(uint32_t freq)
{
    if (freq == 0)
    {
        stopPwmTone();
        return;
    }
    // Prevent redundant hardware LEDC register reconfiguration which causes glitch clicks
    if (freq == _currentFreq)
    {
        return;
    }

    // Passive buzzer optimal audible range clamp (100Hz - 5000Hz)
    if (freq < 100)
        freq = 100;
    if (freq > 5000)
        freq = 5000;

    _currentFreq = freq;

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcChangeFrequency(PIN_BUZZER, freq, 8);
    ledcWrite(PIN_BUZZER, 128);
#else
    ledcSetup(LEDC_AUDIO_CHANNEL, freq, 8);
    ledcWrite(LEDC_AUDIO_CHANNEL, 128);
#endif
}

void Feedback::stopPwmTone()
{
    if (_currentFreq == 0)
        return;
    _currentFreq = 0;

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcWrite(PIN_BUZZER, 0);
#else
    ledcWrite(LEDC_AUDIO_CHANNEL, 0);
#endif
}

void Feedback::playSound(FeedbackSound snd)
{
    if (snd == SOUND_NONE)
    {
        _priorityFxActive = false;
        _currentSound = _engineRunning ? SOUND_IDLE : SOUND_NONE;
        stopPwmTone();
        return;
    }
    _priorityFxActive = true;
    _currentSound = snd;
    _soundStartTime = millis();
}

void Feedback::triggerFireEffect()
{
    playSound(SOUND_FIRE);
}

void Feedback::triggerUltiEffect()
{
    playSound(SOUND_ULTI);
}

void Feedback::triggerHitEffect()
{
    _isHitFlashing = true;
    _hitFlashStartTime = millis();
    playSound(SOUND_HIT);
}

void Feedback::triggerReloadDoneEffect()
{
    playSound(SOUND_RELOAD_DONE);
}

void Feedback::triggerEmptyEffect()
{
    playSound(SOUND_EMPTY);
}

void Feedback::triggerGameStartEffect()
{
    playSound(SOUND_GAME_START);
}

void Feedback::triggerIgnitionCrank()
{
    playSound(SOUND_IGNITION_CRANK);
}

void Feedback::triggerIgnitionSuccess()
{
    playSound(SOUND_IGNITION_SUCCESS);
}

void Feedback::setEngineRunning(bool run)
{
    _engineRunning = run;
    if (!_engineRunning && !_priorityFxActive)
    {
        stopPwmTone();
        _currentSound = SOUND_NONE;
    }
    else if (_engineRunning && !_priorityFxActive)
    {
        _currentSound = SOUND_IDLE;
    }
}

void Feedback::setThrottle(int16_t throttle)
{
    _throttle = constrain(abs(throttle), 0, 255);
}

void Feedback::setIFrame(bool active)
{
    _isIFrame = active;
    if (!_isIFrame && !_isDead)
    {
        _strip.setPixelColor(0, _teamColor);
        _strip.show();
    }
}

void Feedback::setDead(bool dead)
{
    _isDead = dead;
    if (_isDead)
    {
        playSound(SOUND_DEATH);
    }
    else
    {
        _strip.setPixelColor(0, _teamColor);
        _strip.show();
        setEngineRunning(true);
    }
}

void Feedback::updateBuzzer(uint32_t now)
{
    // 1. PRIORITY SOUND EFFECTS
    if (_priorityFxActive)
    {
        uint32_t elapsed = now - _soundStartTime;

        switch (_currentSound)
        {
        case SOUND_FIRE: // Bắn pháo đanh gọn: 2200Hz -> 1400Hz -> 700Hz trong 110ms
            if (elapsed < 30)
            {
                playPwmTone(2200);
            }
            else if (elapsed < 70)
            {
                playPwmTone(1400);
            }
            else if (elapsed < 110)
            {
                playPwmTone(700);
            }
            else
            {
                stopPwmTone();
                _priorityFxActive = false;
                _currentSound = _engineRunning ? SOUND_IDLE : SOUND_NONE;
            }
            break;

        case SOUND_ULTI: // Đại bác Ulti nổ kép uy lực: 2600->1500->800, ngắt nén, 2200->1200->500 trong 320ms
            if (elapsed < 40)
            {
                playPwmTone(2600);
            }
            else if (elapsed < 80)
            {
                playPwmTone(1500);
            }
            else if (elapsed < 110)
            {
                playPwmTone(800);
            }
            else if (elapsed < 140)
            {
                stopPwmTone(); // Khoảng lặng nén kịch tính
            }
            else if (elapsed < 180)
            {
                playPwmTone(2200);
            }
            else if (elapsed < 240)
            {
                playPwmTone(1200);
            }
            else if (elapsed < 320)
            {
                playPwmTone(500);
            }
            else
            {
                stopPwmTone();
                _priorityFxActive = false;
                _currentSound = _engineRunning ? SOUND_IDLE : SOUND_NONE;
            }
            break;

        case SOUND_HIT: // Trúng đạn báo động rung giật chát chúa: 2400Hz -> ngắt -> 2000Hz -> ngắt -> 1600Hz
            if (elapsed < 40)
            {
                playPwmTone(2400);
            }
            else if (elapsed < 70)
            {
                stopPwmTone();
            }
            else if (elapsed < 120)
            {
                playPwmTone(2000);
            }
            else if (elapsed < 150)
            {
                stopPwmTone();
            }
            else if (elapsed < 200)
            {
                playPwmTone(1600);
            }
            else
            {
                stopPwmTone();
                _priorityFxActive = false;
                _currentSound = _engineRunning ? SOUND_IDLE : SOUND_NONE;
            }
            break;

        case SOUND_EMPTY: // Hết đạn: 2 tiếng kim hỏa đập đanh thép "cạch... cạch" (3000Hz)
            if (elapsed < 15)
            {
                playPwmTone(3000);
            }
            else if (elapsed < 45)
            {
                stopPwmTone();
            }
            else if (elapsed < 60)
            {
                playPwmTone(2800);
            }
            else
            {
                stopPwmTone();
                _priorityFxActive = false;
                _currentSound = _engineRunning ? SOUND_IDLE : SOUND_NONE;
            }
            break;

        case SOUND_RELOAD: // Cơ cấu xích tiếp đạn: 3 nhịp cơ khí (800Hz, 1000Hz, 1200Hz)
            if (elapsed < 30)
            {
                playPwmTone(800);
            }
            else if (elapsed < 70)
            {
                stopPwmTone();
            }
            else if (elapsed < 100)
            {
                playPwmTone(1000);
            }
            else if (elapsed < 140)
            {
                stopPwmTone();
            }
            else if (elapsed < 170)
            {
                playPwmTone(1200);
            }
            else
            {
                stopPwmTone();
                _priorityFxActive = false;
                _currentSound = _engineRunning ? SOUND_IDLE : SOUND_NONE;
            }
            break;

        case SOUND_RELOAD_DONE: // Lên nòng sẵn sàng chiến đấu: 1200Hz -> 2400Hz giòn giã
            if (elapsed < 60)
            {
                playPwmTone(1200);
            }
            else if (elapsed < 80)
            {
                stopPwmTone();
            }
            else if (elapsed < 180)
            {
                playPwmTone(2400);
            }
            else
            {
                stopPwmTone();
                _priorityFxActive = false;
                _currentSound = _engineRunning ? SOUND_IDLE : SOUND_NONE;
            }
            break;

        case SOUND_GAME_START: // Kèn lệnh xuất trận 3 nốt hào hùng: C6 (1046Hz) -> E6 (1318Hz) -> G6 (1568Hz)
            if (elapsed < 120)
            {
                playPwmTone(1046);
            }
            else if (elapsed < 140)
            {
                stopPwmTone();
            }
            else if (elapsed < 260)
            {
                playPwmTone(1318);
            }
            else if (elapsed < 280)
            {
                stopPwmTone();
            }
            else if (elapsed < 450)
            {
                playPwmTone(1568);
            }
            else
            {
                stopPwmTone();
                _priorityFxActive = false;
                _currentSound = _engineRunning ? SOUND_IDLE : SOUND_NONE;
            }
            break;

        case SOUND_DEATH: // Xe bị hạ: Chuỗi nốt tử trận rớt dần rồi ngắt lịm
            if (elapsed < 120)
            {
                playPwmTone(1800);
            }
            else if (elapsed < 240)
            {
                playPwmTone(1400);
            }
            else if (elapsed < 360)
            {
                playPwmTone(1000);
            }
            else if (elapsed < 500)
            {
                playPwmTone(600);
            }
            else if (elapsed < 700)
            {
                playPwmTone(350);
            }
            else
            {
                stopPwmTone();
                _priorityFxActive = false;
                setEngineRunning(false);
            }
            break;

        case SOUND_IGNITION_CRANK: // Đang nhấn giữ đề máy (xung cơ khí đề nổ)
            if (elapsed < 40)
            {
                playPwmTone(450);
            }
            else if (elapsed < 80)
            {
                stopPwmTone();
            }
            else if (elapsed < 120)
            {
                playPwmTone(550);
            }
            else if (elapsed < 160)
            {
                stopPwmTone();
            }
            else
            {
                stopPwmTone();
                _priorityFxActive = false;
                _currentSound = SOUND_NONE;
            }
            break;

        case SOUND_IGNITION_SUCCESS: // Đề nổ thành công! Tiếng rồ ga khởi động dũng mãnh
            if (elapsed < 80)
            {
                playPwmTone(500);
            }
            else if (elapsed < 180)
            {
                playPwmTone(900);
            }
            else if (elapsed < 350)
            {
                playPwmTone(1500);
            }
            else
            {
                stopPwmTone();
                _priorityFxActive = false;
                _currentSound = SOUND_IDLE;
            }
            break;

        default:
            stopPwmTone();
            _priorityFxActive = false;
            _currentSound = _engineRunning ? SOUND_IDLE : SOUND_NONE;
            break;
        }
        return;
    }

    // 2. DIESEL ENGINE / TREAD SOUND
    if (_engineRunning)
    {
        if (_throttle > 25)
        {
            // Xe chạy: Nhịp xích và ga động cơ tăng tốc dồn dập
            uint32_t interval = map(_throttle, 25, 255, 140, 45);
            uint32_t freq = map(_throttle, 25, 255, 260, 500);
            if (now - _lastDrivePulse >= interval)
            {
                _lastDrivePulse = now;
                playPwmTone(freq);
            }
            else if (now - _lastDrivePulse >= 12)
            {
                stopPwmTone();
            }
        }
        else
        {
            // Xe nổ máy đứng yên (Idle Diesel Chug): Nhịp nổ xilanh nhịp nhàng, trầm đầm (180Hz, chu kỳ 190ms, xung 12ms)
            if (now - _lastIdlePulse >= 190)
            {
                _lastIdlePulse = now;
                playPwmTone(180);
            }
            else if (now - _lastIdlePulse >= 12)
            {
                stopPwmTone();
            }
        }
    }
    else
    {
        stopPwmTone();
    }
}

void Feedback::updateLed(uint32_t now)
{
    if (_isDead)
    {
        // Red slow pulse
        if (now - _blinkTimer >= 300)
        {
            _blinkTimer = now;
            _blinkState = !_blinkState;
            _strip.setPixelColor(0, _blinkState ? _strip.Color(120, 0, 0) : _strip.Color(0, 0, 0));
            _strip.show();
        }
        return;
    }

    if (_isHitFlashing)
    {
        // White flash on hit for 200ms
        if (now - _hitFlashStartTime < 200)
        {
            _strip.setPixelColor(0, _strip.Color(255, 255, 255));
            _strip.show();
            return;
        }
        else
        {
            _isHitFlashing = false;
        }
    }

    if (_isIFrame)
    {
        // Fast blink (100ms interval) to signify invulnerability
        if (now - _blinkTimer >= 100)
        {
            _blinkTimer = now;
            _blinkState = !_blinkState;
            _strip.setPixelColor(0, _blinkState ? _teamColor : _strip.Color(0, 0, 0));
            _strip.show();
        }
        return;
    }

    // Default: solid team color
    _strip.setPixelColor(0, _teamColor);
    _strip.show();
}

void Feedback::update()
{
    uint32_t now = millis();
    updateBuzzer(now);
    updateLed(now);
}
