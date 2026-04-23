# KẾ HOẠCH TRIỂN KHAI FIRMWARE CHO ESP-NOW SWITCH (DÀNH CHO AI)

Tài liệu này là **Prompt/Bản thiết kế cốt lõi** dành cho AI Assistant. Khi được yêu cầu viết code, AI PHẢI đọc file này để nắm bắt chính xác tiến trình, cấu trúc thư mục, quy tắc lập trình và logic hệ thống của Firmware ESPNowSwitch (chạy board ESP8266 trên nền Arduino framework).

---

## 1. QUY TẮC CHUNG CHO AI (AI INSTRUCTIONS)
Để đảm bảo chất lượng source code, AI **phải áp dụng nghiêm ngặt** các quy tắc từ file `doc/rule.md`:
- **Code Style (OOP & Naming):** Sử dụng C++ Class/Object. `PascalCase` cho tên Class, `camelCase` cho biến/hàm và `UPPER_SNAKE_CASE` cho Define/Hằng số. Hàm/biến private bắt đầu bằng `_`.
- **Quản lý Bộ nhớ (No String in Runtime):** CẤM sinh ra đối tượng `String` bên trong vòng lặp `loop()` hoặc Callback (ISR). Sử dụng C-string (`char array`) tĩnh hoặc truyền tham chiếu để tránh phân mảnh Heap.
- **Xử lý Đa nhiệm & Thời gian thực (Non-blocking):** KHÔNG dùng hàm `delay()`. Toàn bộ thao tác chờ/quét nút/chớp LED phải dùng thủ thuật trừ thời gian `millis()`.
- **An toàn Ngắt (ISR Safety):** KHÔNG gọi lệnh in `Serial.print` hay thao tác tính toán nặng trong hàm ngắt. Chỉ bật cờ (flag) để hàm `loop()` xử lý. Khai báo các hàm ngắt với thuộc tính `IRAM_ATTR`.
- **Thiết kế Định tuyến WiFi (No WiFi-Blocking):** Công tắc phải ngắt hoàn toàn kết nối WiFi (STA/AP) khi đang vận hành mạch (MODE_RUN) để tiết kiệm năng lượng và tản nhiệt, chỉ bật Radio cho thư viện ESP-NOW.
- **Ghi Log chi tiết (Detailed Logging):** Bắt buộc phải in log đầy đủ (`Serial.printf`) cho mọi sự kiện quan trọng (nhận/gửi gói ESP-NOW, nút bấm, thay đổi relay, đọc/ghi EEPROM...), có đính kèm TAG tên Module để dễ debug. Chú ý: Tuyệt đối KHÔNG in log bên trong hàm ngắt (ISR).

---

## 2. CẤU TRÚC THƯ MỤC DỰ KIẾN (PIO/ARDUINO ARCHITECTURE)
AI cần tuân thủ cấu trúc rẽ nhánh OOP sau khi sinh code:

```text
ESPNowSwitch/
├── platformio.ini              # Khai báo môi trường ESP8266, thư viện, core debug level
├── src/
│   ├── main.cpp                # Điểm Entry. Điều phối vòng lặp và chuyển State.
│   ├── CommonTypes.h           # Chứa Struct Payload và các Enum dùng chung.
│   ├── ConfigManager.h/cpp     # Xử lý NVS/EEPROM (Lưu MAC Gateway).
│   ├── HardwareHandler.h/cpp   # Điều khiển Relay, đèn LED và quét nút nhấn (Debounce).
│   ├── WebProvisioning.h/cpp   # Phát SoftAP và WebServer để nhập địa chỉ MAC Gateway.
│   └── EspNowManager.h/cpp     # Core xử lý bản tin Radio (Rx/Tx) bằng espnow.h.
└── doc/
```

---

## 3. LỘ TRÌNH THỰC THI STEP-BY-STEP (EXECUTION PHASES)

Người dùng sẽ yêu cầu AI thực hiện từng Phase. AI **phải thực hiện dứt điểm, test và báo cáo OK** trước khi nhảy sang Phase tiếp theo.

### Phase 1: Mạch Cơ Bắp (HardwareHandler & CommonTypes)
**Mục tiêu:** Xây định nghĩa Struct chuẩn, làm chủ phần cứng (Relay, Button) bằng kỹ thuật triệt tiêu `delay()`.
1. Tạo `CommonTypes.h` khai báo struct `EspNowPayload_t` (có tag `__attribute__((packed))`). Khai báo các Struct/Enum trạng thái.
2. Xây dựng class `HardwareHandler`:
   - Hàm `init()`: Cấu hình `pinMode` an toàn. Cài đặt trạng thái tắt mặc định.
   - Hàm `update()`: (Gọi liên tục trong `main`). Quét nút nhấn bằng `millis()` (Debounce 50ms).
   - Bắt sự kiện: Bấm nhả (Short Press) -> Bật cờ Toggle Relay. Đè giữ > 5 giây (Hold) -> Bật cờ Reset hệ thống.
   - Hàm `setRelay(state)`: Gạt Relay và nháy LED tương ứng.

### Phase 2: Bộ Nhớ (ConfigManager)
**Mục tiêu:** Bộ nhớ tĩnh lưu trữ địa chỉ của Hub trung tâm.
1. Xây dựng Component/Class `ConfigManager` tương tác với thư viện `<EEPROM.h>`.
2. Offset byte 0-5 lưu `uint8_t mac[6]`.
3. Byte 6 làm Flag đánh dấu `0xAA` (Báo hiệu mạch đã được nạp MAC thành công).
4. Các hàm chức năng: `loadMac()`, `saveMac()`, `isConfigured()`, `clear()`.

### Phase 3: Setup Mode (WebProvisioning)
**Mục tiêu:** Cục phát WiFi nội bộ để điện thoại chui vào cài đặt mạch.
1. Xây dựng `WebProvisioning` sử dụng `<ESP8266WebServer.h>`.
2. Khởi tạo chức năng: Khi `ConfigManager.isConfigured() == false`:
   - Bật WiFi dạng SoftAP (vd `ESP_SW_Setup`).
   - Dựng trang HTML tĩnh dạng form nhập tay thông số MAC của Gateway (`XX:XX:XX...`).
   - Lưu MAC bắt được dạng String Parse thành `uint8_t[6]` rồi đẩy vào `ConfigManager.saveMac()`. Cấu hình xong -> Tự gọi `ESP.restart()`.

### Phase 4: Thần Kinh Không Dây (EspNowManager)
**Mục tiêu:** Giao tiếp vô tuyến siêu tốc, lắng nghe và phản hồi Hub trung tâm.
1. Xây dựng class `EspNowManager`. Lệnh đầu tiên phải đưa chip vào `WiFi.mode(WIFI_STA)` và `WiFi.disconnect()` liền.
2. Gọi `esp_now_init()`. Thiết đặt Role bằng `esp_now_set_self_role(ESP_NOW_ROLE_COMBO)`. Đăng ký Master Peer bằng mảng MAC lấy từ `ConfigManager`.
3. Viết Callback Nhận (`OnDataRecv`): Nhận gói `msgType = 1` -> gọi hàm của `HardwareHandler` ép đổi màu Relay, đồng thời bắt thiết bị trả lời ngược.
4. Viết hàm Gởi (`reportState`): Gửi struct gói tin (`msgType = 2`, chứa trạng thái Relay hiện tại) đi tới Gateway MAC. Chờ lấy Result trong hàm `OnDataSent`.

### Phase 5: Hợp Thể & Bắt Lỗi (Integration in Main)
**Mục tiêu:** Ráp các module trên lại tại `main.cpp`.
1. Khởi tạo toàn bộ Object/Hardware.
2. Đọc EEPROM ngay hàm `setup()`.
   - Nếu Rỗng -> Rẽ luồng trạng thái chạy `WebProvisioning.init()`.
   - Nếu Đã Có -> Rẽ luồng trạng thái chạy vòng lặp `HardwareHandler` + `EspNowManager.init()`.
3. Ở hàm `loop()`:
   - Nếu ở Chế Độ Cài Đặt (Provisioning): Chỉ chạy vòng quay WebServer client.
   - Nếu ở Chế Độ Chạy (Run Mode): Liên tục quét `Hardware.update()`. Nếu có cờ nhả Nút Bấm thì Gạt Relay -> Lập tức réo `EspNowManager.reportState()` lên Hub.
4. Quản lý lỗi tự Reset: Nếu bị dồn dập gói tin lỗi, đứt kết nối hoặc WDT báo động -> Mạch tự ngắt khởi động lại `ESP.restart()` cho an toàn.

---

## 4. HƯỚNG DẪN CHO NGƯỜI DÙNG KHI RA LỆNH CHO AI
Khi muốn AI bắt đầu code, hãy copy cú pháp sau để gán ngữ cảnh:
> *"Dựa vào `doc/firmware.md`, hãy thực hiện **Phase [X]** cho tôi. Bắt đầu bằng việc viết khai báo thư viện nếu cần, sau đó viết header `.h` và class `.cpp`. Hãy luôn kiểm tra xem code có dính hàm `delay()`, cấp phát `String` rác không, và NHỚ IN LOG ĐẦY ĐỦ các quá trình hệ thống. Chỉ chuyển sang phần tiếp theo khi tôi nhắn OK."*