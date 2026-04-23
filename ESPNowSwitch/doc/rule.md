# QUY TẮC LẬP TRÌNH NHÚNG ARDUINO (ESP32)

## Nguyên tắc chung

- Giữ code đơn giản, dễ hiểu theo KISS
- Chỉ implement khi cần theo YAGNI
- Không lặp lại code theo DRY
- Ưu tiên hiệu suất và tối ưu bộ nhớ
- Hạn chế tối đa cấp phát động (`new`/`delete`, `malloc`/`free`) trong `loop()` hoặc các task chạy liên tục để tránh phân mảnh bộ nhớ (Heap Fragmentation).
- Hạn chế sử dụng đối tượng `String` gốc của Arduino (nếu nối chuỗi nhiều), ưu tiên dùng C-string (`char array`) khi có thể.
- Đưa các setup cấu hình hoặc cấp phát vào thời điểm khởi động hệ thống (`setup()`).

## Kiến trúc và tổ chức mã nguồn

- Sử dụng lập trình hướng đối tượng C++ theo chuẩn Arduino.
- Cấu trúc modular với các class/component độc lập.
- Mỗi module/class nên có file `.h` (cấu trúc, định nghĩa) và `.cpp` (thực thi).
- Tránh nhồi nhét toàn bộ logic vào hàm `loop()` và `setup()` trong một file `.ino` hoặc `.cpp` duy nhất.
- Đặt tên biến, hàm, class thống nhất:
  - Class: `PascalCase` (vd: `WifiManager`, `SensorHandler`)
  - Function/Method: `camelCase` (vd: `connectWifi()`, `readData()`)
  - Variable: `camelCase` (vd: `sensorData`, `wifiConfig`)
  - Constant/Define: `UPPER_SNAKE_CASE` (vd: `MAX_RETRY`, `PIN_LED`)
  - Private member: prefix `_` (vd: `_isReady`)

## FreeRTOS và Multi-threading

- Tận dụng sức mạnh FreeRTOS của ESP32: Chia các chức năng độc lập thành các Task riêng biệt (`xTaskCreate` / `xTaskCreatePinnedToCore`).
- Hạn chế block hàm `loop()` hoặc task ưu tiên cao bằng hàm `delay()`. Dùng `vTaskDelay()` để nhường CPU cho task khác.
- Đặt priority hợp lý cho task (0-25, cao hơn = ưu tiên xử lý trước).
- Sử dụng Queue (`xQueueSend`, `xQueueReceive`) để truyền dữ liệu giữa các task thay vì biến global tràn lan.

## Đồng bộ và an toàn thread

- Dùng Mutex để bảo vệ biến dùng chung (shared buffer/resource) giữa các Task.
- Gọi các hàm API hoặc thư viện thread-safe (chú ý nhiều thư viện Arduino cũ không an toàn khi truy xuất đồng thời).
- Hàm ngắt (ISR) phải dùng các phiên bản API riêng có hậu tố `FromISR` (vd: `xQueueSendFromISR`).

## Quản lý bộ nhớ

- Kiểm tra bộ nhớ khả dụng định kỳ bằng `ESP.getFreeHeap()` thay vì để hệ thống crash ngầm.
- Tránh tạo các buffer khổng lồ trong vùng stack (biến cục bộ), ưu tiên declare dạng static nếu lưu trữ cố định lâu dài.
- Nếu phải dùng cấp phát động, cần đảm bảo có cơ chế giải phóng rõ ràng (pair new-delete).

## GPIO và Peripheral

- Sử dụng các hàm GPIO chuẩn của Arduino (`pinMode`, `digitalWrite`, `digitalRead`, `attachInterrupt`).
- Các hàm phục vụ ngắt phải đi kèm thuộc tính `IRAM_ATTR` để load thẳng vào RAM, tránh page-fault.
- Giữ hàm ngắt (ISR) cực kỳ ngắn gọn: không in log (`Serial.print`), không delay, không tính toán phức tạp. Chỉ bật cờ (flag) hoặc truyền vào Queue rồi thoát.

## Error Handling

- Ở Arduino ít khi ném exception, do đó cần bám sát giá trị trả về của các hàm (boolean, mã lỗi...).
- Validate dữ liệu đầu vào cẩn thận, đặc biệt khi nhận từ mạng qua MQTT/HTTP/ESPNow hoặc cổng Serial.
- Luôn kiểm tra kết nối WiFi/MQTT trước khi thao tác gửi lệnh.
- Không ignore error, nếu bị lỗi quá nghiêm trọng trong thời gian dài nên chạy lệnh tự khởi động lại chip (`ESP.restart()`).

## Logging

- Tối ưu ghi log: thay vì dùng `Serial.print()` tràn lan, nên định nghĩa macro Log riêng hoặc tận dụng các hàm build-in `log_e()`, `log_w()`, `log_i()`, `log_d()` (Core Debug Level).
- Tắt các Log mức độ Debug/Verbose khi build phiên bản production (Chỉnh Core Debug Level trong `platformio.ini` hoặc Arduino IDE).
- Tuyệt đối không được dùng lệnh Serial hay hàm log in ấn nào bên trong hàm Ngắt (ISR).

## Power Management

- Đưa chip vào các chế độ sleep khi đứng chờ không làm gì.
- Sử dụng `Deep Sleep` bằng `esp_deep_sleep_start()` đối với các device chạy pin lâu dài. Dữ liệu cần giữ sau giấc ngủ được đưa vào cờ `RTC_DATA_ATTR`.
- Giảm tần số chip bằng `setCpuFrequencyMhz()` khi dự án không tính toán nặng (VD: set CPU Frequency ở mức 80MHz thay vì 240MHz mặc định).

## Networking (WiFi/MQTT/ESPNow)

- Đừng làm hàm `loop()` bị đứng (blocking) vì kết nối WiFi. Xử lý reconnect trong một Task độc lập hoặc cơ chế non-blocking bằng `millis()`.
- Tận dụng `WiFi.onEvent()` của tập sinh ESP32 Arduino để xử lý các sự kiện mạng (như `SYSTEM_EVENT_STA_DISCONNECTED`) thay vì liên tục check trạng thái.
- Riêng bộ đôi giao thức mạng và ESPNow: Các hàm Callback nhận data mạng hoạt động ở một luồng đằng sau, luôn dùng Queue/Flag bắn dữ liệu ra để xử lý trong hàm main/task chính.

## Configuration Management

- Định nghĩa rõ các thông số config tĩnh tại `config.h`.
- Với các giá trị runtime cần lưu qua các lần mất điện/khởi động mảng NVS (như SSD), hãy tận dụng thư viện `<Preferences.h>` (tối ưu, theo thẻ khóa-chuỗi) thay cho `<EEPROM.h>` đã cũ cỗi.

## OTA (Over-The-Air Update)

- Trang bị tính năng nạp Firmware tự động qua nền tảng HTTP bằng thư viện `HTTPUpdate` hoặc qua mạng local với thư viện `ArduinoOTA`.
- Chú ý giữ vững WiFi và vô hiệu hoá các Task gây tốn kém RAM, Task gián đoạn phần cứng trong lúc nạp OTA để tránh hỏng thiết bị.

## Testing và Debug

- Sử dụng công cụ (plugin) *ESP32 Exception Decoder* để dò dịch và phân tích dòng code gây lỗi khi chip bị Crash và in đoạn Core Dump trên màn hình Serial.
- Luôn bật compiler warning để có những đoạn xử lý Code sạch sẽ nhất.

## Code Style

- Indent: 4 spaces (hoặc 2 spaces tuỳ thói quen, nhưng cần thống nhất cho cả project).
- Brace style: Thống nhất suốt file.
- Line length: Đề phòng để code dễ nhìn, tối đa khoảng 120 ký tự.
- Format file code thủ công định kì (dùng tính năng Auto Format / Alt+Shift+F của Editor) trước khi đưa code (commit) lên git.
