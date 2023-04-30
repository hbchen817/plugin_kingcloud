#include "reflect.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*reflect_obj_serializer)(void *input, const struct reflect_reflection *tbl, int index, cJSON **obj);
typedef int (*reflect_arr_item_serializer)(void *input, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect, cJSON **obj);
typedef int (*reflect_obj_deserializer)(cJSON *json, void *output, const struct reflect_reflection *tbl, int index);
typedef int (*reflect_arr_item_deserializer)(cJSON *json, void *output, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect);
typedef void *(*reflect_loop_cb)(void *data, const struct reflect_reflection *item, const struct reflect_reflection *arr);

union _reflect_int_val_t {
    char c;
    short s;
    int i;
    long long l;
};

static int map_cjson_type(int type);
static int map_reflect_type(enum reflect_type type);
static void *item_free(void *data, const struct reflect_reflection *item, const struct reflect_reflection *arr);
static void reflection_loop(void *data, const struct reflect_reflection *tbl, const struct reflect_reflection *arr, reflect_loop_cb func);
static int serialize_object(void *input, const struct reflect_reflection *tbl, int index, cJSON **obj);
static int serialize_array(void *input, const struct reflect_reflection *tbl, int index, cJSON **obj);
static int serialize_string(void *input, const struct reflect_reflection *tbl, int index, cJSON **obj);
static int serialize_any(void *input, const struct reflect_reflection *tbl, int index, cJSON **obj);
static int serialize_integer(void *input, const struct reflect_reflection *tbl, int index, cJSON **obj);
static int serialize_float(void *input, const struct reflect_reflection *tbl, int index, cJSON **obj);
static int serialize_bool(void *input, const struct reflect_reflection *tbl, int index, cJSON **obj);
static int serialize_arr_string(void *input, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect, cJSON **obj);
static int serialize_arr_integer(void *input, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect, cJSON **obj);
static int serialize_arr_float(void *input, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect, cJSON **obj);
static int serialize_arr_bool(void *input, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect, cJSON **obj);
static int deserialize_object(cJSON *json, void *output, const struct reflect_reflection *tbl, int index);
static int deserialize_array(cJSON *json, void *output, const struct reflect_reflection *tbl, int index);
static int deserialize_string(cJSON *json, void *output, const struct reflect_reflection *tbl, int index);
static int deserialize_any(cJSON *json, void *output, const struct reflect_reflection *tbl, int index);
static int deserialize_integer(cJSON *json, void *output, const struct reflect_reflection *tbl, int index);
static int deserialize_float(cJSON *json, void *output, const struct reflect_reflection *tbl, int index);
static int deserialize_bool(cJSON *json, void *output, const struct reflect_reflection *tbl, int index);
static int deserialize_object_default(cJSON *json, void *output, const struct reflect_reflection *tbl, int index);
static int deserialize_array_default(cJSON *json, void *output, const struct reflect_reflection *tbl, int index);
static int deserialize_string_default(cJSON *json, void *output, const struct reflect_reflection *tbl, int index);
static int deserialize_integer_default(cJSON *json, void *output, const struct reflect_reflection *tbl, int index);
static int deserialize_float_default(cJSON *json, void *output, const struct reflect_reflection *tbl, int index);
static int deserialize_arr_string(cJSON *json, void *output, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect);
static int deserialize_arr_integer(cJSON *json, void *output, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect);
static int deserialize_arr_float(cJSON *json, void *output, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect);
static int deserialize_arr_bool(cJSON *json, void *output, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect);

const struct reflect_reflection reflect_int[] = {
    {"0integer", 0, sizeof(int), REFLECT_INTEGER, NULL, NULL, 0, 0, {0}},
    {0}
};

const struct reflect_reflection reflect_string_ptr[] = {
    {"0strptr", 0, sizeof(char *), REFLECT_STRING, NULL, NULL, 0, 1, {0}},
    {0}
};

const struct reflect_reflection reflect_string_preallocated[] = {
    {"0str", 0, sizeof(char *), REFLECT_STRING, NULL, NULL, 0, 0, {0}},
    {0}
};

// const struct reflect_reflection reflect_any_ptr[] = {
//     {"0anyptr", 0, sizeof(char *), REFLECT_ANY, NULL, NULL, 0, 1, {0}},
//     {0}
// };

// const struct reflect_reflection reflect_any_preallocated[] = {
//     {"0any", 0, sizeof(char *), REFLECT_ANY, NULL, NULL, 0, 0, {0}},
//     {0}
// };

const struct reflect_reflection reflect_real[] = {
    {"0real", 0, sizeof(double), REFLECT_FLOAT, NULL, NULL, 0, 0, {0}},
    {0}
};

const struct reflect_reflection reflect_float[] = {
    {"0float", 0, sizeof(float), REFLECT_FLOAT, NULL, NULL, 0, 0, {0}},
    {0}
};

const struct reflect_reflection reflect_double[] = {
    {"0double", 0, sizeof(double), REFLECT_FLOAT, NULL, NULL, 0, 0, {0}},
    {0}
};

const struct reflect_reflection reflect_bool[] = {
    {"0bool", 0, sizeof(int), REFLECT_TRUE, NULL, NULL, 0, 0, {0}},
    {0}
};

reflect_obj_serializer serializer_tbl[] = {
    serialize_object,
    serialize_array,
    serialize_string,
    serialize_integer,
    serialize_float,
    serialize_bool,
    serialize_bool,
    serialize_any,
    NULL
};

reflect_arr_item_serializer arr_serializer_tbl[] = {
    NULL,
    NULL,
    serialize_arr_string,
    serialize_arr_integer,
    serialize_arr_float,
    serialize_arr_bool,
    serialize_arr_bool,
    serialize_arr_string,
    NULL
};

reflect_obj_deserializer deserializer_tbl[] = {
    deserialize_object,
    deserialize_array,
    deserialize_string,
    deserialize_integer,
    deserialize_float,
    deserialize_bool,
    deserialize_bool,
    deserialize_any,
    NULL
};

reflect_obj_deserializer deserializer_default_tbl[] = {
    deserialize_object_default,
    deserialize_array_default,
    deserialize_string_default,
    deserialize_integer_default,
    deserialize_float_default,
    deserialize_integer_default,
    deserialize_integer_default,
    deserialize_string_default,
    NULL
};

reflect_arr_item_deserializer arr_deserializer_tbl[] = {
    NULL,
    NULL,
    deserialize_arr_string,
    deserialize_arr_integer,
    deserialize_arr_float,
    deserialize_arr_bool,
    deserialize_arr_bool,
    deserialize_arr_string,
    NULL
};

const char *reflect_err_str(int code) {
    switch (code) {
    case REFLECT_ERR_SUCCESS:
        return "success";
    case REFLECT_ERR_MEMORY:
        return "malloc failed";
    case REFLECT_ERR_TYPE:
        return "type matching error";
    case REFLECT_ERR_MISSING_FIELD:
        return "field not found";
    case REFLECT_ERR_FORMAT:
        return "input json string format error";
    case REFLECT_ERR_ARGS:
        return "args error";
    case REFLECT_ERR_OVERFLOW:
        return "value overflow";
    default:
        return "unknown";
    }
}

int reflect_str2struct(const char *jsonstr, void *output, const struct reflect_reflection *tbl) {
    cJSON *json = cJSON_Parse(jsonstr);
    if (!json) {
        return REFLECT_ERR_FORMAT;
    }
    int ret = reflect_json2struct(json, output, tbl);
    cJSON_Delete(json);
    return ret;
}

int reflect_nstr2struct(const char *jsonstr, size_t len, void *output, const struct reflect_reflection *tbl) {
    cJSON *json = cJSON_ParseWithLength(jsonstr, len);
    if (!json) {
        return REFLECT_ERR_FORMAT;
    }
    int ret = reflect_json2struct(json, output, tbl);
    cJSON_Delete(json);
    return ret;
}

int reflect_json2struct(cJSON *json, void *output, const struct reflect_reflection *tbl) {
    if (!json || !output || !tbl) {
        return REFLECT_ERR_ARGS;
    }
    for (int i = 0; tbl[i].field != NULL; i++) {
        if (!tbl[i].options.deserialized) {
            continue;
        }
        int ret = REFLECT_ERR_SUCCESS;
        cJSON *item = cJSON_GetObjectItemCaseSensitive(json, tbl[i].options.serialized_name ? tbl[i].options.serialized_name : tbl[i].field);
        if (item == NULL) {
            ret = REFLECT_ERR_MISSING_FIELD;
        } else {
            int type0 = map_reflect_type(tbl[i].type);
            if (6 == type0 || map_cjson_type(item->type) == type0) {
                if (deserializer_tbl[tbl[i].type] != NULL) {
                    ret = deserializer_tbl[tbl[i].type](item, output, tbl, i);
                }
            } else {
                ret = REFLECT_ERR_TYPE;
            }
        }
        if (ret != REFLECT_ERR_SUCCESS) {
            deserializer_default_tbl[tbl[i].type](NULL, output, tbl, i);
            if (!tbl[i].options.nullable) {
                return ret;
            }
        }
    }
    return REFLECT_ERR_SUCCESS;
}

int reflect_struct2json(cJSON *obj, void *input, const struct reflect_reflection *tbl) {
    if (!obj || !input || !tbl) {
        return REFLECT_ERR_ARGS;
    }
    for (int i = 0; tbl[i].field != NULL; i++) {
        if (!tbl[i].options.serialized) {
            continue;
        }
        int ret = REFLECT_ERR_SUCCESS;
        cJSON *json = NULL;
        if (serializer_tbl[tbl[i].type] != NULL) {
            ret = serializer_tbl[tbl[i].type](input, tbl, i, &json);
            if (ret != REFLECT_ERR_SUCCESS && !tbl[i].options.nullable) {
                return ret;
            }
        }
        if (ret == REFLECT_ERR_SUCCESS) {
            cJSON_AddItemToObject(obj, tbl[i].options.serialized_name ? tbl[i].options.serialized_name : tbl[i].field, json);
        }
    }
    return REFLECT_ERR_SUCCESS;
}

int reflect_struct2str(char **jsonstr, void *input, const struct reflect_reflection *tbl) {
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        return REFLECT_ERR_MEMORY;
    }
    int ret = reflect_struct2json(json, input, tbl);
    if (ret == REFLECT_ERR_SUCCESS) {
        char *str = cJSON_PrintUnformatted(json);
        if (str == NULL) {
            ret = REFLECT_ERR_MEMORY;
        } else {
            *jsonstr = str;
        }
    }
    cJSON_Delete(json);
    return ret;
}

int reflect_struct2str_preallocated(char *jsonstr, size_t size, void *input, const struct reflect_reflection *tbl) {
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        return REFLECT_ERR_MEMORY;
    }
    int ret = reflect_struct2json(json, input, tbl);
    if (ret == REFLECT_ERR_SUCCESS) {
        if (!cJSON_PrintPreallocated(json, jsonstr, size, 0)) {
            return REFLECT_ERR_OVERFLOW;
        }
    }
    cJSON_Delete(json);
    return ret;
}

void reflect_free_ptr(void *data, const struct reflect_reflection *tbl) {
    reflection_loop(data, tbl, NULL, item_free);
}

static int map_cjson_type(int type) {
    switch (type) {
    case cJSON_False:
    case cJSON_True:
        return 1;
    case cJSON_Number:
        return 2;
    case cJSON_String:
        return 3;
    case cJSON_Array:
        return 4;
    case cJSON_Object:
        return 5;
    case cJSON_Invalid:
    case cJSON_NULL:
    default:
        return 0;
    }
}

static int map_reflect_type(enum reflect_type type) {
    switch (type) {
    case REFLECT_TRUE:
    case REFLECT_FALSE:
        return 1;
    case REFLECT_INTEGER:
    case REFLECT_FLOAT:
        return 2;
    case REFLECT_STRING:
        return 3;
    case REFLECT_ARRAY:
        return 4;
    case REFLECT_OBJECT:
        return 5;
    case REFLECT_ANY:
        return 6;
    case REFLECT_NULL:
    default:
        return 0;
    }
}

static long long get_int_form_ptr(void *ptr, size_t size) {
    if (!ptr) {
        return 0;
    } else if (size == sizeof(char)) {
        return *((char *)ptr);
    } else if (size == sizeof(short)) {
        return *((short *)ptr);
    } else if (size == sizeof(int)) {
        return *((int *)ptr);
    } else if (size == sizeof(long long)) {
        return *((long long *)ptr);
    } else {
        return 0;
    }
}

static const struct reflect_reflection *get_reflection(const char *field, const struct reflect_reflection *tbl, int *index) {
    for (int i = 0; tbl[i].field != NULL; i++) {
        if (strcmp(field, tbl[i].field) == 0) {
            if (index) {
                *index = i;
            }
            return tbl + i;
        }
    }
    return NULL;
}

static void *get_field(void *obj, const char *field, const struct reflect_reflection *tbl, int *index) {
    if (!obj || !field || !tbl) {
        return NULL;
    }
    const struct reflect_reflection *ret = get_reflection(field, tbl, index);
    if (!ret) {
        return NULL;
    }
    return (char *)obj + ret->offset;
}

static void set_field_fast(void *obj, const void *data, const struct reflect_reflection *tbl) {
    if (obj && data && tbl) {
        memcpy((char *)obj + tbl->offset, data, tbl->size);
    }
}

static void set_field_buffered(void *obj, const void *data, size_t data_size, const struct reflect_reflection *tbl) {
    if (obj && data && data_size > 0 && tbl && (tbl->type == REFLECT_STRING || tbl->type == REFLECT_ANY || tbl->type == REFLECT_ARRAY)) {
        memcpy((char *)obj + tbl->offset, data, data_size);
    }
}

static int check_int(long long val, int size) {
    switch (size) {
    case sizeof(char):
        return val > CHAR_MAX || val < CHAR_MIN ? REFLECT_ERR_OVERFLOW : REFLECT_ERR_SUCCESS;
    case sizeof(short):
        return val > SHRT_MAX || val < SHRT_MIN ? REFLECT_ERR_OVERFLOW : REFLECT_ERR_SUCCESS;
    case sizeof(int):
        return val > INT_MAX || val < INT_MIN ? REFLECT_ERR_OVERFLOW : REFLECT_ERR_SUCCESS;
    case sizeof(long long):
        return REFLECT_ERR_SUCCESS;
    default:
        return REFLECT_ERR_OVERFLOW;
    }
}

static int convert_int(long long val, int size, union _reflect_int_val_t *i) {
    int ret = check_int(val, size);
    if (ret != REFLECT_ERR_SUCCESS) {
        return ret;
    }
    switch (size) {
    case sizeof(char):
        i->c = (char)val;
        break;
    case sizeof(short):
        i->s = (short)val;
        break;
    case sizeof(int):
        i->i = (int)val;
        break;
    default:
        i->l = val;
        break;
    }
    return REFLECT_ERR_SUCCESS;
}

static int get_int(cJSON *json, size_t size, union _reflect_int_val_t *i) {
    long long intvalue;
    double floatvalue;
    switch (json->type) {
    case cJSON_True:
        intvalue = 1;
        break;
    case cJSON_False:
        intvalue = 0;
        break;
    case cJSON_Number:
        floatvalue = json->valuedouble;
        if (floatvalue > LLONG_MAX || floatvalue < LLONG_MIN) {
            return REFLECT_ERR_OVERFLOW;
        } else {
            intvalue = (long long)floatvalue;
        }
        break;
    default:
        return REFLECT_ERR_ARGS;
    }
    return convert_int(intvalue, size, i);
}

static void *item_free(void *data, const struct reflect_reflection *item, const struct reflect_reflection *arr) {
    if (item->constructed) {
        free(*(void **)data);
        *(void **)data = NULL;
    }
    return NULL;
}

static void reflection_loop(void *data, const struct reflect_reflection *tbl, const struct reflect_reflection *arr, reflect_loop_cb func) {
    for (int i = 0; tbl[i].field != NULL; i++) {
        char *field = (char *)data + tbl[i].offset;
        if (tbl[i].type == REFLECT_ARRAY) {
            int countIndex = -1;
            void *ptr = get_field(data, tbl[i].arr_count_field, tbl, &countIndex);
            if (!ptr || countIndex < 0) {
                continue;
            }
            long long size = get_int_form_ptr(ptr, tbl[countIndex].size);
            for (long long j = 0; j < size; j++) {
                if (tbl[i].constructed) {
                    reflection_loop(*((char **)field) + j * tbl[i].item_size, tbl[i].reflection, tbl + i, func);
                } else {
                    reflection_loop(field + j * tbl[i].item_size, tbl[i].reflection, tbl + i, func);
                }
            }
        } else if (tbl[i].type == REFLECT_OBJECT) {
            if (tbl[i].constructed) {
                reflection_loop(*((char **)field), tbl[i].reflection, NULL, func);
            } else {
                reflection_loop(field, tbl[i].reflection, NULL, func);
            }
        }
        func(field, tbl + i, arr);
    }
}

static int obj2struct(cJSON *json, void *output, const struct reflect_reflection *tbl) {
    if (!json || !output || !tbl) {
        return REFLECT_ERR_ARGS;
    }
    for (int i = 0; tbl[i].field != NULL; i++) {
        if (!tbl[i].options.deserialized) {
            continue;
        }
        enum reflect_err_code ret = REFLECT_ERR_SUCCESS;
        cJSON *item = cJSON_GetObjectItemCaseSensitive(json, tbl[i].options.serialized_name ? tbl[i].options.serialized_name : tbl[i].field);
        if (!item) {
            ret = REFLECT_ERR_MISSING_FIELD;
        } else {
            int type0 = map_reflect_type(tbl[i].type);
            if (6 == type0 || map_cjson_type(item->type) == type0) {
                if (deserializer_tbl[tbl[i].type] != NULL) {
                    ret = deserializer_tbl[tbl[i].type](item, output, tbl, i);
                }
            } else {
                ret = REFLECT_ERR_TYPE;
            }
        }
        if (ret != REFLECT_ERR_SUCCESS) {
            deserializer_default_tbl[tbl[i].type](NULL, output, tbl, i);
            if (!tbl[i].options.nullable) {
                return ret;
            }
        }
    }
    return REFLECT_ERR_SUCCESS;
}

static int serialize_object(void *input, const struct reflect_reflection *tbl, int index, cJSON **obj) {
    cJSON *json = cJSON_CreateObject();
    int ret = REFLECT_ERR_SUCCESS;
    void *src = (char *)input + tbl[index].offset;
    if (tbl[index].constructed) {
        ret = reflect_struct2json(json, *(void **)src, tbl[index].reflection);
    } else {
        ret = reflect_struct2json(json, src, tbl[index].reflection);
    }
    if (ret == REFLECT_ERR_SUCCESS) {
        *obj = json;
    } else {
        cJSON_Delete(json);
    }
    return ret;
}

static int serialize_array(void *input, const struct reflect_reflection *tbl, int index, cJSON **obj) {
    void *src = (char *)input + tbl[index].offset;
    if (tbl[index].constructed) {
        src = *(void **)src;
        if (src == NULL) {
            return REFLECT_ERR_MISSING_FIELD;
        }
    }
    int countIndex = -1;
    void *ptr = get_field(input, tbl[index].arr_count_field, tbl, &countIndex);
    if (!ptr || countIndex < 0) {
        return REFLECT_ERR_MISSING_FIELD;
    }
    long long size = get_int_form_ptr(ptr, tbl[countIndex].size);
    cJSON *json = cJSON_CreateArray();
    long long successCount = 0;
    for (long long i = 0; i < size; i++) {
        int ret = REFLECT_ERR_SUCCESS;
        cJSON *item;
        if (tbl[index].reflection[0].field[0] == '0') {
            ret = arr_serializer_tbl[tbl[index].reflection[0].type](src + (i * tbl[index].item_size), tbl[index].reflection, 0, &tbl[index], &item);
        } else {
            item = cJSON_CreateObject();
            ret = reflect_struct2json(item, src + (i * tbl[index].item_size), tbl[index].reflection);
        }
        if (ret == REFLECT_ERR_SUCCESS) {
            successCount++;
            cJSON_AddItemToArray(json, item);
        } else {
            cJSON_Delete(item);
        }
    }
    if (successCount == 0) {
        cJSON_Delete(json);
        return REFLECT_ERR_MISSING_FIELD;
    } else {
        *obj = json;
        return REFLECT_ERR_SUCCESS;
    }
}

static int serialize_string(void *input, const struct reflect_reflection *tbl, int index, cJSON **obj) {
    void *src = (char *)input + tbl[index].offset;
    if (tbl[index].constructed) {
        if (*((char **)src) == NULL) {
            return REFLECT_ERR_MISSING_FIELD;
        }
        *obj = cJSON_CreateString(*((char **)src));
    } else {
        *obj = cJSON_CreateString(src);
    }
    return REFLECT_ERR_SUCCESS;
}

static int serialize_any(void *input, const struct reflect_reflection *tbl, int index, cJSON **obj) {
    void *src = (char *)input + tbl[index].offset;
    char * str = NULL;
    if (tbl[index].constructed) {
        if (*((char **)src) == NULL) {
            return REFLECT_ERR_MISSING_FIELD;
        }
        str = *((char **)src);
    } else {
        str = src;
    }
    switch (str[0]) {
    case REFLECT_TRUE:
        *obj = cJSON_CreateBool(1);
        break;
    case REFLECT_FALSE:
        *obj = cJSON_CreateBool(0);
        break;
    case REFLECT_INTEGER:
        {
            long long val;
            memcpy(&val, &str[1], sizeof(long long));
            *obj = cJSON_CreateNumber(val);
        }
        break;
    case REFLECT_FLOAT:
        {
            double val;
            memcpy(&val, &str[1], sizeof(double));
            *obj = cJSON_CreateNumber(val);
        }
        break;
    case REFLECT_STRING:
        *obj = cJSON_CreateString(&str[1]);
        break;
	case REFLECT_ARRAY:
		*obj = cJSON_CreateArray();
		{
			unsigned ptr = 2;
			for (unsigned i = 0; i < (unsigned char)str[1]; i++) {
				switch (str[ptr]) {
				case REFLECT_TRUE:
					cJSON_AddItemToArray(*obj, cJSON_CreateBool(1));
					break;
				case REFLECT_FALSE:
					cJSON_AddItemToArray(*obj, cJSON_CreateBool(0));
					break;
				case REFLECT_INTEGER:
					{
						long long val;
						memcpy(&val, &str[ptr+1], sizeof(long long));
						cJSON_AddItemToArray(*obj, cJSON_CreateNumber(val));
					}
					ptr += sizeof(long long);
					break;
				case REFLECT_FLOAT:
					{
						double val;
						memcpy(&val, &str[ptr+1], sizeof(double));
						cJSON_AddItemToArray(*obj, cJSON_CreateNumber(val));
					}
					ptr += sizeof(double);
					break;
				case REFLECT_STRING:
					cJSON_AddItemToArray(*obj, cJSON_CreateString(&str[ptr+1]));
					ptr += strlen(&str[ptr+1]);
					break;
				default:
					break;
				}
				ptr++;
			}
		}
		break;
    default:
        return REFLECT_ERR_MISSING_FIELD;
    }
    return REFLECT_ERR_SUCCESS;
}

static int serialize_integer(void *input, const struct reflect_reflection *tbl, int index, cJSON **obj) {
    if (tbl[index].size != sizeof(char) && tbl[index].size != sizeof(short) &&
        tbl[index].size != sizeof(int) && tbl[index].size != sizeof(long long)) {
            return REFLECT_ERR_OVERFLOW;
        }
    void *src = (char *)input + tbl[index].offset;
    long long val = get_int_form_ptr(src, tbl[index].size);
    *obj = cJSON_CreateNumber(val);
    return REFLECT_ERR_SUCCESS;
}

static int serialize_float(void *input, const struct reflect_reflection *tbl, int index, cJSON **obj) {
    if (tbl[index].size != sizeof(double) && tbl[index].size != sizeof(float)) {
        return REFLECT_ERR_OVERFLOW;
    }
    void *src = (char *)input + tbl[index].offset;
    double value = 0;
    if (tbl[index].size == sizeof(double)) {
        value = *(double *)src;
    } else {
        char convert_cache[20];
        snprintf(convert_cache, sizeof(convert_cache), "%f", *(float*)src - (int)(*(float *)src));
        char *convert_pend;
        value = strtod(convert_cache, &convert_pend) + (int)(*(float *)src);
    }
    *obj = cJSON_CreateNumber(value);
    return REFLECT_ERR_SUCCESS;
}

static int serialize_bool(void *input, const struct reflect_reflection *tbl, int index, cJSON **obj) {
    if (tbl[index].size != sizeof(char) && tbl[index].size != sizeof(short) &&
        tbl[index].size != sizeof(int) && tbl[index].size != sizeof(long long)) {
            return REFLECT_ERR_OVERFLOW;
        }
    void *src = (char *)input + tbl[index].offset;
    if (tbl[index].size == sizeof(char)) {
        *obj = cJSON_CreateBool(*((char *)src));
    } else if (tbl[index].size == sizeof(short)) {
        *obj = cJSON_CreateBool(*((short *)src));
    } else if (tbl[index].size == sizeof(int)) {
        *obj = cJSON_CreateBool(*((int *)src));
    } else {
        *obj = cJSON_CreateBool(*((long long *)src));
    }
    return REFLECT_ERR_SUCCESS;
}

static int serialize_arr_string(void *input, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect, cJSON **obj) {
    return serialize_string(input, tbl, index, obj);
}

static int serialize_arr_integer(void *input, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect, cJSON **obj) {
    if (arr_reflect->item_size != sizeof(char) && arr_reflect->item_size != sizeof(short) &&
        arr_reflect->item_size != sizeof(int) && arr_reflect->item_size != sizeof(long long)) {
            return REFLECT_ERR_OVERFLOW;
        }
    void *src = (char *)input + tbl[index].offset;
    long long val = get_int_form_ptr(src, arr_reflect->item_size);
    *obj = cJSON_CreateNumber(val);
    return REFLECT_ERR_SUCCESS;
}

static int serialize_arr_float(void *input, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect, cJSON **obj) {
    if (arr_reflect->item_size != sizeof(double) && arr_reflect->item_size != sizeof(float)) {
        return REFLECT_ERR_OVERFLOW;
    }
    void *src = (char *)input + tbl[index].offset;
    double value = 0;
    if (arr_reflect->item_size == sizeof(double)) {
        value = *(double *)src;
    } else {
        char convert_cache[20];
        char *convert_pend;
        snprintf(convert_cache, sizeof(convert_cache), "%f", *(float *)src - (int)(*(float *)src));
        value = strtod(convert_cache, &convert_pend) + (int)(*(float *)src);
    }
    *obj = cJSON_CreateNumber(value);
    return REFLECT_ERR_SUCCESS;
}

static int serialize_arr_bool(void *input, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect, cJSON **obj) {
    if (arr_reflect->item_size != sizeof(char) && arr_reflect->item_size != sizeof(short) &&
        arr_reflect->item_size != sizeof(int) && arr_reflect->item_size != sizeof(long long)) {
            return REFLECT_ERR_OVERFLOW;
        }
    void *src = (char *)input + tbl[index].offset;
    if (arr_reflect->item_size == sizeof(char)) {
        *obj = cJSON_CreateBool(*((char *)src));
    } else if (arr_reflect->item_size == sizeof(short)) {
        *obj = cJSON_CreateBool(*((short *)src));
    } else if (arr_reflect->item_size == sizeof(int)) {
        *obj = cJSON_CreateBool(*((int *)src));
    } else {
        *obj = cJSON_CreateBool(*((long long *)src));
    }
    return REFLECT_ERR_SUCCESS;
}

static int deserialize_object(cJSON *json, void *output, const struct reflect_reflection *tbl, int index) {
    void *temp;
    if (tbl[index].constructed) {
        temp = cJSON_malloc(tbl[index].item_size);
        if (!temp) {
            return REFLECT_ERR_MEMORY;
        }
    } else {
        temp = output + tbl[index].offset;
    }
    memset(temp, 0, tbl[index].item_size);
    int ret = obj2struct(json, temp, tbl[index].reflection);
    if (tbl[index].constructed) {
        if (ret != REFLECT_ERR_SUCCESS) {
            cJSON_free(temp);
            temp = NULL;
        }
        set_field_fast(output, &temp, tbl + index);
    }
    return ret;
}

static int deserialize_array(cJSON *json, void *output, const struct reflect_reflection *tbl, int index) {
    size_t arraySize = cJSON_GetArraySize(json);
    if (arraySize == 0) {
        return REFLECT_ERR_SUCCESS;
    }
    int countIndex = -1;
    get_field(output, tbl[index].arr_count_field, tbl, &countIndex);
    if (countIndex < 0) {
        return REFLECT_ERR_MISSING_FIELD;
    }
    void *mem = NULL;
    if (tbl[index].constructed) {
        mem = malloc(arraySize * tbl[index].item_size);
        if (!mem) {
            return REFLECT_ERR_MEMORY;
        }
    } else {
        mem = (output + tbl[index].offset);
        size_t count = tbl[index].size / tbl[index].item_size;
        arraySize = count > arraySize ? arraySize : count;
    }
    memset(mem, 0, arraySize * tbl[index].item_size);

    long long successCount = 0;
    for (int j = 0; j < arraySize; j++) {
        cJSON *item = cJSON_GetArrayItem(json, j);
        if (item == NULL) {
            continue;
        }
        int ret;
        if (tbl[index].reflection[0].field[0] == '0') {
            ret = arr_deserializer_tbl[tbl[index].reflection[0].type](item, mem + (successCount * tbl[index].item_size), tbl[index].reflection, 0, tbl + index);
        } else {
            ret = obj2struct(item, mem + (successCount * tbl[index].item_size), tbl[index].reflection);
        }
        if (ret == REFLECT_ERR_SUCCESS) {
            successCount++;
        }
    }
    union _reflect_int_val_t val;
    if (convert_int(successCount, tbl[countIndex].size, &val) != REFLECT_ERR_SUCCESS) {
        successCount = 0;
    }
    if (successCount == 0) {
        set_field_fast(output, &successCount, tbl + countIndex);
        if (tbl[index].constructed) {
            free(mem);
            mem = NULL;
        }
        set_field_fast(output, &mem, tbl + index);
        return REFLECT_ERR_MISSING_FIELD;
    } else {
        set_field_fast(output, &val, tbl + countIndex);
        if (tbl[index].constructed) {
            set_field_fast(output, &mem, tbl + index);
        }
        return REFLECT_ERR_SUCCESS;
    }
}

static int deserialize_string(cJSON *json, void *output, const struct reflect_reflection *tbl, int index) {
    const char *str = cJSON_GetStringValue(json);
    if (str == NULL) {
        return REFLECT_ERR_MISSING_FIELD;
    }
    if (tbl[index].constructed) {
        char *dst = (char *)cJSON_malloc(strlen(str) + 1);
        if (!dst) {
            return REFLECT_ERR_MEMORY;
        }
        strcpy(dst, str);
        set_field_fast(output, &dst, tbl + index);
    } else {
        memset(output + tbl[index].offset, 0, tbl[index].size);
        set_field_buffered(output, str, strlen(str) >= tbl[index].size ? tbl[index].size - 1 : strlen(str), tbl + index);
    }
    return REFLECT_ERR_SUCCESS;
}

static int deserialize_any(cJSON *json, void *output, const struct reflect_reflection *tbl, int index) {
    char *str = NULL;
    int size = 0;
    switch (json->type) {
    case cJSON_False:
        size = 1;
        str = reflect_false2any();
        break;
    case cJSON_True:
        size = 1;
        str = reflect_true2any();
        break;
    case cJSON_Number:
        {
            double val = cJSON_GetNumberValue(json);
            long long ival = (long long)val;
            if (val > LLONG_MAX || val < LLONG_MIN || ival != val) {
                size = 1 + sizeof(double);
                str = reflect_float2any(val);
            } else {
                size = 1 + sizeof(long long);
                str = reflect_int2any(ival);
            }
        }
        break;
    case cJSON_String:
        size = 2 + strlen(cJSON_GetStringValue(json));
        str = reflect_str2any(cJSON_GetStringValue(json));
        break;
    case cJSON_NULL:
        size = 1;
        str = reflect_null2any();
        break;
    case cJSON_Array:
		{
			size = 2;
			int arraySize = cJSON_GetArraySize(json);
			if (arraySize > 255) {
				return REFLECT_ERR_MEMORY;
			}
			for (int i = 0; i < arraySize; i++) {
				cJSON *item = cJSON_GetArrayItem(json, i);
				switch (item->type) {
				case cJSON_False:
				case cJSON_True:
				case cJSON_NULL:
				default:
					size++;
					break;
				case cJSON_Number:
					size += 1 + sizeof(double);
					break;
				case cJSON_String:
					size += 2 + strlen(cJSON_GetStringValue(item));
					break;
				}
			}
			str = malloc(size);
			if (str == NULL) {
				return REFLECT_ERR_MEMORY;
			}
			str[0] = REFLECT_ARRAY;
			str[1] = arraySize;
			char *ptr = str + 2;
			for (int i = 0; i < arraySize; i++) {
				cJSON *item = cJSON_GetArrayItem(json, i);
				switch (item->type) {
				case cJSON_False:
					*ptr++ = REFLECT_FALSE;
					break;
				case cJSON_True:
					*ptr++ = REFLECT_TRUE;
					break;
				case cJSON_Number:
					{
						double val = cJSON_GetNumberValue(item);
						long long ival = (long long)val;
						if (val > LLONG_MAX || val < LLONG_MIN || ival != val) {
							*ptr++ = REFLECT_FLOAT;
							memcpy(ptr, &val, sizeof(double));
							ptr += sizeof(double);
						} else {
							*ptr++ = REFLECT_INTEGER;
							memcpy(ptr, &ival, sizeof(long long));
							ptr += sizeof(long long);
						}
					}
					break;
				case cJSON_String:
					*ptr++ = REFLECT_STRING;
					strcpy(ptr, cJSON_GetStringValue(item));
					ptr += strlen(cJSON_GetStringValue(item)) + 1;
					break;
				default:
					*ptr++ = REFLECT_NULL;
					break;
				}
            }
		}
		break;
    case cJSON_Object:
    case cJSON_Invalid:
        break;
    }
    if (str == NULL) {
        return REFLECT_ERR_MISSING_FIELD;
    }
    if (tbl[index].constructed) {
        set_field_fast(output, &str, tbl + index);
    } else {
        memset(output + tbl[index].offset, 0, tbl[index].size);
        set_field_buffered(output, str, size >= tbl[index].size ? tbl[index].size - 1 : size, tbl + index);
        free(str);
    }
    return REFLECT_ERR_SUCCESS;
}

static int deserialize_integer(cJSON *json, void *output, const struct reflect_reflection *tbl, int index) {
    union _reflect_int_val_t value;
    int ret = get_int(json, tbl[index].size, &value);
    if (ret == REFLECT_ERR_SUCCESS) {
        set_field_fast(output, &value, tbl + index);
    }
    return ret;
}

static int deserialize_float(cJSON *json, void *output, const struct reflect_reflection *tbl, int index) {
    if (tbl[index].size == sizeof(double)) {
        double temp = json->valuedouble;
        set_field_fast(output, &temp, tbl + index);
    } else if (tbl[index].size == sizeof(float)) {
        float temp = (float)(json->valuedouble);
        set_field_fast(output, &temp, tbl + index);
    } else {
        return REFLECT_ERR_OVERFLOW;
    }
    return REFLECT_ERR_SUCCESS;
}

static int deserialize_bool(cJSON *json, void *output, const struct reflect_reflection *tbl, int index) {
    union _reflect_int_val_t value;
    int ret = get_int(json, tbl[index].size, &value);
    if (ret == REFLECT_ERR_SUCCESS) {
        set_field_fast(output, &value, tbl + index);
    }
    return ret;
}

static int deserialize_arr_string(cJSON *json, void *output, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect) {
    const char *str = cJSON_GetStringValue(json);
    if (str == NULL) {
        return REFLECT_ERR_MISSING_FIELD;
    }
    if (tbl[index].constructed) {
        char *dst = (char *)cJSON_malloc(strlen(str) + 1);
        if (dst == NULL) {
            return REFLECT_ERR_MEMORY;
        }
        strcpy(dst, str);
        set_field_fast(output, &dst, tbl + index);
    } else {
        size_t max_size = arr_reflect->item_size;
        memset(output + tbl[index].offset, 0, max_size);
        set_field_buffered(output, str, strlen(str) >= max_size ? max_size - 1 : strlen(str), tbl + index);
    }
    return REFLECT_ERR_SUCCESS;
}

static int deserialize_arr_integer(cJSON *json, void *output, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect) {
    union _reflect_int_val_t value;
    int ret = get_int(json, arr_reflect->item_size, &value);
    if (ret != REFLECT_ERR_SUCCESS) {
        return ret;
    }
    set_field_fast(output, &value, tbl + index);
    return ret;
}

static int deserialize_arr_float(cJSON *json, void *output, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect) {
    if (arr_reflect->item_size == sizeof(double)) {
        double v = json->valuedouble;
        set_field_fast(output, &v, reflect_double);
    } else if (arr_reflect->item_size == sizeof(float)) {
        float v = (float)(json->valuedouble);
        set_field_fast(output, &v, reflect_float);
    } else {
        return REFLECT_ERR_OVERFLOW;
    }
    return REFLECT_ERR_SUCCESS;
}

static int deserialize_arr_bool(cJSON *json, void *output, const struct reflect_reflection *tbl, int index, const struct reflect_reflection *arr_reflect) {
    return deserialize_arr_integer(json, output, tbl, index, arr_reflect);
}

static int deserialize_object_default(cJSON *json, void *output, const struct reflect_reflection *tbl, int index) {
    for (int i = 0; tbl[i].reflection[i].field != NULL; i++) {
        if (!tbl[i].options.deserialized) {
            continue;
        }
        deserializer_default_tbl[tbl[index].reflection[i].type](NULL, output, tbl[index].reflection, i);
    }
    if (tbl->constructed) {
        void *temp = NULL;
        set_field_fast(output, &temp, tbl + index);
    }
    return REFLECT_ERR_SUCCESS;
}

static int deserialize_array_default(cJSON *json, void *output, const struct reflect_reflection *tbl, int index) {
    if (tbl->constructed) {
        void *temp = NULL;
        set_field_fast(output, &temp, tbl + index);
    }
    return REFLECT_ERR_SUCCESS;
}

static int deserialize_string_default(cJSON *json, void *output, const struct reflect_reflection *tbl, int index) {
    if (tbl->constructed) {
        char *temp = NULL;
        set_field_fast(output, &temp, tbl + index);
    }
    return REFLECT_ERR_SUCCESS;
}

static int deserialize_integer_default(cJSON *json, void *output, const struct reflect_reflection *tbl, int index) {
    if (tbl[index].size != sizeof(char) && tbl[index].size != sizeof(short) &&
        tbl[index].size != sizeof(int) && tbl[index].size != sizeof(long long)) {
            return REFLECT_ERR_OVERFLOW;
        }
    union _reflect_int_val_t ret;
    convert_int(0, tbl[index].size, &ret);
    set_field_fast(output, &ret, tbl + index);
    return REFLECT_ERR_SUCCESS;
}

static int deserialize_float_default(cJSON *json, void *output, const struct reflect_reflection *tbl, int index) {
    if (tbl[index].size == sizeof(double)) {
        double temp = 0;
        set_field_fast(output, &temp, tbl + index);
    } else if (tbl[index].size == sizeof(float)) {
        float temp = 0;
        set_field_fast(output, &temp, tbl + index);
    } else {
        return REFLECT_ERR_OVERFLOW;
    }
    return REFLECT_ERR_SUCCESS;
}

char * reflect_null2any() {
    char * result = malloc(1);
    result[0] = REFLECT_NULL;
    return result;
}

char * reflect_true2any() {
    char * result = malloc(1);
    result[0] = REFLECT_TRUE;
    return result;
}

char * reflect_false2any() {
    char * result = malloc(1);
    result[0] = REFLECT_FALSE;
    return result;
}

char * reflect_bool2any(bool val) {
    char * result = malloc(1);
    if (val) {
        result[0] = REFLECT_TRUE;
    } else {
        result[0] = REFLECT_FALSE;
    }
    return result;
}

char * reflect_int2any(long long val) {
    char * result = malloc(1 + sizeof(long long));
    result[0] = REFLECT_INTEGER;
    memcpy(&result[1], &val, sizeof(long long));
    return result;
}

char * reflect_float2any(double val) {
    char * result = malloc(1 + sizeof(double));
    result[0] = REFLECT_FLOAT;
    memcpy(&result[1], &val, sizeof(double));
    return result;
}

char * reflect_str2any(const char * val) {
    if (val == NULL) {
        return reflect_null2any();
    }
    char * result = malloc(2 + strlen(val));
    result[0] = REFLECT_STRING;
    strcpy(&result[1], val);
    return result;
}

bool reflect_any2bool(const char *str, bool *val) {
    if (str == NULL) {
        return false;
    } else if (str[0] == REFLECT_TRUE) {
        *val = true;
        return true;
    } else if (str[0] == REFLECT_FALSE) {
        *val = false;
        return true;
    } else {
        return false;
    }
}

bool reflect_any2int(const char *str, long long *val) {
    if (str == NULL || str[0] != REFLECT_INTEGER) {
        return false;
    }
    memcpy(val, &str[1], sizeof(long long));
    return true;
}

bool reflect_any2float(const char *str, double *val) {
    if (str == NULL || str[0] != REFLECT_FLOAT) {
        return false;
    }
    memcpy(val, &str[1], sizeof(double));
    return true;
}

bool reflect_any2str(const char *str, char ** val) {
    if (str == NULL || str[0] != REFLECT_STRING) {
        return false;
    }
    *val = (char *)&str[1];
    return true;
}

int reflect_any2array(const char *str, char *** val) {
	if (str == NULL || str[0] != REFLECT_ARRAY) {
		return -1;
	}
	int count = str[1];
	*val = malloc(count * sizeof(char **));
	char *ptr = (char *)str + 2;
	for (int i = 0; i < count; i++) {
		(*val)[i] = ptr;
		switch (ptr[0]) {
		case REFLECT_FALSE:
		case REFLECT_TRUE:
		case REFLECT_NULL:
		default:
			ptr++;
			break;
		case REFLECT_INTEGER:
			ptr += 1 + sizeof(long long);
			break;
		case REFLECT_FLOAT:
			ptr += 1 + sizeof(double);
			break;
		case REFLECT_STRING:
			ptr += 1 + strlen(ptr);
			break;
		}
	}
	return count;
}

enum reflect_type reflect_get_any_type(const char *str) {
    if (str == NULL) {
        return REFLECT_NULL;
    }
    return str[0];
}
