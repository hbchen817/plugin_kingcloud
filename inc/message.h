// #pragma once
#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include "tsl_model.h"
#include <stdint.h>

#define MSG_PROTOCOL_VERSION 1

#define SCENE_ID_LEN        64
#define PRODUCT_ID_LEN      32
#define PRODUCT_SECRET_LEN  64
#define MAC_LEN             16
#define PROPERTY_NAME_LEN   32
#define EVENT_NAME_LEN      32
#define SERVICE_NAME_LEN    32
#define DEV_ALIAS_LEN       64
#define URL_MAX_LEN         1024
#define VERSION_DESC_LEN    256
#define VERSION_NAME_LEN    32
#define VERSION_UUID_LEN    32
#define VERSION_VER_LEN     32
#define MODULE_NAME_LEN     32
#define MD5SUM_LEN          32
#define INSTALL_CODE_LEN    32
#define MODEL_ID_LEN        32
/**
 * @brief 
 * 1.若是该设备处于CONNECT_STATE_LEAVENETING时,一旦收到该设备的其他属性消息上报时，说明该设备已经醒来,则就直接踢掉该设备，此行为为延迟解绑;
 * 2.若是该设备处于CONNECT_STATE_OFFLINE时，收到用户删除该设备时，将设备的状态更改为CONNECT_STATE_LEAVENETING;
 * 3.若是该设备处于CONNECT_STATE_OFFLINE时, 收到该设备消息时，将设备的状态更改为CONNECT_STATE_ONLINE;
 * 4.若是该设备上报离网消息时,将设备状态更改为CONNECT_STATE_DELETING,然后通知给插件,若是再次从插件端收到删除该设备的消息时，再进行删除数据库中的数据。
 */
typedef enum 
{ 
    CONNECT_STATE_ONLINE = 0,//在线状态<设备连接到网关,且能够正常收到消息>
    CONNECT_STATE_OFFLINE,   //离线状态<网关没有收到设备的离网消息,但是在一定的心跳周期内却没有收到消息>
    CONNECT_STATE_LEAVENETING,//离网过程中状态,<用户主动向网关发出设备离网通知,但是该设备处于离线状态没有收到消息，此时网关将此设备更新为离网过程中的状态>
    CONNECT_STATE_DELETING,   //离网过程中状态,<设备主动离开网络，此时网关将此设备更新为离网过程中的状态>
} RexConnectState_e;

typedef struct 
{
    RexConnectState_e status;          //设备状态
    char mac[MAC_LEN + 1];             //设备mac地址
    char productId[PRODUCT_ID_LEN + 1];//设备映射的pk
    char version[VERSION_LEN + 1];     //设备版本
    RexTslModelInfo_t *tslModel;       //设备物模型信息
    char devAlias[DEV_ALIAS_LEN + 1];  //设备别名
    // char modelId[MODEL_ID_LEN + 1];    //设备modelId
    // int  heartbeat;                    //设备心跳周期,以秒为单位
} RexDev_t;

/* TODO:上游下发的命令类型 */
typedef enum
{
    CMD_PAIR = 0,        // 开启配网命令<应答消息类型对应MSG_TYPE_EXECUTE_RESULT>,若有子设备入网<上报消息类型对应MSG_TYPE_NODE_ADDED>
    CMD_ABORT_PAIRING,   // 停止配网    
    CMD_REMOVE_NODE,     // 删除节点    <上报消息类型对应MSG_TYPE_NODE_REMOVED>
    CMD_RESET_NETWORK,   // 重置网络
    CMD_SET,             // 设置节点属性<上报消息类型对应MSG_TYPE_NODE_STATE_CHANGED>
    CMD_SERVICE,         // 服务
    CMD_CONFIG_HB,       // 配置子设备心跳间隔
    CMD_SCENE_ADD,       // 增加场景
    CMD_SCENE_DEL,       // 删除场景
    CMD_SCENE_UPDATE,    // 更新场景
    CMD_SCENE_TRIGGER,   // 触发手动场景
    CMD_OTA_NOTIFY,      // OTA升级 通知
    CMD_GROUP_ADD,       // 增加组
    CMD_GROUP_DEL,       // 删除组
    CMD_GROUP_GET,       // 获取组信息
} RexCommand_e;

typedef struct 
{
    int         charNum;
    char       *str;
} RexString_t;

typedef struct 
{
    uint8_t     intNum;
    int        *intList;
} RexIntList_t;

typedef struct 
{
    uint8_t      floatNum;
    double      *floatList;
} RexFloatList_t;

typedef struct 
{
    uint8_t      strNum;
    char       **strList;
} RexStringList_t;

typedef struct
{
    RexTslDataType_e       type;
    union
    {
        int                boolVal;
        int32_t            intVal;
        float              floatVal;
        double             doubleVal;
        RexString_t        strVal;
        RexIntList_t       intListVal;
        RexFloatList_t     floatListVal;
        RexStringList_t    strListVal;
    }u;
} RexValue_t;

typedef struct 
{
    char        name[PROPERTY_NAME_LEN + 1];
    RexValue_t  value;
} RexProperty_t;

typedef struct
{
    char        name[PROPERTY_NAME_LEN + 1];
    RexValue_t  value;
}RexParam_t;

typedef struct 
{
    char        name[EVENT_NAME_LEN + 1];     // 网关物模型定义的事件唯一标识identify
    int         paramNum;
    RexParam_t *paramList;

} RexEvent_t;

typedef struct 
{
    char        name[SERVICE_NAME_LEN + 1];     // 网关物模型定义的事件唯一标识identify
    int         paramNum;
    RexParam_t *paramList;

} RexService_t;

typedef enum
{
    PERMIT_TYPE_ALL = 0,       //允许所有设备入网
    PERMIT_TYPE_MAC,           //允许指定设备MAC入网
    PERMIT_TYPE_MAC_LIST,      //白/黑名单入网，允许指定设备MAC列表入网
} RexPermitType_t;

typedef struct
{
    char mac[MAC_LEN + 1];
} RexMacInfo_t;

/* 1. 配网命令 */
typedef struct 
{
    int             duration;                     // 持续时长
    char            productId[PRODUCT_ID_LEN + 1];// 设备型号
    RexPermitType_t type;                         // 允许入网的类型
    int             macNum;                       //  MAC个数
    RexMacInfo_t   *macList;                      //  MAC列表
    char            installCode[INSTALL_CODE_LEN + 1];//Install Code
} RexPairCommand_t;

/* 2. 删除节点命令 */
typedef struct 
{
    char    mac[MAC_LEN + 1];    
} RexRemoveNodeCommand_t;

/* 3.配置心跳命令 */
typedef struct {
    char    mac[MAC_LEN + 1];
    int16_t interval;//单位为秒
} RexConfigHbCommand_t;


/* 4. 设置设备属性命令 
约定如下：
(1)若是用户希望发送广播命令:则mac为"FFFFFFFFFFFFFFFF",groupName为空,productId不能为空。
(2)若是用户希望发送组播命令:则mac为"FFFFFFFFFFFFFFFF",groupName不能为空。
(3)若是用户希望发送单播命令:则mac不能为空,groupName为空。
*/
typedef struct
{
    char           mac[MAC_LEN + 1];
    char           productId[PRODUCT_ID_LEN + 1];   
    RexProperty_t  property;
    char           groupName[MAC_LEN + 1];
} RexSetPropertyCommand_t;

/* 5. 场景命令 */
typedef struct
{
    char     sceneId[SCENE_ID_LEN + 1];   // 场景Id
    uint8_t  sceneState;                  /** 禁用启用(1:启用，0:禁用) */
    char    *sceneConfig;                 // 场景规则配置
    int      sceneConfigLen;              // 场景规则配置长度
} RexScene_t;

typedef struct
{
    int         sceneNum;
    RexScene_t *sceneList;   
} RexAddSceneCommand_t;

typedef struct  
{
    int         sceneNum;
    RexScene_t *sceneList; 
} RexUpdateSceneCommand_t;

typedef struct 
{
    int         sceneNum;
    RexScene_t *sceneList; 
} RexDelSceneCommand_t;

typedef struct 
{
    int         sceneNum;
    RexScene_t *sceneList; 
} RexTriggerSceneCommand_t;

typedef struct 
{
    char mac[MAC_LEN + 1];//网关mac地址
} RexGetDevListCommand_t;

/**
 * @brief 发送服务命令
 * 约定如下：
(1)若是用户希望发送广播命令:则mac为"FFFFFFFFFFFFFFFF",groupName为空,productId不能为空。
(2)若是用户希望发送组播命令:则mac为"FFFFFFFFFFFFFFFF",groupName不能为空。
(3)若是用户希望发送单播命令:则mac不能为空,groupName为空。
 */
typedef struct 
{
    char mac[MAC_LEN + 1];
    char productId[PRODUCT_ID_LEN + 1];
    RexService_t service;  
    char groupName[MAC_LEN + 1];       //  组名称
} RexServiceCommand_t;

typedef struct
{
    uint64_t size;
    char version[VERSION_LEN + 1];
    char url[URL_MAX_LEN + 1];
    char md5[MD5SUM_LEN + 1];
    char module[MODULE_NAME_LEN + 1];
    char mac[MAC_LEN + 1];
    char unused;
}RexOTAUpgradeCommand_t;

/**创建组的命令*/
typedef struct 
{
    char          groupName[MAC_LEN + 1];       //  组名称
    char          productId[PRODUCT_ID_LEN + 1];//  组内部必须是相同的品类
    int           macNum;                       //  AC个数
    RexMacInfo_t *macList;                      //  MAC列表
}RexDevGroup_t;
typedef struct 
{
    RexDevGroup_t addGroup;
}RexAddGroupCommand_t;

typedef struct 
{
    RexDevGroup_t delGroup;
}RexDelGroupCommand_t;

typedef struct
{
    char groupName[MAC_LEN + 1]; //组名称
}RexGetGroupCommand_t;

typedef struct 
{
    int sequence;
    RexCommand_e     type;     // 命令类型
    union {
        RexPairCommand_t         pairCommand;         //配网命令
        RexRemoveNodeCommand_t   removeNodeCommand;   //解绑子设备命令
        RexSetPropertyCommand_t  setPropertyCommand;  //设置子设备属性命令
        RexAddSceneCommand_t     addSceneCommand;     //增加场景命令
        RexUpdateSceneCommand_t  updateSceneCommand;  //更新场景命令 
        RexDelSceneCommand_t     delSceneCommand;     //删除场景命令
        RexTriggerSceneCommand_t triggerSceneCommand; //触发场景命令
        RexConfigHbCommand_t     configHbCommand;     //配置子设备心跳周期命令
        RexGetDevListCommand_t   getDevListCommand;   //获取设备列表命令
        RexServiceCommand_t      serviceCommand;      //服务
        RexOTAUpgradeCommand_t   otaCommand;          //OTA
        RexAddGroupCommand_t     addGroupCommand;     //增加组
        RexDelGroupCommand_t     delGroupCommand;     //删除组
        RexGetGroupCommand_t     getGroupCommand;     //获取组信息
    } u;

} RexCommand_t;

/* TODO:下游网关发送的消息类型 */
typedef enum
{
    MSG_TYPE_SDK_READY = 0,     
    MSG_TYPE_PAIR_FINISHED,     // 网关配网停止
    MSG_TYPE_SCENE_TRIGGERED,   // 场景触发
    MSG_TYPE_NODE_ADDED,        // 新节点入网
    MSG_TYPE_NODE_REMOVED,      // 节点离网
    MSG_TYPE_NODE_CONN_STATE,   // 节点连接状态变更
    MSG_TYPE_NODE_HEARTBEAT,    // 节点心跳消息
    MSG_TYPE_NODE_EVENT,        // 节点事件
    MSG_TYPE_NODE_STATE_CHANGED,// 节点属性状态变更
    MSG_TYPE_OTA_EVENT,
    MSG_TYPE_SERVICE,           //服务类型
    MSG_TYPE_EXECUTE_RESULT,    //执行结果
    MSG_TYPE_USER_DEFINED,      //用户自定义消息
    MSG_TYPE_GROUP_ADD,         //用户发送增加组命令后的结果消息通知
    MSG_TYPE_GROUP_GET,         //用户发送查询组命令后的结果消息通知
    MSG_TYPE_GROUP_DEL,         //用户发送删除组命令后的结果消息通知
    MSG_TYPE_OTA_STATUS,        //OTA状态通知
    MSG_TYPE_NODE_ADDED_WITH_LICENSE,//带有license的子设备入网消息
    MSG_TYPE_END,
} RexMsgType_e;


/* 1. 子设备入网消息 */
typedef struct 
{
    char        mac[MAC_LEN + 1];             //子设备mac地址
    char        productId[PRODUCT_ID_LEN + 1];//子设备品类id,属于网关定义的物模型中的品类id
    char        modelId[MODEL_ID_LEN + 1];    //子设备modelid
    char        unused;
} RexIncludedMsg_t;

/**
 * @brief 新增带有lisence的设备入网
 */
typedef struct 
{
    char        mac[MAC_LEN + 1];
    char        productId[PRODUCT_ID_LEN + 1];
    char        platformProductId[64 + 1];
    char        platformDeviceId[64 + 1];
    char        platformDroductName[64 + 1];
} RexLicenseIncludedMsg_t;

/* 2. 子设备离网消息 */
typedef struct 
{
    char        mac[MAC_LEN + 1];
} RexExcludedMsg_t;

/* 3. 子设备设备状态消息 */
typedef struct 
{
    char               mac[MAC_LEN + 1];
    char               productId[PRODUCT_ID_LEN + 1];
    int                propertyNum;
    RexProperty_t     *propertyList;
} RexPropertyMsg_t;

/* 4. 子设备事件消息 */
typedef struct 
{
    char         mac[MAC_LEN + 1];
    char         productId[PRODUCT_ID_LEN + 1];
    RexEvent_t  *event;
} RexEventMsg_t;

/* 5. 服务消息 */
typedef struct 
{
    char          mac[MAC_LEN + 1];
    char          productId[PRODUCT_ID_LEN + 1];
    RexService_t *service;

} RexServiceMsg_t;

/* 6. 子设备连接状态变更消息 */
typedef struct 
{
    char               mac[MAC_LEN + 1];
    RexConnectState_e  state;
} RexConnStateMsg_t;

/* 7. 子设备心跳消息 */
typedef struct 
{
    char            mac[MAC_LEN + 1];
} RexHeartbeatMsg_t;

/* 定义场景执行动作类型*/
typedef enum
{
    SceneAction_TRIGGER  = 0,
    SceneAction_ADD      = 1,
    SceneAction_UPDATE   = 2,
    SceneAction_DEL      = 3
} RexSceneActionType_e;

/* 8. 场景消息 */
typedef struct 
{
    char                 sceneId[SCENE_ID_LEN + 1];
    uint8_t              status;//场景执行结果
    RexSceneActionType_e action;//场景执行动作(增加,删除,更新,手动触发)
} RexSceneTriggerMsg_t;

typedef enum
{
    RET_COO = 0,
    RET_SDK,
    RET_GWAPP 
}RexResultType_e;

typedef enum
{
    SUCCESS = 0,             //成功
    FAILURE,                 //失败
    INVALID_ARG,             //无效参数
    MEMORY_ALLOC_ERROR= 0x05,//分配内存失败
    NO_NETWORK        = 0x0E,//没有网络
    ALREADY_JOINED    = 0x0F,//已存在网络
    BUSY_ERROR        = 0x0A,//资源繁忙
    NOT_AUTHORIZED    = 0x7E,//没有被认证
    MALFORMED_CMD     = 0x80,//畸形的命令,即接收的命令格式不正确
    UNSUP_CMD,               //不支持的命令
    NOT_FOUND         = 0x8B,//没有找到
    OVERTIME          = 0x94,//超时
    ABORT             = 0x95,//客户端或服务器决定中止升级过程时失败。
    INVALID_IMAGE,           //无效的OTA升级镜像
    WAIT_FOR_DATA,           //服务器还没有可用的数据
    NO_IMAGE_AVAILABLE,      //没有可用于特定客户端的OTA升级映像
    REQUIRE_MORE_IMAGE       //客户端仍然需要更多的OTA升级映像文件才能成功升级
}RexResult_e;

/* 9. 命令执行结果消息*/
typedef struct 
{
    RexResult_e result;
}RexCmdExecuteResultMsg_t;

typedef struct
{
    int step;
}RexOtaProgressMsg_t;

/*10. 用户自定义消息*/
typedef struct
{
    int   length;
    char *message;
}RexUserDefinedMsg_t;

typedef enum {
    REX_DEV_GROUP_JOINED   = 0,//已加入组的状态
    REX_DEV_GROUP_JOINING  = 1,//正在加入组的状态
    REX_DEV_GROUP_DELETING = 2,//正在删除组的状态
}RexDevJoinGroupStatus_e;

/*11. 增加组结果消息*/
typedef struct 
{
    RexDevGroup_t result;
}RexAddGroupResultMsg_t;

/** 删除组结果消息*/
typedef struct 
{
    RexDevGroup_t result;
}RexDelGroupResultMsg_t;

typedef struct
{
    char     mac[MAC_LEN + 1];//设备mac地址
    uint8_t  status;          //设备加入组的状态(三种状态,0:表示已加入组状态,1:正在加入组状态,2:正在删除组状态)
}RexDevGroupStatusInfo_t;

/*12. 获取组信息结果消息*/
typedef struct 
{
    char                     groupName[MAC_LEN + 1];       //组名
    char                     productId[PRODUCT_ID_LEN + 1];//品类
    int                      count;                        //组中的设备个数
    RexDevGroupStatusInfo_t *status;                       //组中的设备状态
}RexDevGroupInfo_t;

typedef struct 
{
    int count;                   //组个数
    RexDevGroupInfo_t *groupList;//组列表
}RexGetGroupInfoMsg_t;

typedef struct RexMessage 
{
    int sequence;
    RexMsgType_e      type;     // 消息类型
    union 
    {
        RexIncludedMsg_t         includedMsg;  //子设备入网消息
        RexLicenseIncludedMsg_t  licenseMsg;  //带有license子设备的入网消息
        RexExcludedMsg_t         excludedMsg;  //子设备离网消息
        RexPropertyMsg_t         propertyMsg;  //子设备属性上报消息
        RexEventMsg_t            eventMsg;     //子设备事件消息
        RexConnStateMsg_t        connStateMsg; //子设备在离线变更消息
        RexHeartbeatMsg_t        heartbeatMsg; 
        RexSceneTriggerMsg_t     sceneTriggerMsg;//场景消息
        RexServiceMsg_t          serviceMsg;     //服务执行结果
        RexCmdExecuteResultMsg_t resultMsg;    //命令执行结果
        RexOtaProgressMsg_t      otaProgressMsg;//升级进度
        RexUserDefinedMsg_t      userDefinedMsg;//自定义消息
        RexAddGroupResultMsg_t   addGroupResultMsg;//用户发送增加组命令后的结果消息通知
        RexGetGroupInfoMsg_t     getGroupInfoMsg;  //用户发送查询组命令后的结果消息通知
        RexDelGroupResultMsg_t   delGroupResultMsg;//用户发送删除组命令后的结果消息通知
    } u;

} RexMessage_t;

typedef enum
{
    DEV_TYPE_COO = 0,
    DEV_TYPE_WIRELESS_SDK,
    DEV_TYPE_GW_APP,
    DEV_TYPE_PLUGIN,
    DEV_TYPE_SYSTEM//openwrt系统固件
}RexDevType_t;

typedef struct 
{
    RexDevType_t type;
    char name[VERSION_NAME_LEN + 1];//名称
    char desc[VERSION_DESC_LEN + 1];//描述信息
    char ver[VERSION_VER_LEN + 1];  //版本
    char uuid[VERSION_UUID_LEN + 1];//标识
    int  enabled;           //当前状态，启用则置为1，否则为0
}RexVersionInfo_t;

#endif//__MESSAGE_H__