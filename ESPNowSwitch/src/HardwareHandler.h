#pragma once
#include <Arduino.h>
#include "CommonTypes.h"

class HardwareHandler {
private:
    // Định nghĩa chân phần cứng (Thay đổi theo module ESP-12F / Board của bạn)
    static const uint8_t PIN_RELAY = 12;
    static const uint8_t PIN_LED = 13;
    static const uint8_t PIN_BUTTON = 4;

    uint8_t _relayState;
    
    // Các biến dùng cho kỹ thuật Non-blocking Debounce
    bool _lastButtonState;
    bool _currentButtonState;
    uint32_t _lastDebounceTime;
    uint32_t _buttonPressTime;
    bool _isPressing;
    bool _isLongPressHandled;

    // Các hằng số thời gian (ms)
    static const uint32_t DELAY_DEBOUNCE = 50;
    static const uint32_t DELAY_LONG_PRESS = 5000;

    void _updateHardwareOutputs();

public:
    HardwareHandler();
    
    // Khởi tạo chân I/O và đặt trạng thái mặc định
    void init();
    
    // Gọi liên tục trong loop() để quét trạng thái vật lý
    ButtonEvent update();
    
    // Setter/Getter cho rơ-le
    void setRelay(uint8_t state);
    uint8_t getRelayState() const;
    void toggleRelay();
};
