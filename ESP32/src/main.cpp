#include <Arduino.h>
#include <ArduinoJson.h>
#include "DHT22.h"
#include <WiFi.h>
#include <OTAUpdate.h>
#include <Password.h>
#include <Lightsensor.h>
#include <SoilMoisture.h> 
#include <fingerprint_module.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

WiFiClient espClient;
PubSubClient client(espClient);

const char* mqtt_server = "192.168.90.50";
const int mqtt_port = 1883;
const char* topic_dht = "home/sensors/dht";
const char* topic_light = "home/sensors/light";
const char* topic_sm = "home/sensors/sm";
const char* mqtt_topic_sub = "home/actuators/mqtt";
const char* mqtt_topic_ota = "home/device/fw_version";

TaskHandle_t taskMQTT;
SensorDHT22 DHT22sensor(26, DHT22, client, topic_dht);
LightSensor_wRelay LightSensor(34, /*relayPin*/33, client, topic_light);
SoilMoistureSensor SoilMoisture(32, /*pumpPin*/19, client, topic_sm);
FingerprintModule fingerprint(&Serial2, 23, 22);


void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void reconnectMQTT() {
  while (!client.connected()) {
    if(WiFi.status() != WL_CONNECTED){
      Serial.println("Wi-Fi lost, reconnecting...");
      connectWiFi();
    }
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32_Client")) {
      Serial.println("Connected!");
      client.subscribe(mqtt_topic_sub);
      Serial.println("[MQTT] Subscribed to topic: " + String(mqtt_topic_sub)); 
      OTAUpdate::getInstance()->publishFirmwareVersion(client, mqtt_topic_ota);
    }else{
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5s...");
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
  }
}
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("[callback] ");
  Serial.println(topic);
  Serial.print(" -> ");
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Received MQTT message: ");
  Serial.println(message);

  // Parse JSON
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, message.c_str());

  if(!error){
    //Xử lý firmware update
    if(doc.containsKey("fw_version") && doc.containsKey("fw_url")){
      String newVersion = doc["fw_version"].as<String>();
      String newUrl = doc["fw_url"].as<String>();
      OTAUpdate::getInstance()->checkForUpdate(newVersion, newUrl);
      //bool ledState = doc["led"];
      //digitalWrite(LED_PIN, ledState ? HIGH : LOW);
      //Serial.print("LED State: ");
      //Serial.println(ledState ? "ON" : "OFF");
    }
    //Xử lý máy bơm
    if(doc.containsKey("pump")){
      bool on = doc["pump"];
      Serial.print("Pump command received: ");
      Serial.println(on ? "ON" : "OFF");
      SoilMoisture.setPumpControl(on);
    }else if(doc.containsKey("pump1")){
      bool on = doc["pump1"];
      Serial.println(on ? "ON" : "OFF");
      SoilMoisture.setPumpControl(on);     
    }
  }else{ 
    Serial.print("JSON Parse Error: ");
    Serial.println(error.c_str());
  }
}
// void MQTTSubscribeTask(void *pvParameters) {
//   Serial.println("[MQTT Task] Started MQTTSubscribeTask");
//   for (;;) {
//     if (!client.connected()) {
//       Serial.println("[MQTT Task] MQTT not connected, calling reconnectMQTT()");
//       reconnectMQTT();
//     }
//     client.loop();
//     vTaskDelay(10 / portTICK_PERIOD_MS);  
//   }
// }

void MQTTSubscribeTask(void* pvParameters){
  Serial.println(">> MQTTSubscribeTask STARTED on core " + String(xPortGetCoreID()));
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  client.setBufferSize(256);
  client.setKeepAlive(30); // 30 giây

  reconnectMQTT();

  for(;;){
    client.loop();
    vTaskDelay(10 / portTICK_PERIOD_MS);  
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(57600, SERIAL_8N1, 16, 17);
  connectWiFi();
  BaseType_t mqttRet = xTaskCreatePinnedToCore(
      MQTTSubscribeTask, 
      "MQTTTask", 
      4096, 
      NULL, 
      2,      // priority
      &taskMQTT, 
      1       // core 1
  );
  Serial.printf("MQTT task creation ret = %d\n", mqttRet);

  //Khởi tạo các sensor-task
  DHT22sensor.begin(); DHT22sensor.start();
  LightSensor.begin(); LightSensor.start();
  SoilMoisture.begin(); SoilMoisture.start();
  fingerprint.begin(); fingerprint.start();
  //xTaskCreatePinnedToCore(MQTTSubscribeTask, "MQTT Subcribe Task", 4096, NULL, 1, &taskMQTT, 1);
}

void loop(){
}

