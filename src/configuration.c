#include "configuration.h"
#include "cJSON.h"
#include "error.h"
#include "instance.h"
#include "yaml.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

any_t any_from_yaml_node(yaml_document_t *doc, yaml_node_t *node);

config_t g_config;

#define NODE_BASIC                 "basic"
#define NODE_BASIC_URL             "url"
#define NODE_BASIC_USERNAME        "username"
#define NODE_BASIC_PASSWORD        "password"
#define NODE_BASIC_BROKER          "broker"
#define NODE_BASIC_PRODUCT_KEY     "productKey"
#define NODE_BASIC_PRODUCT_SECRET  "productSecret"
#define NODE_BASIC_VENDOR_CODE     "vendorCode"
#define NODE_BASIC_MODEL_CODE      "modelCode"
#define NODE_FUNCTIONALITY         "functionality"
#define NODE_GATEWAY               "gateway"
#define NODE_DEVICE                "device"
#define NODE_GW_REGISTER           "gw_register"
#define NODE_GW_MESSAGE            "gw_message_up"
#define NODE_GW_COMMAND            "gw_command"
#define NODE_GW_OTA                "gw_ota"
#define NODE_DEV_REGISTER          "dev_register"
#define NODE_GW_LOGIN              "gw_login"
#define NODE_GW_LOGOUT             "gw_logout"
#define NODE_TOPO_JOIN             "topo_join"
#define NODE_TOPO_LEAVE            "topo_leave"
#define NODE_TOPO_GET              "topo_get"
#define NODE_TOPO_ADD              "topo_add"
#define NODE_TOPO_DELETE           "topo_delete"
#define NODE_GW_PROPERTY_POST      "gw_property_post"
#define NODE_GW_PROPERTY_SET       "gw_property_set"
#define NODE_GW_EVENT_POST         "gw_event_post"
#define NODE_GW_SERVICE_CALL       "gw_service_call"
#define NODE_CREATE_ZIGBEE_NETWORK "create_zigbee_network"
#define NODE_LEAVE_ZIGBEE_NETWORK  "leave_zigbee_network"
#define NODE_START_PERMIT_JOIN     "start_permit_join"
#define NODE_CREATE_SCENE          "create_scene"
#define NODE_DELETE_SCENE          "delete_scene"
#define NODE_TRIGGER_SCENE         "trigger_scene"
#define NODE_ENABLE_SCENE          "enable_scene"
#define NODE_DISABLE_SCENE         "disable_scene"
#define NODE_REPORT_VERSION        "report_version"
#define NODE_OTA_REQUEST           "ota_request"
#define NODE_OTA_PROGRESS          "ota_progress"
#define NODE_DEV_LOGIN             "dev_login"
#define NODE_DEV_LOGOUT            "dev_logout"
#define NODE_DEV_PROPERTY_POST     "dev_property_post"
#define NODE_DEV_PROPERTY_SET      "dev_property_set"
#define NODE_DEV_EVENT_POST        "dev_event_post"
#define NODE_DEV_SERVICE_CALL      "dev_service_call"
#define NODE_DEV_SET_ALIAS         "dev_set_alias"
#define NODE_DEV_OTA_REQUEST       "dev_ota_request"
#define NODE_DEV_OTA_PROGRESS      "dev_ota_progress"
#define NODE_ITEM_ENABLE           "enable"
#define NODE_ITEM_REQUEST          "request"
#define NODE_ITEM_RESPONSE         "response"
#define NODE_ITEM_SUCCESS          "success"
#define NODE_ITEM_PERSISTENT       "persistent"
#define NODE_ITEM_TOPIC            "topic"
#define NODE_ITEM_PAYLOAD          "payload"
#define NODE_ITEM_CONDITIONS       "conditions"

static int load_basic_config(any_t val, config_basic_t *config) {
    if (!any_is_map(&val)) {
        return ERR_CONFIG_FILE_ERROR;
    }
    map_any_iterator *iter = map_any_find(val.u.mval, NODE_BASIC_URL);
    if (iter != NULL && any_is_string(&iter->val0)) {
        config->url = strdup(iter->val0.u.sval);
    }
    iter = map_any_find(val.u.mval, NODE_BASIC_USERNAME);
    if (iter != NULL && any_is_string(&iter->val0)) {
        config->username = strdup(iter->val0.u.sval);
    }
    iter = map_any_find(val.u.mval, NODE_BASIC_PASSWORD);
    if (iter != NULL && any_is_string(&iter->val0)) {
        config->password = strdup(iter->val0.u.sval);
    }
    iter = map_any_find(val.u.mval, NODE_BASIC_BROKER);
    if (iter != NULL && any_is_string(&iter->val0)) {
        config->broker = strdup(iter->val0.u.sval);
    }
    iter = map_any_find(val.u.mval, NODE_BASIC_PRODUCT_KEY);
    if (iter != NULL && any_is_string(&iter->val0)) {
        config->productKey = strdup(iter->val0.u.sval);
    }
    iter = map_any_find(val.u.mval, NODE_BASIC_PRODUCT_SECRET);
    if (iter != NULL && any_is_string(&iter->val0)) {
        config->productSecret = strdup(iter->val0.u.sval);
    }
    iter = map_any_find(val.u.mval, NODE_BASIC_VENDOR_CODE);
    if (iter != NULL && any_is_string(&iter->val0)) {
        config->vendorCode = strdup(iter->val0.u.sval);
    }
    iter = map_any_find(val.u.mval, NODE_BASIC_MODEL_CODE);
    if (iter != NULL && any_is_string(&iter->val0)) {
        config->modelCode = strdup(iter->val0.u.sval);
    }
    return ERR_SUCCESS;
}

static int load_func_config(any_t val, config_func_t *config) {
    if (!any_is_map(&val)) {
        return ERR_CONFIG_FILE_ERROR;
    }
    config->enable         = true;
    map_any_iterator *iter = map_any_find(val.u.mval, NODE_ITEM_ENABLE);
    if (iter != NULL && any_is_bool(&iter->val0)) {
        config->enable = any_is_true(&iter->val0);
    }
    iter = map_any_find(val.u.mval, NODE_ITEM_REQUEST);
    if (iter != NULL && any_is_map(&iter->val0)) {
        map_any_iterator *it = map_any_find(iter->val0.u.mval, NODE_ITEM_TOPIC);
        if (it != NULL && any_is_string(&it->val0)) {
            config->req_topic = strdup(it->val0.u.sval);
        }
        it = map_any_find(iter->val0.u.mval, NODE_ITEM_PAYLOAD);
        if (it != NULL) {
            config->req_payload = any_duplicate(&it->val0);
        }
    }
    iter = map_any_find(val.u.mval, NODE_ITEM_RESPONSE);
    if (iter != NULL && any_is_map(&iter->val0)) {
        map_any_iterator *it = map_any_find(iter->val0.u.mval, NODE_ITEM_TOPIC);
        if (it != NULL && any_is_string(&it->val0)) {
            config->res_topic = strdup(it->val0.u.sval);
        }
        it = map_any_find(iter->val0.u.mval, NODE_ITEM_PAYLOAD);
        if (it != NULL) {
            config->res_payload = any_duplicate(&it->val0);
        }
    }
    iter = map_any_find(val.u.mval, NODE_ITEM_CONDITIONS);
    if (iter != NULL && any_is_map(&iter->val0)) {
        config->conditions = map_any_create();
        for (map_any_iterator *i = map_any_first(iter->val0.u.mval); i != NULL; i = map_any_next(i)) {
            map_any_insert_ex(config->conditions, i->key, any_duplicate(&i->val0));
        }
    }
    return ERR_SUCCESS;
}

struct mapping_s {
    const char    *key;
    config_func_t *conf;
};

int load_config(const char *filename, const char *update) {
    memset(&g_config, 0, sizeof(config_t));
    any_t data = any_from_yaml_file(filename);
    if (update) {
        any_t updateData = any_from_yaml_file(update);
        if (any_is_map(&updateData) && any_is_map(&data)) {
            for (map_any_iterator *iter = map_any_first(updateData.u.mval); iter != NULL; iter = map_any_next(iter)) {
                if (strcmp(iter->key, "basic") == 0) {
                    map_any_insert_ex(data.u.mval, iter->key, any_duplicate(&iter->val0));
                } else if (strcmp(iter->key, "functionality") == 0) {
                    map_any_insert_ex(data.u.mval, iter->key, any_duplicate(&iter->val0));
                }
            }
            any_to_yaml_file(&data, filename);
        }
        any_free(&updateData);
    }
    if (!any_is_map(&data)) {
        return ERR_CONFIG_FILE_ERROR;
    }
    map_any_iterator *iter = map_any_find(data.u.mval, NODE_BASIC);
    if (iter != NULL) {
        load_basic_config(iter->val0, &g_config.basic);
    }
    iter = map_any_find(data.u.mval, NODE_FUNCTIONALITY);
    if (iter != NULL && any_is_map(&iter->val0)) {
        map_any_iterator *it = map_any_find(iter->val0.u.mval, NODE_GATEWAY);
        if (it != NULL && any_is_map(&it->val0)) {
            struct mapping_s mappings[] = {
                {          NODE_GW_REGISTER,           &g_config.gw_register},
                {          NODE_GW_COMMAND,             &g_config.gw_command},
                {          NODE_GW_MESSAGE,          &g_config.gw_message_up},
                {              NODE_GW_OTA,                 &g_config.gw_ota},
                {         NODE_DEV_REGISTER,          &g_config.dev_register},
                {             NODE_GW_LOGIN,              &g_config.gw_login},
                {            NODE_GW_LOGOUT,             &g_config.gw_logout},
                {            NODE_TOPO_JOIN,             &g_config.topo_join},
                {           NODE_TOPO_LEAVE,            &g_config.topo_leave},
                {             NODE_TOPO_GET,              &g_config.topo_get},
                {             NODE_TOPO_ADD,              &g_config.topo_add},
                {          NODE_TOPO_DELETE,           &g_config.topo_delete},
                {     NODE_GW_PROPERTY_POST,      &g_config.gw_property_post},
                {      NODE_GW_PROPERTY_SET,       &g_config.gw_property_set},
                {        NODE_GW_EVENT_POST,         &g_config.gw_event_post},
                {      NODE_GW_SERVICE_CALL,       &g_config.gw_service_call},
                {NODE_CREATE_ZIGBEE_NETWORK, &g_config.create_zigbee_network},
                { NODE_LEAVE_ZIGBEE_NETWORK,  &g_config.leave_zigbee_network},
                {    NODE_START_PERMIT_JOIN,     &g_config.start_permit_join},
                {         NODE_CREATE_SCENE,          &g_config.create_scene},
                {         NODE_DELETE_SCENE,          &g_config.delete_scene},
                {        NODE_TRIGGER_SCENE,         &g_config.trigger_scene},
                {         NODE_ENABLE_SCENE,          &g_config.enable_scene},
                {        NODE_DISABLE_SCENE,         &g_config.disable_scene},
                {       NODE_REPORT_VERSION,        &g_config.report_version},
                {          NODE_OTA_REQUEST,           &g_config.ota_request},
                {         NODE_OTA_PROGRESS,          &g_config.ota_progress}
            };
            for (int i = 0; i < sizeof(mappings) / sizeof(struct mapping_s); i++) {
                map_any_iterator *ii = map_any_find(it->val0.u.mval, mappings[i].key);
                if (ii != NULL) {
                    load_func_config(ii->val0, mappings[i].conf);
                }
            }
        }
        it = map_any_find(iter->val0.u.mval, NODE_DEVICE);
        if (it != NULL && any_is_map(&it->val0)) {
            struct mapping_s mappings[] = {
                {        NODE_DEV_LOGIN,         &g_config.dev_login},
                {       NODE_DEV_LOGOUT,        &g_config.dev_logout},
                {NODE_DEV_PROPERTY_POST, &g_config.dev_property_post},
                { NODE_DEV_PROPERTY_SET,  &g_config.dev_property_set},
                {    NODE_DEV_SET_ALIAS,     &g_config.dev_set_alias},
                {  NODE_DEV_OTA_REQUEST,   &g_config.dev_ota_request},
                { NODE_DEV_OTA_PROGRESS,  &g_config.dev_ota_progress}
            };
            for (int i = 0; i < sizeof(mappings) / sizeof(struct mapping_s); i++) {
                map_any_iterator *ii = map_any_find(it->val0.u.mval, mappings[i].key);
                if (ii != NULL) {
                    load_func_config(ii->val0, mappings[i].conf);
                }
            }
            map_any_iterator *ii = map_any_find(it->val0.u.mval, NODE_DEV_EVENT_POST);
            if (ii != NULL) {
                if (any_is_map(&ii->val0)) {
                    g_config.dev_event_post = malloc(sizeof(config_func_t));
                    if (!g_config.dev_event_post) {
                        return ERR_OUT_OF_MEMORY;
                    }
                    g_config.dev_event_post_cnt = 1;
                    memset(g_config.dev_event_post, 0, sizeof(config_func_t));
                    load_func_config(ii->val0, &g_config.dev_event_post[0]);
                } else if (any_is_sequence(&ii->val0)) {
                    int size                = any_length(&ii->val0);
                    g_config.dev_event_post = malloc(size * sizeof(config_func_t));
                    if (!g_config.dev_event_post) {
                        return ERR_OUT_OF_MEMORY;
                    }
                    g_config.dev_event_post_cnt = size;
                    memset(g_config.dev_event_post, 0, size * sizeof(config_func_t));
                    for (int i = 0; i < size; i++) {
                        load_func_config(ii->val0.u.aval[i], &g_config.dev_event_post[i]);
                    }
                }
            }
            ii = map_any_find(it->val0.u.mval, NODE_DEV_SERVICE_CALL);
            if (ii != NULL) {
                if (any_is_map(&ii->val0)) {
                    g_config.dev_service_call = malloc(sizeof(config_func_t));
                    if (!g_config.dev_service_call) {
                        return ERR_OUT_OF_MEMORY;
                    }
                    g_config.dev_service_call_cnt = 1;
                    memset(g_config.dev_service_call, 0, sizeof(config_func_t));
                    load_func_config(ii->val0, &g_config.dev_service_call[0]);
                } else if (any_is_sequence(&ii->val0)) {
                    int size                  = any_length(&ii->val0);
                    g_config.dev_service_call = malloc(size * sizeof(config_func_t));
                    if (!g_config.dev_service_call) {
                        return ERR_OUT_OF_MEMORY;
                    }
                    g_config.dev_service_call_cnt = size;
                    memset(g_config.dev_service_call, 0, size * sizeof(config_func_t));
                    for (int i = 0; i < size; i++) {
                        load_func_config(ii->val0.u.aval[i], &g_config.dev_service_call[i]);
                    }
                }
            }
        }
    }
    any_free(&data);
    return ERR_SUCCESS;
}

static void free_basic_config(config_basic_t *config) {
    if (config->url) {
        free(config->url);
    }
    if (config->username) {
        free(config->username);
    }
    if (config->password) {
        free(config->password);
    }
    if (config->broker) {
        free(config->broker);
    }
    if (config->productKey) {
        free(config->productKey);
    }
    if (config->productSecret) {
        free(config->productSecret);
    }
    if (config->vendorCode) {
        free(config->vendorCode);
    }
    if (config->modelCode) {
        free(config->modelCode);
    }
}

static void free_func_config(config_func_t *config) {
    if (config->req_topic) {
        free(config->req_topic);
    }
    any_free(&config->req_payload);
    if (config->res_topic) {
        free(config->res_topic);
    }
    any_free(&config->res_payload);
    if (config->conditions) {
        map_any_destroy_ex(config->conditions);
    }
}

void free_config() {
    free_basic_config(&g_config.basic);
    for (config_func_t *config = &g_config.gw_register; config <= &g_config.dev_set_alias; config++) {
        free_func_config(config);
    }
    for (int i = 0; i < g_config.dev_event_post_cnt; i++) {
        free_func_config(&g_config.dev_event_post[i]);
    }
    free(g_config.dev_event_post);
    for (int i = 0; i < g_config.dev_service_call_cnt; i++) {
        free_func_config(&g_config.dev_service_call[i]);
    }
    free(g_config.dev_service_call);
    memset(&g_config, 0, sizeof(config_t));
}

any_t generate_value(any_t pattern, const map_any *context) {
    if (any_is_plain(&pattern)) {
        return any_duplicate(&pattern);
    }
    if (any_is_string(&pattern)) {
        any_t result = {.type = kUnknown};
        int   len    = any_length(&pattern);
        if (len <= 0) {
            any_set_string(&result, NULL);
        } else if (pattern.u.sval[0] == '$' && pattern.u.sval[1] == '{' && pattern.u.sval[len - 1] == '}') {
            if (len - 2 == sizeof(NAME_GW_PRODUCT_KEY) && strncmp(pattern.u.sval + 2, NAME_GW_PRODUCT_KEY, sizeof(NAME_GW_PRODUCT_KEY) - 1) == 0) {
                any_set_const_string(&result, get_gateway_product_key());
            } else if (len - 2 == sizeof(NAME_GW_DEVICE_NAME) && strncmp(pattern.u.sval + 2, NAME_GW_DEVICE_NAME, sizeof(NAME_GW_DEVICE_NAME) - 1) == 0) {
                any_set_const_string(&result, get_gateway_device_name());
            } else if (len - 2 == sizeof(NAME_GW_VENDOR_CODE) && strncmp(pattern.u.sval + 2, NAME_GW_VENDOR_CODE, sizeof(NAME_GW_VENDOR_CODE) - 1) == 0) {
                any_set_const_string(&result, get_gateway_vendor_code());
            } else if (len - 2 == sizeof(NAME_GW_MODEL_ID) && strncmp(pattern.u.sval + 2, NAME_GW_MODEL_ID, sizeof(NAME_GW_MODEL_ID) - 1) == 0) {
                any_set_const_string(&result, get_gateway_hex_model_id());
            } else if (len - 2 == sizeof(NAME_FUNC_TIMESTAMP) && strncmp(pattern.u.sval + 2, NAME_FUNC_TIMESTAMP, sizeof(NAME_FUNC_TIMESTAMP) - 1) == 0) {
                any_set_integer(&result, time(NULL));
            } else if (len - 2 == sizeof(NAME_FUNC_IOTA) && strncmp(pattern.u.sval + 2, NAME_FUNC_IOTA, sizeof(NAME_FUNC_IOTA) - 1) == 0) {
                any_set_integer(&result, get_sequence());
            } else if (context) {
                char *key = malloc(len - 2);
                if (key) {
                    strncpy(key, pattern.u.sval + 2, len - 3);
                    key[len - 3]           = '\0';
                    map_any_iterator *iter = map_any_find(context, key);
                    if (iter != NULL) {
                        result = any_duplicate(&iter->val0);
                    } else {
                        printf("[ERR] %d unable to find such key: %s\n", __LINE__, key);
                    }
                    free(key);
                }
            }
        } else {
            int new_len = len;
            for (int i = 0; i < len - 1; i++) {
                char *key_start = NULL;
                int   key_len   = 0;
                if (pattern.u.sval[i] == '$' && pattern.u.sval[i + 1] == '{') {
                    for (int j = i + 2; j < len; j++) {
                        if (pattern.u.sval[j] == '}') {
                            key_start = pattern.u.sval + i + 2;
                            key_len   = j - i - 2;
                            i         = j;
                            break;
                        }
                    }
                    if (key_len > 0) {
                        new_len -= key_len + 3;
                        if (key_len + 1 == sizeof(NAME_GW_PRODUCT_KEY) && strncmp(key_start, NAME_GW_PRODUCT_KEY, sizeof(NAME_GW_PRODUCT_KEY) - 1) == 0) {
                            new_len += strlen(get_gateway_product_key());
                        } else if (key_len + 1 == sizeof(NAME_GW_DEVICE_NAME) &&
                                   strncmp(key_start, NAME_GW_DEVICE_NAME, sizeof(NAME_GW_DEVICE_NAME) - 1) == 0) {
                            new_len += strlen(get_gateway_device_name());
                        }  else if (key_len + 1 == sizeof(NAME_GW_VENDOR_CODE) &&
                                   strncmp(key_start, NAME_GW_VENDOR_CODE, sizeof(NAME_GW_VENDOR_CODE) - 1) == 0) {
                            new_len += strlen(get_gateway_vendor_code());
                        }  else if (key_len + 1 == sizeof(NAME_GW_MODEL_ID) &&
                                   strncmp(key_start, NAME_GW_MODEL_ID, sizeof(NAME_GW_MODEL_ID) - 1) == 0) {
                            new_len += strlen(get_gateway_hex_model_id());
                        }  else if (context) {
                            char *key = malloc(key_len + 1);
                            if (key) {
                                strncpy(key, key_start, key_len);
                                key[key_len]           = '\0';
                                map_any_iterator *iter = map_any_find(context, key);
                                if (iter != NULL && any_is_string(&iter->val0)) {
                                    new_len += strlen(iter->val0.u.sval);
                                } else {
                                    printf("[ERR] %d unable to find such key(%d): %s\n", __LINE__, key_len, key);
                                }
                                free(key);
                            }
                        }
                    }
                }
            }
            result.u.sval = malloc(new_len + 1);
            char *dest    = result.u.sval;
            for (int i = 0; i < len; i++) {
                char *key_start = NULL;
                int   key_len   = 0;
                if (pattern.u.sval[i] != '$') {
                    *dest++ = pattern.u.sval[i];
                } else {
                    i++;
                    if (i >= len) {
                        break;
                    }
                    if (pattern.u.sval[i] != '{') {
                        *dest++ = pattern.u.sval[i];
                    } else {
                        i++;
                        for (int j = i; j < len; j++) {
                            if (pattern.u.sval[j] == '}') {
                                key_start = pattern.u.sval + i;
                                key_len   = j - i;
                                i         = j;
                                break;
                            }
                        }
                        if (key_len > 0) {
                            if (key_len + 1 == sizeof(NAME_GW_PRODUCT_KEY) && strncmp(key_start, NAME_GW_PRODUCT_KEY, sizeof(NAME_GW_PRODUCT_KEY) - 1) == 0) {
                                strcpy(dest, get_gateway_product_key());
                                dest += strlen(get_gateway_product_key());
                            } else if (key_len + 1 == sizeof(NAME_GW_DEVICE_NAME) &&
                                       strncmp(key_start, NAME_GW_DEVICE_NAME, sizeof(NAME_GW_DEVICE_NAME) - 1) == 0) {
                                strcpy(dest, get_gateway_device_name());
                                dest += strlen(get_gateway_device_name());
                            }  else if (key_len + 1 == sizeof(NAME_GW_VENDOR_CODE) &&
                                       strncmp(key_start, NAME_GW_VENDOR_CODE, sizeof(NAME_GW_VENDOR_CODE) - 1) == 0) {
                                strcpy(dest, get_gateway_vendor_code());
                                dest += strlen(get_gateway_vendor_code());
                            }  else if (key_len + 1 == sizeof(NAME_GW_MODEL_ID) &&
                                       strncmp(key_start, NAME_GW_MODEL_ID, sizeof(NAME_GW_MODEL_ID) - 1) == 0) {
                                strcpy(dest, get_gateway_hex_model_id());
                                dest += strlen(get_gateway_hex_model_id());
                            }  else if (context) {
                                char *key = malloc(key_len + 1);
                                if (key) {
                                    strncpy(key, key_start, key_len);
                                    key[key_len]           = '\0';
                                    map_any_iterator *iter = map_any_find(context, key);
                                    if (iter != NULL && any_is_string(&iter->val0)) {
                                        strcpy(dest, iter->val0.u.sval);
                                        dest += strlen(iter->val0.u.sval);
                                    }
                                    free(key);
                                }
                            }
                        }
                    }
                }
            }
            *dest       = '\0';
            result.type = kString + dest - result.u.sval;
        }
        return result;
    } else if (any_is_sequence(&pattern)) {
        any_t result = {.type = kSequence, .u = {.aval = NULL}};
        int   len    = any_length(&pattern);
        if (len <= 0) {
            return result;
        }
        result.u.aval = malloc(len * sizeof(any_t));
        if (!result.u.aval) {
            return result;
        }
        for (int i = 0; i < len; i++) {
            result.u.aval[i] = generate_value(pattern.u.aval[i], context);
        }
        result.type += len;
        return result;
    } else if (any_is_map(&pattern)) {
        any_t result = {.type = kMap, .u = {.mval = map_any_create()}};
        for (map_any_iterator *iter = map_any_first(pattern.u.mval); iter != NULL; iter = map_any_next(iter)) {
            any_t key = {.type = kUnknown};
            any_set_const_string(&key, iter->key);
            any_t processed_key = generate_value(key, context);
            if (any_is_string(&processed_key)) {
                map_any_insert_ex(result.u.mval, processed_key.u.sval, generate_value(iter->val0, context));
            }
            any_free(&processed_key);
        }
        return result;
    } else {
        any_t result = {.type = kUnknown};
        return result;
    }
}

int format_from_context(char *buffer, size_t len, any_t pattern, const map_any *context) {
    any_t result = generate_value(pattern, context);
    int   count  = any_to_str_buffer(buffer, &result);
    any_free(&result);
    return count;
}

int format_string_from_context(char *buffer, size_t len, const char *pattern, const map_any *context) {
    any_t p = {.type = kUnknown};
    any_set_const_string(&p, pattern);
    any_t result = generate_value(p, context);
    int   ret    = 0;
    if (any_is_null(&result)) {
        return snprintf(buffer, len, "null");
    } else if (any_is_true(&result)) {
        return snprintf(buffer, len, "true");
    } else if (any_is_single(&result)) {
        return snprintf(buffer, len, "%f", result.u.fval);
    } else if (any_is_double(&result)) {
        return snprintf(buffer, len, "%lf", result.u.dval);
    } else if (any_is_integer(&result)) {
        return snprintf(buffer, len, "%lld", result.u.ival);
    } else if (any_is_string(&result)) {
        if (any_length(&result) > 0 && result.u.sval != NULL) {
            ret = snprintf(buffer, len, "%.*s", any_length(&result), result.u.sval);
        }
    }
    any_free(&result);
    return ret;
}

int parse_from_plain_buffer(const char *buffer, size_t len, any_t pattern, map_any *output) {
    if (!buffer || !len || !output || pattern.type == kUnknown) {
        return ERR_INVALID_ARGUMENT;
    }
    if (any_is_plain(&pattern)) {
        return ERR_SUCCESS;
    }
    if (any_is_string(&pattern)) {
        int         length = any_length(&pattern);
        const char *s1     = pattern.u.sval;
        const char *e1     = s1 + length;
        const char *s2     = buffer;
        const char *e2     = buffer + len;
        for (; s1 < e1 && s2 < e2; s1++, s2++) {
            if (*s1 == '$' && s1[1] == '{') {
                for (const char *key_end = s1 + 2; key_end < e1; key_end++) {
                    if (*key_end == '}') {
                        any_t value = {.type = kUnknown};
                        if (key_end + 1 == e1) {
                            if (s2 == buffer) {
                                any_set_string_n(&value, buffer, len);
                            } else {
                                any_set_string_n(&value, s2, e2 - s2);
                            }
                        } else {
                            for (const char *next_start = s2 + 1; next_start < e2; next_start++) {
                                if (*next_start == key_end[1]) {
                                    any_set_string_n(&value, s2, next_start - s2);
                                    s2 = next_start - 1;
                                    break;
                                }
                            }
                        }
                        char *key = malloc(key_end - s1 - 1);
                        if (!key) {
                            return ERR_OUT_OF_MEMORY;
                        }
                        strncpy(key, s1 + 2, key_end - s1 - 2);
                        key[key_end - s1 - 2]  = '\0';
                        s1                     = key_end;
                        map_any_iterator *iter = map_any_find(output, key);
                        if (iter == NULL) {
                            map_any_insert(output, key, value);
                        } else {
                            any_append(&iter->val0, value);
                        }
                        free(key);
                        break;
                    }
                }
            } else if (*s1 != *s2) {
                printf("[ERR] %s != %s\n", s1, s2);
                break;
            }
        }
        return ERR_SUCCESS;
    } else if (any_is_sequence(&pattern) || any_is_map(&pattern)) {
        return parse_from_yaml_buffer(buffer, len, pattern, output);
    }
    return ERR_SUCCESS;
}

static int parse_from_any(any_t src, any_t pattern, map_any *output) {
    if (any_is_plain(&pattern)) {
        return ERR_SUCCESS;
    }
    if (any_is_string(&pattern)) {
        int         length         = any_length(&pattern);
        const char *start          = pattern.u.sval;
        const char *end            = start + length - 1;
        bool        is_single_item = start[0] == '$' && start[1] == '{' && end[0] == '}';
        if (is_single_item) {
            for (const char *c = start + 2; is_single_item && c < end; c++) {
                if (*c == '}') {
                    is_single_item = false;
                }
            }
        }
        if (is_single_item) {
            char *key = malloc(length - 2);
            if (key == NULL) {
                return ERR_OUT_OF_MEMORY;
            }
            strncpy(key, start + 2, length - 3);
            key[length - 3]        = '\0';
            map_any_iterator *iter = map_any_find(output, key);
            if (iter == NULL) {
                map_any_insert(output, key, any_duplicate(&src));
            } else {
                any_append(&iter->val0, any_duplicate(&src));
            }
            free(key);
        } else if (any_is_string(&src)) {
            return parse_from_plain_buffer(src.u.sval, any_length(&src), pattern, output);
        }
    } else if (any_is_sequence(&pattern)) {
        if (any_length(&pattern) > 0 && pattern.u.aval != NULL && any_is_sequence(&src)) {
            for (int i = 0; i < any_length(&src); i++) {
                // pattern should only have *ONE* item in sequence
                parse_from_any(src.u.aval[i], pattern.u.aval[0], output);
            }
        }
    } else if (any_is_map(&pattern) && pattern.u.mval != NULL) {
        if (any_is_map(&src)) {
            for (map_any_iterator *iter = map_any_first(pattern.u.mval); iter != NULL; iter = map_any_next(iter)) {
                map_any_iterator *it = map_any_find(src.u.mval, iter->key);
                if (it != NULL) {
                    parse_from_any(it->val0, iter->val0, output);
                }
            }
        }
    }
    return ERR_SUCCESS;
}

int parse_from_yaml_buffer(const char *buffer, size_t len, any_t pattern, map_any *output) {
    any_t src = any_from_yaml_buffer(buffer, len);
    if (src.type == kUnknown) {
        return ERR_CONFIG_MALFORMED;
    }
    int ret = parse_from_any(src, pattern, output);
    any_free(&src);
    return ret;
}
