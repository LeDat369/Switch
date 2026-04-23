#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

#include <Arduino.h>
#include <espnow.h>
#include <ESP8266WiFi.h>
#include "CommonTypes.h"
#include "ConfigManager.h"
#include "HardwareHandler.h"

class EspNowManager {
private:
    ConfigManager* _configManager;
    HardwareHandler* _hardwareHandler;

    // Singleton pointer dùng cho các hàm callback tĩnh toàn cục (ESP-NOW API không hỗ trợ truyền object context)
    static EspNowManager* _instance;

    // Các cờ và buffer để chuyển dữ liệu từ Hàm Ngắt (ISR) ra ngoài vòng loop (Non-blocking & ISR Safety)
    volatile bool _rxFlag;
    EspNowPayload_t _rxPayload;
    uint8_t _rxMac[6];

    volatile bool _txFlag;
    volatile uint8_t _txStatus;

    // Callback ESP-NOW nhận và gửi gói tin
    static void IRAM_ATTR _onDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len);
    static void IRAM_ATTR _onDataSent(uint8_t *mac, uint8_t sendStatus);

public:
    EspNowManager(ConfigManager* configMgr, HardwareHandler* hwHandler);

    void init();
    void update(); // Quét cờ để xử lý phần cứng và in log thay vì làm trong ngắt (ISR)
    void reportState();
};

#endif // ESPNOW_MANAGER_H
