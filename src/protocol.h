#ifndef _PROTOCOL_H
#define _PROTOCOL_H

/**
 * 宏定义
*/
#define KC_PROTOCOL_VERSION "2.0.0"

typedef enum {
    // 北向数据流
    NORTH_DIR,

    // 南向数据流
    SOUTH_DIR
} FlowDirection;




/**
 * 设备动态注册请求报文结构
*/
struct dev_reg_req {
    // 产品标识，线下分配
    char productKey[64];

    // 加密密文
    char cipherText[256];
};

struct req_cipher_text {
    // 客户端ID
    char clientId[64];

    // 厂商编码
    char vendorCode[64];
};

/**
 * 设备动态注册回复报文
*/
struct dev_reg_rsp {
    // 应答码：200
    char code[8];

    // 返回的数据密文
    char data[256];

    // 返回信息
    char message[16];

    // trace id
    char traceId[64];
};

struct rsp_data {
    // 设备标识
    char deviceKey[64];

    // 设备密钥
    char deviceSecret[128];

    // 客户端ID
    char clientId[64];
};





#endif