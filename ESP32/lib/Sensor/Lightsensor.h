#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include <TaskBase.h>
#include <PubSubClient.h>

class LightSensor_wRelay : public TaskBase {
public:
    LightSensor_wRelay(uint8_t pin, uint8_t relayPin, PubSubClient& mqttClient, const char* mqttTopic);

    void begin();
    bool getLightStatus();

protected:
    void run() override;

private:
    uint8_t pin;
    uint8_t relayPin;
    PubSubClient& client;
    const char* topic;
    bool lightStatus;
};
#endif // LIGHT_SENSOR_H