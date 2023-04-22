#pragma once
#include "any.h"
#include "message.h"

struct DeviceList;

typedef int (*service_response)(int id, int code, void *data);

void init_topics(struct DeviceList *list);

/**
 * 金山云相关topic
*/
int handle_kc_message_up(const char *topic, const char *message, int length, void *context);
int handle_kc_command_down(const char *topic, const char *message, int length, void *context);
int handle_kc_command_ack(const char *topic, const char *message, int length, void *context);
int handle_kc_ota_ack(const char *topic, const char *message, int length, void *context);

// 2.1.1 网关动态注册
int handle_register_gateway_mqtt_reply(const char *topic, const char *message, int length, void *context);

// 2.1.2 子设备动态注册
int handle_sub_register(const char *productKey, const char *deviceName);
int handle_sub_register_reply(const char *topic, const char *message, int length, void *context);

// 2.2.1 子设备入网
int handle_topo_join(const char *productKey, const char *deviceName, const char *deviceSecret);
int handle_topo_join_reply(const char *topic, const char *message, int length, void *context);

// 2.2.2 子设备离网
int handle_topo_leave(const char *productKey, const char *deviceName);
int handle_topo_leave_reply(const char *topic, const char *message, int length, void *context);

// 2.2.3 获取设备的拓扑关系
int handle_topo_get(const char *topic, const char *message, int length, void *context);
int handle_topo_get_reply();

// 2.2.4 通知网关添加设备拓扑关系
int handle_topo_add(const char *topic, const char *message, int length, void *context);
int handle_topo_add_reply();

// 2.2.5 通知网关删除设备拓扑关系
int handle_topo_delete(const char *topic, const char *message, int length, void *context);
int handle_topo_delete_reply();

// 2.3.1 设备上线
void handle_login(void *context, int code);
int  handle_login_reply(const char *topic, const char *message, int length, void *context);

// 2.4.1 子设备上线
int handle_sub_login(const char *productKey, const char *deviceName);
int handle_sub_login_reply(const char *topic, const char *message, int length, void *context);

// 2.4.2 子设备下线
int handle_sub_logout(const char *productKey, const char *deviceName);
int handle_sub_logout_reply(const char *topic, const char *message, int length, void *context);

// 3.1.1 设备上报属性
int handle_event_property_post(const char *productKey, const char *deviceName, const RexPropertyMsg_t *msg);
int handle_event_property_post_reply(const char *topic, const char *message, int length, void *context);

// 3.1.2 设置设备属性
int handle_service_property_set(const char *topic, const char *message, int length, void *context);
int handle_service_property_set_reply(map_any *params);

// 3.1.3 设置上报事件
int handle_event_post(const char *productKey, const char *deviceName, RexEvent_t *event);
int handle_event_post_reply(const char *topic, const char *message, int length, void *context);

// 3.1.4 服务调用
int handle_service(const char *topic, const char *message, int length, void *context);
int handle_service_reply(int id, int code, void *data);

int handle_service_create_zigbee_network(const char *topic, const char *message, int length, void *context);
int handle_service_create_zigbee_network_reply(int id, int code, void *data);

int handle_service_leave_zigbee_network(const char *topic, const char *message, int length, void *context);
int handle_service_leave_zigbee_network_reply(int id, int code, void *data);

int handle_service_start_permit_join(const char *topic, const char *message, int length, void *context);
int handle_service_start_permit_join_reply(int id, int code, void *data);

int handle_service_create_scene(const char *topic, const char *message, int length, void *context);
int handle_service_delete_scene(const char *topic, const char *message, int length, void *context);
int handle_service_trigger_scene(const char *topic, const char *message, int length, void *context);
int handle_service_enable_scene(const char *topic, const char *message, int length, void *context);
int handle_service_disable_scene(const char *topic, const char *message, int length, void *context);

// 3.1.5 设置设备别名
int handle_service_set_alias(const char *topic, const char *message, int length, void *context);

// 4.1.1 设备上报 OTA 模块版本
int handle_report_version(const char *productKey, const char *deviceName, const char *module, const char *version);
int handle_report_version_reply(const char *topic, const char *message, int length, void *context);

// 4.1.2 平台推送 OTA 升级包信息
int handle_ota_request(const char *topic, const char *message, int length, void *context);
int handle_ota_request_reply();
int handle_dev_ota_request(const char *topic, const char *message, int length, void *context);

// 4.1.3 设备上报升级进度
int handle_ota_progress(int id, int code, void *data);
int handle_ota_progress_reply(const char *topic, const char *message, int length, void *context);
int handle_dev_ota_progress(int id, int code, void *data);
