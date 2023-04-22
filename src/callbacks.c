#include "callbacks.h"
#include "cJSON.h"
#include "config.h"
#include "error.h"
#include "if_helper.h"
#include "instance.h"
#include "map.h"
#include "mqtt.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int common_mqtt_publish(const char *topic_pattern_str, any_t payload_pattern, const map_any *context) {
    any_t topic_pattern = {.type = kUnknown};
    any_set_const_string(&topic_pattern, topic_pattern_str);
    any_t topic_value = generate_value(topic_pattern, context);
    if (!any_is_string(&topic_value)) {
        any_free(&topic_value);
        return ERR_CONFIG_MALFORMED;
    }
    char payload_str[256];
    if (format_from_context(payload_str, sizeof(payload_str), payload_pattern, context) < 0) {
        any_free(&topic_value);
        return ERR_CONFIG_MALFORMED;
    }
    log_info("%s: %s", topic_value.u.sval, payload_str);
    mqtt_publish(instance.mqtt, topic_value.u.sval, payload_str);
    any_free(&topic_value);
    return ERR_SUCCESS;
}

static int common_mqtt_subscribe(const char *topic, const char *message, int length, const char *topic_pattern_str, any_t payload_pattern, map_any *output) {
    int ret1 = parse_from_plain_buffer(topic, strlen(topic), any_from_const_string(topic_pattern_str), output);
    int ret2 = parse_from_yaml_buffer(message, length, payload_pattern, output);
    return ret1 ? ret1 : ret2;
}

static int report_version() {
    config_func_t *config = &g_config.report_version;
    if (!config->enable || config->req_topic == NULL) {
        return ERR_SUCCESS;
    }
    int               count       = 0;
    RexVersionInfo_t *versionInfo = NULL;
    int               ret         = instance.getVerInfo(instance.context, &count, &versionInfo);
    if (ret != 0) {
        log_error("call getVerInfo failed %d", ret);
    } else {
        for (int i = 0; i < count; i++) {
            if (versionInfo[i].name[0] != '\0') {
                handle_report_version(instance.productKey, instance.deviceName, versionInfo[i].name, versionInfo[i].ver);
            }
        }
    }
    free(versionInfo);
    return ERR_SUCCESS;
}

int handle_kc_message_up(const char *topic, const char *message, int length, void *context) {
    return 1;
}

int handle_kc_command_down(const char *topic, const char *message, int length, void *context) {
    return 1;
}

int handle_kc_command_ack(const char *topic, const char *message, int length, void *context) {
    return 1;
}

int handle_kc_ota_ack(const char *topic, const char *message, int length, void *context) {
    return 1;
}

// 2.1.1 网关动态注册
int handle_register_gateway_mqtt_reply(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    return 1;
}

// 2.1.2 子设备动态注册
int handle_sub_register(const char *productKey, const char *deviceName) {
    map_device_iterator *iter = map_device_first(instance.devices);
    while (iter != NULL && strcmp(iter->val0->productId, productKey) != 0) {
        iter = map_device_next(iter);
    }
    if (iter != NULL) {
        add_device(deviceName, iter->val0, NULL);
    } else {
        int       count;
        RexDev_t *devices;
        if (instance.getDevList(instance.context, &count, &devices) < 0) {
            log_error("getDevList failed");
        }
        for (int i = 0; i < count; i++) {
            if (strcmp(devices[i].mac, deviceName) == 0) {
                add_device(devices[i].mac, devices[i].tslModel, NULL);
                break;
            }
        }
        free(devices);
    }
    config_func_t *config = &g_config.dev_register;
    if (!config->enable || config->req_topic == NULL) {
        handle_topo_join(productKey, deviceName, NULL);
        return ERR_SUCCESS;
    }
    map_any *context = map_any_create();
    map_any_insert(context, NAME_DEV_PRODUCT_KEY, any_from_const_string(productKey));
    map_any_insert(context, NAME_DEV_DEVICE_NAME, any_from_const_string(productKey));
    int ret = common_mqtt_publish(config->req_topic, config->req_payload, context);
    map_any_destroy_ex(context);
    if (ret == ERR_SUCCESS && config->res_topic == NULL) {
        handle_topo_join(productKey, deviceName, NULL);
    }
    return ret;
}

int handle_sub_register_reply(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.dev_register;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->res_topic, config->res_payload, params);
    bool success = true;
    if (config->success != NULL) {
        for (map_any_iterator *iter = map_any_first(config->success); iter != NULL; iter = map_any_next(iter)) {
            map_any_iterator *result_iter = map_any_find(params, iter->key);
            if (result_iter == NULL || !any_is_equal(&iter->val0, &result_iter->val0)) {
                success = false;
                break;
            }
        }
    }
    if (!success) {
        log_error("%s failed", __FUNCTION__);
    } else {
        const char       *productKey = NULL;
        const char       *deviceName = NULL;
        map_any_iterator *iter       = map_any_find(params, NAME_DEV_PRODUCT_KEY);
        if (iter != NULL && any_is_string(&iter->val0)) {
            productKey = iter->val0.u.sval;
        }
        iter = map_any_find(params, NAME_DEV_DEVICE_NAME);
        if (iter != NULL && any_is_string(&iter->val0)) {
            deviceName = iter->val0.u.sval;
        }
        if (productKey != NULL && deviceName != NULL) {
            handle_topo_join(productKey, deviceName, NULL);
        } else {
            log_error("%s cannot find productKye or deviceName");
        }
    }
    map_any_destroy_ex(params);
    return 1;
}

// 2.2.1 子设备入网
int handle_topo_join(const char *productKey, const char *deviceName, const char *deviceSecret) {
    if (!g_config.topo_join.enable || g_config.topo_join.req_topic == NULL) {
        handle_sub_login(productKey, deviceName);
        return ERR_SUCCESS;
    }
    map_any *context = map_any_create();
    map_any_insert(context, NAME_DEV_PRODUCT_KEY, any_from_const_string(productKey));
    map_any_insert(context, NAME_DEV_DEVICE_NAME, any_from_const_string(deviceName));
    config_func_t *config = &g_config.topo_join;
    int            ret    = common_mqtt_publish(config->req_topic, config->req_payload, context);
    map_any_destroy_ex(context);
    if (ret == ERR_SUCCESS && g_config.topo_join.res_topic == NULL) {
        handle_sub_login(productKey, deviceName);
    }
    return ret;
}

int handle_topo_join_reply(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.topo_join;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->res_topic, config->res_payload, params);
    bool success = true;
    if (config->success != NULL) {
        for (map_any_iterator *iter = map_any_first(config->success); iter != NULL; iter = map_any_next(iter)) {
            map_any_iterator *result_iter = map_any_find(params, iter->key);
            if (result_iter == NULL || !any_is_equal(&iter->val0, &result_iter->val0)) {
                success = false;
                break;
            }
        }
    }
    if (!success) {
        log_error("%s failed", __FUNCTION__);
    } else {
        const char       *productKey = NULL;
        const char       *deviceName = NULL;
        map_any_iterator *iter       = map_any_find(params, NAME_DEV_PRODUCT_KEY);
        if (iter != NULL && any_is_string(&iter->val0)) {
            productKey = iter->val0.u.sval;
        }
        iter = map_any_find(params, NAME_DEV_DEVICE_NAME);
        if (iter != NULL && any_is_string(&iter->val0)) {
            deviceName = iter->val0.u.sval;
        }
        if (productKey != NULL && deviceName != NULL) {
            handle_sub_login(productKey, deviceName);
        } else {
            log_error("%s cannot find productKey(%s) or deviceName(%s)", __FUNCTION__, productKey, deviceName);
        }
    }
    map_any_destroy_ex(params);
    return 1;
}

// 2.2.2 子设备离网
int handle_topo_leave(const char *productKey, const char *deviceName) {
    config_func_t *config = &g_config.topo_leave;
    if (!config->enable || config->req_topic == NULL) {
        return ERR_SUCCESS;
    }
    remove_device(deviceName);
    map_any *context = map_any_create();
    map_any_insert(context, NAME_DEV_PRODUCT_KEY, any_from_const_string(productKey));
    map_any_insert(context, NAME_DEV_DEVICE_NAME, any_from_const_string(deviceName));
    int ret = common_mqtt_publish(config->req_topic, config->req_payload, context);
    map_any_destroy_ex(context);
    if (ret == ERR_SUCCESS && config->res_topic == NULL) {
        handle_sub_logout(productKey, deviceName);
    }
    return ret;
}

int handle_topo_leave_reply(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.topo_leave;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->res_topic, config->res_payload, params);
    bool success = true;
    if (config->success != NULL) {
        for (map_any_iterator *iter = map_any_first(config->success); iter != NULL; iter = map_any_next(iter)) {
            map_any_iterator *result_iter = map_any_find(params, iter->key);
            if (result_iter == NULL || !any_is_equal(&iter->val0, &result_iter->val0)) {
                success = false;
                break;
            }
        }
    }
    if (!success) {
        log_error("%s failed", __FUNCTION__);
    }
    map_any_destroy_ex(params);
    return 1;
}

// 2.2.3 获取设备的拓扑关系
int handle_topo_get(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    return 1;
}

int handle_topo_get_reply() {
    return ERR_SUCCESS;
}

// 2.2.4 通知网关添加设备拓扑关系
int handle_topo_add(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    return 1;
}

int handle_topo_add_reply() {
    return ERR_SUCCESS;
}

// 2.2.5 通知网关删除设备拓扑关系
int handle_topo_delete(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.topo_delete;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->req_topic, config->req_payload, params);
    map_any_iterator *iter = map_any_find(params, NAME_DEV_DEVICE_NAME);
    if (iter != NULL) {
        if (any_is_valid_string(&iter->val0)) {
            remove_device(iter->val0.u.sval);
        } else if (any_is_sequence(&iter->val0)) {
            for (int i = 0; i < any_length(&iter->val0); i++) {
                if (any_is_valid_string(&iter->val0.u.aval[i])) {
                    remove_device(iter->val0.u.aval[i].u.sval);
                }
            }
        }
    }
    any_t code = {.type = kInteger, .u = {.ival = 0}};
    map_any_insert(params, "CODE", code);
    handle_topo_delete_reply(params);
    map_any_destroy_ex(params);
    return 1;
}

int handle_topo_delete_reply(map_any *params) {
    config_func_t *config = &g_config.topo_delete;
    return common_mqtt_publish(config->res_topic, config->res_payload, params);
}

// 2.3.1 设备上线
void handle_login(void *context, int code) {
    config_func_t *config = &g_config.gw_login;
    if (!config->enable || config->req_topic == NULL) {
        report_version();
        return;
    }
    common_mqtt_publish(config->req_topic, config->req_payload, NULL);
    if (config->res_topic == NULL) {
        report_version();
    }
}

int handle_login_reply(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.gw_login;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->res_topic, config->res_payload, params);
    bool success = true;
    if (config->success != NULL) {
        for (map_any_iterator *iter = map_any_first(config->success); iter != NULL; iter = map_any_next(iter)) {
            map_any_iterator *result_iter = map_any_find(params, iter->key);
            if (result_iter == NULL || !any_is_equal(&iter->val0, &result_iter->val0)) {
                success = false;
                break;
            }
        }
    }
    if (!success) {
        log_error("%s failed", __FUNCTION__);
    }
    map_any_destroy_ex(params);
    report_version();
    return 1;
}

// 2.4.1 子设备上线
int handle_sub_login(const char *productKey, const char *deviceName) {
    config_func_t *config = &g_config.dev_login;
    if (!config->enable || config->req_topic == NULL) {
        return ERR_SUCCESS;
    }
    map_any *context = map_any_create();
    map_any_insert(context, NAME_DEV_PRODUCT_KEY, any_from_const_string(productKey));
    map_any_insert(context, NAME_DEV_DEVICE_NAME, any_from_const_string(deviceName));
    int ret = common_mqtt_publish(config->req_topic, config->req_payload, context);
    map_any_destroy_ex(context);
    return ret;
}

int handle_sub_login_reply(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.dev_login;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->res_topic, config->res_payload, params);
    bool success = true;
    if (config->success != NULL) {
        for (map_any_iterator *iter = map_any_first(config->success); iter != NULL; iter = map_any_next(iter)) {
            map_any_iterator *result_iter = map_any_find(params, iter->key);
            if (result_iter == NULL || !any_is_equal(&iter->val0, &result_iter->val0)) {
                success = false;
                break;
            }
        }
    }
    if (!success) {
        log_error("%s failed", __FUNCTION__);
    }
    map_any_destroy_ex(params);
    return 1;
}

// 2.4.2 子设备下线
int handle_sub_logout(const char *productKey, const char *deviceName) {
    config_func_t *config = &g_config.dev_logout;
    if (!config->enable || config->req_topic == NULL) {
        return ERR_SUCCESS;
    }
    map_any *context = map_any_create();
    map_any_insert(context, NAME_DEV_PRODUCT_KEY, any_from_const_string(productKey));
    map_any_insert(context, NAME_DEV_DEVICE_NAME, any_from_const_string(deviceName));
    int ret = common_mqtt_publish(config->req_topic, config->req_payload, context);
    map_any_destroy_ex(context);
    return ret;
}

int handle_sub_logout_reply(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.dev_logout;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->res_topic, config->res_payload, params);
    bool success = true;
    if (config->success != NULL) {
        for (map_any_iterator *iter = map_any_first(config->success); iter != NULL; iter = map_any_next(iter)) {
            map_any_iterator *result_iter = map_any_find(params, iter->key);
            if (result_iter == NULL || !any_is_equal(&iter->val0, &result_iter->val0)) {
                success = false;
                break;
            }
        }
    }
    if (!success) {
        log_error("%s failed", __FUNCTION__);
    }
    map_any_destroy_ex(params);
    return 1;
}

// 3.1.1 设备上报属性
int handle_event_property_post(const char *productKey, const char *deviceName, const RexPropertyMsg_t *msg) {
    config_func_t *config = &g_config.dev_property_post;
    if (!config->enable || config->req_topic == NULL) {
        return ERR_SUCCESS;
    }
    int ret = ERR_SUCCESS;
    for (int i = 0; i < msg->propertyNum; i++) {
        map_any *context = map_any_create();
        map_any_insert(context, NAME_DEV_PRODUCT_KEY, any_from_const_string(productKey));
        map_any_insert(context, NAME_DEV_DEVICE_NAME, any_from_const_string(deviceName));
        map_any_insert(context, NAME_COMMON_PROPERTY_NAME, any_from_const_string(msg->propertyList[i].name));
        switch (msg->propertyList[i].value.type) {
        case TSL_DATATYPE_NONE:
            map_any_insert(context, NAME_COMMON_PROPERTY_VALUE, any_create_null());
            break;
        case TSL_DATATYPE_BOOL:
            map_any_insert(context, NAME_COMMON_PROPERTY_VALUE, any_from_bool(msg->propertyList[i].value.u.boolVal == 1));
            break;
        case TSL_DATATYPE_INT:
        case TSL_DATATYPE_ENUM:
            map_any_insert(context, NAME_COMMON_PROPERTY_VALUE, any_from_int(msg->propertyList[i].value.u.intVal));
            break;
        case TSL_DATATYPE_FLOAT:
            map_any_insert(context, NAME_COMMON_PROPERTY_VALUE, any_from_float(msg->propertyList[i].value.u.floatVal));
            break;
        case TSL_DATATYPE_DOUBLE:
            map_any_insert(context, NAME_COMMON_PROPERTY_VALUE, any_from_float(msg->propertyList[i].value.u.doubleVal));
            break;
        case TSL_DATATYPE_TEXT:
            map_any_insert(context, NAME_COMMON_PROPERTY_VALUE, any_from_const_string(msg->propertyList[i].value.u.strVal.str));
            break;
        default:
            break;
        }
        int ret0 = common_mqtt_publish(config->req_topic, config->req_payload, context);
        if (ret == ERR_SUCCESS && ret0 != ERR_SUCCESS) {
            ret = ret0;
        }
        map_any_destroy_ex(context);
    }
    return ret;
}

int handle_event_property_post_reply(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.dev_property_post;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->res_topic, config->res_payload, params);
    bool success = true;
    if (config->success != NULL) {
        for (map_any_iterator *iter = map_any_first(config->success); iter != NULL; iter = map_any_next(iter)) {
            map_any_iterator *result_iter = map_any_find(params, iter->key);
            if (result_iter == NULL || !any_is_equal(&iter->val0, &result_iter->val0)) {
                success = false;
                break;
            }
        }
    }
    if (!success) {
        log_error("%s failed", __FUNCTION__);
    }
    map_any_destroy_ex(params);
    return 1;
}

static void set_property(const char *productKey, const char *deviceName, const char *propertyName, any_t propertyVal) {
    RexCommand_t cmd;
    memset(&cmd, 0, sizeof(RexCommand_t));
    cmd.sequence              = get_sequence();
    cmd.type                  = CMD_SET;
    map_device_iterator *iter = map_device_find(instance.devices, deviceName);
    if (iter == NULL) {
        return;
    }
    strcpy(cmd.u.setPropertyCommand.mac, deviceName);
    const RexTslModelInfo_t *tsl = iter->val0;
    if (productKey != NULL && strcmp(tsl->productId, productKey) != 0) {
        log_warn("device(%s) invalid productKey(%s), expect(%s)", deviceName, productKey, tsl->productId);
        return;
    }
    strcpy(cmd.u.setPropertyCommand.productId, tsl->productId);
    strncpy(cmd.u.setPropertyCommand.property.name, propertyName, PROPERTY_NAME_LEN);
    for (int j = 0; j < tsl->propertyInfoNum; j++) {
        if (strcmp(tsl->propertyInfoList[j].identifier, propertyName) == 0) {
            cmd.u.setPropertyCommand.property.value.type = tsl->propertyInfoList[j].datatype.type;
            bool match                                   = false;
            switch (tsl->propertyInfoList[j].datatype.type) {
            case TSL_DATATYPE_BOOL:
                if (any_is_true(&propertyVal)) {
                    cmd.u.setPropertyCommand.property.value.u.boolVal = true;
                    match                                             = true;
                } else if (any_is_false(&propertyVal)) {
                    cmd.u.setPropertyCommand.property.value.u.boolVal = false;
                    match                                             = true;
                } else {
                    log_warn("property(%s) should be bool", propertyName);
                }
                break;
            case TSL_DATATYPE_INT:
            case TSL_DATATYPE_ENUM:
                if (any_is_integer(&propertyVal)) {
                    cmd.u.setPropertyCommand.property.value.u.intVal = (int32_t)propertyVal.u.ival;
                    match                                            = true;
                } else {
                    log_warn("property(%s) should be int", propertyName);
                }
                break;
            case TSL_DATATYPE_FLOAT:
                if (any_is_float(&propertyVal)) {
                    cmd.u.setPropertyCommand.property.value.u.floatVal = propertyVal.u.fval;
                    match                                              = true;
                } else {
                    log_warn("property(%s) should be float", propertyName);
                }
                break;
            case TSL_DATATYPE_DOUBLE:
                if (any_is_double(&propertyVal)) {
                    cmd.u.setPropertyCommand.property.value.u.doubleVal = propertyVal.u.dval;
                    match                                               = true;
                } else {
                    log_warn("property(%s) should be double", propertyName);
                }
                break;
            case TSL_DATATYPE_TEXT:
                if (any_is_valid_string(&propertyVal)) {
                    cmd.u.setPropertyCommand.property.value.u.strVal.str     = propertyVal.u.sval;
                    cmd.u.setPropertyCommand.property.value.u.strVal.charNum = any_length(&propertyVal);
                    match                                                    = true;
                } else {
                    log_warn("property(%s) should be string", propertyName);
                }
                break;
            default:
                log_warn("property(%s) unknown datatype(%d)", propertyName, tsl->propertyInfoList[j].datatype.type);
            }
            if (match) {
                int ret = instance.processCmd(instance.context, &cmd);
                if (ret != 0) {
                    log_error("call processCmd %d", ret);
                }
            }
            break;
        }
    }
}

// 3.1.2 设置设备属性
int handle_service_property_set(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.dev_property_set;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->req_topic, config->req_payload, params);
    const char       *productKey = NULL;
    const char       *deviceName = NULL;
    map_any_iterator *iter       = map_any_find(params, NAME_DEV_PRODUCT_KEY);
    if (iter != NULL && any_is_string(&iter->val0)) {
        productKey = iter->val0.u.sval;
    }
    iter = map_any_find(params, NAME_DEV_DEVICE_NAME);
    if (iter != NULL && any_is_string(&iter->val0)) {
        deviceName = iter->val0.u.sval;
    }
    iter                       = map_any_find(params, NAME_COMMON_PROPERTY_NAME);
    map_any_iterator *val_iter = map_any_find(params, NAME_COMMON_PROPERTY_VALUE);
    if (deviceName != NULL && iter != NULL && val_iter != NULL) {
        if (any_is_valid_string(&iter->val0)) {
            set_property(productKey, deviceName, iter->val0.u.sval, val_iter->val0);
        } else if (any_is_sequence(&iter->val0) && any_is_sequence(&val_iter->val0) && any_length(&iter->val0) == any_length(&val_iter->val0)) {
            for (int i = 0; i < any_length(&iter->val0); i++) {
                if (any_is_valid_string(&iter->val0.u.aval[i])) {
                    set_property(productKey, deviceName, iter->val0.u.aval[i].u.sval, val_iter->val0.u.aval[i]);
                }
            }
        }
    }
    any_t code = {.type = kInteger, .u = {.ival = 0}};
    map_any_insert(params, "CODE", code);
    handle_service_property_set_reply(params);
    map_any_destroy_ex(params);
    return 1;
}

int handle_service_property_set_reply(map_any *params) {
    config_func_t *config = &g_config.dev_property_set;
    if (!config->enable || config->res_topic == NULL) {
        return ERR_SUCCESS;
    }
    return common_mqtt_publish(config->res_topic, config->res_payload, params);
}

// 3.1.3 设置上报事件
int handle_event_post(const char *productKey, const char *deviceName, RexEvent_t *event) {
    config_func_t *config = NULL;
    for (int i = 0; i < g_config.dev_event_post_cnt; i++) {
        bool           satisfy = true;
        config_func_t *cfg     = &g_config.dev_event_post[i];
        if (!cfg->enable || !cfg->req_topic) {
            satisfy = false;
        }
        if (satisfy && cfg->conditions) {
            for (map_any_iterator *iter = map_any_first(cfg->conditions); iter != NULL && satisfy; iter = map_any_next(iter)) {
                if (strcmp(iter->key, NAME_DEV_EVENT_NAME) == 0) {
                    if (!any_is_valid_string(&iter->val0) || strcmp(iter->val0.u.sval, event->name) != 0) {
                        satisfy = false;
                    }
                } else if (strcmp(iter->key, NAME_DEV_PRODUCT_KEY) == 0) {
                    if (!any_is_valid_string(&iter->val0) || !strcmp(iter->val0.u.sval, productKey) != 0) {
                        satisfy = false;
                    }
                }
            }
        }
        if (satisfy) {
            config = cfg;
        }
    }
    if (config == NULL) {
        log_warn("unable to find such handler(%s)", event->name);
        return ERR_SUCCESS;
    }
    map_any *context = map_any_create();
    map_any_insert(context, NAME_DEV_PRODUCT_KEY, any_from_const_string(productKey));
    map_any_insert(context, NAME_DEV_DEVICE_NAME, any_from_const_string(deviceName));
    map_any_insert(context, NAME_DEV_EVENT_NAME, any_from_const_string(event->name));
    map_any_iterator *it_param = map_any_insert(context, NAME_COMMON_PARAMS, any_create_sequence());
    if (event->paramNum > 0) {
        for (int i = 0; i < event->paramNum; i++) {
            if (event->paramList[i].value.type == TSL_DATATYPE_TEXT) {
                any_t item = any_create_map();
                any_map_add(&item, NAME_COMMON_KEY, any_from_const_string(event->paramList[i].name));
                any_map_add(&item, NAME_COMMON_VALUE, any_from_const_string(event->paramList[i].value.u.strVal.str));
                any_append(&it_param->val0, item);
            }
        }
    }
    int ret = common_mqtt_publish(config->req_topic, config->req_payload, context);
    map_any_destroy_ex(context);
    return ret;
}

int handle_event_post_reply(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s %.*s", __FUNCTION__, topic, length, message);
    return 1;
}

static int handle_common_service(const char *productKey, const char *deviceName, const char *serviceName, config_func_t *config, map_any *callParams,
                                 map_any *params) {
    if (!productKey || !deviceName || !serviceName || !config) {
        return ERR_INVALID_ARGUMENT;
    }
    RexCommand_t cmd;
    memset(&cmd, 0, sizeof(RexCommand_t));
    cmd.sequence = get_sequence();
    cmd.type     = CMD_SERVICE;
    strcpy(cmd.u.serviceCommand.productId, productKey);
    strcpy(cmd.u.serviceCommand.mac, deviceName);
    strcpy(cmd.u.serviceCommand.service.name, serviceName);
    int paramCount = 0;
    if (callParams) {
        for (map_any_iterator *iter = map_any_first(callParams); iter != NULL; iter = map_any_next(iter)) {
            paramCount++;
        }
    }
    if (paramCount > 0) {
        cmd.u.serviceCommand.service.paramNum  = paramCount;
        cmd.u.serviceCommand.service.paramList = malloc(paramCount * sizeof(RexParam_t));
        if (cmd.u.serviceCommand.service.paramList == NULL) {
            log_error("malloc %d size failed", paramCount * sizeof(RexParam_t));
            return ERR_OUT_OF_MEMORY;
        }
        int i = 0;
        for (map_any_iterator *iter = map_any_first(callParams); iter != NULL; iter = map_any_next(iter), i++) {
            RexParam_t *p = &cmd.u.serviceCommand.service.paramList[i];
            strcpy(p->name, iter->key);
            if (any_is_bool(&iter->val0)) {
                p->value.type      = TSL_DATATYPE_BOOL;
                p->value.u.boolVal = any_is_true(&iter->val0);
            } else if (any_is_single(&iter->val0)) {
                p->value.type       = TSL_DATATYPE_FLOAT;
                p->value.u.floatVal = iter->val0.u.fval;
            } else if (any_is_double(&iter->val0)) {
                p->value.type        = TSL_DATATYPE_DOUBLE;
                p->value.u.doubleVal = iter->val0.u.dval;
            } else if (any_is_integer(&iter->val0)) {
                p->value.type     = TSL_DATATYPE_INT;
                p->value.u.intVal = (int32_t)iter->val0.u.ival;
            } else if (any_is_valid_string(&iter->val0)) {
                p->value.type             = TSL_DATATYPE_TEXT;
                p->value.u.strVal.str     = iter->val0.u.sval;
                p->value.u.strVal.charNum = any_length(&iter->val0);
            }
        }
    }
    int ret = instance.processCmd(instance.context, &cmd);
    if (ret != 0) {
        log_error("call processCmd %d", ret);
        handle_service_reply(cmd.sequence, ret, params);
    } else {
        any_t configVal = {.type = kCustom, .u = {.cval = config}};
        map_any_insert(params, "_CONFIG", configVal);
        map_request_insert(instance.requests, cmd.sequence, handle_service_reply, params);
    }
    if (paramCount > 0) {
        free(cmd.u.serviceCommand.service.paramList);
    }
    return ret;
}

int handle_service(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    int id = (int)(intptr_t)context;
    if (id >= g_config.dev_service_call_cnt) {
        log_error("invalid service id: %d", id);
        return 1;
    }
    config_func_t *config = &g_config.dev_service_call[id];
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->req_topic, config->req_payload, params);
    if (config->conditions) {
        for (map_any_iterator *iter = map_any_first(config->conditions); iter != NULL; iter++) {
            map_any_insert(params, iter->key, iter->val0);
        }
    }
    const char       *productKey  = NULL;
    const char       *deviceName  = NULL;
    const char       *serviceName = NULL;
    map_any_iterator *iter        = map_any_find(params, NAME_DEV_PRODUCT_KEY);
    if (iter != NULL && any_is_string(&iter->val0)) {
        productKey = iter->val0.u.sval;
    }
    iter = map_any_find(params, NAME_DEV_DEVICE_NAME);
    if (iter != NULL && any_is_string(&iter->val0)) {
        deviceName = iter->val0.u.sval;
    }
    iter = map_any_find(params, NAME_DEV_SERVICE_NAME);
    if (iter != NULL && any_is_string(&iter->val0)) {
        serviceName = iter->val0.u.sval;
    }
    if (deviceName != NULL && serviceName != NULL) {
        int      paramCount        = 0;
        map_any *call_params       = map_any_create();
        iter                       = map_any_find(params, NAME_COMMON_PARAM_NAME);
        map_any_iterator *val_iter = map_any_find(params, NAME_COMMON_PARAM_VALUE);
        if (iter != NULL && val_iter != NULL) {
            if (any_is_valid_string(&iter->val0)) {
                paramCount = 1;
                map_any_insert(call_params, iter->val0.u.sval, val_iter->val0);
            } else if (any_is_sequence(&iter->val0) && any_is_sequence(&val_iter->val0) && any_length(&iter->val0) == any_length(&val_iter->val0)) {
                for (int i = 0; i < any_length(&iter->val0); i++) {
                    if (any_is_valid_string(&iter->val0.u.aval[i])) {
                        map_any_insert(call_params, iter->val0.u.aval[i].u.sval, val_iter->val0.u.aval[i]);
                        paramCount++;
                    }
                }
            }
        }
        if (productKey == NULL) {
            map_device_iterator *iter = map_device_find(instance.devices, deviceName);
            if (iter != NULL) {
                const RexTslModelInfo_t *tsl = iter->val0;
                productKey                   = tsl->productId;
            }
        }
        if (handle_common_service(productKey, deviceName, serviceName, config, call_params, params) == ERR_SUCCESS) {
            return 1;
        }
    }
    map_any_destroy_ex(params);
    return 1;
}

int handle_service_reply(int id, int code, void *data) {
    if (data == NULL) {
        return ERR_INVALID_ARGUMENT;
    }
    int      ret    = ERR_SUCCESS;
    map_any *params = data;
    if (code != 0) {
        code += 3000;
    }
    any_t code_value = {.type = kInteger, .u = {.ival = code}};
    map_any_insert(params, "CODE", code_value);
    map_any_iterator *iter = map_any_find(params, "_CONFIG");
    if (iter != NULL) {
        config_func_t *config = iter->val0.u.cval;
        if (config && config->enable && config->res_topic != NULL) {
            ret = common_mqtt_publish(config->res_topic, config->res_payload, params);
        }
    } else {
        log_warn("unable to find config");
    }
    map_any_destroy_ex(params);
    return ret;
}

// 3.1.4 服务调用
int handle_service_create_zigbee_network(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.create_zigbee_network;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->req_topic, config->req_payload, params);
    int               paramCount  = 0;
    map_any          *call_params = map_any_create();
    map_any_iterator *iter        = map_any_find(params, NAME_COMMON_PARAM_NAME);
    map_any_iterator *val_iter    = map_any_find(params, NAME_COMMON_PARAM_VALUE);
    if (iter != NULL && val_iter != NULL) {
        if (any_is_valid_string(&iter->val0)) {
            paramCount = 1;
            map_any_insert(call_params, iter->val0.u.sval, val_iter->val0);
        } else if (any_is_sequence(&iter->val0) && any_is_sequence(&val_iter->val0) && any_length(&iter->val0) == any_length(&val_iter->val0)) {
            for (int i = 0; i < any_length(&iter->val0); i++) {
                if (any_is_valid_string(&iter->val0.u.aval[i])) {
                    map_any_insert(call_params, iter->val0.u.aval[i].u.sval, val_iter->val0.u.aval[i]);
                    paramCount++;
                }
            }
        }
    }
    if (handle_common_service(instance.productKey, "", "FormRandomZigbeeNetwork", config, call_params, params) != ERR_SUCCESS) {
        map_any_destroy_ex(params);
    }
    return 1;
}

int handle_service_create_zigbee_network_reply(int id, int code, void *data) {
    return handle_service_reply(id, code, data);
}

int handle_service_leave_zigbee_network(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.leave_zigbee_network;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->req_topic, config->req_payload, params);
    int               paramCount  = 0;
    map_any          *call_params = map_any_create();
    map_any_iterator *iter        = map_any_find(params, NAME_COMMON_PARAM_NAME);
    map_any_iterator *val_iter    = map_any_find(params, NAME_COMMON_PARAM_VALUE);
    if (iter != NULL && val_iter != NULL) {
        if (any_is_valid_string(&iter->val0)) {
            paramCount = 1;
            map_any_insert(call_params, iter->val0.u.sval, val_iter->val0);
        } else if (any_is_sequence(&iter->val0) && any_is_sequence(&val_iter->val0) && any_length(&iter->val0) == any_length(&val_iter->val0)) {
            for (int i = 0; i < any_length(&iter->val0); i++) {
                if (any_is_valid_string(&iter->val0.u.aval[i])) {
                    map_any_insert(call_params, iter->val0.u.aval[i].u.sval, val_iter->val0.u.aval[i]);
                    paramCount++;
                }
            }
        }
    }
    if (handle_common_service(instance.productKey, "", "LeaveZigbeeNetwork", config, call_params, params) != ERR_SUCCESS) {
        map_any_destroy_ex(params);
    }
    return 1;
}

int handle_service_leave_zigbee_network_reply(int id, int code, void *data) {
    return handle_service_reply(id, code, data);
}

int handle_service_start_permit_join(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.start_permit_join;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->req_topic, config->req_payload, params);
    int               paramCount  = 0;
    map_any          *call_params = map_any_create();
    map_any_iterator *iter        = map_any_find(params, NAME_COMMON_PARAM_NAME);
    map_any_iterator *val_iter    = map_any_find(params, NAME_COMMON_PARAM_VALUE);
    if (iter != NULL && val_iter != NULL) {
        if (any_is_valid_string(&iter->val0)) {
            paramCount = 1;
            map_any_insert(call_params, iter->val0.u.sval, any_duplicate(&val_iter->val0));
        } else if (any_is_sequence(&iter->val0) && any_is_sequence(&val_iter->val0) && any_length(&iter->val0) == any_length(&val_iter->val0)) {
            for (int i = 0; i < any_length(&iter->val0); i++) {
                if (any_is_valid_string(&iter->val0.u.aval[i])) {
                    map_any_insert(call_params, iter->val0.u.aval[i].u.sval, any_duplicate(&val_iter->val0.u.aval[i]));
                    paramCount++;
                }
            }
        }
    }
    RexCommand_t cmd;
    memset(&cmd, 0, sizeof(RexCommand_t));
    cmd.sequence               = get_sequence();
    cmd.type                   = CMD_PAIR;
    cmd.u.pairCommand.duration = 30;
    bool need_free             = false;
    for (map_any_iterator *iter = map_any_first(call_params); iter != NULL; iter = map_any_next(iter)) {
        if (strcmp(iter->key, "duration") == 0) {
            if (any_is_integer(&iter->val0)) {
                cmd.u.pairCommand.duration = (int)iter->val0.u.ival;
            }
        } else if (strcmp(iter->key, "installCode") == 0) {
            if (any_is_valid_string(&iter->val0)) {
                if (any_length(&iter->val0) < sizeof(cmd.u.pairCommand.installCode)) {
                    memcpy(cmd.u.pairCommand.installCode, iter->val0.u.sval, any_length(&iter->val0));
                } else {
                    memcpy(cmd.u.pairCommand.installCode, iter->val0.u.sval, sizeof(cmd.u.pairCommand.installCode));
                }
                cmd.u.pairCommand.type = PERMIT_TYPE_INSTALL_CODE;
            }
        } else if (strcmp(iter->key, "mac") == 0) {
            if (any_is_valid_string(&iter->val0)) {
                cmd.u.pairCommand.macList = malloc(sizeof(RexMacInfo_t));
                memset(cmd.u.pairCommand.macList, 0, sizeof(RexMacInfo_t));
                if (cmd.u.pairCommand.macList != NULL) {
                    need_free                = true;
                    cmd.u.pairCommand.macNum = 1;
                    strncpy(cmd.u.pairCommand.macList[0].mac, iter->val0.u.sval, MAC_LEN);
                    if (cmd.u.pairCommand.type == PERMIT_TYPE_ALL) {
                        cmd.u.pairCommand.type = PERMIT_TYPE_WHITE_LIST;
                    }
                } else {
                    log_error("%s malloc %d bytes failed", __FUNCTION__, sizeof(RexMacInfo_t));
                }
            } else if (any_is_sequence(&iter->val0)) {
                cmd.u.pairCommand.macList = malloc(any_length(&iter->val0) * sizeof(RexMacInfo_t));
                memset(cmd.u.pairCommand.macList, 0, any_length(&iter->val0) * sizeof(RexMacInfo_t));
                if (cmd.u.pairCommand.macList != NULL) {
                    need_free = true;
                    for (int i = 0; i < any_length(&iter->val0); i++) {
                        if (any_is_valid_string(&iter->val0.u.aval[i])) {
                            strncpy(cmd.u.pairCommand.macList[cmd.u.pairCommand.macNum++].mac, iter->val0.u.aval[i].u.sval, MAC_LEN);
                        }
                    }
                    if (cmd.u.pairCommand.type == PERMIT_TYPE_ALL) {
                        cmd.u.pairCommand.type = PERMIT_TYPE_WHITE_LIST;
                    }
                } else {
                    log_error("%s malloc %d bytes failed", __FUNCTION__, any_length(&iter->val0) * sizeof(RexMacInfo_t));
                }
            }
        }
    }
    map_any_destroy_ex(call_params);
    int ret = instance.processCmd(instance.context, &cmd);
    if (ret != 0) {
        log_error("call processCmd %d", ret);
        handle_service_reply(cmd.sequence, ret, NULL);
    } else {
        any_t configVal = {.type = kCustom, .u = {.cval = config}};
        map_any_insert(params, "_CONFIG", configVal);
        map_request_insert(instance.requests, cmd.sequence, handle_service_reply, params);
    }
    if (need_free) {
        free(cmd.u.pairCommand.macList);
    }
    return 1;
}

int handle_service_start_permit_join_reply(int id, int code, void *data) {
    return handle_service_reply(id, code, data);
}

int handle_service_create_scene(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.create_scene;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->req_topic, config->req_payload, params);
    any_t configVal = {.type = kCustom, .u = {.cval = config}};
    map_any_insert(params, "_CONFIG", configVal);
    const char       *sceneId   = NULL;
    char             *sceneJson = NULL;
    int               jsonLen   = 0;
    map_any_iterator *itName    = map_any_find(params, NAME_COMMON_PARAM_NAME);
    map_any_iterator *itVal     = map_any_find(params, NAME_COMMON_PARAM_VALUE);
    if (itName != NULL && itVal != NULL && any_is_sequence(&itName->val0) && any_is_sequence(&itVal->val0) &&
        any_length(&itName->val0) == any_length(&itVal->val0)) {
        for (int i = 0; i < any_length(&itName->val0); i++) {
            if (any_is_valid_string(&itName->val0.u.aval[i]) && any_is_valid_string(&itVal->val0.u.aval[i])) {
                if (strcmp(itName->val0.u.aval[i].u.sval, "id") == 0) {
                    sceneId = itVal->val0.u.aval[i].u.sval;
                } else if (strcmp(itName->val0.u.aval[i].u.sval, "scene") == 0) {
                    sceneJson = itVal->val0.u.aval[i].u.sval;
                    jsonLen   = any_length(&itVal->val0.u.aval[i]);
                }
            }
        }
    }
    if (!sceneId || !sceneJson) {
        handle_service_reply(0, ERR_INVALID_ARGUMENT, params);
        return 1;
    }
    RexScene_t scene;
    memset(&scene, 0, sizeof(scene));
    strncpy(scene.sceneId, sceneId, SCENE_ID_LEN);
    scene.sceneState     = 0xFF;
    scene.sceneConfig    = sceneJson;
    scene.sceneConfigLen = jsonLen;
    RexCommand_t cmd     = {
            .sequence = get_sequence(),
            .type     = CMD_SCENE_UPDATE,
            .u        = {.addSceneCommand = {.sceneNum = 1, .sceneList = &scene}},
    };
    int ret = instance.processCmd(instance.context, &cmd);
    if (ret != 0) {
        log_error("call processCmd %d", ret);
        handle_service_reply(cmd.sequence, ERR_INVALID_ARGUMENT, params);
    } else {
        map_request_insert(instance.requests, cmd.sequence, handle_service_reply, params);
    }
    return 1;
}

int handle_service_delete_scene(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.delete_scene;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->req_topic, config->req_payload, params);
    any_t configVal = {.type = kCustom, .u = {.cval = config}};
    map_any_insert(params, "_CONFIG", configVal);
    const char       *sceneId = NULL;
    map_any_iterator *itName  = map_any_find(params, NAME_COMMON_PARAM_NAME);
    map_any_iterator *itVal   = map_any_find(params, NAME_COMMON_PARAM_VALUE);
    if (itName != NULL && itVal != NULL) {
        if (any_is_valid_string(&itName->val0) && any_is_valid_string(&itVal->val0) && strcmp(itName->val0.u.sval, "id") == 0) {
            sceneId = itVal->val0.u.sval;
        } else if (any_is_sequence(&itName->val0) && any_is_sequence(&itVal->val0) && any_length(&itName->val0) == any_length(&itVal->val0)) {
            for (int i = 0; i < any_length(&itName->val0); i++) {
                if (any_is_valid_string(&itName->val0.u.aval[i]) && any_is_valid_string(&itVal->val0.u.aval[i]) &&
                    strcmp(itName->val0.u.aval[i].u.sval, "id") == 0) {
                    sceneId = itVal->val0.u.aval[i].u.sval;
                }
            }
        }
    }
    if (!sceneId) {
        handle_service_reply(0, ERR_INVALID_ARGUMENT, params);
        return 1;
    }
    RexScene_t scene;
    memset(&scene, 0, sizeof(scene));
    strncpy(scene.sceneId, sceneId, SCENE_ID_LEN);
    scene.sceneState     = 0xFF;
    scene.sceneConfig    = NULL;
    scene.sceneConfigLen = 0;
    RexCommand_t cmd     = {
            .sequence = get_sequence(),
            .type     = CMD_SCENE_DEL,
            .u        = {.delSceneCommand = {.sceneNum = 1, .sceneList = &scene}},
    };
    int ret = instance.processCmd(instance.context, &cmd);
    if (ret != 0) {
        log_error("call processCmd %d", ret);
        handle_service_reply(cmd.sequence, ERR_INVALID_ARGUMENT, params);
    } else {
        map_request_insert(instance.requests, cmd.sequence, handle_service_reply, params);
    }
    return 1;
}

int handle_service_trigger_scene(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.trigger_scene;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->req_topic, config->req_payload, params);
    any_t configVal = {.type = kCustom, .u = {.cval = config}};
    map_any_insert(params, "_CONFIG", configVal);
    const char       *sceneId = NULL;
    map_any_iterator *itName  = map_any_find(params, NAME_COMMON_PARAM_NAME);
    map_any_iterator *itVal   = map_any_find(params, NAME_COMMON_PARAM_VALUE);
    if (itName != NULL && itVal != NULL) {
        if (any_is_valid_string(&itName->val0) && any_is_valid_string(&itVal->val0) && strcmp(itName->val0.u.sval, "id") == 0) {
            sceneId = itVal->val0.u.sval;
        } else if (any_is_sequence(&itName->val0) && any_is_sequence(&itVal->val0) && any_length(&itName->val0) == any_length(&itVal->val0)) {
            for (int i = 0; i < any_length(&itName->val0); i++) {
                if (any_is_valid_string(&itName->val0.u.aval[i]) && any_is_valid_string(&itVal->val0.u.aval[i]) &&
                    strcmp(itName->val0.u.aval[i].u.sval, "id") == 0) {
                    sceneId = itVal->val0.u.aval[i].u.sval;
                }
            }
        }
    }
    if (!sceneId) {
        handle_service_reply(0, ERR_INVALID_ARGUMENT, params);
        return 1;
    }
    RexScene_t scene;
    memset(&scene, 0, sizeof(scene));
    strncpy(scene.sceneId, sceneId, SCENE_ID_LEN);
    scene.sceneState     = 0xFF;
    scene.sceneConfig    = NULL;
    scene.sceneConfigLen = 0;
    RexCommand_t cmd     = {
            .sequence = get_sequence(),
            .type     = CMD_SCENE_TRIGGER,
            .u        = {.triggerSceneCommand = {.sceneNum = 1, .sceneList = &scene}},
    };
    int ret = instance.processCmd(instance.context, &cmd);
    if (ret != 0) {
        log_error("call processCmd %d", ret);
        handle_service_reply(cmd.sequence, ERR_INVALID_ARGUMENT, params);
    } else {
        map_request_insert(instance.requests, cmd.sequence, handle_service_reply, params);
    }
    return 1;
}

int handle_service_enable_scene(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.enable_scene;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->req_topic, config->req_payload, params);
    any_t configVal = {.type = kCustom, .u = {.cval = config}};
    map_any_insert(params, "_CONFIG", configVal);
    const char       *sceneId = NULL;
    map_any_iterator *itName  = map_any_find(params, NAME_COMMON_PARAM_NAME);
    map_any_iterator *itVal   = map_any_find(params, NAME_COMMON_PARAM_VALUE);
    if (itName != NULL && itVal != NULL) {
        if (any_is_valid_string(&itName->val0) && any_is_valid_string(&itVal->val0) && strcmp(itName->val0.u.sval, "id") == 0) {
            sceneId = itVal->val0.u.sval;
        } else if (any_is_sequence(&itName->val0) && any_is_sequence(&itVal->val0) && any_length(&itName->val0) == any_length(&itVal->val0)) {
            for (int i = 0; i < any_length(&itName->val0); i++) {
                if (any_is_valid_string(&itName->val0.u.aval[i]) && any_is_valid_string(&itVal->val0.u.aval[i]) &&
                    strcmp(itName->val0.u.aval[i].u.sval, "id") == 0) {
                    sceneId = itVal->val0.u.aval[i].u.sval;
                }
            }
        }
    }
    if (!sceneId) {
        handle_service_reply(0, ERR_INVALID_ARGUMENT, params);
        return 1;
    }
    RexScene_t scene;
    memset(&scene, 0, sizeof(scene));
    strncpy(scene.sceneId, sceneId, SCENE_ID_LEN);
    scene.sceneState     = 1;
    scene.sceneConfig    = NULL;
    scene.sceneConfigLen = 0;
    RexCommand_t cmd     = {
            .sequence = get_sequence(),
            .type     = CMD_SCENE_UPDATE,
            .u        = {.updateSceneCommand = {.sceneNum = 1, .sceneList = &scene}},
    };
    int ret = instance.processCmd(instance.context, &cmd);
    if (ret != 0) {
        log_error("call processCmd %d", ret);
        handle_service_reply(cmd.sequence, ERR_INVALID_ARGUMENT, params);
    } else {
        map_request_insert(instance.requests, cmd.sequence, handle_service_reply, params);
    }
    return 1;
}

int handle_service_disable_scene(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.disable_scene;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->req_topic, config->req_payload, params);
    any_t configVal = {.type = kCustom, .u = {.cval = config}};
    map_any_insert(params, "_CONFIG", configVal);
    const char       *sceneId = NULL;
    map_any_iterator *itName  = map_any_find(params, NAME_COMMON_PARAM_NAME);
    map_any_iterator *itVal   = map_any_find(params, NAME_COMMON_PARAM_VALUE);
    if (itName != NULL && itVal != NULL) {
        if (any_is_valid_string(&itName->val0) && any_is_valid_string(&itVal->val0) && strcmp(itName->val0.u.sval, "id") == 0) {
            sceneId = itVal->val0.u.sval;
        } else if (any_is_sequence(&itName->val0) && any_is_sequence(&itVal->val0) && any_length(&itName->val0) == any_length(&itVal->val0)) {
            for (int i = 0; i < any_length(&itName->val0); i++) {
                if (any_is_valid_string(&itName->val0.u.aval[i]) && any_is_valid_string(&itVal->val0.u.aval[i]) &&
                    strcmp(itName->val0.u.aval[i].u.sval, "id") == 0) {
                    sceneId = itVal->val0.u.aval[i].u.sval;
                }
            }
        }
    }
    if (!sceneId) {
        handle_service_reply(0, ERR_INVALID_ARGUMENT, params);
        return 1;
    }
    RexScene_t scene;
    memset(&scene, 0, sizeof(scene));
    strncpy(scene.sceneId, sceneId, SCENE_ID_LEN);
    scene.sceneState     = 0;
    scene.sceneConfig    = NULL;
    scene.sceneConfigLen = 0;
    RexCommand_t cmd     = {
            .sequence = get_sequence(),
            .type     = CMD_SCENE_UPDATE,
            .u        = {.updateSceneCommand = {.sceneNum = 1, .sceneList = &scene}},
    };
    int ret = instance.processCmd(instance.context, &cmd);
    if (ret != 0) {
        log_error("call processCmd %d", ret);
        handle_service_reply(cmd.sequence, ERR_INVALID_ARGUMENT, params);
    } else {
        map_request_insert(instance.requests, cmd.sequence, handle_service_reply, params);
    }
    return 1;
}

// 3.1.5 设置设备别名
int handle_service_set_alias(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config    = &g_config.dev_set_alias;
    map_any       *params    = map_any_create();
    any_t          configVal = {.type = kCustom, .u = {.cval = config}};
    map_any_insert_ex(params, "_CONFIG", configVal);
    common_mqtt_subscribe(topic, message, length, config->req_topic, config->req_payload, params);
    map_any_iterator *itAlias   = map_any_find(params, NAME_DEV_ALIAS);
    map_any_iterator *itDevName = map_any_find(params, NAME_DEV_DEVICE_NAME);

    if (itAlias == NULL || itDevName == NULL || !any_is_valid_string(&itDevName->val0) || !any_is_valid_string(&itAlias->val0)) {
        handle_service_reply(0, ERR_INVALID_ARGUMENT, params);
    } else {
        int ret = instance.setDevAlias(instance.context, itDevName->val0.u.sval, itAlias->val0.u.sval);
        handle_service_reply(0, ret, params);
    }
    return 1;
}

// 4.1.1 设备上报 OTA 模块版本
int handle_report_version(const char *productKey, const char *deviceName, const char *module, const char *version) {
    config_func_t *config = &g_config.report_version;
    if (!config->enable || config->req_topic == NULL) {
        return ERR_SUCCESS;
    }
    map_any *context = map_any_create();
    map_any_insert(context, NAME_COMMON_MODULE_NAME, any_from_const_string(module));
    map_any_insert(context, NAME_COMMON_VERSION_STR, any_from_const_string(version));
    int ret = common_mqtt_publish(config->req_topic, config->req_payload, context);
    map_any_destroy_ex(context);
    return ret;
}

int handle_report_version_reply(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    return 1;
}

// 4.1.2 平台推送 OTA 升级包信息
int handle_ota_request(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.ota_request;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->req_topic, config->req_payload, params);
    map_any_iterator *iter = map_any_find(params, NAME_COMMON_OTA_URL);
    if (iter != NULL && any_is_valid_string(&iter->val0) && iter->val0.u.sval != NULL) {
        RexCommand_t cmd;
        memset(&cmd, 0, sizeof(RexCommand_t));
        cmd.sequence = get_sequence();
        cmd.type     = CMD_OTA_NOTIFY;
        strcpy(cmd.u.otaCommand.url, iter->val0.u.sval);
        int ret = instance.processCmd(instance.context, &cmd);
        if (ret != 0) {
            log_error("call processCmd %d", ret);
            handle_ota_progress(cmd.sequence, -1, params);
        } else {
            map_request_insert(instance.requests, cmd.sequence, handle_ota_progress, params);
        }
    }
    return 1;
}

int handle_dev_ota_request(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.dev_ota_request;
    map_any       *params = map_any_create();
    common_mqtt_subscribe(topic, message, length, config->req_topic, config->req_payload, params);
    map_any_iterator *it_url = map_any_find(params, NAME_COMMON_OTA_URL);
    map_any_iterator *it_dn  = map_any_find(params, NAME_DEV_DEVICE_NAME);
    if (it_url && any_is_valid_string(&it_url->val0) && it_dn && any_is_valid_string(&it_dn->val0)) {
        RexCommand_t cmd;
        memset(&cmd, 0, sizeof(RexCommand_t));
        cmd.sequence = get_sequence();
        cmd.type     = CMD_OTA_NOTIFY;
        strcpy(cmd.u.otaCommand.url, it_url->val0.u.sval);
        strcpy(cmd.u.otaCommand.mac, it_dn->val0.u.sval);
        int ret = instance.processCmd(instance.context, &cmd);
        if (ret != 0) {
            log_error("call processCmd %d", ret);
            handle_dev_ota_progress(cmd.sequence, -1, params);
        } else {
            map_request_insert(instance.requests, cmd.sequence, handle_dev_ota_progress, params);
        }
    }
    return 1;
}

int handle_ota_request_reply() {
    return ERR_SUCCESS;
}

// 4.1.3 设备上报升级进度
int handle_ota_progress(int id, int code, void *data) {
    config_func_t *config = &g_config.ota_progress;
    if (!config->enable || config->req_topic == NULL) {
        map_any_destroy_ex(data);
        return ERR_SUCCESS;
    }
    map_any *req_context = (map_any *)data;
    map_any *context     = map_any_create();
    for (map_any_iterator *iter = map_any_first(req_context); iter != NULL; iter = map_any_next(iter)) {
        map_any_insert(context, iter->key, any_duplicate(&iter->val0));
    }
    map_any_insert(context, NAME_COMMON_OTA_STEP, any_from_int(code));
    map_any_insert(context, NAME_COMMON_OTA_DESC, any_from_const_string(""));
    int ret = common_mqtt_publish(config->req_topic, config->req_payload, context);
    map_any_destroy_ex(context);
    if ((code < 0 || code >= 100) && data) {
        map_any_destroy_ex(req_context);
    }
    return ret;
}

int handle_ota_progress_reply(const char *topic, const char *message, int length, void *context) {
    log_info("%s %s: %.*s", __FUNCTION__, topic, length, message);
    return 1;
}

int handle_dev_ota_progress(int id, int code, void *data) {
    config_func_t *config = &g_config.dev_ota_progress;
    if (!config->enable || config->req_topic == NULL) {
        map_any_destroy_ex(data);
        return ERR_SUCCESS;
    }
    map_any *req_context = (map_any *)data;
    map_any *context     = map_any_create();
    for (map_any_iterator *iter = map_any_first(req_context); iter != NULL; iter = map_any_next(iter)) {
        map_any_insert(context, iter->key, any_duplicate(&iter->val0));
    }
    map_any_insert(context, NAME_COMMON_OTA_STEP, any_from_int(code));
    map_any_insert(context, NAME_COMMON_OTA_DESC, any_from_const_string(""));
    int ret = common_mqtt_publish(config->req_topic, config->req_payload, context);
    map_any_destroy_ex(context);
    if ((code < 0 || code >= 100) && data) {
        map_any_destroy_ex(req_context);
    }
    return ret;
}
