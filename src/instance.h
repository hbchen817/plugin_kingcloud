#pragma once
#include "config.h"
#include "deque.h"
#include "map.h"
#include "plugin.h"
#include "timer_manager.h"
#include "tsl_model.h"
#include <stdatomic.h>
#include <threads.h>
#include "any.h"
#include "callbacks.h"


#define SYS_INTERFACE_NAME "eth0.2"

typedef void (*Handler)(void *context);

DECLARE_QUEUE(timer, int /* fd */)
DECLARE_QUEUE(transaction, int /* id */, Handler /* cb */, void * /* ctx */)
DECLARE_MAP(device, char *, const RexTslModelInfo_t * /* tsl */, const char * /* secret*/)
DECLARE_MAP(request, int /* seq */, service_response /* cb */, map_any * /* params */);

struct MqttInstance;

struct DeviceList {
    int       count;
    RexDev_t *devices;
};

struct Instance {
    ProcessCmd           processCmd;
    GetDevList           getDevList;
    ReadConfigure        readConf;
    WriteConfigure       writeConf;
    Log                  log;
    GetZbNetInfo         getZbNetInfo;
    SetDevAlias          setDevAlias;
    GetVerInfo           getVerInfo;
    SetDevList           setDevList;
    RexPluginInfo_t      info;
    void                *context;
    char                 clientId[32];
    char                 modelCode[32];
    char                 vendorCode[32];
    char                 hexModelId[32];
    char                 productKey[32];
    char                 productSecret[40];
    char                 deviceName[32];
    char                 deviceSecret[40];
    char                 macCOO[32];
    char                 versionCOO[VERSION_LEN];
    char                 mqttBroker[256];
    char                 mqttUsername[32];
    char                 mqttPassword[32];
    char                 messageUpTopic[128];
    char                 commandDownTopic[128];
    char                 commandAckTopic[128];
    char                 otaAckTopic[128];
    struct DeviceList    devList;
    atomic_int           sequence;
    struct MqttInstance *mqtt;
    map_device          *devices;
    map_request         *requests;
    mtx_t                mtxDevices;
    timer_manager_t      timerManager;
    queue_timer          activeTimers;
    queue_timer          timerPool;
    atomic_int           transId;
    queue_transaction    transactions;
};

extern struct Instance instance;

void instance_init(RexInitParameter_t *parameter, void *ctx);
void instance_deinit();
int  instance_require_timer();
void instance_release_timer(int fd);

void add_device(const char *mac, const RexTslModelInfo_t *tsl, const char *secret);
void remove_device(const char *mac);

int register_gateway();

const char *get_gateway_product_key();
const char *get_gateway_device_name();
int         get_sequence();

int log_error(const char *fmt, ...);
int log_warn(const char *fmt, ...);
int log_info(const char *fmt, ...);
int log_debug(const char *fmt, ...);
int log_verbose(const char *fmt, ...);
