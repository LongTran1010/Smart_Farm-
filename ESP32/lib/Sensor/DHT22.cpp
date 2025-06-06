#include <DHT22.h>

SensorDHT22::SensorDHT22(uint8_t pin, uint8_t type, PubSubClient& mqttClient, const char* mqttTopic)
  : TaskBase("SensorDHT22Task", 2048, 1, 1), 
    dht(pin, type), client(mqttClient), 
    topic(mqttTopic),
    temperature(0.0), humidity(0.0) {}

void SensorDHT22::begin() {
  dht.begin();
}

float SensorDHT22::getTemperature() {
  return temperature;
}

float SensorDHT22::getHumidity() {
  return humidity;
}

void SensorDHT22::run() {
  while (1) {
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    if (!isnan(temperature) && !isnan(humidity)) {
      Serial.print("Temp: "); Serial.print(temperature); Serial.print(" *C");
      Serial.print(" Humi: "); Serial.print(humidity); Serial.println(" %");
      if(client.connected()){
        String payload = "{\"temperature\": " + String(temperature) + ", \"humidity\": " + String(humidity) + "}";
        Serial.println("Publishing: " + payload);
        client.publish(topic, payload.c_str());
      }else{
        Serial.println("MQTT disconnected, skipping publish...");
      }
    } else {
      Serial.println("Failed to read DHT sensor.");
    }

    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}
