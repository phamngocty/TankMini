# TÀI LIỆU MỤC TIÊU DỰ ÁN & ĐẶC TẢ KỸ THUẬT (PROJECT GOALS & SPECS)

**Dự án:** Micro Tank Arena (ESP32-C3 Laser Tag Mini Tank)  
**Nền tảng:** PlatformIO (`espressif32`, Arduino Core)  
**Tiêu chuẩn phát triển:** Áp dụng nguyên tắc Andrej Karpathy (Think Before Coding, Simplicity First, Surgical Changes, Goal-Driven Execution).

---

## 1. TỔNG QUAN MỤC TIÊU (CORE OBJECTIVES)

1. **Điều khiển xe tăng hai động cơ độc lập (Differential Drive):** Lái mượt mà, hỗ trợ quay tại chỗ, có bộ đệm gia tốc (Slew-rate Limiter) chống sụt áp nguồn (Brownout Protection).
2. **Hệ thống tác chiến Laser Tag bằng tia hồng ngoại (IR):** Phát xung điều chế 38kHz góc hẹp ($10^\circ - 15^\circ$), thu và giải mã gói tin 16-bit (phân biệt Team, Player, Damage, Checksum) để chống gian lận và chống phản xạ tường.
3. **Giao diện điều khiển Web thời gian thực (Zero App Install):** Truy cập qua trình duyệt Web trên smartphone (iOS/Android) với Joystick ảo cảm ứng phản hồi tức thì qua WebSocket (<50ms).
4. **Mạng đa chế độ tự động (Dual/Tri-mode):**
   - *Arena Mode:* Kết nối vào Wi-Fi nhà qua mDNS (`http://tank.local`).
   - *Standalone Mode:* Xe tự phát SoftAP + Captive Portal (DNS port 53) để quét mã QR là vào lái ngay.
   - *ESP-NOW Mesh/Broadcast:* Truyền tin hạ gục (Kill Feed) trực tiếp giữa các xe mà không cần Router trung gian.
5. **Cơ chế cân bằng game (Game Balance Mechanics):** Finite State Machine (FSM) gồm các trạng thái: Đầy máu $\rightarrow$ Giao tranh $\rightarrow$ Trúng đạn (Khựng động cơ 400ms + I-Frame bất tử 1500ms) $\rightarrow$ Hết đạn (Nạp đạn 3.5s) $\rightarrow$ Bị hạ (Quay 360 độ và khóa xe).

---

## 2. PHÂN BỔ PHẦN CỨNG & SƠ ĐỒ CHÂN (GPIO PINOUT MAP)

Vi điều khiển sử dụng: **ESP32-C3 Super Mini** (RISC-V 160MHz).

| Chân GPIO | Ngoại vi ESP32 | Chức năng phần cứng | Vai trò kỹ thuật & Giới hạn |
| :--- | :--- | :--- | :--- |
| **GPIO 0** | LEDC Channel 0 | DRV8833 IN1 | Động cơ Trái - Quay tiến |
| **GPIO 1** | LEDC Channel 1 | DRV8833 IN2 | Động cơ Trái - Quay lùi |
| **GPIO 2** | LEDC Channel 2 | DRV8833 IN3 | Động cơ Phải - Quay tiến (Strapping pin: Không kéo LOW lúc boot) |
| **GPIO 3** | LEDC Channel 3 | DRV8833 IN4 | Động cơ Phải - Quay lùi |
| **GPIO 4** | RMT TX Channel 0 | Cực B Transistor 2N2222 | Phát chùm tia hồng ngoại IR 940nm điều chế sóng mang 38kHz |
| **GPIO 5** | GPIO INT / RMT RX | Chân OUT cảm biến TSOP38238 | Bắt tín hiệu trúng đạn tích cực mức LOW |
| **GPIO 6** | Digital Output | Data In LED RGB WS2812B | Đèn màu phe (Xanh/Đỏ) & hiệu ứng trúng đạn, bất tử |
| **GPIO 7** | LEDC Channel 4 | Chân Còi Buzzer thụ động | Phát âm thanh bắn đạn, nổ máy, còi báo động |
| **GPIO 8** | Digital Output | DRV8833 `nSLEEP` *(Đề xuất)* | HIGH: Mở công suất; LOW: Ngắt điện động cơ tiết kiệm pin |

---

## 3. ĐẶC TẢ GIAO THỨC & KHUNG TRUYỀN DỮ LIỆU

### 3.1. Giao thức đạn hồng ngoại IR (16-bit Payload Frame)
- **Tần số sóng mang:** 38kHz (Duty cycle 33% để bảo vệ LED phát IR).
- **Mã hóa:** Pulse Distance Modulation (NEC-like).
  - *Leader Pulse:* 4.5ms HIGH (38kHz) + 4.5ms LOW.
  - *Bit 0:* 560µs HIGH + 560µs LOW.
  - *Bit 1:* 560µs HIGH + 1690µs LOW.
- **Cấu trúc gói tin 16-bit:**
  $$\text{Data} = [\text{Team ID: 4b}] \;\|\; [\text{Player ID: 4b}] \;\|\; [\text{Damage: 4b}] \;\|\; [\text{Checksum: 4b}]$$
- **Quy tắc kiểm tra hợp lệ:**
  1. $\text{Checksum} == (\text{Team ID} \oplus \text{Player ID} \oplus \text{Damage}) \ \& \ 0x0F$. Nếu sai $\rightarrow$ loại bỏ do nhiễu phản xạ.
  2. $\text{Team ID (đạn)} == \text{Team ID (xe)} \rightarrow$ loại bỏ (chống bắn nhầm đồng đội).
  3. Nếu xe đang trong thời gian I-Frame ($1500\text{ms}$) $\rightarrow$ bỏ qua không tính sát thương.

### 3.2. Giao thức điều khiển WebSocket (Web Client $\leftrightarrow$ Xe)
- **Chu kỳ gửi lệnh (Client $\rightarrow$ Xe):** 20Hz ($50\text{ms}$/lần).
  ```json
  {"type": "drive", "left": 180, "right": -180}
  {"type": "fire"}
  ```
- **Cập nhật trạng thái HUD (Xe $\rightarrow$ Client):** Gửi khi có thay đổi trạng thái (trúng đạn, nạp đạn, chết):
  ```json
  {"hp": 4, "maxHp": 5, "ammo": 7, "maxAmmo": 10, "state": "BATTLE", "isStunned": false}
  ```
- **Cơ chế Fail-safe an toàn:** Nếu sau $300\text{ms}$ xe không nhận được gói tin `drive` từ Web Client, động cơ tự động ngắt về `0` để tránh xe tự trôi mất kiểm soát.

---

## 4. TIÊU CHÍ NGHIỆM THU RÕ RÀNG (VERIFIABLE SUCCESS CRITERIA)

Dựa trên nguyên tắc *Goal-Driven Execution*, mỗi module bắt buộc phải đạt tiêu chí định lượng sau:

| Module | Tiêu chí nghiệm thu định lượng (Pass/Fail) | Phương pháp kiểm chứng |
| :--- | :--- | :--- |
| **Nguồn & Động lực** | Đảo chiều động cơ tức thời từ $+100\%$ sang $-100\%$ không làm sụt áp gây reset ESP32 (Vdd luôn $\ge 3.0\text{V}$). | Đo điện áp nguồn cấp 3.3V của ESP32 qua chân 3V3 / Monitor Serial không có log "Brownout reset". |
| **Quang học IR** | Bắn trúng bia TSOP ở cự ly $3\text{m} - 5\text{m}$ trong góc lệch $\le 15^\circ$; không bắt đạn khi lệch góc $> 25^\circ$. | Test bắn thực tế với nòng gom tia và in log Serial nhận dạng frame. |
| **Lọc đạn & Checksum** | Tỉ lệ giải mã đúng $\ge 98\%$ gói tin trực diện; $100\%$ gói tin sai checksum bị từ chối. | Bắn 100 phát liên tiếp, đếm số gói nhận được trên Serial. |
| **Web Latency** | Độ trễ từ khi ngón tay chạm màn hình tới khi bánh xe phản hồi $\le 60\text{ms}$ qua Wi-Fi AP. | Kiểm tra log timestamp ping-pong WebSocket. |
| **Fail-Safe** | Xe dừng hẳn trong vòng $350\text{ms}$ khi ngắt Wi-Fi điện thoại đột ngột. | Tắt Wi-Fi điện thoại lúc xe đang chạy, quan sát động cơ dừng ngay lập tức. |
