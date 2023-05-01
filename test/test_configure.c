#include "configuration.h"
#include "error.h"
#include "global.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *get_gateway_product_key() {
    return "R101Gateway";
}

const char *get_gateway_device_name() {
    return "70B3D50580AB68DB";
}

const char *get_gateway_vendor_code() {
    return "R101Gateway";
}

const char *get_gateway_hex_model_id() {
    return "R101_Gateway";
}

static int sequence = 1;
int        get_sequence() {
    return atomic_fetch_add(&sequence, 1);
}

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

typedef int (*TEST_FUNC)();

int gw_register() {
    return 0;
}

int gw_command() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    char           topic[256];
    char           payload[256];
    config_func_t *config     = &g_config.gw_command;
    const char    *payloads[] = {R"({"id":11,"code":0})"};
    for (int i = 0; i < 1; i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        int ret = format_string_from_context(topic, sizeof(topic), config->req_topic, NULL);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        map_any_insert(params, "devPK", any_from_const_string("R204040SJKG"));
        map_any_insert(params, "devDN", any_from_const_string("70B3D50580019679"));
        ret = format_from_context(payload, sizeof(payload), config->req_payload, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, NULL);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            while (map_any_first(params) != NULL) {
                map_any_iterator *iter = map_any_first(params);
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            any_set_const_string(&topic_pattern, config->res_topic);
            ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
                fail = 1;
                break;
            }
            ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->res_payload, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
                fail = 1;
                break;
            }
            for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
                char *str = any_to_json_str(&iter->val0);
                fprintf(stdout, "\t%s -> %s\n", iter->key, str);
                free(str);
            }
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}

int gw_message_up() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    char           topic[256];
    char           payload[256];
    config_func_t *config     = &g_config.gw_message_up;
    const char    *payloads[] = {R"({"id":11,"code":0})"};
    for (int i = 0; i < 1; i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        int ret = format_string_from_context(topic, sizeof(topic), config->req_topic, NULL);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        map_any_insert(params, "devPK", any_from_const_string("R204040SJKG"));
        map_any_insert(params, "devDN", any_from_const_string("70B3D50580019679"));
        ret = format_from_context(payload, sizeof(payload), config->req_payload, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, NULL);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            while (map_any_first(params) != NULL) {
                map_any_iterator *iter = map_any_first(params);
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            any_set_const_string(&topic_pattern, config->res_topic);
            ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
                fail = 1;
                break;
            }
            ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->res_payload, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
                fail = 1;
                break;
            }
            for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
                char *str = any_to_json_str(&iter->val0);
                fprintf(stdout, "\t%s -> %s\n", iter->key, str);
                free(str);
            }
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}

int gw_ota() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    char           topic[256];
    char           payload[256];
    config_func_t *config     = &g_config.gw_ota;
    const char    *payloads[] = {R"({"id":11,"code":0})"};
    for (int i = 0; i < 1; i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        int ret = format_string_from_context(topic, sizeof(topic), config->req_topic, NULL);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        map_any_insert(params, "devPK", any_from_const_string("R204040SJKG"));
        map_any_insert(params, "devDN", any_from_const_string("70B3D50580019679"));
        ret = format_from_context(payload, sizeof(payload), config->req_payload, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, NULL);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            while (map_any_first(params) != NULL) {
                map_any_iterator *iter = map_any_first(params);
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            any_set_const_string(&topic_pattern, config->res_topic);
            ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
                fail = 1;
                break;
            }
            ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->res_payload, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
                fail = 1;
                break;
            }
            for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
                char *str = any_to_json_str(&iter->val0);
                fprintf(stdout, "\t%s -> %s\n", iter->key, str);
                free(str);
            }
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}

int dev_register() {
    return 0;
}
int gw_login() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    char           topic[256];
    char           payload[256];
    config_func_t *config     = &g_config.gw_login;
    const char    *payloads[] = {R"({"id":11,"code":0})"};
    for (int i = 0; i < 1; i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        int ret = format_string_from_context(topic, sizeof(topic), config->req_topic, NULL);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        map_any_insert(params, "devPK", any_from_const_string("R204040SJKG"));
        map_any_insert(params, "devDN", any_from_const_string("70B3D50580019679"));
        ret = format_from_context(payload, sizeof(payload), config->req_payload, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, NULL);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            while (map_any_first(params) != NULL) {
                map_any_iterator *iter = map_any_first(params);
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            any_set_const_string(&topic_pattern, config->res_topic);
            ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
                fail = 1;
                break;
            }
            ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->res_payload, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
                fail = 1;
                break;
            }
            for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
                char *str = any_to_json_str(&iter->val0);
                fprintf(stdout, "\t%s -> %s\n", iter->key, str);
                free(str);
            }
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}
int gw_logout() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    char           topic[256];
    char           payload[256];
    config_func_t *config     = &g_config.gw_logout;
    const char    *payloads[] = {R"({"id":11,"code":0})"};
    for (int i = 0; i < 1; i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        int ret = format_string_from_context(topic, sizeof(topic), config->req_topic, NULL);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        map_any_insert(params, "devPK", any_from_const_string("R204040SJKG"));
        map_any_insert(params, "devDN", any_from_const_string("70B3D50580019679"));
        ret = format_from_context(payload, sizeof(payload), config->req_payload, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, NULL);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            while (map_any_first(params) != NULL) {
                map_any_iterator *iter = map_any_first(params);
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            any_set_const_string(&topic_pattern, config->res_topic);
            ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
                fail = 1;
                break;
            }
            ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->res_payload, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
                fail = 1;
                break;
            }
            for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
                char *str = any_to_json_str(&iter->val0);
                fprintf(stdout, "\t%s -> %s\n", iter->key, str);
                free(str);
            }
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}

int topo_join() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    char           topic[256];
    char           payload[256];
    config_func_t *config     = &g_config.topo_join;
    const char    *payloads[] = {R"({"id":11,"code":0,"params":[{"productKey":"R204040WJKG","deviceName":"70B3D5058001967A"}]})"};
    for (int i = 0; i < 1; i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        int ret = format_string_from_context(topic, sizeof(topic), config->req_topic, NULL);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        map_any_insert(params, "devPK", any_from_const_string("R204040SJKG"));
        map_any_insert(params, "devDN", any_from_const_string("70B3D50580019679"));
        ret = format_from_context(payload, sizeof(payload), config->req_payload, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, NULL);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            while (map_any_first(params) != NULL) {
                map_any_iterator *iter = map_any_first(params);
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            any_set_const_string(&topic_pattern, config->res_topic);
            ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
                fail = 1;
                break;
            }
            ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->res_payload, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
                fail = 1;
                break;
            }
            for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
                char *str = any_to_json_str(&iter->val0);
                fprintf(stdout, "\t%s -> %s\n", iter->key, str);
                free(str);
            }
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}

int topo_leave() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    char           topic[256];
    char           payload[256];
    config_func_t *config     = &g_config.topo_leave;
    const char    *payloads[] = {R"({"id":11,"code":0,"params":[{"productKey":"R204040SJKG","deviceName":"70B3D50580019679"}]})"};
    for (int i = 0; i < 1; i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        int ret = format_string_from_context(topic, sizeof(topic), config->req_topic, NULL);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        map_any_insert(params, "devPK", any_from_const_string("R204040SJKG"));
        map_any_insert(params, "devDN", any_from_const_string("70B3D50580019679"));
        ret = format_from_context(payload, sizeof(payload), config->req_payload, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, NULL);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            while (map_any_first(params) != NULL) {
                map_any_iterator *iter = map_any_first(params);
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            any_set_const_string(&topic_pattern, config->res_topic);
            ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
                fail = 1;
                break;
            }
            ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->res_payload, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
                fail = 1;
                break;
            }
            for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
                char *str = any_to_json_str(&iter->val0);
                fprintf(stdout, "\t%s -> %s\n", iter->key, str);
                free(str);
            }
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}
int topo_get() {
    return 0;
}
int topo_add() {
    return 0;
}
int topo_delete() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    config_func_t *config        = &g_config.topo_delete;
    const char    *payloads[]    = {
        R"({"id":1671602282,"status":1,"params":[{"productKey":"R204040SJKG","deviceName":"70B3D50580019679"}]})",
    };
    for (int i = 0; i < sizeof(payloads) / sizeof(payloads[0]); i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        char topic[256];
        int  ret = format_string_from_context(topic, sizeof(topic), config->req_topic, NULL);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        any_set_const_string(&topic_pattern, config->req_topic);
        ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
        if (ret != ERR_SUCCESS) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->req_payload, params);
        if (ret != ERR_SUCCESS) {
            fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
            fail = 1;
            break;
        }
        for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
            char *str = any_to_json_str(&iter->val0);
            fprintf(stdout, "\t%s -> %s\n", iter->key, str);
            free(str);
        }
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, NULL);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            any_t code = {.type = kInteger, .u = {.ival = 0}};
            map_any_insert(params, "CODE", code);
            char payload[256];
            ret = format_from_context(payload, sizeof(payload), config->res_payload, params);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}
int gw_property_post() {
    return 0;
}
int gw_property_set() {
    return 0;
}
int gw_event_post() {
    return 0;
}
int gw_service_call() {
    return 0;
}
int create_scene() {
    return 0;
}
int delete_scene() {
    return 0;
}
int trigger_scene() {
    return 0;
}
int enable_scene() {
    return 0;
}
int disable_scene() {
    return 0;
}
int report_version() {
    return 0;
}
int ota_request() {
    return 0;
}
int ota_progress() {
    return 0;
}
int dev_login() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    char           topic[256];
    char           payload[256];
    config_func_t *config     = &g_config.dev_login;
    const char    *payloads[] = {R"({"id":11,"code":0})"};
    for (int i = 0; i < 1; i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        int ret = format_string_from_context(topic, sizeof(topic), config->req_topic, NULL);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        map_any_insert(params, "devPK", any_from_const_string("R204040SJKG"));
        map_any_insert(params, "devDN", any_from_const_string("70B3D50580019679"));
        ret = format_from_context(payload, sizeof(payload), config->req_payload, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, NULL);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            while (map_any_first(params) != NULL) {
                map_any_iterator *iter = map_any_first(params);
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            any_set_const_string(&topic_pattern, config->res_topic);
            ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
                fail = 1;
                break;
            }
            ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->res_payload, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
                fail = 1;
                break;
            }
            for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
                char *str = any_to_json_str(&iter->val0);
                fprintf(stdout, "\t%s -> %s\n", iter->key, str);
                free(str);
            }
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}
int dev_logout() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    char           topic[256];
    char           payload[256];
    config_func_t *config     = &g_config.dev_logout;
    const char    *payloads[] = {R"({"id":11,"code":0})"};
    for (int i = 0; i < 1; i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        int ret = format_string_from_context(topic, sizeof(topic), config->req_topic, NULL);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        map_any_insert(params, "devPK", any_from_const_string("R204040SJKG"));
        map_any_insert(params, "devDN", any_from_const_string("70B3D50580019679"));
        ret = format_from_context(payload, sizeof(payload), config->req_payload, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, NULL);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            while (map_any_first(params) != NULL) {
                map_any_iterator *iter = map_any_first(params);
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            any_set_const_string(&topic_pattern, config->res_topic);
            ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
                fail = 1;
                break;
            }
            ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->res_payload, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
                fail = 1;
                break;
            }
            for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
                char *str = any_to_json_str(&iter->val0);
                fprintf(stdout, "\t%s -> %s\n", iter->key, str);
                free(str);
            }
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}
int dev_property_post() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    char           topic[256];
    char           payload[256];
    config_func_t *config     = &g_config.dev_property_post;
    const char    *payloads[] = {R"({"id":11,"code":0})"};
    for (int i = 0; i < 1; i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        map_any_insert(params, "devPK", any_from_const_string("R204040SJKG"));
        map_any_insert(params, "devDN", any_from_const_string("70B3D50580019679"));
        map_any_insert(params, "property_name", any_from_const_string("PowerSwitch_1"));
        map_any_insert(params, "property_value", any_from_bool(true));
        int ret = format_string_from_context(topic, sizeof(topic), config->req_topic, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        ret = format_from_context(payload, sizeof(payload), config->req_payload, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, params);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            while (map_any_first(params) != NULL) {
                map_any_iterator *iter = map_any_first(params);
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            any_set_const_string(&topic_pattern, config->res_topic);
            ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
                fail = 1;
                break;
            }
            ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->res_payload, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
                fail = 1;
                break;
            }
            for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
                char *str = any_to_json_str(&iter->val0);
                fprintf(stdout, "\t%s -> %s\n", iter->key, str);
                free(str);
            }
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}
int dev_property_set() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    config_func_t *config        = &g_config.dev_property_set;
    const char    *payloads[]    = {
        R"({"id":6,"params":[{"name":"PowerSwitch_1","value":false}]})",
    };
    for (int i = 0; i < sizeof(payloads) / sizeof(payloads[0]); i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        char topic[256];
        map_any_insert(params, "devPK", any_from_const_string("R204040SJKG"));
        map_any_insert(params, "devDN", any_from_const_string("70B3D50580019679"));
        int ret = format_string_from_context(topic, sizeof(topic), config->req_topic, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        any_set_const_string(&topic_pattern, config->req_topic);
        ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
        if (ret != ERR_SUCCESS) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->req_payload, params);
        if (ret != ERR_SUCCESS) {
            fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
            fail = 1;
            break;
        }
        for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
            char *str = any_to_json_str(&iter->val0);
            fprintf(stdout, "\t%s -> %s\n", iter->key, str);
            free(str);
        }
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, params);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            any_t code = {.type = kInteger, .u = {.ival = 0}};
            map_any_insert(params, "CODE", code);
            char payload[256];
            ret = format_from_context(payload, sizeof(payload), config->res_payload, params);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}
int dev_set_alias() {
    return 0;
}
int dev_ota_request() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    config_func_t *config        = &g_config.dev_ota_request;
    const char    *payloads[]    = {
        R"({"id":11,"url":"https://rex-gitlab.oss-cn-beijing.aliyuncs.com/coo/COO_RWP_Stk6710_1.4.2.ota"})",
    };
    for (int i = 0; i < sizeof(payloads) / sizeof(payloads[0]); i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        char topic[256];
        map_any_insert(params, "devPK", any_from_const_string("R204040SJKG"));
        map_any_insert(params, "devDN", any_from_const_string("70B3D50580019679"));
        int ret = format_string_from_context(topic, sizeof(topic), config->req_topic, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        any_set_const_string(&topic_pattern, config->req_topic);
        ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
        if (ret != ERR_SUCCESS) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->req_payload, params);
        if (ret != ERR_SUCCESS) {
            fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
            fail = 1;
            break;
        }
        for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
            char *str = any_to_json_str(&iter->val0);
            fprintf(stdout, "\t%s -> %s\n", iter->key, str);
            free(str);
        }
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, params);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            any_t code = {.type = kInteger, .u = {.ival = 0}};
            map_any_insert(params, "CODE", code);
            char payload[256];
            ret = format_from_context(payload, sizeof(payload), config->res_payload, params);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}
int dev_ota_progress() {
    return 0;
}

int dev_event_post() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    char           topic[256];
    char           payload[256];
    config_func_t *config     = &g_config.dev_event_post[0];
    const char    *payloads[] = {R"({"id":11,"code":0})"};
    for (int i = 0; i < 1; i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        map_any_insert(params, "devPK", any_from_const_string("R204040SJKG"));
        map_any_insert(params, "devDN", any_from_const_string("70B3D50580019679"));
        map_any_insert(params, "event_name", any_from_const_string("UserDefineDataEvent"));
        map_any_insert(params, "param_name", any_from_const_string("data"));
        map_any_insert(params, "param_value", any_from_const_string("0123456789ABCDEF"));
        int ret = format_string_from_context(topic, sizeof(topic), config->req_topic, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        ret = format_from_context(payload, sizeof(payload), config->req_payload, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, params);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            while (map_any_first(params) != NULL) {
                map_any_iterator *iter = map_any_first(params);
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            any_set_const_string(&topic_pattern, config->res_topic);
            ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
                fail = 1;
                break;
            }
            ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->res_payload, params);
            if (ret != ERR_SUCCESS) {
                fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
                fail = 1;
                break;
            }
            for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
                char *str = any_to_json_str(&iter->val0);
                fprintf(stdout, "\t%s -> %s\n", iter->key, str);
                free(str);
            }
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}
int dev_service_call() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    config_func_t *config        = &g_config.dev_service_call[0];
    const char    *payloads[]    = {
        R"({"id":6,"params":[{"key":"data","value":"0123456789ABCDEF"}]})",
    };
    for (int i = 0; i < sizeof(payloads) / sizeof(payloads[0]); i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        char topic[256];
        map_any_insert(params, "devPK", any_from_const_string("R204040SJKG"));
        map_any_insert(params, "devDN", any_from_const_string("70B3D50580019679"));
        map_any_insert(params, "service_name", any_from_const_string("UserDefineDataService"));
        int ret = format_string_from_context(topic, sizeof(topic), config->req_topic, params);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        any_set_const_string(&topic_pattern, config->req_topic);
        ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
        if (ret != ERR_SUCCESS) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->req_payload, params);
        if (ret != ERR_SUCCESS) {
            fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
            fail = 1;
            break;
        }
        for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
            char *str = any_to_json_str(&iter->val0);
            fprintf(stdout, "\t%s -> %s\n", iter->key, str);
            free(str);
        }
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, params);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            any_t code = {.type = kInteger, .u = {.ival = 0}};
            map_any_insert(params, "CODE", code);
            char payload[256];
            ret = format_from_context(payload, sizeof(payload), config->res_payload, params);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}

int create_zigbee_network() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    config_func_t *config        = &g_config.create_zigbee_network;
    const char    *payloads[]    = {
        R"({"id":2})",
        R"({"id":3,"params":[{"key": "Trug","value": true}]})",
        R"({"id":4,"params":[{"key": "ZigbeeNetworkParam","value": "22irgAXVs3DbaAoWAgEAANtoq4AF1bNwAgAAAI06rAlSBHzHoPwseYZgC8Szk9OZBgtANJGpe09jAkVwAIAbAACAAwA="}]})",
    };
    for (int i = 0; i < sizeof(payloads) / sizeof(payloads[0]); i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        char topic[256];
        int  ret = format_string_from_context(topic, sizeof(topic), config->req_topic, NULL);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        any_set_const_string(&topic_pattern, config->req_topic);
        ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
        if (ret != ERR_SUCCESS) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->req_payload, params);
        if (ret != ERR_SUCCESS) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
            char *str = any_to_json_str(&iter->val0);
            fprintf(stdout, "\t%s -> %s\n", iter->key, str);
            free(str);
        }
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, NULL);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            any_t code = {.type = kInteger, .u = {.ival = 0}};
            map_any_insert(params, "CODE", code);
            char payload[256];
            ret = format_from_context(payload, sizeof(payload), config->res_payload, params);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}

int leave_zigbee_network() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    config_func_t *config        = &g_config.leave_zigbee_network;
    do {
        char topic[256];
        int  ret = format_string_from_context(topic, sizeof(topic), config->req_topic, NULL);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        any_set_const_string(&topic_pattern, config->req_topic);
        ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
        if (ret != ERR_SUCCESS) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        const char *payload = R"({"id":1})";
        ret                 = parse_from_yaml_buffer(payload, strlen(payload), config->req_payload, params);
        if (ret != ERR_SUCCESS) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
            char *str = any_to_json_str(&iter->val0);
            fprintf(stdout, "\t%s -> %s\n", iter->key, str);
            free(str);
        }
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, NULL);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            any_t code = {.type = kInteger, .u = {.ival = 0}};
            map_any_insert(params, "CODE", code);
            char payload[256];
            ret = format_from_context(payload, sizeof(payload), config->res_payload, params);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        }
    } while (false);
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}


void get_current_time_ms(char *buf) {
    struct timespec ts;

    if (buf == NULL) {
        return;
    }

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return;
    }

    snprintf(buf, 32, "%lld", (long long)(ts.tv_sec) * 1000 + (long long)(ts.tv_nsec / 1000000));
}

int start_permit_join() {
    int            fail          = 0;
    map_any       *params        = map_any_create();
    any_t          topic_pattern = {.type = kUnknown};
    config_func_t *config        = &g_config.start_permit_join;
    const char    *payloads[]    = {
        R"({
            "version":"2.0",
            "flowDirection":"0",
            "controlType":"control.func",
            "messageType":"",
            "vendor":"REXENSE.TEST.001",
            "timestamp":"1617008027337",
            "sequence":"1234567",
            "data":[
                {
                    "model":"REXENSE_TEST_001",
                    "parentDeviceId":"",
                    "deviceId":"REXENSE.TEST.001.002",
                    "code":"start_zigbee_join",
                    "value":{
                        "join_second":"90",
                        "join_type":"zigbee"
                    }
                }
            ]
            })"};
    for (int i = 0; i < sizeof(payloads) / sizeof(payloads[0]); i++) {
        fprintf(stdout, "\n");
        while (map_any_first(params) != NULL) {
            map_any_iterator *iter = map_any_first(params);
            any_free(&iter->val0);
            map_any_erase(iter);
        }
        char topic[256];
        int  ret = format_string_from_context(topic, sizeof(topic), config->req_topic, NULL);
        if (ret < 0) {
            fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
            fail = 1;
            break;
        }
        fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
        any_set_const_string(&topic_pattern, config->req_topic);
        ret = parse_from_plain_buffer(topic, strlen(topic), topic_pattern, params);
        if (ret != ERR_SUCCESS) {
            fprintf(stderr, "[ERR] %s %d failed %d\n", __FUNCTION__, __LINE__, ret);
            fail = 1;
            break;
        }
        ret = parse_from_yaml_buffer(payloads[i], strlen(payloads[i]), config->req_payload, params);
        if (ret != ERR_SUCCESS) {
            fprintf(stderr, "[ERR] %s %d %s failed %d\n", __FUNCTION__, __LINE__, payloads[i], ret);
            fail = 1;
            break;
        }
        for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
            char *str = any_to_json_str(&iter->val0);
            fprintf(stdout, "\t%s -> %s\n", iter->key, str);
            free(str);
        }
        if (config->res_topic != NULL) {
            ret = format_string_from_context(topic, sizeof(topic), config->res_topic, NULL);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, topic);
            any_t code = {.type = kInteger, .u = {.ival = 0}};
            map_any_iterator *iter = map_any_find(params, "flowDirection");
            if (NULL != iter) {
                any_free(&iter->val0);
                iter->val0 = any_from_const_string("1");
                // map_any_erase(iter);
            }
            // map_any_insert(params, "flowDirection", any_from_const_string("1"));

            iter = map_any_find(params, "controlType");
            if (NULL != iter) {
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            map_any_insert(params, "controlType", any_from_const_string(""));

            iter = map_any_find(params, "messageType");
            if (NULL != iter) {
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            map_any_insert(params, "messageType", any_from_const_string("ack"));

            iter = map_any_find(params, "code");
            if (NULL != iter) {
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            map_any_insert(params, "code", any_from_const_string("ack"));

            iter = map_any_find(params, "value");
            if (NULL != iter) {
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            map_any_insert(params, "value", any_from_const_string("1"));

            iter = map_any_find(params, "timestamp");
            if (NULL != iter) {
                any_free(&iter->val0);
                map_any_erase(iter);
            }
            char timestamp[64];
            memset(timestamp, 0, sizeof(timestamp));
            get_current_time_ms(timestamp);
            map_any_insert(params, "timestamp", any_from_const_string(timestamp));
            
            char payload[512];
            ret = format_from_context(payload, sizeof(payload), config->res_payload, params);
            if (ret < 0) {
                fprintf(stderr, "[ERR] %s %d failed\n", __FUNCTION__, __LINE__);
                fail = 1;
                break;
            }
            fprintf(stdout, "%s: %s\n", __FUNCTION__, payload);
        }
    }
    any_free(&topic_pattern);
    map_any_destroy_ex(params);
    return fail;
}

struct mapping_s {
    const char *key;
    TEST_FUNC   func;
};

struct mapping_s mappings[] = {
    {          NODE_GW_REGISTER,           gw_register},
    {          NODE_GW_COMMAND,             gw_command},
    {          NODE_GW_MESSAGE,          gw_message_up},
    {              NODE_GW_OTA,                 gw_ota},
    {         NODE_DEV_REGISTER,          dev_register},
    {             NODE_GW_LOGIN,              gw_login},
    {            NODE_GW_LOGOUT,             gw_logout},
    {            NODE_TOPO_JOIN,             topo_join},
    {           NODE_TOPO_LEAVE,            topo_leave},
    {             NODE_TOPO_GET,              topo_get},
    {             NODE_TOPO_ADD,              topo_add},
    {          NODE_TOPO_DELETE,           topo_delete},
    {     NODE_GW_PROPERTY_POST,      gw_property_post},
    {      NODE_GW_PROPERTY_SET,       gw_property_set},
    {        NODE_GW_EVENT_POST,         gw_event_post},
    {      NODE_GW_SERVICE_CALL,       gw_service_call},
    {NODE_CREATE_ZIGBEE_NETWORK, create_zigbee_network},
    { NODE_LEAVE_ZIGBEE_NETWORK,  leave_zigbee_network},
    {    NODE_START_PERMIT_JOIN,     start_permit_join},
    {         NODE_CREATE_SCENE,          create_scene},
    {         NODE_DELETE_SCENE,          delete_scene},
    {        NODE_TRIGGER_SCENE,         trigger_scene},
    {         NODE_ENABLE_SCENE,          enable_scene},
    {        NODE_DISABLE_SCENE,         disable_scene},
    {       NODE_REPORT_VERSION,        report_version},
    {          NODE_OTA_REQUEST,           ota_request},
    {         NODE_OTA_PROGRESS,          ota_progress},
    {            NODE_DEV_LOGIN,             dev_login},
    {           NODE_DEV_LOGOUT,            dev_logout},
    {    NODE_DEV_PROPERTY_POST,     dev_property_post},
    {     NODE_DEV_PROPERTY_SET,      dev_property_set},
    {        NODE_DEV_SET_ALIAS,         dev_set_alias},
    {       NODE_DEV_EVENT_POST,        dev_event_post},
    {     NODE_DEV_SERVICE_CALL,      dev_service_call},
    {      NODE_DEV_OTA_REQUEST,       dev_ota_request},
    {     NODE_DEV_OTA_PROGRESS,      dev_ota_progress}
};

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
    printf("%s: %s", topic_value.u.sval, payload_str);
    any_free(&topic_value);
    return ERR_SUCCESS;
}

static int common_mqtt_subscribe(const char *topic, const char *message, int length, const char *topic_pattern_str, any_t payload_pattern, map_any *output) {
    int ret1 = parse_from_plain_buffer(topic, strlen(topic), any_from_const_string(topic_pattern_str), output);
    int ret2 = parse_from_yaml_buffer(message, length, payload_pattern, output);
    return ret1 ? ret1 : ret2;
}

int handle_dev_ota_progress(int id, int code, void *data) {
    config_func_t *config = &g_config.ota_progress;
    if (!config->enable || config->req_topic == NULL) {
        map_any_destroy(data);
        return 0;
    }
    map_any *req_context = (map_any *)data;
    map_any *context     = map_any_create();
    for (map_any_iterator *iter = map_any_first(req_context); iter != NULL; iter = map_any_next(iter)) {
        map_any_insert(context, iter->key, any_duplicate(&iter->val0));
    }
    map_any_insert(context, NAME_COMMON_OTA_STEP, any_from_int(code));
    map_any_insert(context, NAME_COMMON_OTA_DESC, any_from_const_string(""));
    int ret = common_mqtt_publish(config->req_topic, config->req_payload, context);
    map_any_destroy(context);
    if ((code < 0 || code >= 100) && data) {
        map_any_destroy(req_context);
    }
    return ret;
}

int handle_dev_ota_request(const char *topic, const char *message, int length, void *context) {
    printf("%s %s: %.*s", __FUNCTION__, topic, length, message);
    config_func_t *config = &g_config.dev_ota_request;
    map_any       *params = map_any_create();
    // common_mqtt_subscribe(topic, message, length, config->req_topic, config->req_payload, params);
    parse_from_plain_buffer(topic, strlen(topic), any_from_const_string(config->req_topic), params);
    // map_any_iterator *it_url = map_any_find(params, NAME_COMMON_OTA_URL);
    // map_any_iterator *it_dn  = map_any_find(params, NAME_DEV_DEVICE_NAME);
    // if (it_url && any_is_string(&it_url->val0) && it_url->val0.u.sval && it_dn && any_is_string(&it_dn->val0) && it_dn->val0.u.sval) {
    //     handle_dev_ota_progress(111, -1, params);
    // }
    for (map_any_iterator *iter = map_any_first(params); iter != NULL; iter = map_any_next(iter)) {
        printf("%s(%08X): %s\n", iter->key, iter->val0.type, iter->val0.u.sval);
    }
    map_any_destroy_ex(params);
    return 1;
}

int main(int argc, char **argv) {
    const char *filename = CONFIG_FILE_PATH;
    TEST_FUNC   cases[50];
    int         case_count = 0;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'f':
                filename = &argv[i][2];
                break;
            default:
                break;
            }
        } else {
            for (int j = 0; j < sizeof(mappings) / sizeof(struct mapping_s); j++) {
                if (strcmp(argv[i], mappings[j].key) == 0) {
                    cases[case_count++] = mappings[j].func;
                    break;
                }
            }
        }
    }
    if (case_count == 0) {
        for (int j = 0; j < sizeof(mappings) / sizeof(struct mapping_s); j++) {
            cases[case_count++] = mappings[j].func;
        }
    }
    int ret = load_config(filename, NULL);
    if (ret != ERR_SUCCESS) {
        fprintf(stderr, "load_config failed: %d\n", ret);
        return 1;
    }
    int total_failed = 0;
    for (int i = 0; i < case_count; i++) {
        total_failed += cases[i]();
    }
    if (true) {
        printf("\n----------------------------------------------------------------\n");
        const char *payload = R"({"id": 1671602280,"code": 0,"data": [{"productKey": "R204040SJKG","deviceName": "70B3D50580019679"}]})";
        any_t       result  = any_from_yaml_buffer(payload, strlen(payload));
        char       *str     = any_to_json_str(&result);
        printf("%s\n", str);
        free(str);
        any_free(&result);
    }
    free_config();
    printf("\n%d cases, passed %d, failed %d\n", case_count, case_count - total_failed, total_failed);

    any_t orig   = any_from_yaml_file("/etc/mqtt_plugin.yaml");
    any_t update = any_from_yaml_file("config.yaml");
    if (any_is_map(&update) && any_is_map(&orig)) {
        for (map_any_iterator *iter = map_any_first(update.u.mval); iter != NULL; iter = map_any_next(iter)) {
            if (strcmp(iter->key, "basic") == 0) {
                map_any_insert_ex(orig.u.mval, iter->key, any_duplicate(&iter->val0));
            } else if (strcmp(iter->key, "functionality") == 0) {
                map_any_insert_ex(orig.u.mval, iter->key, any_duplicate(&iter->val0));
            }
        }
    }
    ret = any_to_yaml_file(&orig, "test.yaml");
    any_free(&update);
    any_free(&orig);
    printf("\nany_to_yaml_file: %d\n", ret);
    return 0;
}
