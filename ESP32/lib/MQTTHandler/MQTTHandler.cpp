#include "MqttHandler.h"
#include "OTAUpdate.h" // Gọi OTA từ đây nếu có

MqttHandler::MqttHandler(PubSubClient& client) : mqttClient(client) {}

void MqttHandler::begin() {
    pinMode(LED_PIN, OUTPUT);
    setCallback();
}

void MqttHandler::setCallback() {
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        handleMessage(topic, payload, length);
    });
}

void MqttHandler::setCurrentFirmwareVersion(const String& version) {
    currentVersion = version;
}

void MqttHandler::handleMessage(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("Received MQTT message: ");
    Serial.println(message);

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, message.c_str());

    if (!error) {
        if (doc.containsKey("fw_version") && doc.containsKey("fw_url")) {
            String newVersion = doc["fw_version"].as<String>();
            String newUrl = doc["fw_url"].as<String>();
            checkForUpdates(newVersion, newUrl);
        }
        if (doc.containsKey("led")) {
            bool ledState = doc["led"];
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
            Serial.print("LED State: ");
            Serial.println(ledState ? "ON" : "OFF");
        }
    } else {
        Serial.print("JSON Parse Error: ");
        Serial.println(error.c_str());
    }
}

void MqttHandler::checkForUpdates(const String& version, const String& url) {
    Serial.println("Checking for firmware update...");
    Serial.println("Current version: " + currentVersion);
    Serial.println("Incoming version: " + version);

    if (version != currentVersion) {
        Serial.println("New firmware available. Starting OTA update...");
        OTAUpdate::getInstance()->handleUpdate(version, url);
    } else {
        Serial.println("Firmware is up-to-date.");
    }
}
