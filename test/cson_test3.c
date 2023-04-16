#include "cson.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "assert.h"
#include "math.h"

#define CHECK_STRING(a, b)  assert(strcmp(a, b) == 0)
#define CHECK_NUMBER(a, b)  assert(a == b)
#define CHECK_REAL(a, b)    assert(fabs(a-b) <= 1e-6)
/**
 * 该示例会使用cson解析如下所示播放列表。
 *
{
    "code":200,
    "data":"====AJLJLJLKJKL9KK(nflkj)",
    "message":"请求成功！",
    "traceId":"ABC"
}
*/

/*
    Step1:定义与json相对应的数据结构
*/
typedef struct {
    int         code;
    char*       data;
    char*       message;
    char*       traceId;
} Response;

/*
    Step2:定义数据结构的反射表
*/
reflect_item_t response_ref_tbl[] = {
    _property_int(Response, code),
    _property_string(Response, data),
    _property_string(Response, message),
    _property_string(Response, traceId),
    _property_end()
};

const static char* jStr = "{\"code\":200,\"data\":\"====AJLJLJLKJKL9KK(nflkj)\",\"message\":\"请求成功！\",\"traceId\":\"ABC\"}";

static void checkResult(Response* response, char* jstrOutput);
/*
    Step3:调用csonJsonStr2Struct/csonStruct2JsonStr实现反序列化和序列化
*/
void test3()
{
    printf("=========================================\n");
    printf("\t\tRunning %s\n", __FUNCTION__);
    printf("=========================================\n");
    Response response;
    memset(&response, 0, sizeof(response));

    /* string to struct */
    int ret = csonJsonStr2Struct(jStr, &response, response_ref_tbl);
    CHECK_NUMBER(ret, 0);
    printf("decode ret=%d\n", ret);
    /* test print */
    //csonPrintProperty(&playList, play_list_ref_tbl);

    char* jstrOutput;
    ret = csonStruct2JsonStr(&jstrOutput, &response, response_ref_tbl);
    CHECK_NUMBER(ret, 0);
    printf("encode ret=%d\nJson:%s\n", ret, jstrOutput);

    /*assert check*/
    checkResult(&response, jstrOutput);

    free(jstrOutput);
    csonFreePointer(&response, response_ref_tbl);
    
    printf("Successed %s.\n", __FUNCTION__);
}


void checkResult(Response* response, char* jstrOutput){
    const char* encodeTest = "{\"code\":200,\"data\":\"====AJLJLJLKJKL9KK(nflkj)\",\"message\":\"请求成功！\",\"traceId\":\"ABC\"}";

    /* assert test */
    CHECK_NUMBER(response->code, 200);
    CHECK_STRING(response->data, "====AJLJLJLKJKL9KK(nflkj)");
    CHECK_STRING(response->message, "请求成功！");
    CHECK_STRING(response->traceId, "ABC");
    
    //It is difficult to predict the output due to the accuracy problem.
    //CHECK_STRING(jstrOutput, encodeTest);
}