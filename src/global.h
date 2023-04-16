#ifndef GLOBAL_H
#define GLOBAL_H

/**
 * 金山云一些配置项的KEY
*/
#define KC_CONFIG_URL_KEY "kc.url"
#define KC_CONFIG_PK_KEY "kc.pk"
#define KC_CONFIG_PS_KEY "kc.ps"
#define KC_CONFIG_VENDOR_KEY "kc.vendor"
#define KC_CONFIG_CLIENT_ID_KEY "kc.clientId"

/**
 * 插件配置的文件路径
*/
#define CONFIG_FILE_PATH   "/etc/mqtt_plugin.yaml"

/**
 * YAML配置头指针，记得释放，否则会有内存泄漏
*/
struct dp_conf_node *g_config_root;

#endif