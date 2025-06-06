#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H
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

#ifndef CURRENT_FW_VERSION
#define CURRENT_FW_VERSION "undefined"
#endif

class OTAUpdate {
public:
    static OTAUpdate* getInstance();

    void publishFirmwareVersion(PubSubClient& client, const char* topic);
    void checkForUpdate(const String& fw_version, const String& fw_url);

private:
    OTAUpdate() {}
    void performOTA(const String& url);
};       
#endif