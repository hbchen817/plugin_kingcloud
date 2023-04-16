#include "callbacks.h"
#include "configuration.h"
#include "helper.h"
#include "instance.h"
#include "map.h"
#include "mqtt.h"
#include "mqtt_handler.h"
#include "protocol.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

__attribute((constructor)) void plugin_init(void) {
    srand(time(NULL));
    memset(&instance, 0, sizeof(instance));
    strcpy(instance.info.name, "mqtt_plugin");
    strcpy(instance.info.desc, "connect to mqtt broker");
    strcpy(instance.info.ver, VERSION);
    strcpy(instance.info.uuid, "c7d8cb8059a743c59ae846fa3fdc37ca");
}

__attribute((destructor)) void plugin_fini(void) {
}

int rex_init(RexInitParameter_t *parameter, void *ctx) {
    if (parameter == NULL || parameter->version < 1) {
        return 0;
    }
    instance_init(parameter, ctx);
    log_info("%s", __FUNCTION__);
    return PLUGIN_PROTOCOL_VERSION;
}

void rex_deinit() {
    log_info("%s", __FUNCTION__);
    instance_deinit();
}

int rex_start() {
    log_info("%s", __FUNCTION__);
    int ret = instance.getDevList(instance.context, &instance.devList.count, &instance.devList.devices);
    if (ret < 0) {
        log_error("getDevList failed: %d", ret);
        return 1;
    }
    if (instance.devList.count == 0) {
        log_error("read gateway info failed, should be at least one device");
        return 1;
    }
    strcpy(instance.productKey, instance.devList.devices[0].productId);
    strcpy(instance.deviceName, instance.devList.devices[0].mac);
    strcpy(instance.versionCOO, instance.devList.devices[0].version);
    mqtt_start(instance.mqtt, check_register, NULL);
    return 0;
}

int rex_stop() {
    log_info("%s", __FUNCTION__);
    map_device_iterator *iter;
    while ((iter = map_device_first(instance.devices)) != NULL) {
        if (iter->val1 != NULL) {
            free((char *)iter->val1);
        }
        map_device_erase(iter);
    }
    mqtt_stop(instance.mqtt);
    return 0;
}

int rex_notify(RexMessage_t *msg) {
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    fmt_RexMessage(buf, sizeof(buf) - 1, msg);
    log_info("%s %s", __FUNCTION__, buf);
    map_device_iterator *iter = NULL;
    if (msg->sequence != 0) {
        map_request_iterator *iter = map_request_find(instance.requests, msg->sequence);
        if (iter == NULL) {
            log_warn("unknown sequence(%d)", msg->sequence);
        } else {
            switch (msg->type) {
            case MSG_TYPE_OTA_EVENT:
                iter->val0(msg->sequence, msg->u.otaProgressMsg.step, iter->val1);
                break;
            case MSG_TYPE_EXECUTE_RESULT:
                iter->val0(msg->sequence, msg->u.resultMsg.result, iter->val1);
                break;
            default:
                iter->val0(msg->sequence, 0, iter->val1);
                break;
            }
        }
    } else {
        switch (msg->type) {
        case MSG_TYPE_SDK_READY:
            log_info("SDK READY");
            break;
        case MSG_TYPE_PAIR_FINISHED: // 网关配网停止
            log_info("PAIR FINISHED");
            break;
        case MSG_TYPE_SCENE_TRIGGERED: // 场景触发
            log_info("scene %s unknown operation(%d) %d", msg->u.sceneTriggerMsg.sceneId, msg->u.sceneTriggerMsg.action, msg->u.sceneTriggerMsg.status);
            break;
        case MSG_TYPE_NODE_ADDED: // 新节点入网
            handle_sub_register(msg->u.includedMsg.productId, msg->u.includedMsg.mac);
            break;
        case MSG_TYPE_NODE_REMOVED: // 节点离网
            iter = map_device_find(instance.devices, msg->u.excludedMsg.mac);
            if (iter == NULL) {
                log_error("subdevice %s leavenet but not registered", msg->u.excludedMsg.mac);
            } else {
                const RexTslModelInfo_t *tsl = iter->val0;
                handle_topo_leave(tsl->productId, msg->u.excludedMsg.mac);
                log_info("subdevice %s leavenet", msg->u.excludedMsg.mac);
            }
            break;
        case MSG_TYPE_NODE_CONN_STATE: // 节点连接状态变更
            switch (msg->u.connStateMsg.state) {
            case CONNECT_STATE_ONLINE:
                iter = map_device_find(instance.devices, msg->u.connStateMsg.mac);
                if (iter == NULL) {
                    log_error("subdevice %s online but not registered", msg->u.connStateMsg.mac);
                } else {
                    const RexTslModelInfo_t *tsl = iter->val0;
                    handle_sub_login(tsl->productId, msg->u.connStateMsg.mac);
                    log_info("subdevice %s online", msg->u.connStateMsg.mac);
                }
                break;
            case CONNECT_STATE_OFFLINE:
                iter = map_device_find(instance.devices, msg->u.connStateMsg.mac);
                if (iter == NULL) {
                    log_error("subdevice %s offline but not registered", msg->u.connStateMsg.mac);
                } else {
                    const RexTslModelInfo_t *tsl = iter->val0;
                    handle_sub_logout(tsl->productId, msg->u.connStateMsg.mac);
                    log_info("subdevice %s offline", msg->u.connStateMsg.mac);
                }
                break;
            default:
                log_error("subdevice %s state change to %d", msg->u.connStateMsg.mac, msg->u.connStateMsg.state);
                break;
            }
            break;
        case MSG_TYPE_NODE_HEARTBEAT: // 节点心跳消息
            log_info("subdevice %s heartbeat", msg->u.heartbeatMsg.mac);
            break;
        case MSG_TYPE_NODE_EVENT: // 节点事件
            handle_event_post(msg->u.eventMsg.productId, msg->u.eventMsg.mac, msg->u.eventMsg.event);
            break;
        case MSG_TYPE_NODE_STATE_CHANGED: // 节点属性状态变更
            if (msg->u.propertyMsg.mac[0] == '\0') {
                handle_event_property_post(instance.productKey, instance.deviceName, &msg->u.propertyMsg);
            } else {
                handle_event_property_post(msg->u.propertyMsg.productId, msg->u.propertyMsg.mac, &msg->u.propertyMsg);
            }
            break;
        case MSG_TYPE_OTA_EVENT:
        case MSG_TYPE_SERVICE:        // 服务类型
        case MSG_TYPE_EXECUTE_RESULT: // 执行结果
        default:
            log_error("notify(%d)", msg->type);
            break;
        }
    }
    return 0;
}

const RexPluginInfo_t *rex_get_info() {
    return &instance.info;
}

struct PluginConfig_t {
    char *mqttBroker;
    char *mqttUsername;
    char *mqttPassword;
};
const struct reflect_reflection PluginConfig_reflection[] = {
    REFLECT_DECLARE_STR_PTR(struct PluginConfig_t, mqttBroker),
    REFLECT_DECLARE_STR_PTR(struct PluginConfig_t, mqttUsername),
    REFLECT_DECLARE_STR_PTR(struct PluginConfig_t, mqttPassword),
    REFLECT_DECLARE_END(),
};

int rex_get_data(void *ctx, const char *requestType, const void *requestData, GetDataCallBack getDataCb) {
    if (strcmp(requestType, REQUEST_TYPE_GET_CONF) == 0) {
        struct PluginConfig_t config = {.mqttBroker = instance.mqttBroker, .mqttUsername = instance.mqttUsername, .mqttPassword = instance.mqttPassword};
        if (config.mqttBroker[0] == '\0') {
            config.mqttBroker = g_config.basic.url;
            if (g_config.basic.username) {
                config.mqttUsername = g_config.basic.username;
            }
            if (g_config.basic.password) {
                config.mqttPassword = g_config.basic.password;
            }
        }
        char *payload = NULL;
        int   ret     = reflect_struct2str(&payload, &config, PluginConfig_reflection);
        if (ret == REFLECT_ERR_SUCCESS) {
            log_info("%p, %s", payload, payload);
            getDataCb(ctx, payload);
            free(payload);
            return 0;
        } else {
            log_error("reflect_struct2str failed %d", ret);
            return -1;
        }
    } else if (strcmp(requestType, REQUEST_TYPE_POST_CONF) == 0) {
        struct PluginConfig_t config;
        memset(&config, 0, sizeof(struct PluginConfig_t));
        int ret = reflect_str2struct(requestData, &config, PluginConfig_reflection);
        if (ret == REFLECT_ERR_SUCCESS) {
            bool restart = false;
            if (config.mqttBroker != NULL && strcmp(instance.mqttBroker, config.mqttBroker)) {
                log_info("set broker to: %s", config.mqttBroker);
                restart = true;
                strcpy(instance.mqttBroker, config.mqttBroker);
                instance.writeConf(instance.context, "mqtt_broker", instance.mqttBroker, strlen(instance.mqttBroker));
            }
            if (config.mqttUsername != NULL && strcmp(instance.mqttUsername, config.mqttUsername)) {
                log_info("set username to: %s", config.mqttUsername);
                restart = true;
                strcpy(instance.mqttUsername, config.mqttUsername);
                instance.writeConf(instance.context, "mqtt_username", instance.mqttUsername, strlen(instance.mqttUsername));
            }
            if (config.mqttPassword != NULL && strcmp(instance.mqttPassword, config.mqttPassword)) {
                log_info("set password to: %s", config.mqttPassword);
                restart = true;
                strcpy(instance.mqttPassword, config.mqttPassword);
                instance.writeConf(instance.context, "mqtt_password", instance.mqttPassword, strlen(instance.mqttPassword));
            }
            reflect_free_ptr(&config, PluginConfig_reflection);
            getDataCb(ctx, "{\"code\":200}");
            if (restart) {
                rex_stop();
                mqtt_delete(instance.mqtt);
                instance.mqtt = mqtt_new(instance.mqttBroker, instance.mqttUsername, instance.mqttPassword);
                rex_start();
            }
            return 0;
        } else {
            log_error("reflect_str2struct failed %d", ret);
            return -1;
        }
    } else {
        log_error("unknown requestType: %s", requestType);
        return -1;
    }
}