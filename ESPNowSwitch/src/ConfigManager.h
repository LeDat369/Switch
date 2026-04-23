#pragma once
#include <Arduino.h>
#include <EEPROM.h>

class ConfigManager {
private:
    uint8_t _gatewayMac[6];
    bool _isConfigured;

    // Các hằng số về địa chỉ ô nhớ EEPROM và kích thước
    static const uint16_t EEPROM_SIZE = 32; // Chỉ cần 32 bytes là quá dư dả
    static const uint16_t ADDR_MAC = 0;     // Byte 0-5: Chứa địa chỉ MAC
    static const uint16_t ADDR_FLAG = 6;    // Byte 6: Cờ đánh dấu cấu hình

    static const uint8_t FLAG_VALID = 0xAA; // 0xAA nghĩa là đã lưu cấu hình

public:
    ConfigManager();
    
    // Khởi tạo EEPROM và tải dữ liệu MAC (nếu có)
    void init();

    // Kiểm tra xem mạch đã được cài đặt MAC trung tâm hay chưa
    bool isConfigured() const;

    // Lấy con trỏ/mảng chứa chuỗi 6 bytes địa chỉ MAC của Gateway
    const uint8_t* getGatewayMac() const;

    // Nhận mảng 6 bytes MAC và lưu xuống bộ nhớ flash vi điều khiển
    void saveMac(const uint8_t* mac);

    // Xoá cấu hình hiện tại để đưa mạch về chế độ Setup
    void clear();
};
