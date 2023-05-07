#pragma once
#include <stdbool.h>

struct MqttInstance;
typedef int (*MqttMessageCallback)(const char *topic, const char *message, int length, void *context);
typedef void (*MqttFinishCallback)(void *context, int code);

struct MqttInstance *mqtt_new();
bool mqtt_is_connected(struct MqttInstance *mqtt);
void mqtt_delete(struct MqttInstance *mqtt);
int  mqtt_start(struct MqttInstance *mqtt, MqttFinishCallback onConnect, void *context, char *address, char *username, char *password);
int  mqtt_stop(struct MqttInstance *mqtt);
int  mqtt_subscribe(struct MqttInstance *mqtt, const char *topic, MqttMessageCallback cb, void *context);
int  mqtt_subscribe_ex(struct MqttInstance *mqtt, const char *topic, MqttMessageCallback cb, void *context, MqttFinishCallback fc, void *fcctx);
int  mqtt_unsubscribe(struct MqttInstance *mqtt, const char *topic);
int  mqtt_publish(struct MqttInstance *mqtt, const char *topic, const char *payload);
int  mqtt_publish_ex(struct MqttInstance *mqtt, const char *topic, const char *payload, MqttFinishCallback cb, void *context);
