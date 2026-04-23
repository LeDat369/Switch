#include "ConfigManager.h"
#include <string.h>

static const char* TAG = "CONFIG_MNP";

ConfigManager::ConfigManager() {
    _isConfigured = false;
    memset(_gatewayMac, 0, sizeof(_gatewayMac));
}

void ConfigManager::init() {
    Serial.printf("[%s] Initializing EEPROM (Allocating %d bytes)...\n", TAG, EEPROM_SIZE);
    
    // ESP8266 yêu cầu phải .begin() với kích cỡ bộ nhớ trước khi đọc/ghi
    EEPROM.begin(EEPROM_SIZE);
    
    uint8_t flag = EEPROM.read(ADDR_FLAG);
    
    if (flag == FLAG_VALID) {
        _isConfigured = true;
        for (int i = 0; i < 6; i++) {
            _gatewayMac[i] = EEPROM.read(ADDR_MAC + i);
        }
        
        Serial.printf("[%s] FOUND Gateway MAC in EEPROM: %02X:%02X:%02X:%02X:%02X:%02X\n", TAG,
                      _gatewayMac[0], _gatewayMac[1], _gatewayMac[2],
                      _gatewayMac[3], _gatewayMac[4], _gatewayMac[5]);
    } else {
        _isConfigured = false;
        Serial.printf("[%s] NO Gateway MAC found. Circuit needs provisioning! (Flag: 0x%02X)\n", TAG, flag);
    }
}

bool ConfigManager::isConfigured() const {
    return _isConfigured;
}

const uint8_t* ConfigManager::getGatewayMac() const {
    return _gatewayMac;
}

void ConfigManager::saveMac(const uint8_t* mac) {
    if (mac == nullptr) return;

    Serial.printf("[%s] Saving new Gateway MAC: %02X:%02X:%02X:%02X:%02X:%02X to EEPROM...\n", TAG,
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Ghi 6 byte MAC vào vị trí index 0 -> 5
    for (int i = 0; i < 6; i++) {
        EEPROM.write(ADDR_MAC + i, mac[i]);
        _gatewayMac[i] = mac[i]; // Cập nhật biến RAM nội bộ luôn
    }
    
    // Ghi Cờ hiệu đã lưu (Byte số 6)
    EEPROM.write(ADDR_FLAG, FLAG_VALID);
    _isConfigured = true;
    
    // Yêu cầu Commit lệnh xuống Flash bộ nhớ cứng
    if (EEPROM.commit()) {
        Serial.printf("[%s] Save & Commit SUCCESS.\n", TAG);
    } else {
        Serial.printf("[%s] ERROR: Save & Commit FAILED!\n", TAG);
    }
}

void ConfigManager::clear() {
    Serial.printf("[%s] Wiping Config from EEPROM...\n", TAG);
    
    // Đè byte số 6 thành giá trị không hợp lệ (VD: 0x00)
    EEPROM.write(ADDR_FLAG, 0x00);
    _isConfigured = false;
    memset(_gatewayMac, 0, sizeof(_gatewayMac));
    
    if (EEPROM.commit()) {
         Serial.printf("[%s] EEPROM wiped successfully. Device is unconfigured now.\n", TAG);
    } else {
         Serial.printf("[%s] ERROR: Wipe commit failed!\n", TAG);
    }
}
