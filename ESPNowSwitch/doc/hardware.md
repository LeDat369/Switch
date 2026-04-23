# TÀI LIỆU THIẾT KẾ PHẦN CỨNG ESP-NOW SWITCH

Tài liệu này mô tả chi tiết các thành phần phần cứng, sơ đồ nguyên lý và lưu ý an toàn khi thiết kế mạch công tắc thông minh âm tường (End-Node) sử dụng công nghệ ESP-NOW.

## 1. Yêu cầu thiết kế tổng quan (Design Requirements)
- **Kích thước:** Phải lọt vừa đế âm tường tiêu chuẩn (hình vuông 86x86mm hoặc chữ nhật 118x73mm). Không gian bên trong đế vô cùng chật hẹp, cần mạch (PCB) tối ưu diện tích.
- **An toàn điện (Safety):** Mạch hoạt động trực tiếp với điện lưới 220VAC. Bắt buộc rạch rãnh PCB (Milling) để cách ly quang/từ giữa vùng điện áp cao (High Voltage) và điện áp thấp (Low Voltage).
- **Quản lý nhiệt:** Module cấp nguồn (SMPS) và Relay sẽ sinh nhiệt. Việc tắt WiFi chuyển sang dùng ESP-NOW đã giúp ESP8266 mát hơn rất nhiều, nhưng vẫn cần bố trí linh kiện thoáng khí.

---

## 2. Danh sách linh kiện cốt lõi (BOM - Bill of Materials)

### 2.1. Khối Vi điều khiển (MCU)
- **Lựa chọn:** Module **ESP8266 (ESP-12F)**.
- **Ưu điểm:** Giá thành cực rẻ, tích hợp sẵn anten PCB gai, đủ chân GPIO cho một công tắc 1-3 nút. Tương thích hoàn hảo với thư viện ESP-NOW của khuôn khổ Espressif.

### 2.2. Khối Nguồn (Power Supply AC-DC & DC-DC)
- **Nguồn cấp chính (AC-DC):** Phổ biến nhất là module **Hi-Link HLK-PM01** (220VAC -> 5VDC / 600mA) hoặc các mạch SMPS tháo rời tương đương. Đảm bảo nguồn đủ khoẻ để gánh dòng khởi động của ESP8266 và cuộn hút Relay.
- **Nguồn MCU (DC-DC):** IC hạ áp tuyến tính **AMS1117-3.3** hoặc **HT7333**. Giảm từ 5V xuống 3.3V để nuôi ESP8266. 
- *Bổ trợ:* Tụ hóa (vd: 470µF hoặc 1000µF) và tụ gốm (104) để lọc nhiễu, chống sụt áp khi ESP-NOW phát sóng (Brownout).

### 2.3. Khối Chấp hành (Relay & Isolation)
- **Rơ-le (Relay):** Relay 5V DC (ví dụ form SRD-05VDC), chịu tải đầu ra tối đa 10A/250VAC.
- **Mạch kích & Cách ly:** 
  - Optocoupler **PC817** để cách ly quang học hoàn toàn chân GPIO của chip với mạch kích relay (bảo vệ MCU nếu lỡ rò điện).
  - Transistor NPN (S8050 hoặc 2N3904) làm nhiệu vụ đóng cắt dòng 5V cho cuộn hút Relay.
  - Điod dập xung ngược (Flyback Diode) **1N4148** hoặc **1N4007** móc song song cuộn dây Relay để bảo vệ Transistor xả cảm ứng tĩnh điện.

### 2.4. Khối Giao tiếp (UI)
- **Nút nhấn vật lý:** Các Tact Switch (Nút nhấn nhả) màng cao su nảy hoặc mạch chạm điện dung **TTP223** nếu làm mặt kính cảm ứng.
- **Đèn trạng thái:** 1-2 con LED SMD (ví dụ 0805 Xanh/Đỏ) báo trạng thái cấp điện hoặc kết nối.

---

## 3. Giao tiếp GPIO & Lưu ý khi thiết kế sơ đồ (Schematic)

ESP8266 có một số chân GPIO mang chức năng đặc biệt khi Boot (Khởi động). Việc đấu nối Relay hoặc Nút bấm sai chân sẽ khiến vi điều khiển "Treo" hoặc không khởi động được.

**Khuyến nghị mapping GPIO an toàn:**
- **Không dùng:** `GPIO 0`, `GPIO 2` (Định tuyến Boot mode), `GPIO 15` (Phải kéo xuống GND khi boot).
- **Khuyên dùng cho Nút bấm (Input):** `GPIO 4`, `GPIO 5`, `GPIO 14`. Bật trạng thái điện trở nội kéo lên (Pull-up) trong code.
- **Khuyên dùng cho Relay/LED (Output):** `GPIO 12`, `GPIO 13`.
- **Chân Cấu hình (Boot/Flash):** `GPIO 0` có thể dùng làm nút Config/Reset giữ 5 giây, vì chập GND lúc khởi động sẽ vào chế độ nạp Code, nhưng khi đã chạy xong `setup()` thì đọc ngõ vào thoải mái.

---

## 4. Thiết kế Mạch in (PCB Layout) & An toàn điện lưới

Khi vẽ PCB hoặc hàn mạch lỗ, cần tuyệt đối tuân thủ các quy tắc điện áp cao:
1. **Khoảng cách bò viền (Creepage Distance):** Khoảng cách giữa các đường mạch 220V với các đường mạch 5V/3.3V phải tối thiểu **> 3mm**. Tốt nhất là thiết kế rạch rãnh không khí (Milling Cut) trên PCB, đặc biệt là ngay dưới bụng ngàm cách ly của con Opto PC817 và dưới Rơ-le.
2. **Đường mạch tải (Trace Width):** Các đường nối Cổng đầu vào (IN - Điện lưới L/N) đến Rơ-le và ra bóng đèn (OUT) phải phủ thiếc trần, hoặc dùng đường mạch cực rộng (ví dụ > 80-100 mil) để đáp ứng dòng tải lớn (đóng bóng sợi đốt, motor).
3. **Cầu chì bảo vệ (Fuse/Varistor):** Nên có một điện trở nhiệt (NTC/PTC) hoặc cầu chì thuỷ tinh 1A-2A trước module nguồn HLK-5V để phòng điện áp tăng vọt hoặc cháy nổ module nguồn hạ áp.

---

## 5. Lắp ráp và Triển khai

- **Kiểm định Nguồn rời:** Luôn cắm điện và dùng đồng hồ VOM đo chân đầu ra của khối nguồn xem có ra đúng 5V định mức hay không trước khi nối mạch cấp vào ESP8266.
- **Nạp Firmware mạch kín:** Để tiện cho việc sửa lỗi, trên PCB nên để hờ 4 chân cắm chờ (Header pogo pin): `3.3V`, `GND`, `Rx`, `Tx`, cắm sẵn luôn cả chân `GPIO 0` với tiếp điểm hàn jumper xuống GND để cắm nạp code dễ dàng mà không cẩn tháo chip ESP-12F.
- **Chống ẩm (Conformal Coating):** Do nằm hốc tường, dễ hấp hơi nước trong các đợt nồm ẩm (Ở môi trường Việt Nam). Nếu sản xuất chuyên nghiệp mảng mạch điện tử, cần phủ thêm một lớp keo chống ẩm/phun phủ Acrylic lên vùng gắn MCU sau khi kiểm định (Test) pass các chức năng.