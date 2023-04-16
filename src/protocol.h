#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#include "cson.h"
#include "instance.h"

/**
 * 宏定义
*/
#define KC_PROTOCOL_VERSION             "2.0.0"
#define KC_FLOW_DIRECTION_DOWN          "0"
#define KC_FLOW_DIRECTION_UP            "1"
#define KC_CONTROL_TYPE_UNBIND          "unbind"
#define KC_CONTROL_TYPE_OTA             "ota"
#define KC_CONTROL_TYPE_CONTROL_FUNC    "control.func"
#define KC_CONTROL_TYPE_CONTROL_PROP    "control.prop"
#define KC_CONTROL_TYPE_CONTROL_SCENE   "control.scene"
#define KC_CONTROL_TYPE_QUERY_PROP      "query.prop"
#define KC_CONTROL_TYPE_SCENE_CONFIG    "scene.config"
#define KC_MSG_TYPE_SCENE               "scene"
#define KC_MSG_TYPE_CONTROL_PROP        "control.prop"
#define KC_MSG_TYPE_EVENT               "event"
#define KC_MSG_TYPE_OTA                 "ota"
#define KC_MSG_TYPE_ONLINE              "online"
#define KC_MSG_TYPE_OFFLINE             "offline"
#define KC_MSG_TYPE_UNBIND              "unbind"
#define KC_MSG_TYPE_BIND                "bind"
#define KC_MSG_TYPE_HEARTBEAT           "heartbeat"
#define KC_MSG_TYPE_DISCOVERY           "discovery"
#define KC_MSG_TYPE_ACK                 "ack"

/**
 * 设备注册相关结构体
*/
typedef struct {
    char*       clientId;
    char*       vendorCode;
} cipher_text;

reflect_item_t cipher_text_ref[] = {
    _property_string(cipher_text, clientId),
    _property_string(cipher_text, vendorCode),
    _property_end()
};

typedef struct {
    char*       cipherText;
    char*       productKey;
} device_reg_request;

reflect_item_t device_reg_request_ref[] = {
    _property_string(device_reg_request, cipherText),
    _property_string(device_reg_request, productKey),
    _property_end()
};

typedef struct {
    int         code;
    char*       data;
    char*       message;
    char*       traceId;
} device_reg_response;

reflect_item_t device_reg_response_ref[] = {
    _property_int(device_reg_response, code),
    _property_string(device_reg_response, data),
    _property_string(device_reg_response, message),
    _property_string(device_reg_response, traceId),
    _property_end()
};

typedef struct {
    char*       deviceKey;
    char*       deviceSecret;
    char*       clientId;
} reg_response_data;

reflect_item_t reg_response_data_ref[] = {
    _property_string(reg_response_data, deviceKey),
    _property_string(reg_response_data, deviceSecret),
    _property_string(reg_response_data, clientId),
    _property_end()
};

/**
 * 金山云MQTT通信相关json定义
*/
/*******************************BEGIN: 更改broker地址报文********************************/
typedef struct {
    char*       authServerAddr;
    char*       mqttAddr;
    char*       mqttPort;
} cmd_change_server_value;

reflect_item_t cmd_change_server_value_ref[] = {
    _property_string(cmd_change_server_value, authServerAddr),
    _property_string(cmd_change_server_value, mqttAddr),
    _property_string(cmd_change_server_value, mqttPort),
    _property_end()
};

typedef struct {
    char*       model;
    char*       parentDeviceId;
    char*       deviceId;
    char*       code;
    char*       value;
} cmd_change_server_data;

reflect_item_t cmd_change_server_data_ref[] = {
    _property_string(cmd_change_server_data, model),
    _property_string(cmd_change_server_data, parentDeviceId),
    _property_string(cmd_change_server_data, deviceId),
    _property_string(cmd_change_server_data, code),
    _property_string(cmd_change_server_data, value),
    _property_end()
};

typedef struct {
    char*                       version;
    char*                       flowDirection;
    char*                       controlType;
    char*                       messageType;
    char*                       vendor;
    char*                       timestamp;
    char*                       sequence;
    size_t                      dataNum;   
    cmd_change_server_data*     data;    
} cmd_change_server;

reflect_item_t cmd_change_server_ref[] = {
    _property_string(cmd_change_server, version),
    _property_string(cmd_change_server, flowDirection),
    _property_string(cmd_change_server, controlType),
    _property_string(cmd_change_server, messageType),
    _property_string(cmd_change_server, vendor),
    _property_string(cmd_change_server, timestamp),
    _property_string(cmd_change_server, sequence),
    _property_int_ex(cmd_change_server, dataNum, _ex_args_all),
    _property_array_object(cmd_change_server, data, cmd_change_server_data_ref, 
                            cmd_change_server_data, dataNum),
    _property_end()
};

/*******************************END: 更改broker地址报文********************************/


/*******************************BEGIN: 在线/离线报文********************************/
typedef struct {
    char*       model;
    char*       parentDeviceId;
    char*       deviceId;
    char*       code;
    char*       value;
} msg_online_data;

reflect_item_t msg_online_data_ref[] = {
    _property_string(msg_online_data, model),
    _property_string(msg_online_data, parentDeviceId),
    _property_string(msg_online_data, deviceId),
    _property_string(msg_online_data, code),
    _property_string(msg_online_data, value),
    _property_end()
};

typedef struct {
    char*                       version;
    char*                       flowDirection;
    char*                       controlType;
    char*                       messageType;
    char*                       vendor;
    char*                       timestamp;
    char*                       sequence;
    size_t                      dataNum;   
    msg_online_data*            data;    
} msg_online;

reflect_item_t msg_online_ref[] = {
    _property_string(msg_online, version),
    _property_string(msg_online, flowDirection),
    _property_string(msg_online, controlType),
    _property_string(msg_online, messageType),
    _property_string(msg_online, vendor),
    _property_string(msg_online, timestamp),
    _property_string(msg_online, sequence),
    _property_int_ex(msg_online, dataNum, _ex_args_all),
    _property_array_object(msg_online, data, msg_online_data_ref, 
                            msg_online_data, dataNum),
    _property_end()
};

/*******************************END: 在线/离线报文********************************/

/*******************************BEGIN: ack报文********************************/
typedef struct {
    char*       model;
    char*       parentDeviceId;
    char*       deviceId;
    char*       code;
    char*       value;
} msg_ack_data;

reflect_item_t msg_ack_data_ref[] = {
    _property_string(msg_online_data, model),
    _property_string(msg_online_data, parentDeviceId),
    _property_string(msg_online_data, deviceId),
    _property_string(msg_online_data, code),
    _property_string(msg_online_data, value),
    _property_end()
};

typedef struct {
    char*                       version;
    char*                       flowDirection;
    char*                       controlType;
    char*                       messageType;
    char*                       vendor;
    char*                       timestamp;
    char*                       sequence;
    size_t                      dataNum;   
    msg_ack_data*               data;    
} msg_ack;

reflect_item_t msg_ack_ref[] = {
    _property_string(msg_ack, version),
    _property_string(msg_ack, flowDirection),
    _property_string(msg_ack, controlType),
    _property_string(msg_ack, messageType),
    _property_string(msg_ack, vendor),
    _property_string(msg_ack, timestamp),
    _property_string(msg_ack, sequence),
    _property_int_ex(msg_ack, dataNum, _ex_args_all),
    _property_array_object(msg_ack, data, msg_ack_data_ref, 
                            msg_ack_data, dataNum),
    _property_end()
};

/*******************************END: ack报文********************************/

/*******************************BEGIN: 心跳报文********************************/
typedef struct {
    char*       model;
    char*       parentDeviceId;
    char*       deviceId;
    char*       code;
    char*       value;
} msg_heartbeat_data;

reflect_item_t msg_heartbeat_data_ref[] = {
    _property_string(msg_heartbeat_data, model),
    _property_string(msg_heartbeat_data, parentDeviceId),
    _property_string(msg_heartbeat_data, deviceId),
    _property_string(msg_heartbeat_data, code),
    _property_string(msg_heartbeat_data, value),
    _property_end()
};

typedef struct {
    char*                       version;
    char*                       flowDirection;
    char*                       controlType;
    char*                       messageType;
    char*                       vendor;
    char*                       timestamp;
    char*                       sequence;
    size_t                      dataNum;   
    msg_heartbeat_data*         data;    
} msg_heartbeat;

reflect_item_t msg_heartbeat_ref[] = {
    _property_string(msg_heartbeat, version),
    _property_string(msg_heartbeat, flowDirection),
    _property_string(msg_heartbeat, controlType),
    _property_string(msg_heartbeat, messageType),
    _property_string(msg_heartbeat, vendor),
    _property_string(msg_heartbeat, timestamp),
    _property_string(msg_heartbeat, sequence),
    _property_int_ex(msg_heartbeat, dataNum, _ex_args_all),
    _property_array_object(msg_heartbeat, data, msg_heartbeat_data_ref, 
                            msg_heartbeat_data, dataNum),
    _property_end()
};

/*******************************END: 心跳报文********************************/


/**
 * 构造在线/离线报文
*/
msg_online* buildOnlineOffline(struct Instance *instance, bool online);

#endif