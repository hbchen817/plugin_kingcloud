#pragma once

#define PRODUCT_ID_LEN 32
#define IDENTIFIER_LEN 32
#define NAME_LEN       32
#define VALUE_LEN      32
#define DESC_LEN       32
#define METHOD_LEN     32
#define VERSION_LEN    32

typedef enum {
    TSL_DATATYPE_NONE = 0,
    TSL_DATATYPE_INT,
    TSL_DATATYPE_FLOAT,
    TSL_DATATYPE_DOUBLE,
    TSL_DATATYPE_ENUM,
    TSL_DATATYPE_BOOL,
    TSL_DATATYPE_TEXT,
    TSL_DATATYPE_DATE,
    TSL_DATATYPE_STRUCT,
    TSL_DATATYPE_ARRAY,
    TSL_DATATYPE_MAX
} RexTslDataType_e;

typedef enum {
    TSL_ACCESS_MODE_READ = 0,
    TSL_ACCESS_MODE_WRITE,
    TSL_ACCESS_MODE_READ_AND_WRITE,
    TSL_ACCESS_MODE_MAX
} RexTslAccessMode_e;

typedef enum {
    TSL_EVENT_TYPE_INFO = 0,
    TSL_EVENT_TYPE_ERROR,
    TSL_EVENT_TYPE_WARN,
    TSL_EVENT_TYPE_MAX
} RexTslEventType_e;

typedef enum {
    TSL_SERVICE_CALL_TYPE_ASYNC = 0,
    TSL_SERVICE_CALL_TYPE_SYNC,
    TSL_SERVICE_CALL_TYPE_MAX
} RexTslServiceCallType_e;

typedef struct {
    char name[NAME_LEN + 1];
    char value[VALUE_LEN + 1];
} RexTlsSpec_t;

typedef struct {
    RexTslDataType_e type;
    int              specNum;
    RexTlsSpec_t    *specList;
} RexTslDataTypeInfo_t;

typedef struct {
    char                 identifier[IDENTIFIER_LEN + 1];
    char                 name[NAME_LEN + 1];
    int                  required;
    RexTslAccessMode_e   accessMode;
    RexTslDataTypeInfo_t datatype;
} RexTslPropertyInfo_t;

typedef struct {
    char                  identifier[IDENTIFIER_LEN + 1];
    char                  name[NAME_LEN + 1];
    char                  desc[DESC_LEN + 1];
    char                  method[METHOD_LEN + 1];
    RexTslEventType_e     type;
    int                   required;
    int                   outputDataNum;
    RexTslPropertyInfo_t *outputDataList;
} RexTslEventInfo_t;

typedef struct {
    char                    identifier[IDENTIFIER_LEN + 1];
    char                    name[NAME_LEN + 1];
    char                    method[METHOD_LEN + 1];
    RexTslServiceCallType_e callType;
    int                     required;
    int                     inputDataNum;
    RexTslPropertyInfo_t   *inputDataList;
    int                     outputDataNum;
    RexTslPropertyInfo_t   *outputDataList;
} RexTslServiceInfo_t;

typedef struct {
    char                  version[VERSION_LEN + 1];
    char                  productId[PRODUCT_ID_LEN + 1];
    int                   propertyInfoNum;
    RexTslPropertyInfo_t *propertyInfoList;
    int                   eventInfoNum;
    RexTslEventInfo_t    *eventInfoList;
    int                   serviceInfoNum;
    RexTslServiceInfo_t  *serviceInfoList;
} RexTslModelInfo_t;
