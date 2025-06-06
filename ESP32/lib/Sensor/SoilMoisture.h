// SoilMoisture.h
#ifndef SOIL_MOISTURE_SENSOR_H
#define SOIL_MOISTURE_SENSOR_H

#include <TaskBase.h>
#include <PubSubClient.h>
#include <Arduino.h>

class SoilMoistureSensor : public TaskBase {
public:
    SoilMoistureSensor(uint8_t analogpin, uint8_t relay_sm_pin, PubSubClient& mqttClient, const char* pubTopic);

    void begin();
    int getMoistureLevel() const;
    void setPumpControl(bool on); 
protected:
    void run() override;

private:
    uint8_t analogpin;
    uint8_t relay_sm_pin;
    PubSubClient& client;
    const char* pubTopic; 
    int moistureLevel;
    float moisture_percent; 
    bool pumpState;
};
#endif // SOIL_MOISTURE_SENSOR_H