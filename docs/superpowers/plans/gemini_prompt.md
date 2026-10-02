Phương án sử dụng **Passive Buzzer / Loa mini $8\Omega$** điều chế bằng xung **LEDC PWM** trên ESP32-C3 kết hợp bộ tổng hợp âm thanh **Web Audio API** trên giao diện Web là lựa chọn tối ưu nhất cho cảm giác chiến đấu thực thụ.

Dưới đây là mã nguồn tích hợp hoàn chỉnh cho cả 2 phần: **Firmware trên ESP32-C3** và **Buồng lái Web HUD** (`data/index.html`).

---

### 1. Mã nguồn Firmware ESP32-C3: `TankAudio.h`

Module này sử dụng kênh **LEDC 4** (để không xung đột với kênh $0 - 3$ của DRV8833) và xử lý non-blocking hoàn toàn bằng `millis()`:

```cpp
#pragma once
#include <Arduino.h>

enum AudioEffect {
  FX_NONE,
  FX_IDLE,
  FX_DRIVE,
  FX_FIRE,
  FX_ULTI,
  FX_HIT,
  FX_RELOAD_DONE,
  FX_EMPTY,
  FX_DEAD
};

class TankAudio {
public:
  void begin(uint8_t pin = 7, uint8_t ledcChannel = 4) {
    _pin = pin;
    _channel = ledcChannel;
    ledcSetup(_channel, 100, 8); // Độ phân giải 8-bit
    ledcAttachPin(_pin, _channel);
    ledcWrite(_channel, 0);
  }

  void startEngine() {
    _engineRunning = true;
    _currentFx = FX_IDLE;
  }

  void stopEngine() {
    _engineRunning = false;
    _currentFx = FX_NONE;
    ledcWrite(_channel, 0);
  }

  void setThrottle(int speed) {
    if (!_engineRunning || _priorityFxActive) return;
    _throttle = constrain(abs(speed), 0, 255);
  }

  void triggerFire() { setPriorityFx(FX_FIRE); }
  void triggerUlti() { setPriorityFx(FX_ULTI); }
  void triggerHit()  { setPriorityFx(FX_HIT); }
  void triggerReloadDone() { setPriorityFx(FX_RELOAD_DONE); }
  void triggerEmpty() { setPriorityFx(FX_EMPTY); }
  void triggerDeath() { setPriorityFx(FX_DEAD); }

  void update() {
    unsigned long now = millis();

    // 1. XỬ LÝ CÁC HIỆU ỨNG CHIẾN ĐẤU ƯU TIÊN
    if (_priorityFxActive) {
      unsigned long elapsed = now - _fxStartTime;

      switch (_currentFx) {
        case FX_FIRE: // Nổ pháo: 380Hz -> 28Hz trong 220ms
          if (elapsed < 220) {
            uint32_t f = map(elapsed, 0, 220, 380, 28);
            playTone(f);
          } else { endPriorityFx(); }
          break;

        case FX_ULTI: // Đại bác Ulti: Nổ kép rung chuyển
          if (elapsed < 140) {
            playTone(map(elapsed, 0, 140, 450, 40));
          } else if (elapsed < 180) {
            ledcWrite(_channel, 0);
          } else if (elapsed < 600) {
            playTone(map(elapsed, 180, 600, 180, 18));
          } else { endPriorityFx(); }
          break;

        case FX_HIT: // Trúng đạn (Koong): 850Hz -> 110Hz trong 200ms
          if (elapsed < 200) {
            playTone(map(elapsed, 0, 200, 850, 110));
          } else { endPriorityFx(); }
          break;

        case FX_EMPTY: // Hết đạn (Cạch kim hỏa): 260Hz -> 80Hz trong 60ms
          if (elapsed < 60) {
            playTone(map(elapsed, 0, 60, 260, 80));
          } else { endPriorityFx(); }
          break;

        case FX_RELOAD_DONE: // Lên nòng: 400Hz (70ms) -> 750Hz (140ms)
          if (elapsed < 70) {
            playTone(400);
          } else if (elapsed < 90) {
            ledcWrite(_channel, 0);
          } else if (elapsed < 230) {
            playTone(750);
          } else { endPriorityFx(); }
          break;

        case FX_DEAD: // Chết máy: 140Hz hạ về 12Hz rồi tắt
          if (elapsed < 1000) {
            playTone(map(elapsed, 0, 1000, 140, 12));
          } else {
            endPriorityFx();
            stopEngine();
          }
          break;

        default:
          endPriorityFx();
          break;
      }
      return;
    }

    // 2. XỬ LÝ ÂM ĐỘNG CƠ DIESEL THƯỜNG TRỰC
    if (_engineRunning) {
      if (_throttle > 25) {
        // Xe chạy: Tần số tăng theo độ nhấn ga (46Hz -> 135Hz)
        uint32_t runFreq = map(_throttle, 25, 255, 46, 135);
        playTone(runFreq);
      } else {
        // Xe đứng yên (Idle): Nhịp nổ piston 46Hz ngắt quãng (nổ 25ms, nghỉ 55ms)
        if (now - _lastIdlePulse >= 80) {
          _lastIdlePulse = now;
          playTone(46);
        } else if (now - _lastIdlePulse >= 25) {
          ledcWrite(_channel, 0);
        }
      }
    } else {
      ledcWrite(_channel, 0);
    }
  }

private:
  uint8_t _pin;
  uint8_t _channel;
  bool _engineRunning = false;
  bool _priorityFxActive = false;
  int _throttle = 0;
  AudioEffect _currentFx = FX_NONE;
  unsigned long _fxStartTime = 0;
  unsigned long _lastIdlePulse = 0;

  void playTone(uint32_t freq) {
    if (freq < 15) freq = 15;
    ledcChangeFrequency(_channel, freq, 8);
    ledcWrite(_channel, 128); // 50% Duty cycle
  }

  void setPriorityFx(AudioEffect fx) {
    _priorityFxActive = true;
    _currentFx = fx;
    _fxStartTime = millis();
  }

  void endPriorityFx() {
    _priorityFxActive = false;
    _currentFx = _engineRunning ? FX_IDLE : FX_NONE;
  }
};

```

---

### 2. Giao diện Web Buồng lái tích hợp Web Audio API (`data/index.html`)

Tích hợp âm thanh đồng bộ: loa trên xe phát xung PWM đanh thép, đồng thời điện thoại người lái cũng phát âm thanh vòm và rung xúc giác qua Web Audio API:

```html
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Tank War Cockpit HUD</title>
  <style>
    * { box-sizing: border-box; touch-action: none; user-select: none; }
    body {
      background: #0d1117; color: #fff; font-family: monospace;
      margin: 0; padding: 10px; height: 100vh; display: flex; flex-direction: column;
    }
    #top-bar {
      display: flex; justify-content: space-between; padding: 10px 15px;
      background: #161b22; border: 1px solid #30363d; border-radius: 8px;
    }
    .hud-stat { font-size: 16px; font-weight: bold; }
    #cockpit {
      flex: 1; display: flex; justify-content: space-between; align-items: center; padding: 10px;
    }
    .dpad-grid { display: grid; grid-template-columns: repeat(3, 75px); gap: 8px; }
    .pad-btn {
      height: 75px; background: #21262d; border: 1px solid #30363d; border-radius: 12px;
      color: #58a6ff; font-size: 26px; display: flex; align-items: center; justify-content: center;
    }
    .pad-btn:active { background: #30363d; filter: brightness(1.4); }
    .action-group { display: flex; flex-direction: column; gap: 12px; width: 140px; }
    .act-btn {
      height: 65px; border-radius: 10px; border: none; color: #fff;
      font-size: 16px; font-weight: bold; cursor: pointer;
    }
    .btn-fire { background: #da3633; box-shadow: 0 0 10px rgba(218,54,51,0.4); }
    .btn-ulti { background: #8957e5; box-shadow: 0 0 10px rgba(137,87,229,0.4); }
    .btn-reload { background: #1f6feb; }
    .act-btn:active { filter: brightness(1.5); transform: scale(0.96); }
  </style>
</head>
<body>

  <div id="top-bar">
    <div class="hud-stat" style="color: #3fb950;">MÁU: <span id="hp">5</span>/5</div>
    <div class="hud-stat" style="color: #d29922;">ĐẠN: <span id="ammo">10</span>/10</div>
    <div class="hud-stat" style="color: #bc8cff;">ULTI: <span id="ulti">0</span>%</div>
  </div>

  <div id="cockpit">
    <!-- D-Pad Điều Khiển Lái -->
    <div class="dpad-grid">
      <div></div>
      <button class="pad-btn" ontouchstart="sendDrive(220,220)" ontouchend="sendDrive(0,0)">▲</button>
      <div></div>
      <button class="pad-btn" ontouchstart="sendDrive(-200,200)" ontouchend="sendDrive(0,0)">◄</button>
      <button class="pad-btn" ontouchstart="sendDrive(0,0)">■</button>
      <button class="pad-btn" ontouchstart="sendDrive(200,-200)" ontouchend="sendDrive(0,0)">►</button>
      <div></div>
      <button class="pad-btn" ontouchstart="sendDrive(-220,-220)" ontouchend="sendDrive(0,0)">▼</button>
      <div></div>
    </div>

    <!-- Hỏa Lực -->
    <div class="action-group">
      <button class="act-btn btn-fire" ontouchstart="handleFire()">💥 BẮN PHÁO</button>
      <button class="act-btn btn-ulti" ontouchstart="handleUlti()">⚡ ĐẠI BÁC</button>
      <button class="act-btn btn-reload" ontouchstart="handleReload()">⚙️ NẠP ĐẠN</button>
    </div>
  </div>

  <script>
    let ws;
    let audioCtx = null;
    let currentAmmo = 10, currentHp = 5, currentUlti = 0;

    function initAudio() {
      if (!audioCtx) audioCtx = new (window.AudioContext || window.webkitAudioContext)();
      if (audioCtx.state === 'suspended') audioCtx.resume();
    }

    // TỔNG HỢP ÂM THANH WEB AUDIO API (ĐỒNG BỘ TRÊN ĐIỆN THOẠI)
    function playSnd(type) {
      initAudio();
      const now = audioCtx.currentTime;
      const osc = audioCtx.createOscillator();
      const gain = audioCtx.createGain();

      if (type === 'fire') {
        osc.type = 'sawtooth';
        osc.frequency.setValueAtTime(380, now);
        osc.frequency.exponentialRampToValueAtTime(28, now + 0.22);
        gain.gain.setValueAtTime(0.6, now);
        gain.gain.exponentialRampToValueAtTime(0.01, now + 0.25);
        osc.connect(gain); gain.connect(audioCtx.destination);
        osc.start(now); osc.stop(now + 0.25);
        if (navigator.vibrate) navigator.vibrate(60);
      } else if (type === 'empty') {
        osc.type = 'square';
        osc.frequency.setValueAtTime(260, now);
        osc.frequency.exponentialRampToValueAtTime(80, now + 0.06);
        gain.gain.setValueAtTime(0.4, now);
        gain.gain.exponentialRampToValueAtTime(0.01, now + 0.07);
        osc.connect(gain); gain.connect(audioCtx.destination);
        osc.start(now); osc.stop(now + 0.07);
      } else if (type === 'hit') {
        osc.type = 'triangle';
        osc.frequency.setValueAtTime(850, now);
        osc.frequency.exponentialRampToValueAtTime(110, now + 0.2);
        gain.gain.setValueAtTime(0.6, now);
        gain.gain.exponentialRampToValueAtTime(0.01, now + 0.22);
        osc.connect(gain); gain.connect(audioCtx.destination);
        osc.start(now); osc.stop(now + 0.22);
        if (navigator.vibrate) navigator.vibrate([80, 50, 80]);
      }
    }

    function initWebSocket() {
      ws = new WebSocket(`ws://${location.hostname}/ws`);
      ws.onmessage = (e) => {
        const d = JSON.parse(e.data);
        if (d.hp !== undefined) {
          if (d.hp < currentHp) playSnd('hit');
          currentHp = d.hp;
          document.getElementById('hp').innerText = d.hp;
        }
        if (d.ammo !== undefined) {
          currentAmmo = d.ammo;
          document.getElementById('ammo').innerText = d.ammo;
        }
        if (d.ulti !== undefined) {
          currentUlti = d.ulti;
          document.getElementById('ulti').innerText = d.ulti;
        }
      };
    }

    function sendDrive(l, r) {
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ type: "drive", left: l, right: r }));
      }
    }

    function handleFire() {
      if (currentAmmo > 0) {
        playSnd('fire');
        if (ws && ws.readyState === WebSocket.OPEN) ws.send(JSON.stringify({ type: "fire" }));
      } else {
        playSnd('empty');
      }
    }

    function handleUlti() {
      if (currentUlti >= 100) {
        if (ws && ws.readyState === WebSocket.OPEN) ws.send(JSON.stringify({ type: "ulti" }));
      } else {
        playSnd('empty');
      }
    }

    function handleReload() {
      if (ws && ws.readyState === WebSocket.OPEN) ws.send(JSON.stringify({ type: "reload" }));
    }

    window.addEventListener('load', () => {
      initWebSocket();
      document.body.addEventListener('touchstart', initAudio, { once: true });
    });
  </script>
</body>
</html>

```

---

Bản thiết kế âm thanh và mã nguồn buồng lái đã đồng bộ hoàn chỉnh. Bước tiếp theo, bạn muốn bắt tay vào **kiểm thử mạch thực tế với loa $8\Omega$** hay tiến hành **gộp các file vào dự án PlatformIO để nạp thẳng lên xe**?
