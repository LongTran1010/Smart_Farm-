// SoilMoisture.cpp
#include <SoilMoisture.h>
#include <Arduino.h>
#include <ArduinoJson.h>

SoilMoistureSensor::SoilMoistureSensor(uint8_t analogpin, uint8_t relay_sm_pin, PubSubClient& mqttClient, const char* pubTopic)
  : TaskBase("SoilMoistureTask", 2048, 1, 1), 
    analogpin(analogpin), relay_sm_pin(relay_sm_pin), 
    client(mqttClient), 
    pubTopic(pubTopic),
    moistureLevel(0),
    moisture_percent(0.0),
    pumpState(false) {}

void SoilMoistureSensor::begin() {
    pinMode(analogpin, INPUT);
    pinMode(relay_sm_pin, OUTPUT);
    digitalWrite(relay_sm_pin, LOW); // Relay off by default
}

void SoilMoistureSensor::run() {
    while (1) {
        moistureLevel = analogRead(analogpin);
        moisture_percent = 100 - map(moistureLevel, 0, 4095, 0, 100); // Convert to percentage

        Serial.print("Soil Moisture: ");
        Serial.print(moisture_percent);
        Serial.println(" %");

        if(client.connected()){
            String payload = "{\"soil_moisture\": " + String(moisture_percent) + "}";
            Serial.println("Publishing: " + payload);
            client.publish(pubTopic, payload.c_str());
        }else{
            Serial.println("MQTT disconnected, skipping publish...");
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

int SoilMoistureSensor::getMoistureLevel() const{
    return moistureLevel;
}

void SoilMoistureSensor::setPumpControl(bool on) {
    pumpState = on;
    digitalWrite(relay_sm_pin, on ? HIGH : LOW); // Set pump state
    Serial.println(on ? "Pump ON (Manual Mode)" : "Pump OFF (Manual Mode)");
}