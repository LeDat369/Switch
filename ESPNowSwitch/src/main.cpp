#include <Arduino.h>
#include "CommonTypes.h"
#include "ConfigManager.h"
#include "HardwareHandler.h"
#include "WebProvisioning.h"
#include "EspNowManager.h"

// ---------------------------------------------------------
// GLOBAL INSTANCES (Triển khai đối tượng không xài con trỏ new/delete để tránh phân mảnh bộ nhớ)
// ---------------------------------------------------------
ConfigManager g_configManager;
HardwareHandler g_hardwareHandler;
WebProvisioning g_webProvisioning(&g_configManager);
EspNowManager g_espNowManager(&g_configManager, &g_hardwareHandler);

AppState g_currentMode = AppState::MODE_PROVISION;
static const char* TAG = "CORE_MAIN";

void setup() {
    Serial.begin(74880); // Baudrate chuẩn nhất của ESP8266 bootloader, tránh rác boot
    Serial.println();
    Serial.println();
    Serial.printf("[%s] =======================================\n", TAG);
    Serial.printf("[%s]        ESP-NOW SWITCH BOOTING\n", TAG);
    Serial.printf("[%s] Device MAC Address: %s\n", TAG, WiFi.macAddress().c_str());
    Serial.printf("[%s] =======================================\n", TAG);

    // 1. Phục hồi trạng thái chân IO càng sớm càng tốt để Relay không nhảy nhót
    g_hardwareHandler.init();

    // 2. Đọc bộ nhớ EEPROM
    g_configManager.init();

    // 3. Phân luồng điều khiển
    if (g_configManager.isConfigured()) {
        g_currentMode = AppState::MODE_RUN;
        Serial.printf("[%s] Boot Mode: RUN (ESP-NOW Connected)\n", TAG);
        
        // Khởi động Radio
        g_espNowManager.init();
    } else {
        g_currentMode = AppState::MODE_PROVISION;
        Serial.printf("[%s] Boot Mode: SETUP (SoftAP & WebServer)\n", TAG);
        
        // Khởi động Cục phát điểm truy cập
        g_webProvisioning.init();
    }
}

void loop() {
    if (g_currentMode == AppState::MODE_PROVISION) {
        // Phục vụ người dùng cấu hình địa chỉ MAC qua điện thoại
        g_webProvisioning.update();
        
    } else if (g_currentMode == AppState::MODE_RUN) {
        // 1. Quét thay đổi của Nút nhấn vật lý
        ButtonEvent btnEvent = g_hardwareHandler.update();

        // 2. Xử lý sự kiện từ người dùng (nhấn nút cơ)
        if (btnEvent == ButtonEvent::EVENT_TOGGLE) {
            Serial.printf("[%s] Button Toggle Detected -> Switching Relay...\n", TAG);
            g_hardwareHandler.toggleRelay();
            
            // Lập tức bắn báo cáo cấu hình mới lên Hub trung tâm
            g_espNowManager.reportState();
        } 
        else if (btnEvent == ButtonEvent::EVENT_RESET) {
            Serial.printf("[%s] Factory Reset Detected (5s Long Press) -> Wiping Memory...\n", TAG);
            g_configManager.clear();
            
            Serial.printf("[%s] System restarting to Setup Mode...\n", TAG);
            delay(10); // Cho phép delay cực nhỏ (10ms) để cổng Serial tuôn hết chữ ra máy tính
            ESP.restart();
        }

        // 3. Quét sự kiện từ hệ thống Radio (nhận từ ngắt)
        g_espNowManager.update();
    }
}