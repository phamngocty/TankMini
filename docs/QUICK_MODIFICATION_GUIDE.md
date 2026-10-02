# HƯỚNG DẪN CHỈNH SỬA NHANH (QUICK MODIFICATION GUIDE)
## BẢN ĐỒ TRA CỨU CẤP TỐC DÀNH CHO DEVELOPER & AI

> Khi người dùng yêu cầu chỉnh sửa một tính năng cụ thể, **chỉ cần tra bảng dưới đây và sửa đúng tệp/dòng được chỉ định**, không cần đọc lại toàn bộ mã nguồn của dự án!

---

## 1. BẢNG TRA CỨU "YÊU CẦU $\rightarrow$ TỆP & DÒNG CẦN SỬA"

| Yêu cầu cần thay đổi | Tệp cần sửa | Vị trí biến / Hàm cụ thể | Hướng dẫn chỉnh sửa |
| :--- | :--- | :--- | :--- |
| **Đổi Team (Phe) hoặc Tank ID** | [`src/main.cpp`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/src/main.cpp) | Dòng 13 - 14 | Sửa `TANK_TEAM_ID` (`1`=Xanh, `2`=Đỏ) và `TANK_PLAYER_ID` (`1`..`15`). |
| **Đổi chân GPIO phần cứng** | [`include/MotorDriver.h`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/include/MotorDriver.h)<br>[`include/IrTransceiver.h`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/include/IrTransceiver.h)<br>[`include/Feedback.h`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/include/Feedback.h) | `PIN_IN1`..`PIN_IN4`<br>`PIN_TX`, `PIN_RX`<br>`PIN_RGB`, `PIN_BUZZER` | Sửa trực tiếp số chân GPIO trong các hằng số `static constexpr uint8_t PIN_...`. |
| **Đổi Máu (HP), Băng đạn (Ammo)** | [`include/TankGameEngine.h`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/include/TankGameEngine.h) | Dòng 18 - 19 | Sửa `DEFAULT_MAX_HP` (mặc định 5) và `DEFAULT_MAX_AMMO` (mặc định 10). |
| **Đổi tốc độ bắn / Cooldown** | [`include/TankGameEngine.h`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/include/TankGameEngine.h) | Dòng 20 | Sửa `FIRE_COOLDOWN_MS` (mặc định 800ms = 0.8 giây/viên). |
| **Đổi thời gian nạp đạn (Reload)** | [`include/TankGameEngine.h`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/include/TankGameEngine.h) | Dòng 21 | Sửa `RELOAD_TIME_MS` (mặc định 3500ms = 3.5 giây). |
| **Đổi thời gian khựng (Hit-Stun)** | [`include/TankGameEngine.h`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/include/TankGameEngine.h) | Dòng 22 | Sửa `HIT_STUN_TIME_MS` (mặc định 400ms khựng bánh xe khi trúng đạn). |
| **Đổi thời gian Bất Tử (I-Frame)** | [`include/TankGameEngine.h`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/include/TankGameEngine.h) | Dòng 23 | Sửa `IFRAME_TIME_MS` (mặc định 1500ms miễn nhiễm đạn sau khi bị bắn). |
| **Đổi Sát thương Chiêu cuối (Ulti)**| [`src/TankGameEngine.cpp`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/src/TankGameEngine.cpp) | Trong hàm `handleUlti()` | Sửa `_ir->fire(3)` thành giá trị sát thương mong muốn (ví dụ 4 hoặc 5 để 1-hit). |
| **Đổi độ mượt tăng tốc (Slew-rate)** | [`include/MotorDriver.h`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/include/MotorDriver.h) | Dòng 16 | Sửa `SLEW_STEP` (Mặc định 20: Số càng lớn xe tăng tốc càng bốc, số càng nhỏ xe càng êm và chống sụt áp tốt hơn). |
| **Đổi thời gian ngắt an toàn (Fail-safe)**| [`include/MotorDriver.h`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/include/MotorDriver.h) | Dòng 17 | Sửa `TIMEOUT_MS` (mặc định 300ms mất tín hiệu là tự ngắt bánh xe). |
| **Đổi tên Wi-Fi SoftAP** | [`src/TankNetwork.cpp`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/src/TankNetwork.cpp) | Trong hàm `initSoftAp()` | Sửa định dạng chuỗi `snprintf(apName, ...)` tạo tên Wi-Fi phát ra. |
| **Đổi tên Wi-Fi Router đấu giải (Arena)**| [`src/TankNetwork.cpp`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/src/TankNetwork.cpp) | Trong hàm `begin()` | Sửa SSID `"TankWar_Arena"` và mật khẩu `"tankarena123"`. |
| **Đổi âm thanh phần cứng xe (Buzzer)** | [`src/Feedback.cpp`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/src/Feedback.cpp) | Hàm `updateBuzzer()` & `playPwmTone()` | Tinh chỉnh tần số quét LEDC PWM ($15\text{Hz} - 850\text{Hz}$) cho tiếng nổ máy Diesel, bắn pháo, nạp đạn, đạn nảy giáp, tử trận. |
| **Đổi âm thanh buồng lái Web** | [`data/index.html`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/data/index.html) & [`include/WebDashboard.h`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/include/WebDashboard.h) | Object `Sound` (Web Audio API) | Tinh chỉnh 9 hàm âm thanh Web Audio: `playFire()`, `playUlti()`, `playHit()`, `playEmpty()`, `playReloading()`, `playReloadDone()`, `playDeath()`, `playGameStart()`, `setThrottle()`. |
| **Đổi chế độ lái mặc định** | [`data/index.html`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/data/index.html) & [`include/WebDashboard.h`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/include/WebDashboard.h) | Biến `config.joystickMode` | Đặt `true` (Joystick 360°) hoặc `false` (2 cần gạt xích dọc). |
| **Sửa giao diện Web HUD** | [`data/index.html`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/data/index.html) & [`include/WebDashboard.h`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/include/WebDashboard.h) | Toàn bộ file | **LƯU Ý:** Khi sửa file `data/index.html`, **BẮT BUỘC** phải copy nội dung HTML vào chuỗi `INDEX_HTML[] PROGMEM` trong `include/WebDashboard.h`! |

---

## 2. QUY TRÌNH CHỈNH SỬA GIAO DIỆN WEB (2 BƯỚC BẮT BUỘC)

Hệ thống được thiết kế để Web App vừa lưu trong Flash LittleFS vừa được nhúng cứng vào bộ nhớ PROGMEM, giúp người dùng không cần chạy lệnh nạp LittleFS mà vẫn mở được Web ngay:

1. **Bước 1:** Chỉnh sửa file [`data/index.html`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/data/index.html) (thêm nút, đổi màu sắc, thay đổi bố cục).
2. **Bước 2:** Copy nguyên văn nội dung của file `data/index.html` dán đè vào giữa `R"rawliteral(` và `)rawliteral";` trong tệp [`include/WebDashboard.h`](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/include/WebDashboard.h).
3. **Bước 3:** Chạy lệnh biên dịch và nạp code:
   ```powershell
   & "C:\Users\phamn\.platformio\penv\Scripts\pio.exe" run -t upload
   ```

---

## 3. LỆNH TERMINAL THƯỜNG DÙNG

- **Biên dịch kiểm tra lỗi:**
  ```powershell
  & "C:\Users\phamn\.platformio\penv\Scripts\pio.exe" run
  ```
- **Nạp chương trình vào ESP32-C3 qua cổng USB:**
  ```powershell
  & "C:\Users\phamn\.platformio\penv\Scripts\pio.exe" run -t upload
  ```
- **Mở màn hình giám sát Serial Monitor (115200 baud):**
  ```powershell
  & "C:\Users\phamn\.platformio\penv\Scripts\pio.exe" device monitor
  ```
- **Dọn dẹp file build cũ:**
  ```powershell
  & "C:\Users\phamn\.platformio\penv\Scripts\pio.exe" run -t clean
  ```
