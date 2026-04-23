#ifndef WEB_PROVISIONING_H
#define WEB_PROVISIONING_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "ConfigManager.h"

class WebProvisioning {
private:
    ESP8266WebServer _server;
    ConfigManager* _configManager;
    
    void _handleRoot();
    void _handleSave();
    void _handleNotFound();
    
    bool _parseMac(const char* macStr, uint8_t* macArr);

public:
    WebProvisioning(ConfigManager* configManager);
    
    void init();
    void update();
};

#endif // WEB_PROVISIONING_H
