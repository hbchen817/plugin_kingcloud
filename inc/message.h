#pragma once

#include "tsl_model.h"
#include <stdint.h>

#define MSG_PROTOCOL_VERSION 1

#define SCENE_ID_LEN       64
#define PRODUCT_ID_LEN     32
#define PRODUCT_SECRET_LEN 64
#define MAC_LEN            16
#define PROPERTY_NAME_LEN  32
#define EVENT_NAME_LEN     32
#define SERVICE_NAME_LEN   32
#define DEV_ALIAS_LEN      64
#define URL_MAX_LEN        1024
#define VERSION_DESC_LEN   256
#define VERSION_NAME_LEN   32
#define VERSION_UUID_LEN   32
#define VERSION_VER_LEN    32
#define MODULE_NAME_LEN    32
#define MD5SUM_LEN         32

typedef enum {
    CONNECT_STATE_ONLINE = 0, // 在线状态：设备连接到网关，且能够正常收到消息
    CONNECT_STATE_OFFLINE,    // 离线状态：网关没有收到设备的离网消息，但是在一定的心跳周期内却没有收到消息
    CONNECT_STATE_LEAVING,    // 离网过程中：用户主动向网关发出设备离网通知，但是该设备处于离线状态或正在处理过程中
    CONNECT_STATE_DELETING    // 离网过程中：设备主动离开网络，此时网关将此设备更新为离网过程中的状态
} RexConnectState_e;

typedef struct {
    RexConnectState_e  status; // 子设备的状态
    char               mac[MAC_LEN + 1];
    char               productId[PRODUCT_ID_LEN + 1];
    char               version[VERSION_LEN + 1];
    RexTslModelInfo_t *tslModel;                    // 设备物模型信息
    char               devAlias[DEV_ALIAS_LEN + 1]; // 设备别名
} RexDev_t;

/* TODO:上游下发的命令类型 */
typedef enum {
    CMD_PAIR = 0,      // 开启扫网
    CMD_ABORT_PAIRING, // 停止扫网
    CMD_REMOVE_NODE,   // 删除子设备
    CMD_RESET_NETWORK, // 重置网络(未实现)
    CMD_SET,           // 设置子设备属性
    CMD_SERVICE,       // 服务
    CMD_CONFIG_HB,     // 配置子设备心跳间隔
    CMD_SCENE_ADD,     // 增加场景
    CMD_SCENE_DEL,     // 删除场景
    CMD_SCENE_UPDATE,  // 更新场景
    CMD_SCENE_TRIGGER, // 触发手动场景
    CMD_OTA_NOTIFY,
    CMD_MAX
} RexCommand_e;

typedef struct {
    int   charNum;
    char *str;
} RexString_t;

typedef struct {
    uint8_t intNum;
    int    *intList;
} RexIntList_t;

typedef struct {
    uint8_t floatNum;
    double *floatList;
} RexFloatList_t;

typedef struct {
    uint8_t strNum;
    char  **strList;
} RexStringList_t;

typedef struct {
    RexTslDataType_e type;
    union {
        int             boolVal;
        int32_t         intVal;
        float           floatVal;
        double          doubleVal;
        RexString_t     strVal;
        RexIntList_t    intListVal;
        RexFloatList_t  floatListVal;
        RexStringList_t strListVal;
    } u;
} RexValue_t;

typedef struct {
    char       name[PROPERTY_NAME_LEN + 1];
    RexValue_t value;
} RexProperty_t;

typedef struct {
    char       name[PROPERTY_NAME_LEN + 1];
    RexValue_t value;
} RexParam_t;

typedef struct {
    char        name[EVENT_NAME_LEN + 1]; // 网关物模型定义的事件唯一标识identify
    int         paramNum;
    RexParam_t *paramList;
} RexEvent_t;

typedef struct {
    char        name[SERVICE_NAME_LEN + 1]; // 网关物模型定义的事件唯一标识identify
    int         paramNum;
    RexParam_t *paramList;
} RexService_t;

typedef enum {
    PERMIT_TYPE_ALL = 0,      // 允许所有设备入网
    PERMIT_TYPE_INSTALL_CODE, // 允许通过 install code 入网
    PERMIT_TYPE_WHITE_LIST    // 允许白名单入网
} RexPermitType_t;

typedef struct {
    char mac[MAC_LEN + 1];
} RexMacInfo_t;

/* 1. 配网命令 */
typedef struct {
    int             duration;                      // 持续时长
    char            productId[PRODUCT_ID_LEN + 1]; // 设备型号
    RexPermitType_t type;                          // 允许入网类型
    int             macNum;                        // MAC 个数
    RexMacInfo_t   *macList;                       // MAC 列表
    uint8_t         installCode[16];               // Install Code
} RexPairCommand_t;

/* 2. 删除节点命令 */
typedef struct {
    char mac[MAC_LEN + 1];
} RexRemoveNodeCommand_t;

/* 3.配置心跳命令 */
typedef struct {
    char    mac[MAC_LEN + 1];
    int16_t interval; // 单位为秒
} RexConfigHbCommand_t;

/* 4. 设置设备属性命令 */
typedef struct {
    char          mac[MAC_LEN + 1];
    char          productId[PRODUCT_ID_LEN + 1];
    RexProperty_t property;
} RexSetPropertyCommand_t;

/* 5. 场景命令 */
typedef struct {
    char    sceneId[SCENE_ID_LEN + 1]; // 场景Id
    uint8_t sceneState;                // 场景状态/*更新场景状态时使用有效*/
    char   *sceneConfig;               // 场景规则配置
    int     sceneConfigLen;            // 场景规则配置长度
} RexScene_t;

typedef struct {
    int         sceneNum;
    RexScene_t *sceneList;
} RexAddSceneCommand_t;

typedef struct {
    int         sceneNum;
    RexScene_t *sceneList;
} RexUpdateSceneCommand_t;

typedef struct {
    int         sceneNum;
    RexScene_t *sceneList;
} RexDelSceneCommand_t;

typedef struct {
    int         sceneNum;
    RexScene_t *sceneList;
} RexTriggerSceneCommand_t;

typedef struct {
    char mac[MAC_LEN + 1];
} RexGetDevListCommand_t;

typedef struct {
    char         mac[MAC_LEN + 1];
    char         productId[PRODUCT_ID_LEN + 1];
    RexService_t service;
} RexServiceCommand_t;

typedef struct {
    uint64_t size;
    char     version[VERSION_LEN + 1];
    char     url[URL_MAX_LEN + 1];
    char     md5[MD5SUM_LEN + 1];
    char     module[MODULE_NAME_LEN + 1];
    char     mac[MAC_LEN + 1];
    char     unused;
} RexOTAUpgradeCommand_t;

typedef struct {
    int          sequence;
    RexCommand_e type; // 命令类型
    union {
        RexPairCommand_t         pairCommand;         // 配网命令
        RexRemoveNodeCommand_t   removeNodeCommand;   // 解绑子设备命令
        RexSetPropertyCommand_t  setPropertyCommand;  // 设置子设备属性命令
        RexAddSceneCommand_t     addSceneCommand;     // 增加场景命令
        RexUpdateSceneCommand_t  updateSceneCommand;  // 更新场景命令
        RexDelSceneCommand_t     delSceneCommand;     // 删除场景命令
        RexTriggerSceneCommand_t triggerSceneCommand; // 触发场景命令
        RexConfigHbCommand_t     configHbCommand;     // 配置子设备心跳周期命令
        RexGetDevListCommand_t   getDevListCommand;   // 获取设备列表命令
        RexServiceCommand_t      serviceCommand;      // 服务
        RexOTAUpgradeCommand_t   otaCommand;
    } u;
} RexCommand_t;

/* TODO:下游网关发送的消息类型 */
typedef enum {
    MSG_TYPE_SDK_READY = 0,
    MSG_TYPE_PAIR_FINISHED,      // 网关配网停止
    MSG_TYPE_SCENE_TRIGGERED,    // 场景触发
    MSG_TYPE_NODE_ADDED,         // 新节点入网
    MSG_TYPE_NODE_REMOVED,       // 节点离网
    MSG_TYPE_NODE_CONN_STATE,    // 节点连接状态变更
    MSG_TYPE_NODE_HEARTBEAT,     // 节点心跳消息
    MSG_TYPE_NODE_EVENT,         // 节点事件
    MSG_TYPE_NODE_STATE_CHANGED, // 节点属性状态变更
    MSG_TYPE_OTA_EVENT,
    MSG_TYPE_SERVICE,        // 服务类型
    MSG_TYPE_EXECUTE_RESULT, // 执行结果
    MSG_TYPE_USER_DEFINED,   // 用户自定义消息
    MSG_TYPE_MAX
} RexMsgType_e;

/* 1. 子设备入网消息 */
typedef struct {
    char mac[MAC_LEN + 1];
    char productId[PRODUCT_ID_LEN + 1];
} RexIncludedMsg_t;

/* 2. 子设备离网消息 */
typedef struct {
    char mac[MAC_LEN + 1];
} RexExcludedMsg_t;

/* 3. 子设备设备状态消息 */
typedef struct {
    char           mac[MAC_LEN + 1];
    char           productId[PRODUCT_ID_LEN + 1];
    int            propertyNum;
    RexProperty_t *propertyList;
} RexPropertyMsg_t;

/* 4. 子设备事件消息 */
typedef struct {
    char        mac[MAC_LEN + 1];
    char        productId[PRODUCT_ID_LEN + 1];
    RexEvent_t *event;
} RexEventMsg_t;

/* 5. 服务消息 */
typedef struct {
    char          mac[MAC_LEN + 1];
    char          productId[PRODUCT_ID_LEN + 1];
    RexService_t *service;
} RexServiceMsg_t;

/* 6. 子设备连接状态变更消息 */
typedef struct {
    char              mac[MAC_LEN + 1];
    RexConnectState_e state;
} RexConnStateMsg_t;

/* 7. 子设备心跳消息 */
typedef struct {
    char mac[MAC_LEN + 1];
} RexHeartbeatMsg_t;

/* 定义场景执行动作类型*/
typedef enum {
    SceneAction_TRIGGER = 0,
    SceneAction_ADD     = 1,
    SceneAction_UPDATE  = 2,
    SceneAction_DEL     = 3,
    SceneAction_MAX,
} RexSceneActionType_e;

/* 8. 场景消息 */
typedef struct {
    char                 sceneId[SCENE_ID_LEN + 1];
    uint8_t              status; // 场景执行结果
    RexSceneActionType_e action; // 场景执行动作(增加,删除,更新,手动触发)
} RexSceneTriggerMsg_t;

typedef enum { RET_COO = 0, RET_SDK, RET_GWAPP } RexResultType_e;

typedef enum {
    SUCCESS = 0,       // 成功
    FAILURE,           // 失败
    INVALID_ARG,       // 无效参数
    ALREADY_JOINED,    // 已存在网络
    OVERTIME,          // 命令超时
    MEMORY_ALLOC_ERROR // 分配内存失败
} RexResult_e;

// 9. 命令执行结果消息
typedef struct {
    RexResult_e result;
} RexCmdExecuteResultMsg_t;

typedef struct {
    int step;
} RexOtaProgressMsg_t;

// 10. 用户自定义消息
typedef struct {
    int   length;
    char *message;
} RexUserDefinedMsg_t;

typedef struct RexMessage {
    int          sequence;
    RexMsgType_e type; // 消息类型
    union {
        RexIncludedMsg_t         includedMsg;  // 子设备入网消息
        RexExcludedMsg_t         excludedMsg;  // 子设备离网消息
        RexPropertyMsg_t         propertyMsg;  // 子设备属性上报消息
        RexEventMsg_t            eventMsg;     // 子设备事件消息
        RexConnStateMsg_t        connStateMsg; // 子设备在离线变更消息
        RexHeartbeatMsg_t        heartbeatMsg;
        RexSceneTriggerMsg_t     sceneTriggerMsg; // 场景消息
        RexServiceMsg_t          serviceMsg;      // 服务执行结果
        RexCmdExecuteResultMsg_t resultMsg;       // 命令执行结果
        RexOtaProgressMsg_t      otaProgressMsg;  // 升级进度
        RexUserDefinedMsg_t      userDefinedMsg;  // 自定义消息
    } u;
} RexMessage_t;

typedef enum {
    DEV_TYPE_COO = 0,
    DEV_TYPE_WIRELESS_SDK,
    DEV_TYPE_GW_APP,
    DEV_TYPE_PLUGIN,
} RexDevType_t;

typedef struct {
    RexDevType_t type;
    char         name[VERSION_NAME_LEN + 1]; // 名称
    char         desc[VERSION_DESC_LEN + 1]; // 描述信息
    char         ver[VERSION_VER_LEN + 1];   // 版本
    char         uuid[VERSION_UUID_LEN + 1]; // 标识
    int          enabled;                    // 当前状态，启用则置为1，否则为0
} RexVersionInfo_t;
