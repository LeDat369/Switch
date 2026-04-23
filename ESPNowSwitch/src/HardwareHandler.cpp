#include "HardwareHandler.h"

// TAG Module dùng để in log
static const char* TAG = "HARDWARE";

HardwareHandler::HardwareHandler() {
    _relayState = 0; // Khởi tạo tắt
    _lastButtonState = HIGH;      // Nút INPUT_PULLUP mặc định HIGH ở trạng thái nhả
    _currentButtonState = HIGH;   
    _lastDebounceTime = 0;
    _buttonPressTime = 0;
    _isPressing = false;
    _isLongPressHandled = false;
}

void HardwareHandler::init() {
    Serial.printf("[%s] Initializing hardware pins...\n", TAG);

    // Cấu hình Nút nhấn. Trạng thái bình thường HIGH, nhấn xuống nối chập GND -> LOW.
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    
    // Cấu hình ngõ ra
    pinMode(PIN_RELAY, OUTPUT);
    pinMode(PIN_LED, OUTPUT);
    
    // Set mặc định khi bật điện
    setRelay(0);

    Serial.printf("[%s] Hardware initialized. Initial state: OFF.\n", TAG);
}

void HardwareHandler::_updateHardwareOutputs() {
    digitalWrite(PIN_RELAY, _relayState);
    digitalWrite(PIN_LED, !_relayState); // Logic LED ngược: 0 thì đèn sáng
}

void HardwareHandler::setRelay(uint8_t state) {
    if (state == 2) {
        _relayState = !_relayState;
    } else {
        _relayState = state ? 1 : 0;
    }
    
    _updateHardwareOutputs();
    Serial.printf("[%s] Relay switched to state: %s\n", TAG, _relayState ? "ON" : "OFF");
}

void HardwareHandler::toggleRelay() {
    setRelay(2);
}

uint8_t HardwareHandler::getRelayState() const {
    return _relayState;
}

ButtonEvent HardwareHandler::update() {
    ButtonEvent event = ButtonEvent::EVENT_NONE;
    int reading = digitalRead(PIN_BUTTON); // Đọc trạng thái chân vật lý

    // Kiểm tra có thay đổi đầu vào không (Nảy nhiễu)
    if (reading != _lastButtonState) {
        _lastDebounceTime = millis();
    }

    // Nếu trạng thái giữ nguyên quá thời gian lọc (Debounce Time > 50ms)
    if ((millis() - _lastDebounceTime) > DELAY_DEBOUNCE) {
        
        // Nếu điện áp xác nhận mới khác cờ hiện tại
        if (reading != _currentButtonState) {
            _currentButtonState = reading;

            // Nút nhấn vào (LOW đi xuống)
            if (_currentButtonState == LOW) {
                _isPressing = true;
                _buttonPressTime = millis(); // Ghi nhận thời điểm bấm
                _isLongPressHandled = false; // Xoá cờ xử lý đè
                Serial.printf("[%s] Button pressed down.\n", TAG);
            } 
            // Nút nhấc tay lên (HIGH đi lên)
            else { 
                _isPressing = false;
                uint32_t pressDuration = millis() - _buttonPressTime;
                
                // Nếu chưa đè tới mức 5000ms thì gọi là Short Press -> Toggle
                if (!_isLongPressHandled && pressDuration > DELAY_DEBOUNCE) {
                    Serial.printf("[%s] Button Short Clicked (Duration: %u ms)\n", TAG, pressDuration);
                    event = ButtonEvent::EVENT_TOGGLE;
                } else if (_isLongPressHandled) {
                    Serial.printf("[%s] Button released after Long Hold.\n", TAG);
                }
            }
        }
    }

    // MÁY TRẠNG THÁI LONG PRESS: Nút đang bị chèn chặt xuống chưa thả tay ra
    if (_isPressing && !_isLongPressHandled) {
        if ((millis() - _buttonPressTime) >= DELAY_LONG_PRESS) {
            // Xác nhận việc giữ phím đã vượt mốc 5 giây
            _isLongPressHandled = true;
            Serial.printf("[%s] Button LONG PRESS detected (>5000ms).\n", TAG);
            event = ButtonEvent::EVENT_RESET; 
        }
    }

    _lastButtonState = reading; // Kế thừa cho vòng lặp sau
    return event;
}
