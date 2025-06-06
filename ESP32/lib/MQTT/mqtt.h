#ifndef MQTT_TASK_H
#define MQTT_TASK_H

#include <TaskBase.h>
#include <PubSubClient.h>

struct MQTTMessage {
    const char* topic;
    const char* payload;
};

class MQTTTask : public TaskBase {

#endif // MQTT_TASK_H