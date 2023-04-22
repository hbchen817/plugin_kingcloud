#ifndef __DP_CONF_H
#define __DP_CONF_H

#define LOG_ERROR(fmt, ...) printf("%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) printf("%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) printf("%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) printf("%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

#define DP_CONF_NODE_NAME_MAX   1024
#define DP_CONF_NODE_LEVEL_MAX  128

#define NAME_GW_PRODUCT_KEY        "gwPK"
#define NAME_GW_DEVICE_NAME        "gwDN"
#define NAME_DEV_PRODUCT_KEY       "devPK"
#define NAME_DEV_DEVICE_NAME       "devDN"
#define NAME_DEV_EVENT_NAME        "event_name"
#define NAME_DEV_SERVICE_NAME      "service_name"
#define NAME_DEV_ALIAS             "alias"
#define NAME_COMMON_CODE           "CODE"
#define NAME_COMMON_PROPERTY_NAME  "property_name"
#define NAME_COMMON_PROPERTY_VALUE "property_value"
#define NAME_COMMON_PARAM_NAME     "param_name"
#define NAME_COMMON_PARAM_VALUE    "param_value"
#define NAME_COMMON_PARAMS         "params"
#define NAME_COMMON_KEY            "key"
#define NAME_COMMON_VALUE          "value"
#define NAME_COMMON_MODULE_NAME    "module_name"
#define NAME_COMMON_VERSION_STR    "version_str"
#define NAME_COMMON_OTA_URL        "URL"
#define NAME_COMMON_OTA_ID         "OTA_ID"
#define NAME_COMMON_OTA_STEP       "OTA_STEP"
#define NAME_COMMON_OTA_DESC       "OTA_DESC"
#define NAME_FUNC_TIMESTAMP        "TIMESTAMP"
#define NAME_FUNC_IOTA             "IOTA"

enum dp_conf_value_type {
    DP_CONF_VALUE_TYPE_NONE,
    DP_CONF_VALUE_TYPE_SCALAR,
    DP_CONF_VALUE_TYPE_MAPPING,
    DP_CONF_VALUE_TYPE_SEQUENCE,
};

struct dp_conf_node {
    enum dp_conf_value_type value_type;

    char                *name;
    char                *value;

    struct dp_conf_node *parent;
    struct dp_conf_node *head;
    struct dp_conf_node *next;
};

/**
 * 解析YAML格式文件
 * 返回配置根
 * */
extern struct dp_conf_node *conf_parse_file(const char *yaml_file);

/**
 * 仅查找单层子节点
 * */
extern struct dp_conf_node *conf_node_lookup_child(struct dp_conf_node *node, const char *key);

/**
 * 查找配置树形结构，通过点分割字符串定位配置节点，比如"server.redis.port"
 * */
extern struct dp_conf_node *conf_get_node(struct dp_conf_node *node, const char *name);

/**
 * 将一个配置节点从所在的树形结构中移除
 * 该节点下属的子节点结构不变，且无内存释放
 * */
extern void conf_remove_node(struct dp_conf_node *node);

/**
 * 向日志输出配置信息
 * */
extern void conf_dump(struct dp_conf_node *node, const char *prefix);

/**
 * 释放以该节点为根的所有结构内存，内部隐含移除操作
 * */
extern void conf_free_root(struct dp_conf_node *root);

/**
 * 获取配置的字符串值，通过点分割字符串定位配置
 * 返回0成功，其他失败
 * */
extern int conf_get_value(struct dp_conf_node *node, const char *name, char **ptr);

/**
 * 获取配置的有符号长整型值，通过点分割字符串定位配置
 * 返回0成功，其他失败
 * */
extern int conf_get_long(struct dp_conf_node *node, const char *name, long *ptr);

/**
 * 获取配置的布尔值，通过点分割字符串定位配置。
 * 配置文件中真值允许设置为"true, yes, on, 1"
 * 返回0成功，其他失败
 * 返回成功后，*ptr为1代表布尔为真，*ptr为0代表布尔为假
 * */
extern int conf_get_bool(struct dp_conf_node *node, const char *name, int *ptr);

/**
 * 设置或更新一个配置，通过点分割字符串定位配置，中间层级类型必须是mapping
 * 返回0成功，其他失败
 * */
extern int conf_set(struct dp_conf_node *node, const char *name, char *value);

/**
 * 填充从根到配置节点的全路径名
 * 返回写入的字符串长度，不包含结尾的NULL
 * 如果缓冲区长度不足，则会截断，并返回期望的缓冲区长度，不包含结尾的NULL
 * */
extern int conf_fill_path(char *buf, int len, const struct dp_conf_node *node);

#endif