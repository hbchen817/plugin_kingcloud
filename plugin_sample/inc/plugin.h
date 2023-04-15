#pragma once
#include "message.h"

#define DLL_EXPORT __attribute__ ((visibility("default")))

#define PLUGIN_PROTOCOL_VERSION 1
#define PLUGIN_DESC_LEN         32
#define PLUGIN_NAME_LEN         32
#define PLUGIN_UUID_LEN         32
#define PLUGIN_VER_LEN          32

typedef int (*ProcessCmd)(void *, RexCommand_t *);
typedef int (*GetDevList)(void *, int *size, RexDev_t **devList);//after used devList,need user free by self
typedef int (*GetZbNetInfo)(void *,char *netInfoBuffer,int bufferLen);
typedef int (*ReadConfigure)(void *, const char *key, void *buffer, int bufferLen); 
typedef int (*WriteConfigure)(void *, const char *key, const void *buffer, int bufferLen); 
typedef int (*Log)(void *, int level, char * format,...);
typedef int (*SetDevAlias)(void *, const char *mac, const char *alias);
typedef int (*GetVerInfo)(void *, int *size, RexVersionInfo_t **verInfoList);
typedef int (*SetDevList)(void *, int size, RexDev_t *devList);
typedef void (*GetDataCallBack)(void *, void *responseData);

typedef struct {
    int            version;      // 协议版本
    ProcessCmd     processCmd;   // 处理命令
    GetDevList     getDevList;   // 获取设备列表
    ReadConfigure  readConf;     // 读配置
    WriteConfigure writeConf;    // 写配置
    Log            log;          // 写日志
    GetZbNetInfo   getZbNetInfo; // 获取网络信息
    SetDevAlias    setDevAlias;  // 设置设备别名
    GetVerInfo     getVerInfo;   // 获取网关内部所有相关依赖的版本信息
    SetDevList     setDevList;   // 云端将设备列表同步给网关
    void          *endptr;       // 尾部指针赋值为NULL
} RexInitParameter_t;

typedef struct {
    char name[PLUGIN_NAME_LEN + 1]; // 插件名称
    char desc[PLUGIN_DESC_LEN + 1]; // 插件描述信息
    char ver[PLUGIN_VER_LEN + 1];   // 插件版本
    char uuid[PLUGIN_UUID_LEN + 1]; // 插件标识
} RexPluginInfo_t;

DLL_EXPORT int rex_init(RexInitParameter_t *parameter,void *ctx);
DLL_EXPORT void rex_deinit();
DLL_EXPORT int rex_start();
DLL_EXPORT int rex_stop();
DLL_EXPORT int rex_notify(RexMessage_t *msg);
DLL_EXPORT const RexPluginInfo_t * rex_get_info();
DLL_EXPORT int rex_get_data(void *ctx, const char *requestType, const void *requestData, GetDataCallBack getDataCb);

#define REQUEST_TYPE_GET_CONF         "GETPLUGINCONF"
#define REQUEST_TYPE_POST_CONF        "POSTPLUGINCONF"
