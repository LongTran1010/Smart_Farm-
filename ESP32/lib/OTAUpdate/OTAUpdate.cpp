#include "OTAUpdate.h"
#include <Update.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>
//Đảm bảo rằng chỉ có một phiên bản duy nhất của OTAUpdate tồn tại trong toàn bộ chương trình (lifetime như biến toàn cục)
//Chỉ có thể truy cập được phiên bản này thông qua hàm getInstance()
OTAUpdate* OTAUpdate::getInstance() {//Singleton pattern to ensure only one instance of OTAUpdate exists
    static OTAUpdate instance;
    return &instance;
}

void OTAUpdate::publishFirmwareVersion(PubSubClient& client, const char* topic) {
    String payload = "{\"FirmwareVersion\": \"" CURRENT_FW_VERSION "\"}";
    client.publish(topic, payload.c_str());
    Serial.println("[OTA] Published firmware version: " + String(CURRENT_FW_VERSION));
}

void OTAUpdate::checkForUpdate(const String& fw_version, const String& fw_url) {
    Serial.println("[OTA] Checking for update...");
    Serial.println("Current version: " + String(CURRENT_FW_VERSION));
    Serial.println("Incoming version: " + fw_version);
    if (fw_version != CURRENT_FW_VERSION) {
        Serial.println("[OTA] New version available. Starting OTA update...");
        performOTA(fw_url);
    } else {
        Serial.println("[OTA] No new version available.");
    }
}

void OTAUpdate::performOTA(const String& firmware_url) {
    WiFiClientSecure client;
    client.setInsecure(); // Disable SSL certificate verification (not recommended for production)
    HTTPClient http;
    ESP.getFreeHeap();
    Serial.print("[OTA] Downloading firmware from: ");
    Serial.println(firmware_url);
    http.begin(client, firmware_url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[OTA] HTTP GET failed: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return;
    }
    int totalSize = http.getSize();
    Serial.print("[OTA] Firmware size: ");
    Serial.println(totalSize);
    if (totalSize <= 0) {
        Serial.println("[OTA] Invalid firmware file!");
        http.end();
        return;
    }
    WiFiClient* stream = http.getStreamPtr();
    if (!Update.begin(totalSize)) {
        Serial.println("[OTA] Not enough space for OTA.");
        http.end();
        return;
    }

    size_t written = Update.writeStream(*stream);
    if (written == totalSize && Update.end()) {
        Serial.println("[OTA] Update successful. Restarting...");
        ESP.restart();
    } else {
        Serial.println("[OTA] Update failed.");
    }
    http.end();
}