#pragma once
#include <Arduino.h>

// Struct giao tiếp ESP-NOW (Phải Pack chặt để tránh Padding)
typedef struct __attribute__((packed)) {
    uint8_t msgType; // 1: Control (Hub -> Node), 2: Report (Node -> Hub)
    uint8_t state;   // 0: OFF, 1: ON, 2: TOGGLE
} EspNowPayload_t;

// Trạng thái vận hành của thiết bị
enum class AppState {
    MODE_RUN,        // Chạy bình thường bằng ESP-NOW
    MODE_PROVISION   // Đang phát SoftAP chờ cấu hình MAC (Setup Mode)
};

// Loại sự kiện do Nút nhấn sinh ra sau khi xử lý Debounce
enum class ButtonEvent {
    EVENT_NONE,
    EVENT_TOGGLE,    // Short press -> Chuyển đổi trạng thái Relay
    EVENT_RESET      // Long press > 5s -> Xoá EEPROM, khởi động lại vào Setup Mode
};
