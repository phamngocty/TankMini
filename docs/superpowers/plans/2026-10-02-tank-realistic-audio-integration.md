# Realistic Tactical Audio Engine Integration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Tích hợp hệ thống âm thanh chiến đấu thiết giáp chân thực (sử dụng xung LEDC PWM tần số thấp non-blocking trên ESP32-C3 cho Loa mini/Passive Buzzer, và hệ thống tổng hợp âm thanh Web Audio API 9 hiệu ứng trên Web HUD) dựa trên nguyên mẫu `tank.html` và thiết kế trong `gemini_prompt.md`.

**Architecture:** 
- Phía Firmware ESP32-C3: Sử dụng kênh LEDC 4 (tần số 15Hz - 850Hz, duty cycle 50%, độc lập với LEDC 0-3 của motor DRV8833) trong `Feedback.h/cpp` với máy trạng thái non-blocking qua `millis()` để phát tiếng động cơ diesel (idle ngắt quãng và rồ ga theo tốc độ), pháo nổ gằn, đại bác kép, đạn đập giáp, lên nòng và tử trận.
- Phía Web HUD (`WebDashboard.h`): Nhúng bộ tổng hợp âm thanh Web Audio API hoàn chỉnh với 9 hiệu ứng âm thanh từ `tank.html` đồng bộ trực tiếp với sự kiện telemetry (máu giảm, hết đạn, nạp đạn, chiêu cuối) và cần lái ảo (rồ ga động cơ).

**Tech Stack:** ESP32 Arduino Core (LEDC API v2/v3 compatible), C++, Web Audio API (Sawtooth/Triangle/Square/Sine Oscillators, Gain Envelopes), HTML5/JavaScript WebSocket.

---

### Task 1: Nâng cấp Header `Feedback.h` cho LEDC PWM Audio Engine

**Files:**
- Modify: `include/Feedback.h`

- [x] **Step 1: Khai báo mở rộng `FeedbackSound` enum và các hàm điều khiển âm thanh**
- [x] **Step 2: Kiểm tra cú pháp header**

---

### Task 2: Cài đặt LEDC PWM Audio Engine trong `Feedback.cpp`

**Files:**
- Modify: `src/Feedback.cpp`

- [x] **Step 1: Khởi tạo kênh LEDC trong `Feedback::begin`**
- [x] **Step 2: Viết logic `updateBuzzer` non-blocking mô phỏng 9 hiệu ứng âm thanh**
- [x] **Step 3: Biên dịch kiểm tra `Feedback.cpp`**

---

### Task 3: Đồng bộ luồng sự kiện game trong `TankGameEngine.cpp`

**Files:**
- Modify: `src/TankGameEngine.cpp`

- [x] **Step 1: Cập nhật gọi hiệu ứng âm thanh trong `TankGameEngine`**
- [x] **Step 2: Biên dịch kiểm tra `TankGameEngine.cpp`**

---

### Task 4: Nâng cấp bộ tổng hợp Web Audio API trong `WebDashboard.h`

**Files:**
- Modify: `include/WebDashboard.h`

- [x] **Step 1: Mở rộng object `Sound` với toàn bộ 9 hiệu ứng từ `tank.html`**
- [x] **Step 2: Kết nối cần lái D-Pad / Joystick ảo với tiếng rồ ga động cơ Web Audio API**
- [x] **Step 3: Kết nối sự kiện WebSocket Telemetry với âm thanh buồng lái**

---

### Task 5: Biên dịch tổng thể và nghiệm thu hệ thống (Verification)

**Files:**
- PlatformIO Build Verification

- [x] **Step 1: Biên dịch toàn bộ dự án**
- [x] **Step 2: Kiểm tra dung lượng RAM và Flash**
