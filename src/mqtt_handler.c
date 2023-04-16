// #include "mqtt_handler.h"
// #include "callbacks.h"
// #include "configuration.h"
// #include "instance.h"
// #include "mqtt.h"
// #include "protocol.h"
// #include <stdio.h>

// #define SUBSCRIBE_DOWNSTREAM_TOPIC(name, func)                                                                                                                 \
//     if (g_config.name.enable && g_config.name.req_topic) {                                                                                                     \
//         if (format_string_from_context(topic, sizeof(topic), g_config.name.req_topic, NULL) < 0) {                                                             \
//             log_error("invalid topic: %s", g_config.name.req_topic);                                                                                           \
//         } else {                                                                                                                                               \
//             mqtt_subscribe_ex(instance.mqtt, topic, func, NULL, logout_topics, strdup(topic));                                                                 \
//         }                                                                                                                                                      \
//     }
// #define SUBSCRIBE_UPSTREAM_TOPIC(name, func)                                                                                                                   \
//     if (g_config.name.enable && g_config.name.res_topic) {                                                                                                     \
//         if (format_string_from_context(topic, sizeof(topic), g_config.name.res_topic, NULL) < 0) {                                                             \
//             log_error("invalid topic: %s", g_config.name.res_topic);                                                                                           \
//         } else {                                                                                                                                               \
//             mqtt_subscribe_ex(instance.mqtt, topic, func, NULL, logout_topics, strdup(topic));                                                                 \
//         }                                                                                                                                                      \
//     }

// static void logout_topics(void *context, int code) {
//     log_info("subscribe: %s", (const char *)context);
//     free(context);
// }

// void init_topics(struct DeviceList *list) {
//     char topic[256];
//     SUBSCRIBE_UPSTREAM_TOPIC(dev_register, handle_sub_register_reply)
//     SUBSCRIBE_UPSTREAM_TOPIC(gw_login, handle_login_reply)
//     SUBSCRIBE_UPSTREAM_TOPIC(topo_join, handle_topo_join_reply)
//     SUBSCRIBE_UPSTREAM_TOPIC(topo_leave, handle_topo_leave_reply)
//     SUBSCRIBE_DOWNSTREAM_TOPIC(topo_get, handle_topo_get)
//     SUBSCRIBE_DOWNSTREAM_TOPIC(topo_add, handle_topo_add)
//     SUBSCRIBE_DOWNSTREAM_TOPIC(topo_delete, handle_topo_delete)
//     SUBSCRIBE_DOWNSTREAM_TOPIC(create_zigbee_network, handle_service_create_zigbee_network)
//     SUBSCRIBE_DOWNSTREAM_TOPIC(leave_zigbee_network, handle_service_leave_zigbee_network)
//     SUBSCRIBE_DOWNSTREAM_TOPIC(start_permit_join, handle_service_start_permit_join)
//     SUBSCRIBE_DOWNSTREAM_TOPIC(create_scene, handle_service_create_scene)
//     SUBSCRIBE_DOWNSTREAM_TOPIC(delete_scene, handle_service_delete_scene)
//     SUBSCRIBE_DOWNSTREAM_TOPIC(trigger_scene, handle_service_trigger_scene)
//     SUBSCRIBE_DOWNSTREAM_TOPIC(enable_scene, handle_service_enable_scene)
//     SUBSCRIBE_DOWNSTREAM_TOPIC(disable_scene, handle_service_disable_scene)
//     SUBSCRIBE_UPSTREAM_TOPIC(report_version, handle_report_version_reply)
//     SUBSCRIBE_DOWNSTREAM_TOPIC(ota_request, handle_ota_request)
//     SUBSCRIBE_UPSTREAM_TOPIC(ota_progress, handle_ota_progress_reply)
//     log_info("%d devices", instance.devList.count);
//     if (instance.devList.count > 0) {
//         struct DeviceList *list = &instance.devList;
//         for (int i = 1; i < list->count; i++) {
//             switch (list->devices[i].status) {
//             case CONNECT_STATE_LEAVING:
//                 log_info("%s is leaving", list->devices[i].mac);
//                 break;
//             case CONNECT_STATE_DELETING: {
//                 log_info("%s is deleting", list->devices[i].mac);
//                 RexCommand_t cmd;
//                 memset(&cmd, 0, sizeof(RexCommand_t));
//                 cmd.type = CMD_REMOVE_NODE;
//                 strcpy(cmd.u.removeNodeCommand.mac, list->devices[i].mac);
//                 int ret = instance.processCmd(instance.context, &cmd);
//                 if (ret != 0) {
//                     log_error("processCmd failed %d", ret);
//                 }
//             } break;
//             case CONNECT_STATE_ONLINE:
//                 add_device(list->devices[i].mac, list->devices[i].tslModel, NULL);
//                 handle_sub_login(list->devices[i].tslModel->productId, list->devices[i].mac);
//                 break;
//             case CONNECT_STATE_OFFLINE:
//                 add_device(list->devices[i].mac, list->devices[i].tslModel, NULL);
//                 break;
//             default:
//                 break;
//             }
//         }
//         free(list->devices);
//         list->count   = 0;
//         list->devices = NULL;
//     }
// }

// void register_gateway_mqtt(void *context, int code) {
//     if (!g_config.gw_register.enable || g_config.gw_register.req_topic == NULL) {
//         handle_login(NULL, 0);
//         return;
//     }
//     char  topic[256];
//     any_t topic_pattern = {.type = kUnknown};
//     any_set_const_string(&topic_pattern, g_config.gw_register.req_topic);
//     if (format_from_context(topic, sizeof(topic), topic_pattern, NULL) < 0) {
//         return;
//     }
//     char payload[256];
//     if (format_from_context(payload, sizeof(payload), g_config.gw_register.req_payload, NULL) < 0) {
//         return;
//     }
//     log_info("%s: %s", topic, payload);
//     mqtt_publish(instance.mqtt, topic, payload);
// }

// void check_register() {
//     int ret = instance.getDevList(instance.context, &instance.devList.count, &instance.devList.devices);
//     if (ret < 0) {
//         log_error("getDevList failed: %d", ret);
//         return;
//     }
//     if (instance.devList.count == 0) {
//         log_error("read gateway info failed, should be at least one device");
//         return;
//     }
//     init_topics(&instance.devList);
//     if (g_config.gw_register.res_topic == NULL) {
//         register_gateway_mqtt(&instance.devList, 0);
//         return;
//     }
//     char  topic[256];
//     any_t topic_pattern = {.type = kUnknown};
//     any_set_const_string(&topic_pattern, g_config.gw_register.res_topic);
//     if (format_from_context(topic, sizeof(topic), topic_pattern, NULL) < 0) {
//         log_error("invalid topic: %s", g_config.gw_register.res_topic);
//         return;
//     }
//     if (mqtt_subscribe_ex(instance.mqtt, topic, handle_register_gateway_mqtt_reply, &instance.devList, register_gateway_mqtt, &instance.devList) != 0) {
//         log_error("subscribe %s failed", topic);
//         free(instance.devList.devices);
//         instance.devList.devices = NULL;
//     }
// }
