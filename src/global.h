#ifndef GLOBAL_H
#define GLOBAL_H

/**
 * 金山云一些配置项的KEY
*/
#define KC_CONFIG_URL_KEY               "kc.url"
#define KC_CONFIG_PK_KEY                "kc.pk"
#define KC_CONFIG_PS_KEY                "kc.ps"
#define KC_CONFIG_MODEL_CODE_KEY        "kc.model.code"
#define KC_CONFIG_VENDOR_KEY            "kc.vendor"
#define KC_CONFIG_CLIENT_ID_KEY         "kc.clientId"
#define KC_CONFIG_BROKER_KEY            "kc.broker"

/**
 * 插件配置的文件路径
*/
#define CONFIG_FILE_PATH   "/etc/mqtt_plugin.yaml"

/**
 * YAML配置头指针，记得释放，否则会有内存泄漏
*/
extern struct dp_conf_node *g_config_root;

typedef struct {
    // 数据上行topic
    // {vendor_code}/{hex_mode_id}/{clientId}/message/up
    char message_up_topic[256];

    // 命令下行topic
    // {vendor_code}/{hex_mode_id}/{clientId}/command/down
    char command_down_topic[256];

    // 命令ack topic
    // {vendor_code}/{hex_mode_id}/{clientId}/command/ack
    char command_ack_topic[256];

    // ota相关topic
    // vendor_code/hex_mode_id/{clientId}/ota/ack
    char ota_topic[256];
} server_topics;

extern server_topics g_server_topics;

extern int init(struct Instance *instance);
extern int destroy();
#endif