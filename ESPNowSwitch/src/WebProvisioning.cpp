#include "WebProvisioning.h"
#include <stdio.h> // cho sscanf

static const char* TAG = "WEB_PROV";
static const char* AP_SSID = "ESP_SW_Setup";

// HTML tĩnh được lưu trong vùng nhớ Flash (PROGMEM) để tiết kiệm RAM. Không dùng phép cộng String.
static const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP-NOW Switch Setup</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; background-color: #f4f4f9; }
        .container { max-width: 400px; margin: 0 auto; background: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
        input { font-size: 18px; padding: 10px; width: 80%; text-align: center; margin-bottom: 20px; text-transform: uppercase; border: 1px solid #ccc; border-radius: 4px;}
        button { font-size: 18px; padding: 10px 20px; background-color: #28a745; color: white; border: none; border-radius: 4px; cursor: pointer; }
        button:hover { background-color: #218838; }
    </style>
</head>
<body>
    <div class="container">
        <h2>ESP-NOW Switch</h2>
        <p>Enter the Gateway MAC Address:</p>
        <form action="/save" method="POST">
            <input type="text" name="mac" placeholder="AA:BB:CC:DD:EE:FF" maxlength="17" required>
            <br>
            <button type="submit">Save & Restart</button>
        </form>
    </div>
</body>
</html>
)rawliteral";

WebProvisioning::WebProvisioning(ConfigManager* configManager) 
    : _server(80), _configManager(configManager) {
}

void WebProvisioning::init() {
    Serial.printf("[%s] Starting Provisioning Mode...\n", TAG);
    
    // Đảm bảo chỉ cấp phát AP định tuyến nội bộ
    WiFi.mode(WIFI_AP);
    
    if (WiFi.softAP(AP_SSID)) {
        Serial.printf("[%s] SoftAP Started Successfully.\n", TAG);
        Serial.printf("[%s] Connect to SSID: %s\n", TAG, AP_SSID);
        Serial.printf("[%s] Setup Page IP: %s\n", TAG, WiFi.softAPIP().toString().c_str());
    } else {
        Serial.printf("[%s] ERROR: Failed to start SoftAP!\n", TAG);
    }
    
    // Gắn Callback cho các URL route (dùng std::bind do đây là hàm trong class)
    _server.on("/", HTTP_GET, std::bind(&WebProvisioning::_handleRoot, this));
    _server.on("/save", HTTP_POST, std::bind(&WebProvisioning::_handleSave, this));
    _server.onNotFound(std::bind(&WebProvisioning::_handleNotFound, this));
    
    _server.begin();
    Serial.printf("[%s] WebServer started on port 80.\n", TAG);
}

void WebProvisioning::update() {
    // Luôn gọi để bắt request từ trình duyệt. Hàm này non-blocking.
    _server.handleClient();
}

void WebProvisioning::_handleRoot() {
    Serial.printf("[%s] Client accessed root page.\n", TAG);
    _server.send_P(200, "text/html", htmlPage);
}

void WebProvisioning::_handleSave() {
    if (!_server.hasArg("mac")) {
        Serial.printf("[%s] ERROR: Form submitted without 'mac' argument.\n", TAG);
        _server.send(400, "text/plain", "Missing MAC address field.");
        return;
    }
    
    String macStr = _server.arg("mac");
    macStr.trim();
    macStr.toUpperCase();
    
    Serial.printf("[%s] Received Request to save MAC: %s\n", TAG, macStr.c_str());
    
    uint8_t macArr[6] = {0};
    if (_parseMac(macStr.c_str(), macArr)) {
        Serial.printf("[%s] MAC parsed successfully. Sending to ConfigManager...\n", TAG);
        
        _configManager->saveMac(macArr);
        
        _server.send(200, "text/html", "<h2>Saved Successfully!</h2><p>Device is restarting to run mode...</p>");
        
        Serial.printf("[%s] MAC Saved. Restarting device...\n", TAG);
        
        // Quét bộ đệm để đảm bảo gói tin HTTP 200 được gửi đi trước khi ngắt mạch
        _server.client().flush();
        ESP.restart();
    } else {
        Serial.printf("[%s] ERROR: Invalid MAC format: %s\n", TAG, macStr.c_str());
        _server.send(400, "text/plain", "Invalid MAC format. Please use AA:BB:CC:DD:EE:FF format.");
    }
}

void WebProvisioning::_handleNotFound() {
    Serial.printf("[%s] ERROR: 404 Not Found URI: %s\n", TAG, _server.uri().c_str());
    _server.send(404, "text/plain", "404: Not Found");
}

bool WebProvisioning::_parseMac(const char* macStr, uint8_t* macArr) {
    if (macStr == nullptr || strlen(macStr) != 17) {
        return false;
    }
    
    unsigned int temp[6];
    // Dùng sscanf bóc tách chuỗi hex sang dạng uint. 
    int parsed = sscanf(macStr, "%x:%x:%x:%x:%x:%x", 
                        &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5]);
                        
    if (parsed == 6) {
        for (int i = 0; i < 6; i++) {
            macArr[i] = (uint8_t)temp[i];
        }
        return true;
    }
    
    return false;
}
