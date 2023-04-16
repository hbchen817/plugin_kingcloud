#include "instance.h"
#include "callbacks.h"
#include "config.h"
#include "error.h"
#include "global.h"
#include "helper.h"
#include "if_helper.h"
#include "map.h"
#include "mqtt.h"
#include "protocol.h"
#include "cJSON.h"
#include "aes.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>
#include <curl/curl.h>

#define LOG_LEVEL_FATAL  0
#define LOG_LEVEL_ERROR  1
#define LOG_LEVEL_WARN   2
#define LOG_LEVEL_INFO   3
#define LOG_LEVEL_DEBUG  4
#define LOG_LEVEL_VERBOS 5

IMPLEMENT_QUEUE(timer, int /* fd */)
IMPLEMENT_QUEUE(transaction, int /* id */, Handler /* cb */, void * /* ctx */)
IMPLEMENT_MAP(device, _MAP_COMPARE_STRING, _MAP_COPY_STRING, _MAP_FREE_STRING, char *, const RexTslModelInfo_t * /* tsl */, const char * /* secret*/)
IMPLEMENT_MAP(request, _MAP_COMPARE_ARITHMATIC, _MAP_COPY_ASSIGNMENT, _MAP_FREE_PLAIN, int /* seq */, service_response /* cb */, map_any * /* params */);

struct Instance instance;

static int dummy_process_cmd(void *ctx, RexCommand_t *cmd) {
    return 0;
}

static int dummy_get_dev_list(void *ctx, int *size, RexDev_t **devList) {
    return 0;
}

static int dummy_read_conf(void *ctx, const char *key, void *buffer, int bufferLen) {
    return 0;
}

static int dummy_write_conf(void *ctx, const char *key, const void *buffer, int bufferLen) {
    return 0;
}

static int dummy_log(void *ctx, int level, char *format, ...) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    char   time_buf[100];
    size_t rc = strftime(time_buf, sizeof(time_buf), "%F %T", localtime(&ts.tv_sec));
    rc += snprintf(time_buf + rc, sizeof(time_buf) - rc, ".%06ld", ts.tv_nsec / 1000);
    switch (level) {
    case 0:
        printf("%s [F] ", time_buf);
        break;
    case 1:
        printf("%s [E] ", time_buf);
        break;
    case 2:
        printf("%s [W] ", time_buf);
        break;
    case 3:
        printf("%s [I] ", time_buf);
        break;
    case 4:
        printf("%s [D] ", time_buf);
        break;
    case 5:
        printf("%s [V] ", time_buf);
        break;
    default:
        printf("%s [U] ", time_buf);
        break;
    }
    va_list args;
    va_start(args, format);
    rc += vprintf(format, args);
    va_end(args);
    rc += printf("\n");
    return rc;
}

static int dummy_getZbNetInfo(void *ctx, char *netInfoBuffer, int bufferLen) {
    return 0;
}

static int dummy_setDevAlias(void *ctx, const char *mac, const char *alias) {
    return -1;
}

static int dummy_getVerInfo(void *ctx, int *size, RexVersionInfo_t **verInfoList) {
    return 0;
}

static int dummy_setDevList(void *ctx, int size, RexDev_t *devList) {
    return 0;
}

void instance_init(RexInitParameter_t *parameter, void *ctx) {
    instance.processCmd   = dummy_process_cmd;
    instance.getDevList   = dummy_get_dev_list;
    instance.readConf     = dummy_read_conf;
    instance.writeConf    = dummy_write_conf;
    instance.log          = dummy_log;
    instance.getZbNetInfo = dummy_getZbNetInfo;
    instance.setDevAlias  = dummy_setDevAlias;
    instance.getVerInfo   = dummy_getVerInfo;
    instance.setDevList   = dummy_setDevList;
    if (parameter->processCmd != NULL) {
        instance.processCmd = parameter->processCmd;
        if (parameter->getDevList != NULL) {
            instance.getDevList = parameter->getDevList;
            if (parameter->readConf != NULL) {
                instance.readConf = parameter->readConf;
                if (parameter->writeConf != NULL) {
                    instance.writeConf = parameter->writeConf;
                    if (parameter->log != NULL) {
                        instance.log = parameter->log;
                        if (parameter->getZbNetInfo != NULL) {
                            instance.getZbNetInfo = parameter->getZbNetInfo;
                            if (parameter->setDevAlias != NULL) {
                                instance.setDevAlias = parameter->setDevAlias;
                                if (parameter->getVerInfo != NULL) {
                                    instance.getVerInfo = parameter->getVerInfo;
                                    if (parameter->setDevList != NULL) {
                                        instance.setDevList = parameter->setDevList;
                                    } else {
                                        log_warn("setDevList is NULL");
                                    }
                                } else {
                                    log_warn("getVerInfo is NULL");
                                }
                            } else {
                                log_warn("setDevAlias is NULL");
                            }
                        } else {
                            log_warn("getZbNetInfo is NULL");
                        }
                    }
                }
            }
        }
    }
    instance.context       = ctx;
    
    // 读取自定义yaml配置
    g_config_root = conf_parse_file(CONFIG_FILE_PATH);
    if (NULL == g_config_root) {
        log_error("load from config failed");
        return;
    }

    // 读取product key和vendor code
    // 按照约定，这两个金山云会线下分配
    struct dp_conf_node *node = conf_get_node(g_config_root, KC_CONFIG_PK_KEY);
    if (NULL == node) {
        log_error("please configure product key in yaml!");
        return;
    }
    strcpy(instance.productKey, node->value);

    node = conf_get_node(g_config_root, KC_CONFIG_PS_KEY);
    if (NULL == node) {
        log_error("please configure product secret in yaml!");
        return;
    }
    strcpy(instance.productSecret, node->value);

    node = conf_get_node(g_config_root, KC_CONFIG_VENDOR_KEY);
    if (NULL == node) {
        log_error("please configure vendor code in yaml!");
        return;
    }
    strcpy(instance.vendorCode, node->value);

    node = conf_get_node(g_config_root, KC_CONFIG_CLIENT_ID_KEY);
    if (NULL == node) {
        log_error("please configure client id in yaml!");
        return;
    }
    strcpy(instance.vendorCode, node->value);

    node = conf_get_node(g_config_root, KC_CONFIG_BROKER_KEY);
    if (NULL == node) {
        log_error("please configure broker url in yaml!");
        return;
    }
    strcpy(instance.mqttBroker, node->value);

    // 按照规定初始化topic
    memset(&g_server_topics, 0, sizeof(server_topics));

    // 序列号初始化
    atomic_store(&instance.sequence, (int)time(NULL));

    // device初始化
    mtx_init(&instance.mtxDevices, mtx_plain);

    // 创建device map
    instance.devices      = map_device_create();

    // 创建request map
    instance.requests     = map_request_create();

    // 创建定时器
    instance.timerManager = tm_create(0, "tm_timeout");

    // 定时队列初始化
    queue_timer_init(&instance.activeTimers);
    queue_timer_init(&instance.timerPool);

    // 事务相关初始化
    atomic_init(&instance.transId, 0);
    queue_transaction_init(&instance.transactions);

    // 请求金山云平台，获取相关参数
    int ret = register_gateway();
    if (ret != ERR_SUCCESS) {
        log_error("register gateway error");
        return;
    }

    // TODO: 如何生成mqtt user name & mqtt password

    // 创建mqtt实例
    instance.mqtt = mqtt_new(instance.mqttBroker, instance.mqttUsername, instance.mqttPassword);
}

void instance_deinit() {
    mqtt_delete(instance.mqtt);
    map_device_destroy(instance.devices);
    map_request_destroy(instance.requests);
    mtx_destroy(&instance.mtxDevices);
    for (queue_timer_iterator *iter = queue_timer_front(&instance.activeTimers); iter != NULL; iter = queue_timer_front(&instance.activeTimers)) {
        tm_timer_stop(instance.timerManager, iter->val0);
        close(iter->val0);
        queue_timer_pop_front(&instance.activeTimers);
    }
    for (queue_timer_iterator *iter = queue_timer_front(&instance.timerPool); iter != NULL; iter = queue_timer_front(&instance.timerPool)) {
        close(iter->val0);
        queue_timer_pop_front(&instance.activeTimers);
    }
    queue_timer_deinit(&instance.activeTimers);
    queue_timer_deinit(&instance.timerPool);
    queue_transaction_deinit(&instance.transactions);
    tm_destroy(instance.timerManager);
    free_config();
}

int instance_require_timer() {
    if (queue_timer_empty(&instance.timerPool)) {
        int fd = tm_timer_create(instance.timerManager);
        queue_timer_emplace_back(&instance.activeTimers, fd);
        return fd;
    } else {
        queue_timer_iterator *iter = queue_timer_front(&instance.timerPool);
        queue_timer_move_to_back(&instance.timerPool, iter, &instance.activeTimers);
        return iter->val0;
    }
}

void instance_release_timer(int fd) {
    for (queue_timer_iterator *iter = queue_timer_front(&instance.activeTimers); iter != NULL; iter = iter->next) {
        if (iter->val0 == fd) {
            queue_timer_move_to_front(&instance.activeTimers, iter, &instance.timerPool);
            return;
        }
    }
}

#define SUBSCRIBE_DOWNSTREAM_TOPIC(name, func)                                                                                                                 \
    if (g_config.name.enable && g_config.name.req_topic) {                                                                                                     \
        if (format_string_from_context(topic, sizeof(topic), g_config.name.req_topic, context) < 0) {                                                          \
            log_error("invalid topic: %s", g_config.name.req_topic);                                                                                           \
        } else {                                                                                                                                               \
            mqtt_subscribe_ex(instance.mqtt, topic, func, NULL, logout_topics, strdup(topic));                                                                 \
        }                                                                                                                                                      \
    }
#define SUBSCRIBE_UPSTREAM_TOPIC(name, func)                                                                                                                   \
    if (g_config.name.enable && g_config.name.res_topic) {                                                                                                     \
        if (format_string_from_context(topic, sizeof(topic), g_config.name.res_topic, context) < 0) {                                                          \
            log_error("invalid topic: %s", g_config.name.res_topic);                                                                                           \
        } else {                                                                                                                                               \
            mqtt_subscribe_ex(instance.mqtt, topic, func, NULL, logout_topics, strdup(topic));                                                                 \
        }                                                                                                                                                      \
    }

static void logout_topics(void *context, int code) {
    log_info("subscribe: %s", (const char *)context);
    free(context);
}

void add_device(const char *mac, const RexTslModelInfo_t *tsl, const char *secret) {
    if (mac == NULL || tsl == NULL) {
        return;
    }
    log_info("%s(%s)", __FUNCTION__, mac);
    map_any *context = map_any_create();
    map_any_insert(context, NAME_DEV_PRODUCT_KEY, any_from_const_string(tsl->productId));
    map_any_insert(context, NAME_DEV_DEVICE_NAME, any_from_const_string(mac));
    char topic[256];
    SUBSCRIBE_UPSTREAM_TOPIC(dev_login, handle_sub_login_reply)
    SUBSCRIBE_UPSTREAM_TOPIC(dev_logout, handle_sub_logout_reply)
    SUBSCRIBE_UPSTREAM_TOPIC(dev_property_post, handle_event_property_post_reply)
    SUBSCRIBE_DOWNSTREAM_TOPIC(dev_property_set, handle_service_property_set)
    SUBSCRIBE_DOWNSTREAM_TOPIC(dev_set_alias, handle_service_set_alias)
    SUBSCRIBE_DOWNSTREAM_TOPIC(dev_ota_request, handle_dev_ota_request)
    SUBSCRIBE_UPSTREAM_TOPIC(dev_ota_progress, handle_ota_progress_reply)
    for (int j = 0; j < tsl->eventInfoNum; j++) {
        map_any_iterator *iter = map_any_find(context, NAME_DEV_EVENT_NAME);
        if (iter == NULL) {
            map_any_insert(context, NAME_DEV_EVENT_NAME, any_from_const_string(tsl->eventInfoList[j].identifier));
        } else {
            any_set_const_string(&iter->val0, tsl->eventInfoList[j].identifier);
        }
        for (int i = 0; i < g_config.dev_event_post_cnt; i++) {
            bool           satisfy = true;
            config_func_t *config  = &g_config.dev_event_post[i];
            if (!config->enable || !config->res_topic) {
                satisfy = false;
            }
            if (satisfy && config->conditions) {
                for (map_any_iterator *iter = map_any_first(config->conditions); iter != NULL && satisfy; iter = map_any_next(iter)) {
                    if (strcmp(iter->key, NAME_DEV_EVENT_NAME) == 0) {
                        if (!any_is_string(&iter->val0) || strcmp(iter->val0.u.sval, tsl->eventInfoList[j].identifier) != 0) {
                            satisfy = false;
                        }
                    } else if (strcmp(iter->key, NAME_DEV_PRODUCT_KEY) == 0) {
                        if (!any_is_string(&iter->val0) || strcmp(iter->val0.u.sval, tsl->productId) != 0) {
                            satisfy = false;
                        }
                    }
                }
            }
            if (satisfy) {
                if (format_string_from_context(topic, sizeof(topic), config->res_topic, context) < 0) {
                    log_error("invalid topic: %s", config->res_topic);
                } else {
                    mqtt_subscribe_ex(instance.mqtt, topic, handle_event_post_reply, NULL, logout_topics, strdup(topic));
                }
            }
        }
    }
    map_any_iterator *iter = map_any_find(context, NAME_DEV_EVENT_NAME);
    if (iter != NULL) {
        any_free(&iter->val0);
        map_any_erase(iter);
    }
    for (int j = 0; j < tsl->serviceInfoNum; j++) {
        map_any_iterator *iter = map_any_find(context, NAME_DEV_SERVICE_NAME);
        if (iter == NULL) {
            map_any_insert(context, NAME_DEV_SERVICE_NAME, any_from_const_string(tsl->serviceInfoList[j].identifier));
        } else {
            any_set_const_string(&iter->val0, tsl->serviceInfoList[j].identifier);
        }
        for (int i = 0; i < g_config.dev_service_call_cnt; i++) {
            bool           satisfy = true;
            config_func_t *config  = &g_config.dev_service_call[i];
            if (!config->enable || !config->req_topic) {
                satisfy = false;
            }
            if (satisfy && config->conditions) {
                for (map_any_iterator *iter = map_any_first(config->conditions); iter != NULL && satisfy; iter = map_any_next(iter)) {
                    if (strcmp(iter->key, NAME_DEV_SERVICE_NAME) == 0) {
                        if (!any_is_string(&iter->val0) || strcmp(iter->val0.u.sval, tsl->serviceInfoList[j].identifier) != 0) {
                            satisfy = false;
                        }
                    } else if (strcmp(iter->key, NAME_DEV_PRODUCT_KEY) == 0) {
                        if (!any_is_string(&iter->val0) || strcmp(iter->val0.u.sval, tsl->productId) != 0) {
                            satisfy = false;
                        }
                    }
                }
            }
            if (satisfy) {
                if (format_string_from_context(topic, sizeof(topic), config->req_topic, context) < 0) {
                    log_error("invalid topic: %s", config->res_topic);
                } else {
                    mqtt_subscribe_ex(instance.mqtt, topic, handle_service, (void *)(intptr_t)i, logout_topics, strdup(topic));
                }
            }
        }
    }
    map_any_destroy_ex(context);
    mtx_lock(&instance.mtxDevices);
    map_device_insert(instance.devices, mac, tsl, NULL);
    mtx_unlock(&instance.mtxDevices);
}

#define UNSUBSCRIBE_DOWNSTREAM_TOPIC(name)                                                                                                                     \
    if (g_config.name.enable && g_config.name.req_topic) {                                                                                                     \
        if (format_string_from_context(topic, sizeof(topic), g_config.name.req_topic, context) < 0) {                                                          \
            log_error("invalid topic: %s", g_config.name.req_topic);                                                                                           \
        } else {                                                                                                                                               \
            mqtt_unsubscribe(instance.mqtt, topic);                                                                                                            \
        }                                                                                                                                                      \
    }
#define UNSUBSCRIBE_UPSTREAM_TOPIC(name)                                                                                                                       \
    if (g_config.name.enable && g_config.name.res_topic) {                                                                                                     \
        if (format_string_from_context(topic, sizeof(topic), g_config.name.res_topic, context) < 0) {                                                          \
            log_error("invalid topic: %s", g_config.name.res_topic);                                                                                           \
        } else {                                                                                                                                               \
            mqtt_unsubscribe(instance.mqtt, topic);                                                                                                            \
        }                                                                                                                                                      \
    }

void remove_device(const char *mac) {
    mtx_lock(&instance.mtxDevices);
    map_device_iterator *iter = map_device_find(instance.devices, mac);
    mtx_unlock(&instance.mtxDevices);
    if (iter == NULL) {
        log_error("no such device %s", mac);
    } else {
        RexCommand_t cmd;
        memset(&cmd, 0, sizeof(RexCommand_t));
        cmd.type = CMD_REMOVE_NODE;
        strcpy(cmd.u.removeNodeCommand.mac, mac);
        int ret = instance.processCmd(instance.context, &cmd);
        if (ret != 0) {
            log_error("processCmd failed %d", ret);
        }
        const RexTslModelInfo_t *tsl = iter->val0;
        handle_sub_logout(tsl->productId, mac);
        map_any *context = map_any_create();
        map_any_insert(context, NAME_DEV_PRODUCT_KEY, any_from_const_string(tsl->productId));
        map_any_insert(context, NAME_DEV_DEVICE_NAME, any_from_const_string(mac));
        char topic[256];
        UNSUBSCRIBE_UPSTREAM_TOPIC(dev_login)
        UNSUBSCRIBE_UPSTREAM_TOPIC(dev_logout)
        UNSUBSCRIBE_UPSTREAM_TOPIC(dev_property_post)
        UNSUBSCRIBE_DOWNSTREAM_TOPIC(dev_property_set)
        UNSUBSCRIBE_DOWNSTREAM_TOPIC(dev_set_alias)
        for (int j = 0; j < tsl->eventInfoNum; j++) {
            map_any_iterator *it = map_any_find(context, NAME_DEV_EVENT_NAME);
            if (it == NULL) {
                map_any_insert(context, NAME_DEV_EVENT_NAME, any_from_const_string(tsl->eventInfoList[j].identifier));
            } else {
                any_set_const_string(&it->val0, tsl->eventInfoList[j].identifier);
            }
            for (int i = 0; i < g_config.dev_event_post_cnt; i++) {
                bool           satisfy = true;
                config_func_t *config  = &g_config.dev_event_post[i];
                if (!config->enable || !config->res_topic) {
                    satisfy = false;
                }
                if (satisfy && config->conditions) {
                    for (map_any_iterator *iter = map_any_first(config->conditions); iter != NULL && satisfy; iter = map_any_next(iter)) {
                        if (strcmp(iter->key, NAME_DEV_EVENT_NAME) == 0) {
                            if (!any_is_string(&iter->val0) || strcmp(iter->val0.u.sval, tsl->eventInfoList[j].identifier) != 0) {
                                satisfy = false;
                            }
                        } else if (strcmp(iter->key, NAME_DEV_PRODUCT_KEY) == 0) {
                            if (!any_is_string(&iter->val0) || strcmp(iter->val0.u.sval, tsl->productId) != 0) {
                                satisfy = false;
                            }
                        }
                    }
                }
                if (satisfy) {
                    if (format_string_from_context(topic, sizeof(topic), config->res_topic, context) < 0) {
                        log_error("invalid topic: %s", config->res_topic);
                    } else {
                        mqtt_unsubscribe(instance.mqtt, topic);
                    }
                }
            }
        }
        map_any_iterator *it = map_any_find(context, NAME_DEV_EVENT_NAME);
        if (it != NULL) {
            any_free(&it->val0);
            map_any_erase(it);
        }
        for (int j = 0; j < tsl->serviceInfoNum; j++) {
            map_any_iterator *it = map_any_find(context, NAME_DEV_SERVICE_NAME);
            if (it == NULL) {
                map_any_insert(context, NAME_DEV_SERVICE_NAME, any_from_const_string(tsl->serviceInfoList[j].identifier));
            } else {
                any_set_const_string(&it->val0, tsl->serviceInfoList[j].identifier);
            }
            for (int i = 0; i < g_config.dev_service_call_cnt; i++) {
                bool           satisfy = true;
                config_func_t *config  = &g_config.dev_service_call[i];
                if (!config->enable || !config->req_topic) {
                    satisfy = false;
                }
                if (satisfy && config->conditions) {
                    for (map_any_iterator *iter = map_any_first(config->conditions); iter != NULL && satisfy; iter = map_any_next(iter)) {
                        if (strcmp(iter->key, NAME_DEV_SERVICE_NAME) == 0) {
                            if (!any_is_equal_str(&iter->val0, tsl->serviceInfoList[j].identifier)) {
                                satisfy = false;
                            }
                        } else if (strcmp(iter->key, NAME_DEV_PRODUCT_KEY) == 0) {
                            if (!any_is_equal_str(&iter->val0, tsl->productId)) {
                                satisfy = false;
                            }
                        }
                    }
                }
                if (satisfy) {
                    if (format_string_from_context(topic, sizeof(topic), config->req_topic, context) < 0) {
                        log_error("invalid topic: %s", config->res_topic);
                    } else {
                        mqtt_unsubscribe(instance.mqtt, topic);
                    }
                }
            }
        }
        map_any_destroy_ex(context);
        mtx_lock(&instance.mtxDevices);
        if (iter->val1 != NULL) {
            free((char *)iter->val1);
        }
        map_device_erase(iter);
        mtx_unlock(&instance.mtxDevices);
    }
}

struct message_view_t {
    int  len;
    char msg[1024];
};

static size_t write_data(void *data, size_t size, size_t nmemb, void *userp) {
    struct message_view_t *msg = (struct message_view_t *)userp;
    memcpy(&msg->msg[msg->len], data, size * nmemb);
    msg->len += size * nmemb;
    return size * nmemb;
}

int register_gateway() {
    CURL *curl = curl_easy_init();
    if (!curl) {
        return -1;
    }
    struct curl_slist *headers = curl_slist_append(NULL, "Host: develop.rexense.com");
    headers                    = curl_slist_append(headers, "Content-Type: application/json");
    headers                    = curl_slist_append(headers, "Connection: Keep-Alive");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // 从配置中读取金山云的url地址
    struct dp_conf_node *val = conf_get_node(g_config_root, KC_CONFIG_URL_KEY);
    curl_easy_setopt(curl, CURLOPT_URL, val->value);

    // 根据协议文档，构造cipherText JSON串，格式如下:
    // {
    //      "clientId":"",
    //      "vendorCode":""
    //  }
    cipher_text cipher_text;
    memset(&cipher_text, 0, sizeof(cipher_text));
    cipher_text.clientId = instance.clientId;
    cipher_text.vendorCode = instance.vendorCode;
    char* cipher_text_json;
    int ret = csonStruct2JsonStr(cipher_text_json, &cipher_text, cipher_text_ref);
    if (ERR_SUCCESS != ret) {
        log_error("cson struct[cipher_text] to json error");
        return 1;
    }

    // TODO: 按照协议规定，需要对cipher_text_json进行aes加密

    // 开始构造请求json串，按照预定，格式如下：
    // {
    //      "cipherText": "",
    //      "productKey": ""
    // }
    device_reg_request reg_request;
    memset(&reg_request, 0, sizeof(device_reg_request));
    reg_request.productKey = instance.productKey;
    reg_request.cipherText = cipher_text_json;
    char* reg_request_json;
    ret = csonStruct2JsonStr(reg_request_json, &reg_request, device_reg_request_ref);
    if (ERR_SUCCESS != ret) {
        log_error("cson struct[device_reg_request] to json error");
        free(cipher_text_json);
        return 1;
    }

    // 尝试请求金山云
    log_info("register gateway: %s", reg_request_json);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, reg_request_json);
    struct message_view_t ret_msg;
    memset(&ret_msg, 0, sizeof(ret_msg));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret_msg);
    int ret = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    free(cipher_text_json);
    free(reg_request_json);

    // 如果请求成功了，可以解密mqtt一些参数了
    if (ret == 0) {
        ret_msg.msg[ret_msg.len] = '\0';
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code == 200) {
            device_reg_response reg_response;
            memset(&reg_response, 0, sizeof(device_reg_response));
            ret = csonJsonStr2Struct(ret_msg.msg, &reg_response, device_reg_response_ref);
            if (ret == ERR_SUCCESS) {
                if (reg_response.code == 200) {
                    log_info("register gateway: %s", ret_msg.msg);

                    // TODO: 针对reg_response中的data字段做AES解密

                    // 将相关变量赋值给instance
                    reg_response_data data;
                    memset(&data, 0, sizeof(reg_response_data));
                    ret = csonJsonStr2Struct(reg_response.data, &data, reg_response_data_ref);
                    if (ret != ERR_SUCCESS) {
                        log_warn("gateway data is illegal: %s", reg_response.data);
                        ret = 1;
                    } else {
                        strcpy(instance.deviceName, data.deviceKey);
                        strcpy(instance.deviceSecret, data.deviceSecret);
                    }
                    csonFreePointer(&data, reg_response_data_ref);
                } else {
                    log_warn("register failed: %s", ret_msg.msg);
                    ret = 1;
                }
                csonFreePointer(&reg_response, device_reg_request_ref);
            }
        } else {
            log_warn("register failed: %d %s", response_code, ret_msg.msg);
            ret = 1;
        }
    }
    return ret;
}

const char *get_gateway_product_key() {
    return instance.productKey;
}

const char *get_gateway_device_name() {
    return instance.deviceName;
}

int get_sequence() {
    return atomic_fetch_add(&instance.sequence, 1);
}

#define FORMAT(x) _Generic((x), struct __pthread * : "mqtt(%s) %p ", long unsigned int : "mqtt(%s) %ld ", default : "mqtt(%s) %08x ")

int log_error(const char *fmt, ...) {
    va_list args1;
    va_start(args1, fmt);
    va_list args2;
    va_copy(args2, args1);
    char buf[48 + vsnprintf(NULL, 0, fmt, args1)];
    va_end(args1);
    thrd_t id = thrd_current();
    int    n  = sprintf(buf, FORMAT(id), instance.deviceName, id);
    vsnprintf(buf + n, sizeof buf, fmt, args2);
    va_end(args2);
    return instance.log(instance.context, LOG_LEVEL_ERROR, buf);
}

int log_warn(const char *fmt, ...) {
    va_list args1;
    va_start(args1, fmt);
    va_list args2;
    va_copy(args2, args1);
    char buf[48 + vsnprintf(NULL, 0, fmt, args1)];
    va_end(args1);
    thrd_t id = thrd_current();
    int    n  = sprintf(buf, FORMAT(id), instance.deviceName, id);
    vsnprintf(buf + n, sizeof buf, fmt, args2);
    va_end(args2);
    return instance.log(instance.context, LOG_LEVEL_WARN, buf);
}

int log_info(const char *fmt, ...) {
    va_list args1;
    va_start(args1, fmt);
    va_list args2;
    va_copy(args2, args1);
    char buf[48 + vsnprintf(NULL, 0, fmt, args1)];
    va_end(args1);
    thrd_t id = thrd_current();
    int    n  = sprintf(buf, FORMAT(id), instance.deviceName, id);
    vsnprintf(buf + n, sizeof buf, fmt, args2);
    va_end(args2);
    return instance.log(instance.context, LOG_LEVEL_INFO, buf);
}

int log_debug(const char *fmt, ...) {
    va_list args1;
    va_start(args1, fmt);
    va_list args2;
    va_copy(args2, args1);
    char buf[48 + vsnprintf(NULL, 0, fmt, args1)];
    va_end(args1);
    thrd_t id = thrd_current();
    int    n  = sprintf(buf, FORMAT(id), instance.deviceName, id);
    vsnprintf(buf + n, sizeof buf, fmt, args2);
    va_end(args2);
    return instance.log(instance.context, LOG_LEVEL_DEBUG, buf);
}

int log_verbose(const char *fmt, ...) {
    va_list args1;
    va_start(args1, fmt);
    va_list args2;
    va_copy(args2, args1);
    char buf[48 + vsnprintf(NULL, 0, fmt, args1)];
    va_end(args1);
    thrd_t id = thrd_current();
    int    n  = sprintf(buf, FORMAT(id), instance.deviceName, id);
    vsnprintf(buf + n, sizeof buf, fmt, args2);
    va_end(args2);
    return instance.log(instance.context, LOG_LEVEL_VERBOS, buf);
}
