# Micro Tank Arena (ESP32-C3 Laser Tag) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Xây dựng hoàn chỉnh firmware điều khiển và hệ thống game Laser Tag cho xe tăng mini ESP32-C3, hỗ trợ điều khiển Web thời gian thực, bắn nhận hồng ngoại mã hóa 16-bit và tự động chuyển đổi mạng AP/STA/ESP-NOW.

**Architecture:** Kiến trúc module hóa tách biệt: Tầng phần cứng cấp thấp (Motor LEDC PWM, RMT IR Transceiver, WS2812B/Buzzer Feedback) được bao bọc bởi Game State Machine (FSM). Tầng mạng (AsyncWebServer + WebSocket + DNSServer + ESP-NOW) phục vụ giao diện Web Controller được nén trong LittleFS và đồng bộ trạng thái xe tức thời.

**Tech Stack:** PlatformIO, ESP-IDF / Arduino-ESP32, ESP32-C3 RISC-V, LittleFS, ESPAsyncWebServer, AsyncTCP, WebSockets, HTML5 Canvas, RMT Peripheral.

---

### Task 1: Cấu hình Dự án PlatformIO & Phân vùng Bộ nhớ

**Files:**
- Create: `platformio.ini`
- Create: `partitions.csv`
- Create: `src/main.cpp` (Khung chương trình cơ bản)

- [x] **Step 1: Viết cấu hình `platformio.ini` tối ưu cho ESP32-C3 Super Mini**
  - Cấu hình board `esp32-c3-devkitm-1` hoặc `lolin_c3_mini`.
  - Khai báo các thư viện cần thiết: `ESPAsyncWebServer`, `AsyncTCP`, `FastLED` hoặc `Adafruit NeoPixel`, `ArduinoJson`.
  - Thiết lập phân vùng bộ nhớ hỗ trợ LittleFS cho Web UI (tối thiểu 1MB SPIFFS/LittleFS).

- [x] **Step 2: Viết file phân vùng `partitions.csv`**
  - Tạo bảng phân vùng 4MB flash (App 1.8MB, LittleFS 1.2MB, NVS, OTA data).

- [x] **Step 3: Viết file `src/main.cpp` tối giản để kiểm tra biên dịch**
  - Khởi tạo Serial 115200, nháy LED hoặc in log boot để xác nhận toolchain PlatformIO hoạt động hoàn hảo.

- [x] **Step 4: Chạy biên dịch kiểm tra môi trường**
  - Lệnh: `pio run`
  - Kết quả kỳ vọng: `SUCCESS` (không có lỗi thiếu thư viện hay sai board).

---

### Task 2: Module Điều Khiển Động Cơ & Chống Sụt Áp (Motor Driver)

**Files:**
- Create: `include/MotorDriver.h`
- Create: `src/MotorDriver.cpp`

- [x] **Step 1: Định nghĩa giao diện lớp `MotorDriver`**
  - Cấu hình 4 kênh LEDC PWM tương ứng GPIO 0, 1, 2, 3 và chân bật cầu H GPIO 8 (`nSLEEP`).
  - Cung cấp hàm `setSpeeds(int leftSpeed, int rightSpeed)` với dải giá trị từ `-255` đến `+255`.
  - Cung cấp hàm `stop()` và `emergencyBrake()`.

- [x] **Step 2: Triển khai thuật toán Slew-Rate Limiting (Tăng tốc mềm)**
  - Giới hạn bước nhảy PWM theo chu kỳ (ví dụ: tối đa thay đổi $\pm 25$ mỗi $10\text{ms}$) để triệt tiêu dòng khởi động cực đại (Inrush Current), chống sụt áp nguồn (Brownout reset).

- [x] **Step 3: Triển khai cơ chế Fail-Safe Timeout**
  - Kiểm tra `lastCommandTime`: Nếu không nhận lệnh mới trong vòng $300\text{ms}$, động cơ tự động hãm về 0.

- [x] **Step 4: Viết kịch bản kiểm thử điều khiển động cơ trong `main.cpp`**
  - Chạy chu kỳ: Tiến chậm $\rightarrow$ Tăng tốc $\rightarrow$ Đảo chiều lùi $\rightarrow$ Quay tại chỗ $\rightarrow$ Dừng.
  - Đo điện áp và giám sát Serial để khẳng định xe không bị reset do sụt nguồn.

---

### Task 3: Module Quang Học IR Transceiver (RMT 38kHz)

**Files:**
- Create: `include/IrProtocol.h`
- Create: `include/IrTransceiver.h`
- Create: `src/IrTransceiver.cpp`

- [x] **Step 1: Định nghĩa cấu trúc khung truyền 16-bit (`IrProtocol.h`)**
  - Định nghĩa struct: `TeamID (4 bits)`, `PlayerID (4 bits)`, `Damage (4 bits)`, `Checksum (4 bits)`.
  - Viết hàm tiện ích mã hóa `encodePacket(...)` và giải mã/kiểm tra `validateChecksum(...)`.

- [x] **Step 2: Triển khai Driver phát IR 38kHz bằng ngoại vi RMT (GPIO 4)**
  - Cấu hình RMT TX điều chế sóng mang 38kHz (Duty cycle 33%).
  - Phát chùm xung mở đầu 4.5ms HIGH + 4.5ms LOW, sau đó phát 16-bit dữ liệu theo chuẩn khoảng cách xung (Pulse Distance).

- [x] **Step 3: Triển khai Driver thu xung & giải mã cảm biến TSOP38238 (GPIO 5)**
  - Sử dụng ngắt GPIO cạnh xuống (FALLING Edge) hoặc RMT RX để đo độ rộng xung.
  - Lọc nhiễu: Bỏ qua các xung không đúng tần số hoặc không khớp Leader pulse.
  - Đóng gói sự kiện `OnHitReceived(uint8_t team, uint8_t player, uint8_t damage)`.

- [x] **Step 4: Kiểm thử vòng lặp phát - thu (Loopback Test)**
  - Dùng chính xe phát IR và hướng vào cảm biến thu (hoặc 2 xe đối diện) để kiểm chứng tỉ lệ giải mã chính xác $\ge 98\%$.

---

### Task 4: Module Phản Hồi Âm Thanh & Thị Giác (Feedback System)

**Files:**
- Create: `include/Feedback.h`
- Create: `src/Feedback.cpp`

- [x] **Step 1: Tích hợp LED RGB WS2812B (GPIO 6)**
  - Hiển thị màu nhận diện phe: Xanh dương (Team Blue), Đỏ (Team Red).
  - Hiệu ứng nháy trắng/đỏ khi bị bắn trúng.
  - Hiệu ứng nháy chu kỳ $100\text{ms}$ khi trong trạng thái bất tử (I-Frame).

- [x] **Step 2: Tích hợp Còi Buzzer Thụ Động (GPIO 7)**
  - Sử dụng LEDC tạo các âm điệu (Tones):
    - *Tiếng súng nổ (Fire):* Xung giảm tần nhanh $2000\text{Hz} \rightarrow 500\text{Hz}$ trong $60\text{ms}$.
    - *Tiếng trúng đạn (Hit):* Chuỗi beep đanh gắt.
    - *Tiếng xe nổ/Chết (Death):* Âm trầm kéo dài.

- [x] **Step 3: Tích hợp hiệu ứng giật nòng cơ học (Mechanical Recoil)**
  - Khi bắn, nháy 2 động cơ lùi nhẹ $40\text{ms}$ tạo cảm giác nòng súng nổ giật thực tế.

---

### Task 5: Bộ Máy Quản Lý Trạng Thái Game (Tank Game Engine)

**Files:**
- Create: `include/TankGameEngine.h`
- Create: `src/TankGameEngine.cpp`

- [x] **Step 1: Xây dựng Finite State Machine (FSM)**
  - Các trạng thái: `STATE_LOBBY`, `STATE_BATTLE`, `STATE_HIT_STUN`, `STATE_RELOADING`, `STATE_DEAD`.
  - Thuộc tính người chơi: `maxHp = 5`, `currentHp`, `maxAmmo = 10`, `currentAmmo`, `teamId`, `playerId`.

- [x] **Step 2: Logic bắn đạn và nạp đạn**
  - Tốc độ bắn: Tối thiểu cách nhau $800\text{ms}$.
  - Khi `currentAmmo == 0`: Chuyển sang trạng thái `STATE_RELOADING` trong $3500\text{ms}$, sau đó nạp đầy lại 10 viên.

- [x] **Step 3: Logic nhận sát thương, Hit-Stun & I-Frame**
  - Khi nhận gói tin IR hợp lệ (khác `teamId`):
    - Trừ máu theo `Damage`.
    - Kích hoạt `Hit-Stun`: Khựng toàn bộ động cơ trong $400\text{ms}$.
    - Kích hoạt `I-Frame`: Bật cờ miễn nhiễm đạn trong $1500\text{ms}$.
  - Nếu `currentHp <= 0`: Chuyển sang `STATE_DEAD`, kích hoạt hiệu ứng xoay $360^\circ$ và khóa xe.

---

### Task 6: Giao Diện Điều Khiển Web & Đóng Gói LittleFS (Web HUD)

**Files:**
- Create: `data/index.html`
- Create: `data/style.css`
- Create: `data/app.js`

- [x] **Step 1: Thiết kế giao diện Web Mobile-First**
  - Tối ưu thẻ viewport không cho zoom/scroll (`touch-action: none`).
  - Joystick ảo cảm ứng (Virtual Touch Joystick) hoặc D-Pad phản hồi cảm ứng đa điểm (`touchstart`, `touchmove`, `touchend`).
  - Nút bắn (`FIRE`) nổi bật, thanh máu HP (Health Bar), số đạn (Ammo Counter), bảng thông báo trạng thái.

- [x] **Step 2: Lập trình kết nối WebSocket thời gian thực (`app.js`)**
  - Tự động kết nối tới `ws://[IP]/ws`.
  - Gửi gói tin điều khiển động cơ với tần số $20\text{Hz}$ ($50\text{ms}$/lần).
  - Kích hoạt bộ rung phản hồi trên điện thoại (`navigator.vibrate(50)`) khi bấm nút Bắn hoặc khi bị trúng đạn.

- [x] **Step 3: Nén và tải Web App vào LittleFS**
  - Nén file Gzip (hoặc nhúng PROGMEM chuỗi) để tối ưu RAM và tốc độ load trang (<200ms).

---

### Task 7: Hạ Tầng Mạng Đa Chế Độ (SoftAP, Captive Portal, WebSocket, ESP-NOW)

**Files:**
- Create: `include/TankNetwork.h`
- Create: `src/TankNetwork.cpp`

- [x] **Step 1: Triển khai quét mạng tự động khi khởi động**
  - Xe quét tìm SSID mặc định (`TankWar_Arena`).
  - Nếu tìm thấy $\rightarrow$ Chạy **Arena Mode** (Wi-Fi STA, mDNS `tank.local`).
  - Nếu không tìm thấy $\rightarrow$ Chạy **Standalone Mode** (SoftAP riêng `Tank_[Team]_[ID]`).

- [x] **Step 2: Triển khai Captive Portal (DNS Server port 53)**
  - Trong chế độ Standalone, chuyển hướng mọi truy vấn HTTP về trang điều khiển xe (`192.168.4.1`).

- [x] **Step 3: Tích hợp ESP-NOW Broadcast**
  - Khóa kênh Wi-Fi cố định (Channel 1) trên tất cả các xe.
  - Khi xe hạ gục đối thủ hoặc bị tiêu diệt, gửi bản tin broadcast ESP-NOW thông báo toàn sàn đấu.

---

### Task 8: Tích Hợp Hệ Thống & Kiểm Thử Toàn Diện (End-to-End Test)

**Files:**
- Modify: `src/main.cpp`

- [x] **Step 1: Gắn kết toàn bộ các module trong `setup()` và `loop()`**
  - Khởi tạo `MotorDriver`, `IrTransceiver`, `Feedback`, `TankGameEngine`, `TankNetwork`.
  - Đồng bộ luồng xử lý non-blocking (không sử dụng hàm `delay()`).

- [x] **Step 2: Kiểm thử kịch bản vận hành thực tế**
  - Quét mã QR kết nối vào xe bằng iPhone/Android.
  - Điều khiển xe di chuyển và kiểm tra độ nhạy của Joystick.
  - Thử nghiệm 2 xe đối kháng: Bắn trúng, giật lùi, nháy đèn, khựng xe, đếm số mạng hạ gục.
