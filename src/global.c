#include "global.h"
#include "config.h"
#include "instance.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct dp_conf_node *g_config_root;
server_topics g_server_topics;

int init(struct Instance *instance) {
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
    strcpy(instance->productKey, node->value);

    node = conf_get_node(g_config_root, KC_CONFIG_PS_KEY);
    if (NULL == node) {
        log_error("please configure product secret in yaml!");
        return;
    }
    strcpy(instance->productSecret, node->value);

    node = conf_get_node(g_config_root, KC_CONFIG_VENDOR_KEY);
    if (NULL == node) {
        log_error("please configure vendor code in yaml!");
        return;
    }
    strcpy(instance->vendorCode, node->value);

    node = conf_get_node(g_config_root, KC_CONFIG_MODEL_CODE_KEY);
    if (NULL == node) {
        log_error("please configure model code in yaml!");
        return;
    }
    strcpy(instance->modelCode, node->value);

    // 按照预定，将设备模型的编码.替换为 _
    replace_char(instance->hexModelId, instance->modelCode, '.', '_');

    node = conf_get_node(g_config_root, KC_CONFIG_CLIENT_ID_KEY);
    if (NULL == node) {
        log_error("please configure client id in yaml!");
        return;
    }
    strcpy(instance->vendorCode, node->value);

    node = conf_get_node(g_config_root, KC_CONFIG_BROKER_KEY);
    if (NULL == node) {
        log_error("please configure broker url in yaml!");
        return;
    }
    strcpy(instance->mqttBroker, node->value);

    // 按照规定初始化topic
    memset(&g_server_topics, 0, sizeof(server_topics));

    // {vendor_code}/{hex_mode_id}/{clientId}/message/up 
    sprintf(g_server_topics.message_up_topic, "%s/%s/%s/message/up", 
                instance->vendorCode, instance->hexModelId, instance->clientId);
    
    // {vendor_code}/{hex_mode_id}/{clientId}/command/down
    sprintf(g_server_topics.command_down_topic, "%s/%s/%s/command/down", 
                instance->vendorCode, instance->hexModelId, instance->clientId);
    
    // {vendor_code}/{hex_mode_id}/{clientId}/command/ack
    sprintf(g_server_topics.command_ack_topic, "%s/%s/%s/command/ack", 
                instance->vendorCode, instance->hexModelId, instance->clientId);
    
    // {vendor_code}/{hex_mode_id}/{clientId}/ota/ack
    sprintf(g_server_topics.ota_topic, "%s/%s/%s/ota/ack", 
                instance->vendorCode, instance->hexModelId, instance->clientId);
}

int destroy() {
    if (NULL != g_config_root) {
        conf_free_root(g_config_root);
        g_config_root = NULL;
    }
}

static void replace_char(char *result, char *source, char s1, char s2) {
    int i = 0;
    char *q = NULL;
    q = source;
    for(i = 0; i < strlen(source); i++) {
        if(q[i] == s1) {
            q[i] = s2;
        }
    }
    strcpy(result, q);
}

static void replace_string(char *result, char *source, char* s1, char *s2) {
    char *q = NULL;
    char *p = NULL;
    p = source;
    while((q=strstr(p, s1)) != NULL) {
        strncpy(result, p, q-p);
        // very important, must attention!
        result[q - p]= '\0';
        strcat(result, s2);
        strcat(result, q+strlen(s1));
        strcpy(p,result);
    }
    strcpy(result, p);
}