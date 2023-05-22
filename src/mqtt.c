#include "mqtt.h"
#include "MQTTAsync.h"
#include "deque.h"
#include "error.h"
#include "instance.h"
#include "timer_manager.h"
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <threads.h>
#include <unistd.h>

#define MQTTAsync_willOptions_initializer                                                                                                                      \
    {                                                                                                                                                          \
        {'M', 'Q', 'T', 'W'}, 1, NULL, NULL, 0, 0, { 0, NULL }                                                                                                 \
    }

#define MQTTAsync_connectOptions_initializer_                                                                                                                  \
    {                                                                                                                                                          \
        {'M', 'Q', 'T', 'C'},    /* struct_id         */                                                                                                       \
            8,                   /* struct_version    */                                                                                                       \
            60,                  /* keepAliveInterval */                                                                                                       \
            1,                   /* cleansession      */                                                                                                       \
            65535,               /* maxInflight       */                                                                                                       \
            NULL,                /* will              */                                                                                                       \
            NULL,                /* username          */                                                                                                       \
            NULL,                /* password          */                                                                                                       \
            30,                  /* connectTimeout    */                                                                                                       \
            0,                   /* retryInterval     */                                                                                                       \
            NULL,                /* ssl               */                                                                                                       \
            NULL,                /* onSuccess         */                                                                                                       \
            NULL,                /* onFailure         */                                                                                                       \
            NULL,                /* context           */                                                                                                       \
            0,                   /* serverURIcount    */                                                                                                       \
            NULL,                /* serverURIs        */                                                                                                       \
            MQTTVERSION_DEFAULT, /* MQTTVersion       */                                                                                                       \
            0,                   /* automaticReconnect*/                                                                                                       \
            1,                   /* minRetryInterval  */                                                                                                       \
            60,                  /* maxRetryInterval  */                                                                                                       \
            {0, NULL},           /* binarypwd         */                                                                                                       \
            0,                   /* cleanstart        */                                                                                                       \
            NULL,                /* connectProperties */                                                                                                       \
            NULL,                /* willProperties    */                                                                                                       \
            NULL,                /* onSuccess5        */                                                                                                       \
            NULL,                /* onFailure5        */                                                                                                       \
            NULL,                /* httpHeaders       */                                                                                                       \
            NULL,                /* httpProxy         */                                                                                                       \
            NULL                 /* httpsProxy        */                                                                                                       \
    }

struct UserContext {
    struct MqttInstance *mqtt;
    MqttFinishCallback   cb;
    void                *context;
};

DECLARE_QUEUE(pending_msg, char * /* topic */, char * /* message */, struct UserContext * /* ctx */)
IMPLEMENT_QUEUE(pending_msg, char * /* topic */, char * /* message */, struct UserContext * /* ctx */)

struct TopicItem {
    char               *topic;
    int                 levels;
    struct iovec       *sections;
    MqttMessageCallback cb;
    void               *context;
    struct TopicItem   *next;
};

static struct TopicItem *TopicItem_new(const char *topic, MqttMessageCallback cb, void *context) {
    int levels = 1;
    for (const char *p = topic; *p != '\0'; p++) {
        if (*p == '/') {
            levels++;
        }
    }
    char             *buffer = (char *)malloc(sizeof(struct TopicItem) + strlen(topic) + 1 + levels * sizeof(struct iovec));
    struct TopicItem *item   = (struct TopicItem *)buffer;
    item->topic              = buffer + sizeof(struct TopicItem);
    strcpy(item->topic, topic);
    item->levels   = levels;
    item->sections = (struct iovec *)(item->topic + strlen(topic) + 1);
    const char *p  = item->topic;
    const char *q  = p;
    for (int i = 0;; p++) {
        if (*p == '\0' || *p == '/') {
            item->sections[i].iov_len  = p - q;
            item->sections[i].iov_base = (void *)q;
            if (*p == '\0') {
                break;
            }
            q = p + 1;
            i++;
        }
    }
    item->cb      = cb;
    item->context = context;
    item->next    = NULL;
    return item;
}

static struct TopicItem *TopicItem_delete(struct TopicItem *item) {
    struct TopicItem *next = item->next;
    free(item);
    return next;
}

struct MqttInstance {
    MQTTAsync          client;
    bool               running;
    bool               connecting;
    bool               connected;
    mtx_t              mutex;
    struct TopicItem  *topicList;
    char              *address;
    char              *username;
    char              *password;
    int                waitTime;
    queue_pending_msg  pendingMessages;
    timer_manager_t    timer;
    MqttFinishCallback connectCallback;
    void              *connectContext;
};

static void connlost(void *context, char *cause);
static int  messageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message);
static void onConnect(void *context, MQTTAsync_successData *response);
static void onConnectFailure(void *context, MQTTAsync_failureData *response);
static void onSubscribe(void *context, MQTTAsync_successData *response);
static void onSubscribeFailure(void *context, MQTTAsync_failureData *response);
static void onSend(void *context, MQTTAsync_successData *response);
static void onSendFailure(void *context, MQTTAsync_failureData *response);
static int  do_mqtt_start(struct MqttInstance *mqtt);
static void retry_start_mqtt(void *param);

static int  waitTime[] = {0, 1, 2, 5, 10, 20, 30, 60, 120, 180, 300, 600};
static void increase_delay_time(struct MqttInstance *mqtt) {
    if (mqtt->waitTime < sizeof(waitTime) / sizeof(waitTime[0]) - 1) {
        mqtt->waitTime++;
    }
}

static void connlost(void *context, char *cause) {
    log_warn("Connection lost: %s", cause ? cause : "(unknown)");
    struct MqttInstance *mqtt = (struct MqttInstance *)context;
    mqtt->connected           = false;
    while (mqtt->topicList != NULL) {
        mqtt->topicList = TopicItem_delete(mqtt->topicList);
    }
    retry_start_mqtt(mqtt);
}

static int do_mqtt_start(struct MqttInstance *mqtt) {
    if (!mqtt->running) {
        return MQTTASYNC_SUCCESS;
    }
    MQTTAsync_willOptions    willOptions = MQTTAsync_willOptions_initializer;
    MQTTAsync_connectOptions options     = MQTTAsync_connectOptions_initializer_;
    config_func_t           *config      = &g_config.gw_logout;
    any_t                    topic_value = {.type = kUnknown};
    char                     payload_str[512];
    if (config->enable && config->req_topic != NULL) {
        any_t topic_pattern = {.type = kUnknown};
        any_set_const_string(&topic_pattern, config->req_topic);
        topic_value = generate_value(topic_pattern, NULL);
        if (any_is_string(&topic_value)) {
            if (format_from_context(payload_str, sizeof(payload_str), config->req_payload, NULL) >= 0) {
                willOptions.topicName = topic_value.u.sval;
                willOptions.message   = payload_str;
                options.will          = &willOptions;
            }
        }
    }
    MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
    options.username              = mqtt->username;
    options.password              = mqtt->password;
    options.keepAliveInterval     = 20;
    options.cleansession          = 1;
    options.onSuccess             = onConnect;
    options.onFailure             = onConnectFailure;
    options.context               = mqtt;
    if (strncmp(mqtt->address, "ssl://", 6) == 0) {
        options.ssl = &ssl_opts;
    }
    int rc = MQTTAsync_connect(mqtt->client, &options);
    if (rc != MQTTASYNC_SUCCESS) {
        log_error("Failed to start connect: %d", rc);
    } else {
        mqtt->connecting = true;
    }
    any_free(&topic_value);
    return rc;
}

static void retry_start_mqtt(void *param) {
    struct MqttInstance *mqtt = (struct MqttInstance *)param;
    if (do_mqtt_start(mqtt) != MQTTASYNC_SUCCESS) {
        increase_delay_time(mqtt);
        tm_run_after(mqtt->timer, 1000 * waitTime[mqtt->waitTime], retry_start_mqtt, mqtt);
    }
}

static void onConnect(void *context, MQTTAsync_successData *response) {
    struct MqttInstance *mqtt = (struct MqttInstance *)context;
    log_info("Successful connected %s", mqtt->address);
    mqtt->connected  = true;
    mqtt->connecting = false;
    mqtt->waitTime   = 0;
    if (mqtt->connectCallback != NULL) {
        mqtt->connectCallback(mqtt->connectContext, 0);
    }
    mtx_lock(&mqtt->mutex);
    while (!queue_pending_msg_empty(&mqtt->pendingMessages)) {
        MQTTAsync_responseOptions   opts   = MQTTAsync_responseOptions_initializer;
        MQTTAsync_message           pubmsg = MQTTAsync_message_initializer;
        queue_pending_msg_iterator *iter   = queue_pending_msg_front(&mqtt->pendingMessages);
        opts.onSuccess                     = onSend;
        opts.onFailure                     = onSendFailure;
        opts.context                       = iter->val2;
        pubmsg.payload                     = iter->val1;
        pubmsg.payloadlen                  = strlen(iter->val1);
        pubmsg.qos                         = 1;
        pubmsg.retained                    = 0;
        int rc                             = MQTTAsync_sendMessage(mqtt->client, iter->val0, &pubmsg, &opts);
        if (rc != MQTTASYNC_SUCCESS) {
            log_error("Failed to start sendMessage: %d", rc);
        }
        free(iter->val0);
        free(iter->val1);
        queue_pending_msg_pop_front(&mqtt->pendingMessages);
    }
    mtx_unlock(&mqtt->mutex);
}

static void onConnectFailure(void *context, MQTTAsync_failureData *response) {
    log_error("Connect failed: %d(%s)", response ? response->code : 0, response && response->message ? response->message : "nil");
    struct MqttInstance *mqtt = (struct MqttInstance *)context;
    if (mqtt->running) {
        increase_delay_time(mqtt);
        log_info("Reconnect after %d seconds", waitTime[mqtt->waitTime]);
        tm_run_after(mqtt->timer, 1000 * waitTime[mqtt->waitTime], retry_start_mqtt, mqtt);
        mqtt->connecting = false;
    } else {
        mqtt->connecting = false;
    }
}

static void onDisconnect(void *context, MQTTAsync_successData *response) {
    log_info("Successful disconnection");
    struct MqttInstance *mqtt = (struct MqttInstance *)context;
    mqtt->connected           = false;
}

static void onDisconnectFailure(void *context, MQTTAsync_failureData *response) {
    log_error("Disconnect failed: %d", response ? response->code : 0);
    struct MqttInstance *mqtt = (struct MqttInstance *)context;
    mqtt->connected           = false;
}

static void onSubscribe(void *context, MQTTAsync_successData *response) {
    if (context != NULL) {
        struct UserContext *ctx = (struct UserContext *)context;
        if (ctx->cb != NULL) {
            ctx->cb(ctx->context, 0);
        }
        free(ctx);
    }
}

static void onSubscribeFailure(void *context, MQTTAsync_failureData *response) {
    log_error("Subscribe failed: %d", response ? response->code : 0);
    if (context != NULL) {
        struct UserContext *ctx = (struct UserContext *)context;
        if (ctx->cb != NULL) {
            ctx->cb(ctx->context, response && response->code ? response->code : -1);
        }
        free(ctx);
    }
}

static void onUnsubscribe(void *context, MQTTAsync_successData *response) {
    log_verbose("Unsubscribe succeeded");
}

static void onUnsubscribeFailure(void *context, MQTTAsync_failureData *response) {
    log_error("Unsubscribe failed: %d", response ? response->code : 0);
}

static void onSend(void *context, MQTTAsync_successData *response) {
    log_verbose("Message with token value %d delivery confirmed", response->token);
    if (context != NULL) {
        struct UserContext *ctx = (struct UserContext *)context;
        if (ctx->cb != NULL) {
            ctx->cb(ctx->context, 0);
        }
        free(ctx);
    }
}

static void onSendFailure(void *context, MQTTAsync_failureData *response) {
    log_error("Message send failed token %d error code %d", response->token, response->code);
    if (context != NULL) {
        struct UserContext *ctx = (struct UserContext *)context;
        if (ctx->cb != NULL) {
            ctx->cb(ctx->context, response ? response->code : -1);
        }
        free(ctx);
    }
}

static int messageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message) {
    int         levels = 1;
    const char *p = topicName, *q = topicName;
    for (int i = 0; i < topicLen; i++, p++) {
        if (*p == '/') {
            levels++;
        }
    }
    struct iovec *sections = malloc(sizeof(struct iovec) * levels);
    p                      = topicName;
    for (int i = 0, j = 0;; j++, p++) {
        if (j == topicLen || *p == '/') {
            sections[i].iov_len  = p - q;
            sections[i].iov_base = (void *)q;
            if (j == topicLen) {
                break;
            }
            q = p + 1;
            i++;
        }
    }
    struct MqttInstance *mqtt = (struct MqttInstance *)context;
    mtx_lock(&mqtt->mutex);
    for (struct TopicItem *temp = mqtt->topicList; temp != NULL; temp = temp->next) {
        if (temp->levels > levels) {
            continue;
        }
        int match = 1;
        if (temp->sections[temp->levels - 1].iov_len == 1 && *(char *)temp->sections[temp->levels - 1].iov_base == '#') {
            for (int i = 0; i < temp->levels - 1; i++) {
                if (temp->sections[i].iov_len == 1 && *(char *)temp->sections[i].iov_base == '+') {
                    continue;
                } else if (temp->sections[i].iov_len == sections[i].iov_len &&
                           (sections[i].iov_len == 0 || strncmp(temp->sections[i].iov_base, sections[i].iov_base, sections[i].iov_len) == 0)) {
                    continue;
                } else {
                    match = 0;
                    break;
                }
            }
        } else if (temp->levels < levels) {
            continue;
        } else {
            for (int i = 0; i < temp->levels; i++) {
                if (temp->sections[i].iov_len == 1 && *(char *)temp->sections[i].iov_base == '+') {
                    continue;
                } else if (temp->sections[i].iov_len == sections[i].iov_len &&
                           (sections[i].iov_len == 0 || strncmp(temp->sections[i].iov_base, sections[i].iov_base, sections[i].iov_len) == 0)) {
                    continue;
                } else {
                    match = 0;
                    break;
                }
            }
        }
        if (match) {
            mtx_unlock(&mqtt->mutex);
            free(sections);
            int rc = temp->cb(topicName, message->payload, message->payloadlen, temp->context);
            MQTTAsync_freeMessage(&message);
            MQTTAsync_free(topicName);

            // 这里返回true，因为不管处理成功还是失败，只需要结果上报即可
            // 否则，这里会做无限重试，但是上面已经free掉message了
            return 1;
        }
    }
    mtx_unlock(&mqtt->mutex);
    free(sections);
    log_warn("unhandled message %s: %.*s", topicName, message->payloadlen, (char *)message->payload);
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

struct MqttInstance *mqtt_new() {
    int length = sizeof(struct MqttInstance);

    // 地址、用户名、密码默认都预留64个字符吧
    length += 64 * 3;
    struct MqttInstance *mqtt = (struct MqttInstance *)malloc(length);
    memset(mqtt, 0, length);
    mqtt->address = (char *)mqtt + sizeof(struct MqttInstance);
    mqtt->username = mqtt->address + 64;
    mqtt->password = mqtt->username + 64;
    
    queue_pending_msg_init(&mqtt->pendingMessages);
    mtx_init(&mqtt->mutex, mtx_plain);
    mqtt->timer = tm_create(0, "mqtt_retry");
    return mqtt;
}

bool mqtt_is_connected(struct MqttInstance *mqtt) {
    return mqtt != NULL && mqtt->connected;
}

void mqtt_delete(struct MqttInstance *mqtt) {
    if (mqtt != NULL) {
        tm_destroy(mqtt->timer);
        
        while (mqtt->topicList != NULL) {
            mqtt->topicList = TopicItem_delete(mqtt->topicList);
        }
        queue_pending_msg_deinit(&mqtt->pendingMessages);
        free(mqtt);
    }
}

int mqtt_start(struct MqttInstance *mqtt, MqttFinishCallback cb, void *context, 
                    char *address, char *username, char *password) {
    if (mqtt == NULL) {
        return ERR_MQTT_UNINITIALIZED;
    }

    if ((NULL != address) && (address[0] != '\0')) {
        strcpy(mqtt->address, address);
    }

    if ((NULL != username) && (username[0] != '\0')) {
        strcpy(mqtt->username, username);
    }

    if ((NULL != password) && (password[0] != '\0')) {
        strcpy(mqtt->password, password);
    }

    log_debug("create mqtt client, %s: address[%s], username[%s], password[%s]", 
                __FUNCTION__, mqtt->address, mqtt->username, mqtt->password);

    int rc = MQTTAsync_create(&mqtt->client, mqtt->address, instance.deviceName, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        log_error("Failed to create client object: %d", rc);
        free(mqtt);
        return ERR_MQTT_UNINITIALIZED;
    }

    rc = MQTTAsync_setCallbacks(mqtt->client, mqtt, connlost, messageArrived, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        log_error("Failed to set callback: %d", rc);
        free(mqtt);
        return ERR_MQTT_UNINITIALIZED;
    }

    mqtt->running         = true;
    if (NULL != cb) {
        mqtt->connectCallback = cb;
    }
    mqtt->connectContext  = context;
    if (do_mqtt_start(mqtt) != MQTTASYNC_SUCCESS) {
        increase_delay_time(mqtt);
        tm_run_after(mqtt->timer, 1000 * waitTime[mqtt->waitTime], retry_start_mqtt, mqtt);
    }
    return ERR_SUCCESS;
}

int mqtt_stop(struct MqttInstance *mqtt) {
    if (mqtt == NULL) {
        return ERR_MQTT_UNINITIALIZED;
    }
    mqtt->running = false;
    if (mqtt->connected) {
        MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
        opts.onSuccess                   = onDisconnect;
        opts.onFailure                   = onDisconnectFailure;
        opts.context                     = mqtt;
        int rc                           = MQTTAsync_disconnect(mqtt->client, &opts);
        if (rc != MQTTASYNC_SUCCESS) {
            log_error("Failed to start disconnect: %d", rc);
            return rc;
        }

        while (mqtt->connected || mqtt->connecting) {
            usleep(10000);
        }

        if (mqtt->client != NULL) {
            MQTTAsync_destroy(&mqtt->client);
            mqtt->client = NULL;
        }
    }
    return ERR_SUCCESS;
}

int mqtt_subscribe(struct MqttInstance *mqtt, const char *topic, MqttMessageCallback cb, void *context) {
    return mqtt_subscribe_ex(mqtt, topic, cb, context, NULL, NULL);
}

int mqtt_subscribe_ex(struct MqttInstance *mqtt, const char *topic, MqttMessageCallback cb, void *context, MqttFinishCallback fc, void *fcctx) {
    if (mqtt == NULL) {
        return ERR_MQTT_UNINITIALIZED;
    }
    if (!mqtt->connected) {
        log_error("failed to start subscribe because not connected");
        return ERR_MQTT_DISCONNECTED;
    }
    struct UserContext *ctx = NULL;
    if (fc != NULL) {
        ctx          = malloc(sizeof(struct UserContext));
        ctx->mqtt    = mqtt;
        ctx->cb      = fc;
        ctx->context = fcctx;
    }
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.onSuccess                 = onSubscribe;
    opts.onFailure                 = onSubscribeFailure;
    opts.context                   = ctx;
    int rc                         = MQTTAsync_subscribe(mqtt->client, topic, 1, &opts);
    if (rc != MQTTASYNC_SUCCESS) {
        log_error("Failed to start subscribe: %d", rc);
        return WRAP_MQTT_ERROR(rc);
    }
    mtx_lock(&mqtt->mutex);
    if (mqtt->topicList == NULL) {
        mqtt->topicList = TopicItem_new(topic, cb, context);
    } else {
        struct TopicItem *temp = mqtt->topicList;
        if (strcmp(temp->topic, topic) == 0) {
            if (cb == NULL) {
                mqtt->topicList = temp->next;
                free(temp->topic);
                free(temp);
            } else {
                temp->cb = cb;
            }
        } else {
            while (temp->next != NULL && strcmp(temp->next->topic, topic) != 0) {
                temp = temp->next;
            }
            if (temp->next != NULL) {
                if (cb == NULL) {
                    temp->next = TopicItem_delete(temp->next);
                } else {
                    temp->next->cb = cb;
                }
            } else {
                temp->next = TopicItem_new(topic, cb, context);
            }
        }
    }
    mtx_unlock(&mqtt->mutex);
    return ERR_SUCCESS;
}

int mqtt_unsubscribe(struct MqttInstance *mqtt, const char *topic) {
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.onSuccess                 = onUnsubscribe;
    opts.onFailure                 = onUnsubscribeFailure;
    opts.context                   = mqtt;
    int rc                         = MQTTAsync_unsubscribe(mqtt->client, topic, &opts);
    if (rc != MQTTASYNC_SUCCESS) {
        log_error("Failed to start unsubscribe: %d", rc);
    }
    return WRAP_MQTT_ERROR(rc);
}

int mqtt_publish(struct MqttInstance *mqtt, const char *topic, const char *payload) {
    return mqtt_publish_ex(mqtt, topic, payload, NULL, NULL);
}

int mqtt_publish_ex(struct MqttInstance *mqtt, const char *topic, const char *payload, MqttFinishCallback cb, void *context) {
    if (mqtt == NULL) {
        return ERR_MQTT_UNINITIALIZED;
    }
    struct UserContext *ctx = NULL;
    if (cb != NULL) {
        ctx          = malloc(sizeof(struct UserContext));
        ctx->mqtt    = mqtt;
        ctx->cb      = cb;
        ctx->context = context;
    }
    mtx_lock(&mqtt->mutex);
    if (!mqtt->connected) {
        queue_pending_msg_emplace_back(&mqtt->pendingMessages, strdup(topic), strdup(payload), ctx);
        mtx_unlock(&mqtt->mutex);
        return 1;
    }
    mtx_unlock(&mqtt->mutex);
    MQTTAsync_responseOptions opts   = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message         pubmsg = MQTTAsync_message_initializer;
    opts.onSuccess                   = onSend;
    opts.onFailure                   = onSendFailure;
    opts.context                     = ctx;
    pubmsg.payload                   = (char *)payload;
    pubmsg.payloadlen                = strlen(payload);
    pubmsg.qos                       = 1;
    pubmsg.retained                  = 0;
    int rc                           = MQTTAsync_sendMessage(mqtt->client, topic, &pubmsg, &opts);
    if (rc == MQTTASYNC_DISCONNECTED) {
        log_error("Failed to start sendMessage: MQTTASYNC_DISCONNECTED");
        mqtt->connected = false;
        if (!mqtt->running) {
            free(ctx);
            return 1;
        } else {
            queue_pending_msg_emplace_back(&mqtt->pendingMessages, strdup(topic), strdup(payload), ctx);
            mqtt_start(mqtt, NULL, NULL, NULL, NULL, NULL);
            return 1;
        }
    } else if (rc != MQTTASYNC_SUCCESS) {
        log_error("Failed to start sendMessage: %d", rc);
        free(ctx);
        return WRAP_MQTT_ERROR(rc);
    }
    return ERR_SUCCESS;
}