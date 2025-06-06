#include <Lightsensor.h>
#include <Arduino.h>
#include <ArduinoJson.h>

LightSensor_wRelay::LightSensor_wRelay(uint8_t pin, uint8_t relayPin, PubSubClient& mqttClient, const char* mqttTopic)
  : TaskBase("LightSensorTask", 2048, 1, 1), 
    pin(pin), relayPin(relayPin), 
    client(mqttClient), 
    topic(mqttTopic),
    lightStatus(false) {}

void LightSensor_wRelay::begin() {
  pinMode(pin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Relay off by default
}

bool LightSensor_wRelay::getLightStatus() {
  return lightStatus;
}

void LightSensor_wRelay::run() {
  while (1) {
    lightStatus = digitalRead(pin);
    Serial.print("Light Status (1 = Sang, 0 = Toi): ");
    Serial.println(lightStatus);

    digitalWrite(relayPin, lightStatus ? LOW : HIGH); // Turn on relay if light is detected
    if(client.connected()){
      String payload = "{\"light_status\": " + String(lightStatus) + "}";
      Serial.println("Publishing: " + payload);
      client.publish(topic, payload.c_str());
    }else{
        Serial.println("MQTT disconnected, skipping publish...");
    }

    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}