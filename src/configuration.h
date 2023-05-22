#pragma once
#include "any.h"
#include <stdbool.h>

#define NAME_GW_PRODUCT_KEY                                 "gwPK"
#define NAME_GW_DEVICE_NAME                                 "gwDN"
#define NAME_GW_VENDOR_CODE                                 "gwVC"
#define NAME_GW_MODEL_ID                                    "gwMD"
#define NAME_DEV_PRODUCT_KEY                                "devPK"
#define NAME_DEV_DEVICE_NAME                                "devDN"
#define NAME_DEV_EVENT_NAME                                 "event_name"
#define NAME_DEV_SERVICE_NAME                               "service_name"
#define NAME_DEV_ALIAS                                      "alias"
#define NAME_COMMON_CODE                                    "CODE"
#define NAME_COMMON_PROPERTY_NAME                           "property_name"
#define NAME_COMMON_PROPERTY_VALUE                          "property_value"
#define NAME_COMMON_PARAM_NAME                              "param_name"
#define NAME_COMMON_PARAM_VALUE                             "param_value"
#define NAME_COMMON_PARAMS                                  "params"
#define NAME_COMMON_KEY                                     "key"
#define NAME_COMMON_VALUE                                   "value"
#define NAME_COMMON_MODULE_NAME                             "module_name"
#define NAME_COMMON_VERSION_STR                             "version_str"
#define NAME_COMMON_OTA_URL                                 "URL"
#define NAME_COMMON_OTA_ID                                  "OTA_ID"
#define NAME_COMMON_OTA_STEP                                "OTA_STEP"
#define NAME_COMMON_OTA_DESC                                "OTA_DESC"
#define NAME_FUNC_TIMESTAMP                                 "TIMESTAMP"
#define NAME_FUNC_IOTA                                      "IOTA"
#define NAME_KC_COMMON_VERSION                              "version"
#define NAME_KC_COMMON_FLOW_DIRECTION                       "flowDirection"
#define NAME_KC_COMMON_CONTROL_TYPE                         "control.func"
#define NAME_KC_COMMON_MESSAGE_TYPE                         "messageType"
#define NAME_KC_COMMON_VENDOR                               "vendor"
#define NAME_KC_COMMON_TIMESTAMP                            "timestamp"
#define NAME_KC_COMMON_SEQUENCE                             "sequence"
#define NAME_KC_COMMON_DATA                                 "data"
#define NAME_KC_DATA_MODEL                                  "model"
#define NAME_KC_DATA_PARENT_DEVICE_ID                       "parentDeviceId"
#define NAME_KC_DATA_DEVICE_ID                              "deviceId"
#define NAME_KC_DATA_CODE                                   "code"
#define NAME_KC_DATA_VALUE                                  "value"

typedef struct config_basic_s {
    char *url;
    char *username;
    char *password;
    char *broker;
    char *productKey;
    char *productSecret;
    char *vendorCode;
    char *modelCode;
} config_basic_t;

typedef struct config_func_s {
    bool     enable;
    char    *req_topic;
    any_t    req_payload;
    char    *res_topic;
    any_t    res_payload;
    map_any *conditions;
    map_any *success;
} config_func_t;

typedef struct config_s {
    config_basic_t basic;
    config_func_t  gw_register;
    config_func_t  dev_register;
    config_func_t  gw_message_up;
    config_func_t  gw_command;
    config_func_t  gw_ota;
    config_func_t  gw_login;
    config_func_t  gw_logout;
    config_func_t  topo_join;
    config_func_t  topo_leave;
    config_func_t  topo_get;
    config_func_t  topo_add;
    config_func_t  topo_delete;
    config_func_t  gw_property_post;
    config_func_t  gw_property_set;
    config_func_t  gw_event_post;
    config_func_t  gw_service_call;
    config_func_t  create_zigbee_network;
    config_func_t  leave_zigbee_network;
    config_func_t  start_permit_join;
    config_func_t  stop_permit_join;
    config_func_t  create_scene;
    config_func_t  delete_scene;
    config_func_t  trigger_scene;
    config_func_t  enable_scene;
    config_func_t  disable_scene;
    config_func_t  report_version;
    config_func_t  ota_request;
    config_func_t  ota_progress;
    config_func_t  dev_login;
    config_func_t  dev_logout;
    config_func_t  dev_property_post;
    config_func_t  dev_property_set;
    config_func_t  dev_ota_request;
    config_func_t  dev_ota_progress;
    config_func_t  dev_set_alias;
    int            dev_event_post_cnt;
    config_func_t *dev_event_post;
    int            dev_service_call_cnt;
    config_func_t *dev_service_call;
} config_t;

extern config_t g_config;

int  load_config(const char *filename, const char *update);
void free_config();

typedef struct format_context_s {
    const char *dev_product_key;
    const char *dev_device_name;
    const char *event_name;
    const char *service_name;
    map_any    *params;
} format_context_t;

any_t generate_value(any_t pattern, const map_any *context);
int   format_from_context(char *buffer, size_t len, any_t pattern, const map_any *context);
int   format_string_from_context(char *buffer, size_t len, const char *pattern, const map_any *context);

int parse_from_plain_buffer(const char *buffer, size_t len, any_t pattern, map_any *output);
int parse_from_yaml_buffer(const char *buffer, size_t len, any_t pattern, map_any *output);

enum StrToIntError {
    STR_TO_INT_OK = 0,
    STR_TO_INT_INVALID_ARGUMENT,
    STR_TO_INT_OVERFLOW,
    STR_TO_INT_UNDERFLOW,
    STR_TO_INT_INCONVERTIBLE
};
