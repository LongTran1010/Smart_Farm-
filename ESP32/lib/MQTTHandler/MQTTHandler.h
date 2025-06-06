#pragma once

#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "TaskBase.h"
#define LED_PIN 2
class MqttHandler : public TaskBase {
public:
    MqttHandler(PubSubClient& client);
    void begin();
    void setCallback();
    void handleMessage(char* topic, byte* payload, unsigned int length);
    void setCurrentFirmwareVersion(const String& version);

private:
    PubSubClient& mqttClient;
    String currentVersion = "1.0.0";
    void checkForUpdates(const String& version, const String& url);
};