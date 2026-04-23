# KẾ HOẠCH CHI TIẾT DỰ ÁN ESP-NOW SWITCH (END-NODE)

## 1. Kiến trúc hệ thống và Nguyên lý hoạt động

Dự án **ESPNowSwitch** đóng vai trò là "Thiết bị đầu cuối" (End-Node) trong hệ sinh thái Gateway Hub Home. Nhiệm vụ chính của nó là trực tiếp đóng/cắt dòng điện 220V cho các tải tiêu thụ (bóng đèn, quạt...) dựa trên lệnh không dây hoặc thao tác bấm vật lý.

**Đặc điểm cốt lõi:**
- **Không sử dụng Wi-Fi truyền thống:** Vi điều khiển được lập trình tắt hoàn toàn chức năng dò tìm kết nối Wi-Fi (STA/AP). Điều này giúp tiết kiệm tối đa điện năng, triệt tiêu nhiệt lượng tỏa ra khi nhét mạch vào trong hộp âm tường kín, đồng thời loại bỏ rủi ro rớt mạng do sóng Wi-Fi yếu.
- **Giao tiếp thuần ESP-NOW:** Công tắc kết nối trực tiếp điểm - điểm (Point-to-Point) với Gateway Hub bằng sóng vô tuyến ESP-NOW tốc độ cao (độ trễ ~vài ms).
- **Phản hồi 2 chiều (Bi-directional):** 
  - *Nhận:* Lắng nghe lệnh điều khiển từ Hub để nhảy Relay.
  - *Gửi:* Bất cứ khi nào trạng thái Relay thay đổi (do Hub ra lệnh hoặc do người dùng dập công tắc tay), mạch lập tức bắn lại một gói tin báo cáo trạng thái (Status Report) về Hub để đồng bộ giao diện Web.

---

## 2. Thiết kế Phần cứng (Hardware)

Thiết kế mạch hướng tới sự nhỏ gọn tối đa để vừa vặn trong đế âm tường chuẩn chữ nhật hoặc vuông.

- **Vi điều khiển (MCU):** Sử dụng chip **ESP8266** (Module ESP-12F hoặc ESP-01S + board chuyển). Đây là lựa chọn hoàn hảo về chi phí cho thiết bị đầu cuối, trong khi vẫn hỗ trợ đầy đủ bộ thư viện ESP-NOW độc lập.
- **Mạch Nguồn (AC-DC):** Khối nguồn xung siêu nhỏ (SMPS) như Hi-Link lõi 5V (HLK-PM01) hoặc các module hạ áp 220V-5V/3.3V cách ly. Cần thiết kế an toàn dòng rò.
- **Khối Thực thi (Công suất):** Relay 5V (đóng ngắt 10A - 220VAC) kết hợp với Transistor/Opto-isolator để kích dòng, cách ly an toàn mạch điều khiển số và điện lưới.
- **Giao diện Người dùng (UI Vật lý):**
  - Nút nhấn cơ (Tact switch) hoặc cảm ứng điện dung (TTP223) dùng để bật tắt trực tiếp.
  - 1 LED báo hiệu trạng thái Relay và tình trạng kết nối.

---

## 3. Kiến trúc Firmware (Arduino Core ESP32/ESP8266)

Firmware được phát triển theo mô hình Non-blocking (Không chặn luồng), tận dụng các hàm callback của hệ thống mạng. Mã nguồn bám sát tiêu chuẩn C++ hướng đối tượng trên nền tảng Arduino.

### 3.1. Cấu trúc Gói tin (Payload) - Khớp với Gateway Hub
Dữ liệu gửi/nhận phải đồng nhất kích thước và định dạng với cấu trúc đã khai báo trên Hub trung tâm.
```cpp
typedef struct __attribute__((packed)) {
    uint8_t msg_type; // 1: Lệnh điều khiển (Control) từ Hub, 2: Báo cáo trạng thái (Report) từ Node
    uint8_t state;    // 0: OFF, 1: ON, 2: Toggle
} espnow_payload_t;
```

### 3.2. Luồng Xử lý ESP-NOW (Rx/Tx)
- **Cấu hình ban đầu:** Chỉ kích hoạt kênh sóng (Channel) cứng cố định (thưởng là kênh 1 hoặc kênh của Hub). Đăng ký địa chỉ MAC của Gateway Hub làm Master Peer mặc định.
- **Tiếp nhận lệnh (OnDataRecv):** Khi có sóng từ Hub bay tới với `msg_type = 1`:
  - Giải mã JSON/Struct lấy giá trị `state`.
  - Ra lệnh `digitalWrite()` cập nhật trạng thái GPIO kích Relay và LED.
  - Lập tức gọi hàm báo cáo trạng thái gửi về Hub trung tâm.
- **Báo cáo (OnDataSent):** Khi Relay thay đổi, đóng gói dữ liệu với `msg_type = 2` và `state` hiện tại, gọi `esp_now_send()` ném vào mặt của Hub.

### 3.3. Xử lý Nút nhấn / Tác động vật lý
- Quét nút nhấn trong hàm `loop()` với kỹ thuật **Debounce bằng millis()** (khoảng 50-100ms) để lọc nhiễu dội phím. (Tuyệt đối không dùng `delay()`).
- Bắt sự kiện nhấn phím (Rising/Falling edge):
  - Kích đảo trạng thái Relay (Toggle).
  - Tự động gọi hàm bắn ESP-NOW báo cáo trạng thái mới về Hub để Web / App cập nhật giao diện nốt.

### 3.4. Bộ máy Cấu hình & Lưu trữ MAC trung tâm (Provisioning Phase)
- Để mạch biết gửi sóng đi đâu, trong chip sẽ dùng lõi tĩnh `<EEPROM.h>` để lưu địa chỉ mảng 6 Bytes MAC của Hub.
- **Run Mode (Chạy bình thường):** Khi bật điện lên, kiểm tra EEPROM có dữ liệu hợp lệ không. Có thì moi ra, tắt WiFi truyền thống, nạp thẳng MAC vô danh sách ESP-NOW rồi chạy.
- **Setup Mode (Chế độ cài đặt):** Nếu EEPROM rỗng hoặc bạn **Đè chặt công tắc tay 5 giây**, mạch lập tức huỷ ESP-NOW, bật trạm biến thành cục phát WiFi (Cấu hình `WiFi.softAP()`). Bạn dùng điện thoại bắt WiFi đó, vô trang Web nội bộ trên MCU gõ chữ MAC của cục Gateway Hub dán vào. Mạch lưu chữ đó lại -> Khởi động khởi thu sóng.

---

## 4. Lộ trình triển khai Firmware (5 Bước)

* **Bước 1: Code Framework Điều khiển Cục bộ (Cơ bắp)**
  - Mở dự án bằng Arduino IDE/PlatformIO. Gắn chân `PIN_RELAY`, `PIN_BUTTON`.
  - Phá vỡ tư duy `delay()`, dùng `millis()` viết hàm Debounce đọc nút nhấn và gạt chuyển Relay một cách bóng mượt.
  
* **Bước 2: Xây WebServer Cấu hình Trên ESP8266 (Bộ não)**
  - Viết Code mở cờ `WiFi.softAP()`.
  - Dùng `<ESP8266WebServer.h>` tạo một trang dạng `<form>` đơn giản để nhập MAC của Gateway. Lưu mảng MAC đó tĩnh xuống `<EEPROM.h>`.

* **Bước 3: Tích hợp Lõi ESP-NOW & Định tuyến (Gắn cánh)**
  - Thêm `#include <espnow.h>`. Đọc EEPROM lấy mảng MAC ra.
  - Viết lệnh `WiFi.mode(WIFI_STA)` sau đó ép ngắt kết nối `WiFi.disconnect()` (Chế độ tiết kiệm năng lượng).
  - Chạy `esp_now_init()` và nạp chùm địa chỉ MAC moi ở EEPROM vào danh sách hạt giống.

* **Bước 4: Đồng bộ hoá Truyền nhận Data đôi bờ (Nối hệ thần kinh)**
  - Viết hàm `OnDataRecv`: Ép kiểu dữ liệu nhận được vào Struct `espnow_payload_t`. Thêm logic "Nhận lệnh -> Nhảy Relay".
  - Viết hàm `OnDataSent`: Kết thúc quy trình khi bị chọc tay vào mạch, relay kêu xạch 1 phát, thì cuối dòng ném một câu `esp_now_send(HUB_MAC, ...)` để báo án.
  
* **Bước 5: Thử nghiệm kịch bản Edge-cases & Hard-Test (Bắt Bug)**
  - Cắt điện cục Hub tổng -> Ấn công tắc xem mạng ESP-NOW có treo chip không.
  - Test spam chọc nút bấm bạo lực liên tục xem vi điều khiển ESP8266 có bị Restart vì quá tải luồng dữ liệu (Catch WDT Reset) hay không. Tối ưu lại RAM là xong.
