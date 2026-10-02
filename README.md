# MICRO TANK ARENA (ESP32-C3 LASER TAG)

Hệ thống xe tăng chiến đấu mini điều khiển hai động cơ độc lập qua giao diện Web thời gian thực (hỗ trợ cả iOS và Android mà không cần cài đặt App), tích hợp súng - giáp hồng ngoại (IR) mã hóa 16-bit, âm thanh buồng lái và đa chế độ mạng tự động (Standalone SoftAP Captive Portal + Arena Router + ESP-NOW).

---

## ⚡ HƯỚNG DẪN BẮT ĐẦU NHANH (QUICK START)

1. **Cắm mạch ESP32-C3 Super Mini** vào máy tính qua cáp USB Type-C.
2. **Nạp Firmware** bằng lệnh PlatformIO:
   ```powershell
   & "C:\Users\phamn\.platformio\penv\Scripts\pio.exe" run -t upload
   ```
3. **Kết nối & Lái xe:**
   - Dùng điện thoại mở Wi-Fi và kết nối vào mạng `Tank_Blue_01` (không cần mật khẩu).
   - Trang điều khiển buồng lái chiến đấu (Tactical Cockpit) sẽ **tự động bung ra trên màn hình điện thoại** (Captive Portal). Nếu không tự mở, gõ `192.168.4.1` vào trình duyệt Safari/Chrome.

---

## 📚 TÀI LIỆU KỸ THUẬT & HƯỚNG DẪN CHO LẬP TRÌNH VIÊN / AI

Khi cần chỉnh sửa bất kỳ tính năng nào, **hãy tra cứu các tài liệu dưới đây thay vì đọc lại toàn bộ code**:

- 📖 **[Sổ tay Kỹ thuật Hệ thống (TECHNICAL_MANUAL.md)](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/docs/TECHNICAL_MANUAL.md)**: Sơ đồ kiến trúc, bảng phân bổ chân GPIO, mô hình máy trạng thái (FSM), giao thức đạn hồng ngoại 16-bit và WebSocket 20Hz.
- ⚡ **[Hướng dẫn Chỉnh sửa Nhanh (QUICK_MODIFICATION_GUIDE.md)](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/docs/QUICK_MODIFICATION_GUIDE.md)**: Bảng tra cứu cấp tốc "Yêu cầu thay đổi $\rightarrow$ Tệp và dòng cần sửa" (Đổi chân GPIO, đổi HP, đổi số đạn, đổi tốc độ, chỉnh sửa giao diện Web).
- 🎯 **[Mục tiêu Dự án & Tiêu chí Nghiệm thu (PROJECT_GOALS.md)](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/docs/PROJECT_GOALS.md)**: Các tiêu chuẩn kỹ thuật định lượng và rào chắn bảo vệ phần cứng nhúng.
- 🛡️ **[Quy tắc Phát triển Karpathy Guidelines (CLAUDE.md / GEMINI.md)](file:///c:/Users/phamn/Documents/PlatformIO/TankMini/CLAUDE.md)**: 4 nguyên tắc Andrej Karpathy + 4 rào chắn chống sụt áp và chống vòng lặp vô hạn.

---

## 🎮 TÍNH NĂNG NỔI BẬT

- **Hệ thống Âm thanh Thiết giáp Cơ học Thực tế (Realistic Tactical Audio Engine):**
  - **Trên phần cứng xe (ESP32-C3 LEDC PWM Loa mini / Passive Buzzer):** Hoàn toàn không còn tiếng "tít tít" điện tử. Mô phỏng tiếng máy nổ Diesel gằn nhịp nhàng khi dừng chờ ($46\text{Hz}$) và rồ ga tăng dần theo tốc độ xe ($46\text{Hz} - 135\text{Hz}$), tiếng pháo nổ gằn đanh dứt khoát, đại bác kép rung chuyển, đạn dội giáp thép, xích tải nạp đạn cơ khí, chuông lên nòng và tiếng sụp máy tử trận.
  - **Trên Buồng lái Web (Web Audio API 9 Hiệu ứng):** Tự động phát âm thanh vòm đồng bộ qua loa điện thoại kết hợp phản hồi rung xúc giác (Haptic Vibration) khi lái xe, bắn pháo hoặc trúng đạn.
- **Điều khiển Linh hoạt (360° Joystick & Dual Tread Levers):** Hỗ trợ chuyển đổi mượt mà giữa cần lái ảo 360° công thái học và 2 cần gạt xích dọc độc lập, cho phép quay tròn tại chỗ cực nhanh.
- **Khay đạn pháo đồ họa trực quan (Visual 10-Shell Rack):** Mô phỏng 10 vỏ đạn pháo phát sáng, co lại khi bắn và trượt nạp lại khi thay đạn.
- **Nút Nạp đạn chủ động (Manual Reload):** Tự nạp đạn bất cứ lúc nào kèm âm thanh cơ cấu nạp 3 nhịp cơ khí.
- **Chiêu Cuối Đại Bác (Ultimate Ability - Overload Blast):** Tích nộ $100\%$ để bắn đạn xung kích gây 3 sát thương kèm hiệu ứng giật nòng cực mạnh và âm thanh kép rung chuyển.
- **Hồ sơ Xe & Bảng Cài đặt Buồng Lái (Settings ⚙ & Tank Profile):** Tùy chỉnh Callsign (Biệt danh xe), Phe (Xanh/Đỏ), ID xe lưu vĩnh viễn vào NVS Flash; hỗ trợ đảo chiều động cơ Trái/Phải, bật/tắt rung haptic và bật/tắt âm thanh buồng lái.
