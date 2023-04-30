#pragma once
#include "reflect.h"

extern const char * MQTT_PROTOCOL_VERSION;

struct DeviceRegisterReq_t {
    char * deviceName;
    char * productKey;
    int random;
    char * signMethod;
    char * sign;
};
extern const struct reflect_reflection DeviceRegisterReq_reflection[];

struct CommonReq_t {
    char id[12];
    const char *version;
    const char *method;
};
extern const struct reflect_reflection CommonReq_reflection[];

struct DeviceParam_t {
    const char *deviceName;
    const char *productKey;
};
extern const struct reflect_reflection DeviceParam_reflection[];
struct DeviceListReq_t {
    char id[12];
    char *version;
    char *method;
    struct DeviceParam_t *params;
    int count;
};
extern const struct reflect_reflection DeviceListReq_reflection[];

struct TopoAddParam_t {
    const char *deviceName;
    const char *productKey;
    long long timestamp;
    char *signMethod;
    char *sign;
};
extern const struct reflect_reflection TopoAddParam_reflection[];
struct TopoAddReq_t {
    char id[12];
    char *version;
    char *method;
    struct TopoAddParam_t *params;
    int count;
};
extern const struct reflect_reflection TopoAddReq_reflection[];

struct TopoChangeReq_t {
    char id[12];
    char *version;
    char *method;
    int status;
    struct DeviceParam_t *subList;
    int count;
};
extern const struct reflect_reflection TopoChangeReq_reflection[];

struct DeviceLogin_t {
    const char *productKey;
    const char *deviceName;
    long long timestamp;
    char *signMethod;
    char *sign;
    bool cleanSession;
};
extern const struct reflect_reflection DeviceLogin_reflection[];
struct DeviceLoginReq_t {
    char id[12];
    char *version;
    char *method;
    struct DeviceLogin_t *deviceList;
    int count;
};
extern const struct reflect_reflection DeviceLoginReq_reflection[];

struct DeviceProperty_t {
    char *name;
    char *value;
    long long time;
};
extern const struct reflect_reflection DeviceProperty_reflection[];
struct PropertyPostReq_t {
    char id[12];
    char *version;
    char *method;
    struct DeviceProperty_t *params;
    int count;
};
extern const struct reflect_reflection PropertyPostReq_reflection[];

struct DevicePropertySet_t {
    char *name;
    char *value;
};
extern const struct reflect_reflection DevicePropertySet_reflection[];
struct PropertySetReq_t {
    char id[12];
    char *version;
    char *method;
    struct DevicePropertySet_t *params;
    int count;
};
extern const struct reflect_reflection PropertySetReq_reflection[];

struct KeyValuePair_t {
    char *key;
    char *value;
};
extern const struct reflect_reflection KeyValuePair_reflection[];
struct EventPostReq_t {
    char id[12];
    char *version;
    char *method;
    long long time;
    struct KeyValuePair_t *params;
    int count;
};
extern const struct reflect_reflection EventPostReq_reflection[];

struct ServiceCallReq_t {
    char id[12];
    char *version;
    char *method;
    struct KeyValuePair_t *params;
    int count;
};
extern const struct reflect_reflection ServiceCallReq_reflection[];

struct AliasSetReq_t {
    char id[12];
    char *version;
    char *method;
    char *alias;
};
extern const struct reflect_reflection AliasSetReq_reflection[];

struct OtaInform_t {
    char id[12];
    const char *version;
    const char *module;
};
extern const struct reflect_reflection OtaInform_reflection[];

struct OtaUpgrade_t {
    char id[12];
    char *version;
    char *module;
    char *url;
    char *md5;
    long long size;
    struct KeyValuePair_t *params;
    int count;
};
extern const struct reflect_reflection OtaUpgrade_reflection[];

struct OtaProgress_t {
    char id[12];
    char *module;
    int step;
    const char *desc;
};
extern const struct reflect_reflection OtaProgress_reflection[];

struct OtaFirmwareGetReq_t {
    char id[12];
    char *version;
    char *method;
    char *module;
};
extern const struct reflect_reflection OtaFirmwareGetReq_reflection[];

struct CommonRes_t {
    char id[12];
    int code;
};
extern const struct reflect_reflection CommonRes_reflection[];

struct RegisterData_t {
    char *productKey;
    char *deviceName;
    char *deviceSecret;
};
extern const struct reflect_reflection RegisterData_reflection[];

struct DeviceRegisterRes_t {
    int code;
    struct RegisterData_t device;
};
extern const struct reflect_reflection DeviceRegisterRes_reflection[];

struct SubDeviceRegisterRes_t {
    char id[12];
    int code;
    struct RegisterData_t *data;
    int count;
};
extern const struct reflect_reflection SubDeviceRegisterRes_reflection[];

struct DeviceData_t {
    char *productKey;
    char *deviceName;
	char *alias;
};
extern const struct reflect_reflection DeviceData_reflection[];
struct DeviceList_t {
    char id[12];
    int code;
    struct DeviceData_t *data;
    int count;
};
extern const struct reflect_reflection DeviceList_reflection[];

struct KeyValueData_t {
    char *key;
    char *value;
};
extern const struct reflect_reflection KeyValueData_reflection[];

struct ServiceCallRes_t {
    char id[12];
    int code;
    struct KeyValueData_t *data;
    int count;
};
extern const struct reflect_reflection ServiceCallRes_reflection[];

struct OtaFirmwareGetRes_t {
    char id[12];
    char *version;
    char *module;
    char *url;
    char *md5;
    int size;
};
extern const struct reflect_reflection OtaFirmwareGetRes_reflection[];
