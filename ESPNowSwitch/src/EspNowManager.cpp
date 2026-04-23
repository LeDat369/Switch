#include "EspNowManager.h"

static const char* TAG = "ESPNOW_MNP";

// Cấp phát con trỏ tĩnh cho ESP-NOW Callbacks
EspNowManager* EspNowManager::_instance = nullptr;

EspNowManager::EspNowManager(ConfigManager* configMgr, HardwareHandler* hwHandler) {
    _configManager = configMgr;
    _hardwareHandler = hwHandler;
    
    _rxFlag = false;
    _txFlag = false;
    _txStatus = 0;
    
    memset(&_rxPayload, 0, sizeof(_rxPayload));
    memset(_rxMac, 0, sizeof(_rxMac));

    _instance = this; // Gán instance hiện tại cho callback tĩnh
}

void EspNowManager::init() {
    Serial.printf("[%s] Initializing ESP-NOW Radio...\n", TAG);

    // 1. Phải ngắt hoàn toàn kết nối WiFi chặn để tối ưu nhiễu sóng và tản nhiệt
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Khởi tạo thư viện ESP-NOW
    if (esp_now_init() != 0) {
        Serial.printf("[%s] ERROR: ESP-NOW Init Failed! Restarting chip in 2 seconds...\n", TAG);
        delay(2000); // Lỗi lõi phần cứng, trường hợp này cho phép delay cứu vãn trước khi restart
        ESP.restart();
    }

    // 2. Thiết lập Role
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO); // Vừa gửi vừa nhận

    // Đăng ký Callbacks
    esp_now_register_recv_cb(_onDataRecv);
    esp_now_register_send_cb(_onDataSent);

    // 3. Đăng ký Master Peer (Hub trung tâm) vào bảng định tuyến từ EEPROM
    const uint8_t* gwMac = _configManager->getGatewayMac();
    
    int addStatus = esp_now_add_peer((uint8_t*)gwMac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
    if (addStatus == 0) {
        Serial.printf("[%s] Registered Gateway MAC as Peer: %02X:%02X:%02X:%02X:%02X:%02X\n", TAG,
                      gwMac[0], gwMac[1], gwMac[2], gwMac[3], gwMac[4], gwMac[5]);
    } else {
        Serial.printf("[%s] ERROR: Failed to add Gateway Peer (%d)\n", TAG, addStatus);
    }
}

void EspNowManager::update() {
    // Luân phiên quét luồng Dữ liệu Nhận Data từ ngắt sang vòng lặp chuẩn (Non-blocking ISR Safety)
    if (_rxFlag) {
        _rxFlag = false;
        
        Serial.printf("[%s] RX Packet <- MAC %02X:%02X:%02X:%02X:%02X:%02X | Type: %d | Relay CMD: %d\n", TAG,
                      _rxMac[0], _rxMac[1], _rxMac[2], _rxMac[3], _rxMac[4], _rxMac[5],
                      _rxPayload.msgType, _rxPayload.state);

        // Xử lý gói tin lệnh đổi rơ-le từ Gateway (msgType = 1)
        if (_rxPayload.msgType == 1) {
            
            // Cập nhật Hardware (Ép đổi màu rơ le)
            if (_rxPayload.state == 2) {
                _hardwareHandler->toggleRelay();
            } else if (_rxPayload.state == 0 || _rxPayload.state == 1) {
                _hardwareHandler->setRelay(_rxPayload.state);
            }
            
            // Đồng thời bắt thiết bị phải gửi trả kết quả Relay mới nhất
            reportState();
        }
    }

    // Xử lý thông báo kết quả Gửi Data từ ngắt (OnDataSent)
    if (_txFlag) {
        _txFlag = false;
        Serial.printf("[%s] TX Result -> Gateway: %s\n", TAG, _txStatus == 0 ? "SUCCESS" : "FAILED_TIMEOUT");
    }
}

void EspNowManager::reportState() {
    EspNowPayload_t sendPayload;
    sendPayload.msgType = 2; // Báo cáo trạng thái Node lên Hub (Report)
    sendPayload.state = _hardwareHandler->getRelayState();

    const uint8_t* gwMac = _configManager->getGatewayMac();
    
    Serial.printf("[%s] Sending Report [Type: 2, State: %d] to Gateway...\n", TAG, sendPayload.state);
    
    int result = esp_now_send((uint8_t*)gwMac, (uint8_t*)&sendPayload, sizeof(sendPayload));
    if (result != 0) {
         Serial.printf("[%s] ERROR: ESP-NOW Transmit Call failed immediately (Code: %d)\n", TAG, result);
    }
}

// ------------------------------------------------------------------------------------------
// LƯU Ý VỀ AN TOÀN NGẮT (ISR): Các hàm callback này chạy dưới dạng System Task. 
// TUYỆT ĐỐI KHÔNG IN LOG Serial hay chạy hàm nặng ở đây. Chỉ bật cờ (FLAG) cho loop().
// ------------------------------------------------------------------------------------------

void IRAM_ATTR EspNowManager::_onDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
    if (_instance == nullptr) return;
    
    // Check nếu độ dài gói lệch với Struct đóng gói thì rớt luôn
    if (len != sizeof(EspNowPayload_t)) return;

    // Buffer gói tin và bật Flag
    memcpy((void*)&_instance->_rxPayload, incomingData, sizeof(EspNowPayload_t));
    memcpy((void*)_instance->_rxMac, mac, 6);
    
    _instance->_rxFlag = true;
}

void IRAM_ATTR EspNowManager::_onDataSent(uint8_t *mac, uint8_t sendStatus) {
    if (_instance == nullptr) return;

    // Lệnh này trả về gốc từ giao thức Wifi RF (sendStatus 0 = ACK Success)
    _instance->_txStatus = sendStatus;
    _instance->_txFlag = true;
}
