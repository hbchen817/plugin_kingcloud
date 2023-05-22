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
#include <curl/curl.h>
#include "error.h"

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
    // strcpy(instance.productKey, instance.devList.devices[0].productId);
    strcpy(instance.deviceName, instance.devList.devices[0].mac);
    strcpy(instance.versionCOO, instance.devList.devices[0].version);

    // 开启mqtt
    // 针对金云场景，首先请求一下金山云，更改默认的用户名和密码
    ret = register_kc_gateway(&instance);
    if (0 != ret) {
        log_error("register kc gateway error: %d", ret);
        return 1;
    }

    mqtt_start(instance.mqtt, check_register, NULL, 
                g_config.basic.broker, instance.mqttUsername, instance.mqttPassword);

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
                iter->val0(msg->sequence, 0, iter->val1);
                break;
            case MSG_TYPE_EXECUTE_RESULT:
                iter->val0(msg->sequence, msg->u.resultMsg.result, iter->val1);
                break;
            default:
                iter->val0(msg->sequence, 0, iter->val1);
                break;
            }

            // 应该删除对应seq，不然内存爆炸
            map_request_erase(iter);
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
            handle_sub_register(msg->u.includedMsg.productId, msg->u.includedMsg.mac, msg->u.includedMsg.modelId);
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
            handle_ota_progress(msg->sequence, msg->u.otaProgressMsg.step);
            break;
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
    // if (strcmp(requestType, REQUEST_TYPE_GET_CONF) == 0) {
    //     struct PluginConfig_t config = {.mqttBroker = instance.mqttBroker, .mqttUsername = instance.mqttUsername, .mqttPassword = instance.mqttPassword};
    //     if (config.mqttBroker[0] == '\0') {
    //         config.mqttBroker = g_config.basic.url;
    //         if (g_config.basic.username) {
    //             config.mqttUsername = g_config.basic.username;
    //         }
    //         if (g_config.basic.password) {
    //             config.mqttPassword = g_config.basic.password;
    //         }
    //     }
    //     char *payload = NULL;
    //     int   ret     = reflect_struct2str(&payload, &config, PluginConfig_reflection);
    //     if (ret == REFLECT_ERR_SUCCESS) {
    //         log_info("%p, %s", payload, payload);
    //         getDataCb(ctx, payload);
    //         free(payload);
    //         return 0;
    //     } else {
    //         log_error("reflect_struct2str failed %d", ret);
    //         return -1;
    //     }
    // } else if (strcmp(requestType, REQUEST_TYPE_POST_CONF) == 0) {
    //     struct PluginConfig_t config;
    //     memset(&config, 0, sizeof(struct PluginConfig_t));
    //     int ret = reflect_str2struct(requestData, &config, PluginConfig_reflection);
    //     if (ret == REFLECT_ERR_SUCCESS) {
    //         bool restart = false;
    //         if (config.mqttBroker != NULL && strcmp(instance.mqttBroker, config.mqttBroker)) {
    //             log_info("set broker to: %s", config.mqttBroker);
    //             restart = true;
    //             strcpy(instance.mqttBroker, config.mqttBroker);
    //             instance.writeConf(instance.context, "mqtt_broker", instance.mqttBroker, strlen(instance.mqttBroker));
    //         }
    //         if (config.mqttUsername != NULL && strcmp(instance.mqttUsername, config.mqttUsername)) {
    //             log_info("set username to: %s", config.mqttUsername);
    //             restart = true;
    //             strcpy(instance.mqttUsername, config.mqttUsername);
    //             instance.writeConf(instance.context, "mqtt_username", instance.mqttUsername, strlen(instance.mqttUsername));
    //         }
    //         if (config.mqttPassword != NULL && strcmp(instance.mqttPassword, config.mqttPassword)) {
    //             log_info("set password to: %s", config.mqttPassword);
    //             restart = true;
    //             strcpy(instance.mqttPassword, config.mqttPassword);
    //             instance.writeConf(instance.context, "mqtt_password", instance.mqttPassword, strlen(instance.mqttPassword));
    //         }
    //         reflect_free_ptr(&config, PluginConfig_reflection);
    //         getDataCb(ctx, "{\"code\":200}");
    //         if (restart) {
    //             rex_stop();
    //             mqtt_delete(instance.mqtt);
    //             instance.mqtt = mqtt_new(instance.mqttBroker, instance.mqttUsername, instance.mqttPassword);
    //             rex_start();
    //         }
    //         return 0;
    //     } else {
    //         log_error("reflect_str2struct failed %d", ret);
    //         return -1;
    //     }
    // } else {
    //     log_error("unknown requestType: %s", requestType);
    //     return -1;
    // }
    return 0;
}

#define IV_LEN AES_BLOCK_SIZE

struct message_view_t {
    int  len;
    char msg[2048];
};

static size_t write_data(void *data, size_t size, size_t nmemb, void *userp) {
    struct message_view_t *msg = (struct message_view_t *)userp;
    memcpy(&msg->msg[msg->len], data, size * nmemb);
    msg->len += size * nmemb;
    return size * nmemb;
}

int register_kc_gateway(struct Instance *instance) {
    // 根据协议文档，构造cipherText JSON串，格式如下:
    // {
    //      "clientId":"",
    //      "vendorCode":""
    //  }
    cipher_text cipher_text;
    memset(&cipher_text, 0, sizeof(cipher_text));
    cipher_text.clientId = instance->deviceName;
    cipher_text.vendorCode = instance->vendorCode;
    char* cipher_text_json;
    int ret = csonStruct2JsonStr(&cipher_text_json, &cipher_text, cipher_text_ref);
    if (ERR_SUCCESS != ret) {
        log_error("cson struct[cipher_text] to json error");
        return 1;
    }
    log_info("create cipher text: %s", cipher_text_json);

    // 按照协议规定，需要对cipher_text_json进行aes加密
    // 将产品秘钥(product_secret)作为一个十六进制字符串，解析为一个字节数组(byte[])
    unsigned char product_secret[16] = {0};
    unsigned int product_secret_len = 0;
    decodeHex(instance->deviceSecret, product_secret, &product_secret_len);

    // AES加密
    unsigned char cipher_json_tmp[128] = {0};
    memcpy(cipher_json_tmp, cipher_text_json, strlen(cipher_text_json));
    unsigned char cipherText_bin[128] = {0};
    int cipherText_bin_len = 0;
    AES_ECB_PKCS5Padding_encrypt(cipher_json_tmp, cipherText_bin, &cipherText_bin_len, product_secret);
    char *cipherText_hex = bytes_to_hex(cipherText_bin, cipherText_bin_len);

    // 开始构造请求json串，按照预定，格式如下：
    // {
    //      "cipherText": "",
    //      "productKey": ""
    // }
    device_reg_request reg_request;
    memset(&reg_request, 0, sizeof(device_reg_request));
    reg_request.productKey = instance->productKey;
    reg_request.cipherText = cipherText_hex;
    char* reg_request_json;
    ret = csonStruct2JsonStr(&reg_request_json, &reg_request, device_reg_request_ref);
    if (ERR_SUCCESS != ret) {
        log_error("cson struct[device_reg_request] to json error");
        free(cipher_text_json);
        free(cipherText_hex);
        return 1;
    }

    // 尝试请求金山云
    log_info("begin request gateway: %s, url: %s", reg_request_json, g_config.basic.url);
    CURL *curl = curl_easy_init();
    if (!curl) {
        free(cipher_text_json);
        free(cipherText_hex);
        return -1;
    }
    struct curl_slist *headers  = curl_slist_append(NULL, "Content-Type: application/json");
    headers                     = curl_slist_append(headers, "Connection: Keep-Alive");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // 从配置中读取金山云的url地址
    curl_easy_setopt(curl, CURLOPT_URL, g_config.basic.url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, reg_request_json);
    struct message_view_t ret_msg;
    memset(&ret_msg, 0, sizeof(ret_msg));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret_msg);
    ret = curl_easy_perform(curl);
    if (ERR_SUCCESS != ret) {
        log_error("curl gateway: %s error: %d", g_config.basic.url, ret);
        free(cipher_text_json);
        free(reg_request_json);
        free(cipherText_hex);
        curl_easy_cleanup(curl);
        return ret;
    }

    // free 
    free(cipher_text_json);
    free(cipherText_hex);
    free(reg_request_json);  

    // 如果请求成功了，可以解密mqtt一些参数了
    long response_code = 0L;
    ret_msg.msg[ret_msg.len] = '\0';

    /* 获取响应码 */
    ret = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if(ret != CURLE_OK) {
        log_error("error getting response code: %s\n", curl_easy_strerror(ret));
    }

    curl_easy_cleanup(curl);
    if (response_code == 200) {
        log_warn("register response: %ld %s", response_code, ret_msg.msg);
        device_reg_response reg_response;
        memset(&reg_response, 0, sizeof(device_reg_response));
        ret = csonJsonStr2Struct(ret_msg.msg, &reg_response, device_reg_response_ref);
        if (ret == ERR_SUCCESS) {
            if (reg_response.code == 200) {
                /* Decode encrypted data from hexadecimal string */
                char* data_hex = reg_response.data;
                unsigned char data_bin[1024] = {0};
                unsigned int data_bin_len = 0;
                decodeHex(data_hex, data_bin, &data_bin_len);

                unsigned char data_str[1024] = {0};
                AES_ECB_PKCS5Padding_decrypt(data_bin, data_bin_len, data_str, product_secret);

                // 将相关变量赋值给instance
                reg_response_data data;
                memset(&data, 0, sizeof(reg_response_data));
                ret = csonJsonStr2Struct((char *)data_str, &data, reg_response_data_ref);
                if (ret != ERR_SUCCESS) {
                    log_warn("gateway data is illegal: %s", reg_response.data);
                    ret = 1;
                } else {
                    log_info("get mqtt username: %s, password: %s", data.deviceKey, data.deviceSecret);
                    strcpy(instance->mqttUsername, data.deviceKey);
                    strcpy(instance->mqttPassword, data.deviceSecret);
                }
                csonFreePointer(&data, reg_response_data_ref);
            } else {
                log_warn("register failed: %ld %s %s", reg_response.code, 
                            reg_response.message, reg_response.traceId);
                ret = 1;
            }
            csonFreePointer(&reg_response, device_reg_response_ref);
        }
    } else {
        log_warn("register failed: %ld %s", response_code, ret_msg.msg);
        ret = 1;
    }

    return ret;
}
