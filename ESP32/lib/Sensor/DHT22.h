#ifndef DHT22_SENSOR_H
#define DHT22_SENSOR_H

#include <TaskBase.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <DHT_U.h>

class SensorDHT22 : public TaskBase {
public:
    SensorDHT22(uint8_t pin, uint8_t type, PubSubClient& mqttClient, const char* mqttTopic);

    void begin();
    float getTemperature();
    float getHumidity();

protected:
    void run() override;

private:
    DHT dht;
    PubSubClient& client;
    const char* topic;
    float temperature;
    float humidity;
};
#endif // DHT22_SENSOR_H
