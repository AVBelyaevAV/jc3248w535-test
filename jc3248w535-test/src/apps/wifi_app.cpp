#include "wifi_app.h"

void WiFiApp::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.begin("YOUR_SSID", "YOUR_PASS");
    Serial.print("WiFi connecting");
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries++ < 30) {
        delay(500); Serial.print('.');
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi OK: %s IP=%s\n",
                      WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\nWiFi FAIL (check credentials)");
    }
}

void WiFiApp::loop() {
    if (WiFi.status() != WL_CONNECTED) {
        static uint32_t lt = 0;
        if (millis() - lt > 10000) { lt = millis(); WiFi.reconnect(); }
    }
}