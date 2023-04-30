#include "protocol.h"

const char * MQTT_PROTOCOL_VERSION = "1.0";

const struct reflect_reflection DeviceRegisterReq_reflection[] = {
    REFLECT_DECLARE_STR_PTR(struct DeviceRegisterReq_t, deviceName),
    REFLECT_DECLARE_STR_PTR(struct DeviceRegisterReq_t, productKey),
    REFLECT_DECLARE_INT(struct DeviceRegisterReq_t, random),
    REFLECT_DECLARE_STR_PTR(struct DeviceRegisterReq_t, signMethod),
    REFLECT_DECLARE_STR_PTR(struct DeviceRegisterReq_t, sign),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection CommonReq_reflection[] = {
    REFLECT_DECLARE_STR(struct CommonReq_t, id),
    REFLECT_DECLARE_STR_PTR(struct CommonReq_t, version),
    REFLECT_DECLARE_STR_PTR(struct CommonReq_t, method),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection DeviceParam_reflection[] = {
    REFLECT_DECLARE_STR_PTR(struct DeviceParam_t, deviceName),
    REFLECT_DECLARE_STR_PTR(struct DeviceParam_t, productKey),
    REFLECT_DECLARE_END()
};
const struct reflect_reflection DeviceListReq_reflection[] = {
    REFLECT_DECLARE_STR(struct DeviceListReq_t, id),
    REFLECT_DECLARE_STR_PTR(struct DeviceListReq_t, version),
    REFLECT_DECLARE_STR_PTR(struct DeviceListReq_t, method),
    REFLECT_DECLARE_ARRAY_PTR_OBJECT(struct DeviceListReq_t, params, count, DeviceParam_reflection),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection TopoAddParam_reflection[] = {
    REFLECT_DECLARE_STR_PTR(struct TopoAddParam_t, deviceName),
    REFLECT_DECLARE_STR_PTR(struct TopoAddParam_t, productKey),
    REFLECT_DECLARE_INT(struct TopoAddParam_t, timestamp),
    REFLECT_DECLARE_STR_PTR(struct TopoAddParam_t, signMethod),
    REFLECT_DECLARE_STR_PTR(struct TopoAddParam_t, sign),
    REFLECT_DECLARE_END()
};
const struct reflect_reflection TopoAddReq_reflection[] = {
    REFLECT_DECLARE_STR(struct TopoAddReq_t, id),
    REFLECT_DECLARE_STR_PTR(struct TopoAddReq_t, version),
    REFLECT_DECLARE_STR_PTR(struct TopoAddReq_t, method),
    REFLECT_DECLARE_ARRAY_PTR_OBJECT(struct TopoAddReq_t, params, count, TopoAddParam_reflection),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection TopoChangeReq_reflection[] = {
    REFLECT_DECLARE_STR(struct TopoChangeReq_t, id),
    REFLECT_DECLARE_STR_PTR(struct TopoChangeReq_t, version),
    REFLECT_DECLARE_STR_PTR(struct TopoChangeReq_t, method),
    REFLECT_DECLARE_INT(struct TopoChangeReq_t, status),
    REFLECT_DECLARE_ARRAY_PTR_OBJECT(struct TopoChangeReq_t, subList, count, DeviceParam_reflection),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection DeviceLogin_reflection[] = {
    REFLECT_DECLARE_STR_PTR(struct DeviceLogin_t, productKey),
    REFLECT_DECLARE_STR_PTR(struct DeviceLogin_t, deviceName),
    REFLECT_DECLARE_INT(struct DeviceLogin_t, timestamp),
    REFLECT_DECLARE_STR_PTR(struct DeviceLogin_t, signMethod),
    REFLECT_DECLARE_STR_PTR(struct DeviceLogin_t, sign),
    REFLECT_DECLARE_BOOL(struct DeviceLogin_t, cleanSession),
    REFLECT_DECLARE_END()
};
const struct reflect_reflection DeviceLoginReq_reflection[] = {
    REFLECT_DECLARE_STR(struct DeviceLoginReq_t, id),
    REFLECT_DECLARE_STR_PTR(struct DeviceLoginReq_t, version),
    REFLECT_DECLARE_STR_PTR(struct DeviceLoginReq_t, method),
    REFLECT_DECLARE_ARRAY_PTR_OBJECT(struct DeviceLoginReq_t, deviceList, count, DeviceLogin_reflection),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection DeviceProperty_reflection[] = {
    REFLECT_DECLARE_STR_PTR(struct DeviceProperty_t, name),
    REFLECT_DECLARE_ANY_PTR(struct DeviceProperty_t, value),
    REFLECT_DECLARE_INT(struct DeviceProperty_t, time),
    REFLECT_DECLARE_END()
};
const struct reflect_reflection PropertyPostReq_reflection[] = {
    REFLECT_DECLARE_STR(struct PropertyPostReq_t, id),
    REFLECT_DECLARE_STR_PTR(struct PropertyPostReq_t, version),
    REFLECT_DECLARE_STR_PTR(struct PropertyPostReq_t, method),
    REFLECT_DECLARE_ARRAY_PTR_OBJECT(struct PropertyPostReq_t, params, count, DeviceProperty_reflection),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection DevicePropertySet_reflection[] = {
    REFLECT_DECLARE_STR_PTR(struct DevicePropertySet_t, name),
    REFLECT_DECLARE_ANY_PTR(struct DevicePropertySet_t, value),
    REFLECT_DECLARE_END()
};
const struct reflect_reflection PropertySetReq_reflection[] = {
    REFLECT_DECLARE_STR(struct PropertySetReq_t, id),
    REFLECT_DECLARE_STR_PTR(struct PropertySetReq_t, version),
    REFLECT_DECLARE_STR_PTR(struct PropertySetReq_t, method),
    REFLECT_DECLARE_ARRAY_PTR_OBJECT(struct PropertySetReq_t, params, count, DevicePropertySet_reflection),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection KeyValuePair_reflection[] = {
    REFLECT_DECLARE_STR_PTR(struct KeyValuePair_t, key),
    REFLECT_DECLARE_ANY_PTR(struct KeyValuePair_t, value),
    REFLECT_DECLARE_END()
};
const struct reflect_reflection EventPostReq_reflection[] = {
    REFLECT_DECLARE_STR(struct EventPostReq_t, id),
    REFLECT_DECLARE_STR_PTR(struct EventPostReq_t, version),
    REFLECT_DECLARE_STR_PTR(struct EventPostReq_t, method),
    REFLECT_DECLARE_INT(struct EventPostReq_t, time),
    REFLECT_DECLARE_ARRAY_PTR_OBJECT(struct EventPostReq_t, params, count, KeyValuePair_reflection),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection ServiceCallReq_reflection[] = {
    REFLECT_DECLARE_STR(struct ServiceCallReq_t, id),
    REFLECT_DECLARE_STR_PTR(struct ServiceCallReq_t, version),
    REFLECT_DECLARE_STR_PTR(struct ServiceCallReq_t, method),
    REFLECT_DECLARE_ARRAY_PTR_OBJECT(struct ServiceCallReq_t, params, count, KeyValuePair_reflection),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection AliasSetReq_reflection[] = {
    REFLECT_DECLARE_STR(struct AliasSetReq_t, id),
    REFLECT_DECLARE_STR_PTR(struct AliasSetReq_t, version),
    REFLECT_DECLARE_STR_PTR(struct AliasSetReq_t, method),
    REFLECT_DECLARE_STR_PTR(struct AliasSetReq_t, alias),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection OtaInform_reflection[] = {
    REFLECT_DECLARE_STR(struct OtaInform_t, id),
    REFLECT_DECLARE_STR_PTR(struct OtaInform_t, version),
    REFLECT_DECLARE_STR_PTR(struct OtaInform_t, module),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection OtaUpgrade_reflection[] = {
    REFLECT_DECLARE_STR(struct OtaUpgrade_t, id),
    REFLECT_DECLARE_STR_PTR(struct OtaUpgrade_t, version),
    REFLECT_DECLARE_STR_PTR(struct OtaUpgrade_t, module),
    REFLECT_DECLARE_STR_PTR(struct OtaUpgrade_t, url),
    REFLECT_DECLARE_STR_PTR(struct OtaUpgrade_t, md5),
    REFLECT_DECLARE_INT(struct OtaUpgrade_t, size),
    REFLECT_DECLARE_ARRAY_PTR_OBJECT(struct OtaUpgrade_t, params, count, KeyValuePair_reflection),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection OtaProgress_reflection[] = {
    REFLECT_DECLARE_STR(struct OtaProgress_t, id),
    REFLECT_DECLARE_STR_PTR(struct OtaProgress_t, module),
    REFLECT_DECLARE_INT(struct OtaProgress_t, step),
    REFLECT_DECLARE_STR_PTR(struct OtaProgress_t, desc),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection OtaFirmwareGetReq_reflection[] = {
    REFLECT_DECLARE_STR(struct OtaFirmwareGetReq_t, id),
    REFLECT_DECLARE_STR_PTR(struct OtaFirmwareGetReq_t, version),
    REFLECT_DECLARE_STR_PTR(struct OtaFirmwareGetReq_t, method),
    REFLECT_DECLARE_STR_PTR(struct OtaFirmwareGetReq_t, module),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection CommonRes_reflection[] = {
    REFLECT_DECLARE_STR(struct CommonRes_t, id),
    REFLECT_DECLARE_INT(struct CommonRes_t, code),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection RegisterData_reflection[] = {
    REFLECT_DECLARE_STR_PTR(struct RegisterData_t, productKey),
    REFLECT_DECLARE_STR_PTR(struct RegisterData_t, deviceName),
    REFLECT_DECLARE_STR_PTR(struct RegisterData_t, deviceSecret),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection DeviceRegisterRes_reflection[] = {
    REFLECT_DECLARE_INT(struct DeviceRegisterRes_t, code),
    REFLECT_DECLARE_OBJECT(struct DeviceRegisterRes_t, device, RegisterData_reflection),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection SubDeviceRegisterRes_reflection[] = {
    REFLECT_DECLARE_STR(struct SubDeviceRegisterRes_t, id),
    REFLECT_DECLARE_INT(struct SubDeviceRegisterRes_t, code),
    REFLECT_DECLARE_ARRAY_PTR_OBJECT(struct SubDeviceRegisterRes_t, data, count, RegisterData_reflection),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection DeviceData_reflection[] = {
    REFLECT_DECLARE_STR_PTR(struct DeviceData_t, productKey),
    REFLECT_DECLARE_STR_PTR(struct DeviceData_t, deviceName),
	REFLECT_DECLARE_STR_PTR(struct DeviceData_t, alias),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection DeviceList_reflection[] = {
    REFLECT_DECLARE_STR(struct DeviceList_t, id),
    REFLECT_DECLARE_INT(struct DeviceList_t, code),
    REFLECT_DECLARE_ARRAY_PTR_OBJECT(struct DeviceList_t, data, count, DeviceData_reflection),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection KeyValueData_reflection[] = {
    REFLECT_DECLARE_STR_PTR(struct KeyValueData_t, key),
    REFLECT_DECLARE_ANY_PTR(struct KeyValueData_t, value),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection ServiceCallRes_reflection[] = {
    REFLECT_DECLARE_STR(struct ServiceCallRes_t, id),
    REFLECT_DECLARE_INT(struct ServiceCallRes_t, code),
    REFLECT_DECLARE_ARRAY_PTR_OBJECT(struct ServiceCallRes_t, data, count, KeyValueData_reflection),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection OtaFirmwareGetRes_reflection[] = {
    REFLECT_DECLARE_STR(struct OtaFirmwareGetRes_t, id),
    REFLECT_DECLARE_STR_PTR(struct OtaFirmwareGetRes_t, version),
    REFLECT_DECLARE_STR_PTR(struct OtaFirmwareGetRes_t, module),
    REFLECT_DECLARE_STR_PTR(struct OtaFirmwareGetRes_t, url),
    REFLECT_DECLARE_STR_PTR(struct OtaFirmwareGetRes_t, md5),
    REFLECT_DECLARE_INT(struct OtaFirmwareGetRes_t, size),
    REFLECT_DECLARE_END()
};
