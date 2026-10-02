# SỔ TAY KỸ THUẬT DỰ ÁN (TECHNICAL SYSTEM MANUAL)
## MICRO TANK ARENA (ESP32-C3 LASER TAG)

Tài liệu này được biên soạn để **bất kỳ kỹ sư hoặc AI nào tiếp quản dự án đều có thể hiểu toàn bộ hệ thống trong 5 phút** và thực hiện chỉnh sửa chính xác, an toàn mà không cần đọc lại từng dòng mã nguồn.

---

## 1. SƠ ĐỒ CẤU TRÚC FILE & TRÁCH NHIỆM TỪNG MODULE

```
TankMini/
├── platformio.ini              # Cấu hình biên dịch PlatformIO, thư viện, cờ USB-CDC
├── partitions.csv              # Bảng phân vùng Flash 4MB (App 1.8MB, LittleFS 2MB)
├── data/
│   └── index.html              # Mã nguồn giao diện buồng lái Web HUD (HTML5/CSS/JS)
├── include/
│   ├── MotorDriver.h           # Header lái động cơ DRV8833 & Slew-Rate Limiter
│   ├── IrProtocol.h            # Định nghĩa khung truyền 16-bit đạn hồng ngoại & Checksum
│   ├── IrTransceiver.h         # Header phát RMT 38kHz & thu giải mã TSOP38238
│   ├── Feedback.h              # Header điều khiển LED RGB WS2812B & Còi Buzzer
│   ├── TankGameEngine.h        # Header bộ máy trạng thái game (HP, Ammo, I-Frame, Ulti)
│   ├── WebDashboard.h          # Nhúng file HTML PROGMEM (chạy web không cần nạp LittleFS)
│   └── TankNetwork.h           # Header mạng SoftAP, Captive Portal, WebSocket & ESP-NOW
├── src/
│   ├── MotorDriver.cpp         # Điều khiển PWM 20kHz 4 chân, chống giật sụt áp, Fail-safe
│   ├── IrTransceiver.cpp       # Driver phát sóng mang RMT & ngắt đo microsecond giải mã đạn
│   ├── Feedback.cpp            # Bộ tạo âm thanh đa tần (tone) & hiệu ứng nháy đèn
│   ├── TankGameEngine.cpp      # Logic chiến đấu: Nhận đạn, trừ máu, khựng xe, nạp đạn, Ulti
│   ├── TankNetwork.cpp         # Tự động chọn AP/STA, DNS server 53, điều phối WebSocket
│   └── main.cpp                # Khởi tạo 5 module và chạy vòng lặp non-blocking
└── docs/                       # Thư mục tài liệu kỹ thuật & kế hoạch
```

---

## 2. BẢNG SƠ ĐỒ ĐẤU NỐI PHẦN CỨNG (GPIO WIRING MAP)

Vi điều khiển trung tâm: **ESP32-C3 Super Mini** (Chạy xung nhịp 160MHz).

| Chân GPIO | Thiết bị ngoại vi | Chức năng chi tiết | Module phần mềm phụ trách |
| :--- | :--- | :--- | :--- |
| **GPIO 0** | DRV8833 `IN1` | LEDC PWM: Động cơ Trái (Quay tiến) | [`MotorDriver.cpp`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/src/MotorDriver.cpp) |
| **GPIO 1** | DRV8833 `IN2` | LEDC PWM: Động cơ Trái (Quay lùi) | [`MotorDriver.cpp`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/src/MotorDriver.cpp) |
| **GPIO 2** | DRV8833 `IN3` | LEDC PWM: Động cơ Phải (Quay tiến) *(Strapping Pin)* | [`MotorDriver.cpp`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/src/MotorDriver.cpp) |
| **GPIO 3** | DRV8833 `IN4` | LEDC PWM: Động cơ Phải (Quay lùi) | [`MotorDriver.cpp`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/src/MotorDriver.cpp) |
| **GPIO 4** | Transistor 2N2222 | Cực B lái LED IR 940nm (Sóng mang RMT 38kHz) | [`IrTransceiver.cpp`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/src/IrTransceiver.cpp) |
| **GPIO 5** | TSOP38238 `OUT` | Chân nhận tín hiệu hồng ngoại (Tích cực LOW) | [`IrTransceiver.cpp`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/src/IrTransceiver.cpp) |
| **GPIO 6** | WS2812B `DIN` | Tín hiệu điều khiển đèn LED RGB báo phe & bất tử | [`Feedback.cpp`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/src/Feedback.cpp) |
| **GPIO 7** | Passive Buzzer | Còi báo động, âm phát súng, nổ máy, nạp đạn | [`Feedback.cpp`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/src/Feedback.cpp) |
| **GPIO 8** | DRV8833 `nSLEEP`| Kéo HIGH để mở công suất; Kéo LOW tắt động cơ | [`MotorDriver.cpp`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/src/MotorDriver.cpp) |

> [!WARNING]
> **Lưu ý phần cứng đặc thù của ESP32-C3:**
> - **GPIO 2** là Strapping Pin: Không được nối điện trở kéo xuống GND từ bên ngoài khi khởi động.
> - **GPIO 9** là nút BOOT mặc định của mạch.

---

## 3. CƠ CHẾ BẢO VỆ PHẦN CỨNG & RÀO CHẮN AN TOÀN

1. **Chống sụt áp vi điều khiển (Brownout Prevention):**
   - Động cơ N20 khi đảo chiều tức thời có thể hút dòng đỉnh 1.2A gây reset ESP32.
   - Thuật toán `Slew-Rate Limiter` trong [`MotorDriver.cpp`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/src/MotorDriver.cpp) giới hạn gia tốc PWM (tối đa $\pm 20$ đơn vị mỗi $10\text{ms}$), triệt tiêu xung dòng đột ngột.
2. **Cơ chế tự ngắt an toàn (Fail-Safe 300ms):**
   - Nếu mất sóng Wi-Fi hoặc điện thoại đóng trình duyệt quá $300\text{ms}$, động cơ tự động hãm về 0 tránh xe tự trôi mất kiểm soát.
3. **Khóa kênh sóng (Channel 1 Locking):**
   - SoftAP và ESP-NOW đồng thời sử dụng cố định **Wi-Fi Channel 1** để gửi nhận tin hạ gục (Kill Feed) giữa các xe mà không bị lệch tần.

---

## 4. GIAO THỨC TRUYỀN DỮ LIỆU & LUỒNG THÔNG TIN

### 4.1. Giao thức đạn hồng ngoại 16-bit (Optical IR Protocol)
- Tần số: 38kHz, điều chế bằng RMT phần cứng (Duty 33%).
- Cấu trúc gói tin:
  ```
  Bit 15..12 : Team ID   (4 bit: 1 = Blue, 2 = Red)
  Bit 11..8  : Player ID (4 bit: 1 đến 15)
  Bit 7..4   : Damage    (4 bit: 1 = Bắn thường, 3 = Chiêu cuối Ulti)
  Bit 3..0   : Checksum  (4 bit: Team ^ Player ^ Damage & 0x0F)
  ```
- **Quy tắc:** Nếu `Team ID đạn == Team ID xe` $\rightarrow$ Hủy gói tin (Chống bắn nhầm đồng đội). Nếu sai Checksum $\rightarrow$ Loại bỏ (Chống phản xạ tường).

### 4.2. Giao thức WebSocket 20Hz (Web Client $\leftrightarrow$ Xe)
- **Lệnh điều khiển từ Web Client $\rightarrow$ Xe:**
  - Lái độc lập 2 xích: `{"type": "drive", "left": 180, "right": -180}` (Giá trị: `-255` đến `+255`).
  - Bắn pháo chính: `{"type": "fire"}`.
  - Nạp đạn thủ công: `{"type": "reload"}`.
  - Kích hoạt Ulti: `{"type": "ulti"}`.
  - Hồi sinh ván mới: `{"type": "reset"}`.
- **Trạng thái gửi từ Xe $\rightarrow$ Web Client (Mỗi 100ms hoặc khi có sự kiện):**
  ```json
  {
    "type": "hud",
    "hp": 4,
    "maxHp": 5,
    "ammo": 8,
    "maxAmmo": 10,
    "ulti": 65,
    "isIFrame": false,
    "isDead": false,
    "state": "BATTLE"
  }
  ```

---

## 5. MÔ HÌNH MÁY TRẠNG THÁI GAME (GAME FSM)

```
                 [Khởi động / Reset]
                          │
                          ▼
                     STATE_BATTLE
                    (HP: 5, Ammo: 10)
                     │          │
        Bị bắn trúng │          │ Bắn hết đạn / Bấm Reload
                     ▼          ▼
             STATE_HIT_STUN   STATE_RELOAD
             (Khựng 400ms)    (Khóa nòng 3.5s)
                     │          │
           Hết stun  │          │ Nạp xong
                     └───►◄─────┘
                           │
                 Nếu HP về 0
                           ▼
                      STATE_DEAD
            (Quay 360 độ rồi khóa vĩnh viễn)
```

---

## 6. HỆ THỐNG ÂM THANH THIẾT GIÁP THỰC TẾ (REALISTIC TACTICAL AUDIO ENGINE)

Hệ thống được thiết kế theo kiến trúc **Âm thanh Kép Đồng bộ (Dual-Engine Audio)**:

### 6.1. Âm thanh phần cứng trên xe (ESP32-C3 LEDC PWM Driver)
- **Phần cứng:** Loa mini $8\Omega$ hoặc Passive Buzzer gắn vào chân **GPIO 7**.
- **Kênh điều chế:** Sử dụng kênh **LEDC 4** (tần số $15\text{Hz} - 850\text{Hz}$, Duty cycle $50\%$), hoàn toàn độc lập với kênh LEDC 0–3 của DRV8833.
- **Cơ chế:** Non-blocking $100\%$ qua hàm `millis()`, chia làm 2 tầng ưu tiên:
  1. **Hiệu ứng ưu tiên (Priority FX):** Chiếm quyền phát khi có sự kiện chiến đấu (bắn, trúng đạn, nạp đạn, tử trận).
  2. **Tiếng nổ động cơ thường trực (Continuous Diesel Engine):**
     - Khi đứng yên (Idle): Nhịp nổ piston $46\text{Hz}$ ngắt quãng ($25\text{ms}$ phát xung, $55\text{ms}$ ngắt nghỉ).
     - Khi chạy: Tần số quét động tuyến tính theo độ mở ga: $f = \text{map}(\text{throttle}, 25, 255, 46\text{Hz}, 135\text{Hz})$.

### 6.2. Bảng đối chiếu tần số & đặc tính âm thanh (Audio Frequency Matrix)

| Hiệu ứng | Ký hiệu Firmware | Tần số phần cứng (LEDC PWM) | Dạng sóng Web Audio API | Mô tả cảm giác âm học |
| :--- | :--- | :--- | :--- | :--- |
| **Nổ máy chờ** | `SOUND_IDLE` | $46\text{Hz}$ ngắt quãng (chu kỳ $80\text{ms}$) | Sawtooth $46\text{Hz}$ + Piston LFO | Tiếng máy Diesel nổ gằn từng nhịp khi dừng xe |
| **Rồ ga chạy** | `SOUND_DRIVE` | $46\text{Hz} \to 135\text{Hz}$ theo ga | Sawtooth $46\text{Hz} \to 135\text{Hz}$ | Tiếng xích tải và tua máy gầm lên khi tăng tốc |
| **Bắn pháo** | `SOUND_FIRE` | $380\text{Hz} \to 28\text{Hz}$ ($220\text{ms}$) | Sawtooth $380\text{Hz} \to 28\text{Hz}$ | Pháo nổ đanh gọn, hạ tần số dứt khoát |
| **Đại bác Ulti**| `SOUND_ULTI` | Xung 1: $450\to 40\text{Hz}$, Xung 2: $180\to 18\text{Hz}$ | Square $450\text{Hz}$ + Sawtooth $180\text{Hz}$ | Đề pa kích nổ kèm dư chấn siêu trầm |
| **Trúng đạn** | `SOUND_HIT` | $850\text{Hz} \to 110\text{Hz}$ ($200\text{ms}$) | Triangle $850\text{Hz} \to 110\text{Hz}$ | Tiếng đạn đập giáp vang dội + Rung Haptic |
| **Hết đạn** | `SOUND_EMPTY` | $260\text{Hz} \to 80\text{Hz}$ ($60\text{ms}$) | Square $260\text{Hz} \to 80\text{Hz}$ | Tiếng cạch kim hỏa búa gõ rỗng |
| **Đang nạp** | `SOUND_RELOAD`| 3 xung: $160\text{Hz}, 200\text{Hz}, 240\text{Hz}$ | Triangle 3 nhịp xích tải ($70\text{ms}$) | Cơ cấu máy móc tiếp đạn vào buồng nạp |
| **Lên nòng xong**|`SOUND_RELOAD_DONE`| $400\text{Hz}$ ($70\text{ms}$) $\to 750\text{Hz}$ ($140\text{ms}$)| Sine $400\text{Hz} \to 750\text{Hz}$ | Chuông cơ khí nạp xong sẵn sàng bắn |
| **Kèn lệnh** | `SOUND_GAME_START` | $300\text{Hz}, 400\text{Hz}, 600\text{Hz}$ | Sawtooth 3 nốt kèn thiết giáp | Hiệu lệnh xung trận đầu ván đấu |
| **Tử trận** | `SOUND_DEATH` | $140\text{Hz} \to 15\text{Hz}$ ($1000\text{ms}$) $\to$ Tắt | Sawtooth $140\text{Hz} \to 12\text{Hz}$ | Động cơ sụp đổ, lịm dần rồi tắt máy |
