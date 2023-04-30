#include "reflect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct device {
    char *id;
    char name[20];
    float temperature;
    unsigned long tick;
    unsigned int ip;
    unsigned int netmask;
    unsigned int gateway;
};

const struct reflect_reflection device_reflection[] = {
    REFLECT_DECLARE_ANY_PTR(struct device, id),
    REFLECT_DECLARE_STR(struct device, name),
    REFLECT_DECLARE_FLOAT(struct device, temperature),
    REFLECT_DECLARE_INT(struct device, tick),
    REFLECT_DECLARE_INT(struct device, ip),
    REFLECT_DECLARE_INT(struct device, netmask),
    REFLECT_DECLARE_INT(struct device, gateway),
    REFLECT_DECLARE_END()
};

struct date {
    int year;
    int month;
    int day;
    int hour;
    int min;
    int sec;
};

enum booktype {
    BOOK_TECH = 0,
    BOOK_CARTOON
};

struct book {
    char *name;
    struct date time;
    enum booktype type;
    char isbn[11];
    char **authors;
    int author_cnt;
};

struct book_complex {
    char *name;
    struct date time;
    enum booktype type;
    char isbn[11];
    char **authors;
    int author_cnt;
    short chapter_pages[15];
    int chapter_page_cnt;
};

const struct reflect_reflection date_reflection[] = {
    REFLECT_DECLARE_INT(struct date, year),
    REFLECT_DECLARE_INT(struct date, month),
    REFLECT_DECLARE_INT(struct date, day),
    REFLECT_DECLARE_INT(struct date, min),
    REFLECT_DECLARE_INT(struct date, hour),
    REFLECT_DECLARE_INT(struct date, sec),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection book_reflection[] = {
    REFLECT_DECLARE_STR_PTR_EX(struct book, name),
    REFLECT_DECLARE_OBJECT_EX(struct book, time, date_reflection, .serialized = false),
    REFLECT_DECLARE_INT(struct book, type),
    REFLECT_DECLARE_STR_EX(struct book, isbn, .serialized_name = "ISBN"),
    REFLECT_DECLARE_ARRAY_PTR_STR_PTR(struct book, authors, author_cnt),
    REFLECT_DECLARE_END()
};

const struct reflect_reflection book_complex_reflection[] = {
    REFLECT_DECLARE_STR_PTR_EX(struct book_complex, name),
    REFLECT_DECLARE_OBJECT_EX(struct book_complex, time, date_reflection),
    REFLECT_DECLARE_INT(struct book_complex, type),
    REFLECT_DECLARE_STR_EX(struct book_complex, isbn, .serialized_name = "ISBN"),
    REFLECT_DECLARE_ARRAY_PTR_STR_PTR(struct book_complex, authors, author_cnt),
    REFLECT_DECLARE_ARRAY_INT(struct book_complex, chapter_pages, chapter_page_cnt),
    REFLECT_DECLARE_END()
};

void simple_serialize() {
    char idStr[10] = {0};
    idStr[0] = REFLECT_STRING;
    strcpy(&idStr[1], "Hello");
    struct device d = {
        .id = idStr,
        .ip = 0x0A01A8C0,
        .gateway = 0x0101A8C0,
        .netmask = 0x00FFFFFF,
        .name = "Hello World",
        .temperature = 36.2f,
        .tick = 123
    };
    char buf[300];
    int ret = reflect_struct2str_preallocated(buf, sizeof(buf), &d, device_reflection);
    if (ret == REFLECT_ERR_SUCCESS) {
        printf("Serialized JSON string: %s\n", buf);
    } else {
        printf("Serialize failed: %s", reflect_err_str(ret));
    }
}

void simple_deserialize() {
    const char *json = "{\"id\":\"null\",\"name\":\"Hello World\",\"temperature\":36.2,\"tick\":123,\"ip\":167880896,\"netmask\":16777215,\"gateway\":16885952}";
    struct device d;
    int ret = reflect_str2struct(json, &d, device_reflection);
    if (ret == REFLECT_ERR_SUCCESS) {
        printf("Deserialized struct: id[");
        if (d.id != NULL) {
            switch (d.id[0]) {
            case REFLECT_STRING:
                printf("\"%s\"", &d.id[1]);
                break;
            case REFLECT_INTEGER:
                {
                    long long val;
                    memcpy(&val, &d.id[1], sizeof(long long));
                    printf("%lld", val);
                }
                break;
            case REFLECT_FLOAT:
                {
                    double val;
                    memcpy(&val, &d.id[1], sizeof(double));
                    printf("%lf", val);
                }
                break;
            case REFLECT_TRUE:
                printf("true");
                break;
            case REFLECT_FALSE:
                printf("false");
                break;
            case REFLECT_NULL:
                printf("null");
                break;
            }
        }
        printf("], name[%s], temprature[%f], tick[%ld], ip[%08X], netmask[%08X], gateway[%08X]\n",
            d.name, d.temperature, d.tick, d.ip, d.netmask, d.gateway);
        reflect_free_ptr(&d, device_reflection);
    } else {
        printf("Deserialize failed: %s", reflect_err_str(ret));
    }
}

void reflect_ex() {
    struct book b = {0};
    const char *json = "{\"name\":\"AAA\",\"ISBN\":\"1234567890\",\"type\":0"
        ",\"authors\":[\"a\",\"b\",\"c\"],\"time\":{\"year\":2020,\"month\":11"
        ",\"day\":10,\"hour\":12,\"min\":12,\"sec\":12}}";
    int ret = reflect_str2struct(json, &b, book_reflection);
    if (ret == REFLECT_ERR_SUCCESS) {
        char *p;
        reflect_struct2str(&p, &b, book_reflection);
        printf("%s\n", p);
        reflect_free_ptr(&b, book_reflection);
        free(p);
    }
}

void reflect_complex() {
    struct book_complex b = {0};
        const char* json = "{\"name\":\"AAA\",\"ISBN\":\"1234567890\",\"type\":0,\"authors\""
        ":[\"a\",\"b\",\"c\"],\"time\":{\"year\":2020,\"month\":11,\"day\":10,\"hour\":12"
        ",\"min\":12,\"sec\":12}, \"chapter_pages\":[10,15,28,77]}";
    int ret = reflect_str2struct(json, &b, book_complex_reflection);
    if (ret == REFLECT_ERR_SUCCESS) {
        char *p;
        reflect_struct2str(&p, &b, book_complex_reflection);
        printf("%s\n", p);
        reflect_free_ptr(&b, book_complex_reflection);
        free(p);
    }
}

struct MqttPayloadHeader_t {
    char id[11];
    char version[4];
    char method[64];
};
static void init_payload_header(void *payload, int id, const char * method) {
    struct MqttPayloadHeader_t * header = (struct MqttPayloadHeader_t *)payload;
    sprintf(header->id, "%d", id);
    strcpy(header->version, "1.0");
    strcpy(header->method, method);
}

struct DeviceInfo_t {
    const char *productKey;
    const char *deviceName;
};
struct ThingSubRegister_t {
    char id[11];
    char version[4];
    char method[64];
    struct DeviceInfo_t *subDevices;
    int cntDevices;
};
const struct reflect_reflection DeviceInfo_reflection[] = {
    REFLECT_DECLARE_STR_PTR(struct DeviceInfo_t, productKey),
    REFLECT_DECLARE_STR_PTR(struct DeviceInfo_t, deviceName),
    REFLECT_DECLARE_END()
};
const struct reflect_reflection ThingSubRegister_reflection[] = {
    REFLECT_DECLARE_STR(struct ThingSubRegister_t, id),
    REFLECT_DECLARE_STR(struct ThingSubRegister_t, version),
    REFLECT_DECLARE_STR(struct ThingSubRegister_t, method),
    REFLECT_DECLARE_ARRAY_PTR_OBJECT(struct ThingSubRegister_t, subDevices, cntDevices, DeviceInfo_reflection),
    REFLECT_DECLARE_END()
};

void reflect_register_sub_device(int id, const char *productKey, const char *deviceName) {
    struct ThingSubRegister_t payload;
    init_payload_header(&payload, id, "thing.sub.register");
    payload.cntDevices = 1;
    struct DeviceInfo_t subDevice = {
        .productKey = productKey,
        .deviceName = deviceName
    };
    payload.subDevices = &subDevice;
    char *payloadStr;
    reflect_struct2str(&payloadStr, &payload, ThingSubRegister_reflection);

    printf("%s\n", payloadStr);
    free(payloadStr);
}

struct KeyValuePair_t {
    char *key;
    char *value;
};
const struct reflect_reflection KeyValuePair_reflection[] = {
    REFLECT_DECLARE_STR_PTR(struct KeyValuePair_t, key),
    REFLECT_DECLARE_ANY_PTR(struct KeyValuePair_t, value),
    REFLECT_DECLARE_END()
};

struct ServiceCallReq_t {
    char id[12];
    char *version;
    char *method;
    struct KeyValuePair_t *params;
    int count;
};
const struct reflect_reflection ServiceCallReq_reflection[] = {
    REFLECT_DECLARE_STR(struct ServiceCallReq_t, id),
    REFLECT_DECLARE_STR_PTR(struct ServiceCallReq_t, version),
    REFLECT_DECLARE_STR_PTR(struct ServiceCallReq_t, method),
    REFLECT_DECLARE_ARRAY_PTR_OBJECT(struct ServiceCallReq_t, params, count, KeyValuePair_reflection),
    REFLECT_DECLARE_END()
};

void reflect_service_start_permit_join(const char * message) {
    struct ServiceCallReq_t req;
    memset(&req, 0, sizeof(struct ServiceCallReq_t));
	printf("%s\n", message);
    int ret = reflect_str2struct(message, &req, ServiceCallReq_reflection);
    if (ret != REFLECT_ERR_SUCCESS) {
        printf("deserialize failed: %s", reflect_err_str(ret));
        return;
    }
    printf("id:%s\n", req.id);
    printf("version:%s\n", req.version);
    printf("method:%s\n", req.method);
    printf("params:%p\n", req.params);
    printf("count:%d\n", req.count);
	for (int i = 0; i < req.count; i++) {
		printf("\t%s: ", req.params[i].key);
		switch (reflect_get_any_type(req.params[i].value)) {
		case REFLECT_OBJECT:
			printf("[OBJECT]\n");
			break;
		case REFLECT_ARRAY:
			{
				char ** val;
				int count = reflect_any2array(req.params[i].value, &val);
				if (count < 0) {
					printf("[ARRAY]\n");
				} else {
					printf("[");
					for (int i = 0; i < count; i++) {
						if (i > 0) {
							printf(",");
						}
						switch (reflect_get_any_type(val[i])) {
						case REFLECT_OBJECT:
							printf("[OBJECT]");
							break;
						case REFLECT_ARRAY:
							printf("[ARRAY]");
							break;
						case REFLECT_STRING:
							{
								char *str;
								if (reflect_any2str(val[i], &str)) {
									printf("\"%s\"", str);
								} else {
									printf("[STRING]");
								}
							}
							break;
						case REFLECT_INTEGER:
							{
								long long ival;
								if (reflect_any2int(val[i], &ival)) {
									printf("%lld", ival);
								} else {
									printf("[INTEGER]");
								}
							}
							break;
						case REFLECT_FLOAT:
							{
								double fval;
								if (reflect_any2float(val[i], &fval)) {
									printf("%lf", fval);
								} else {
									printf("[FLOAT]");
								}
							}
							break;
						case REFLECT_TRUE:
							printf("true");
							break;
						case REFLECT_FALSE:
							printf("false");
							break;
						case REFLECT_NULL:
							printf("null");
							break;
						default:
							printf("[UNKNOWN]");
							break;
						}
					}
					printf("]\n");
				}
				free(val);
			}
			break;
		case REFLECT_STRING:
			{
				char *str;
				if (reflect_any2str(req.params[i].value, &str)) {
					printf("\"%s\"\n", str);
				} else {
					printf("[STRING]\n");
				}
			}
			break;
		case REFLECT_INTEGER:
			{
				long long val;
				if (reflect_any2int(req.params[i].value, &val)) {
					printf("%lld\n", val);
				} else {
					printf("[INTEGER]\n");
				}
			}
			break;
		case REFLECT_FLOAT:
			{
				double val;
				if (reflect_any2float(req.params[i].value, &val)) {
					printf("%lf\n", val);
				} else {
					printf("[FLOAT]\n");
				}
			}
			break;
		case REFLECT_TRUE:
			printf("true\n");
			break;
		case REFLECT_FALSE:
			printf("false\n");
			break;
		case REFLECT_NULL:
			printf("null\n");
			break;
		default:
			printf("[UNKNOWN]\n");
			break;
		}
	}
    reflect_free_ptr(&req, ServiceCallReq_reflection);
}

int main(int argc, char **argv) {
    simple_serialize();
    simple_deserialize();
    reflect_ex();
    reflect_complex();
    reflect_register_sub_device(1234, "abcdefg", "hijklmn");
    reflect_service_start_permit_join("{\"id\":\"29\",\"version\":\"1.0\",\"params\":[{\"key\":\"duration\",\"value\":30},{\"key\":\"installCode\",\"value\":\"123456\"},{\"key\":\"mac\",\"value\":[\"abc\",\"def\"]}],\"method\":\"thing.service.start_permit_join\"}");
    return 0;
}