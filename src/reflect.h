#include "cJSON.h"
#include <stdbool.h>
#include <stddef.h>

enum reflect_err_code {
    REFLECT_ERR_SUCCESS       = 0,  // success
    REFLECT_ERR_MEMORY        = 1,  // malloc failed
    REFLECT_ERR_TYPE          = 2,  // type matching error
    REFLECT_ERR_MISSING_FIELD = 3,  // field not found
    REFLECT_ERR_FORMAT        = 4,  // input json string format error
    REFLECT_ERR_ARGS          = 5,  // args error
    REFLECT_ERR_OVERFLOW      = 6   // value overflow
};

enum reflect_type {
    REFLECT_OBJECT = 0,
    REFLECT_ARRAY,
    REFLECT_STRING,
    REFLECT_INTEGER,
    REFLECT_FLOAT,
    REFLECT_TRUE,
    REFLECT_FALSE,
    REFLECT_ANY,
    REFLECT_NULL
};

struct reflect_option {
    bool nullable;
    bool serialized;
    bool deserialized;
    const char *serialized_name;
};

struct reflect_reflection {
    const char *field;
    size_t offset;
    size_t size;
    enum reflect_type type;
    const struct reflect_reflection *reflection;
    const char *arr_count_field;
    size_t item_size;
    bool constructed;
    const struct reflect_option options;
};

extern const struct reflect_reflection reflect_int[];
extern const struct reflect_reflection reflect_string_ptr[];
extern const struct reflect_reflection reflect_string_preallocated[];
extern const struct reflect_reflection reflect_real[];
extern const struct reflect_reflection reflect_double[];
extern const struct reflect_reflection reflect_float[];
extern const struct reflect_reflection reflect_bool[];

const char *reflect_err_str(int code);
int reflect_str2struct(const char *jsonstr, void *output, const struct reflect_reflection *tbl);
int reflect_nstr2struct(const char *jsonstr, size_t len, void *output, const struct reflect_reflection *tbl);
int reflect_json2struct(cJSON *json, void *output, const struct reflect_reflection *tbl);

int reflect_struct2str(char **jsonstr, void *input, const struct reflect_reflection *tbl);
int reflect_struct2str_preallocated(char *jsonstr, size_t size, void *input, const struct reflect_reflection *tbl);
int reflect_struct2json(cJSON *obj, void *input, const struct reflect_reflection *tbl);

void reflect_free_ptr(void *data, const struct reflect_reflection *tbl);

char * reflect_null2any();
char * reflect_true2any();
char * reflect_false2any();
char * reflect_bool2any(bool val);
char * reflect_int2any(long long val);
char * reflect_float2any(double val);
char * reflect_str2any(const char * val);
bool reflect_any2bool(const char *str, bool *val);
bool reflect_any2int(const char *str, long long *val);
bool reflect_any2float(const char *str, double *val);
bool reflect_any2str(const char *str, char ** val);
int reflect_any2array(const char *str, char *** val);
enum reflect_type reflect_get_any_type(const char *str);

#define _REFLECT_SIZE(type, field)          sizeof(((type *)0)->field)
#define _REFLECT_PTR_ITEM_SIZE(type, field) sizeof(*(((type *)0)->field))
#define _REFLECT_EX_DEFAULT                 {.nullable = true, .serialized = true, .deserialized = true, .serialized_name = NULL}

#define REFLECT_DECLARE(type, field, jtype, r, item_size, constructed) \
    {#field, offsetof(type, field), _REFLECT_SIZE(type, field), jtype, r, NULL, item_size, constructed,\
    {.nullable = true, .serialized = true, .deserialized = true, .serialized_name = NULL}}
#define REFLECT_DECLARE_EX(type, field, jtype, r, item_size, constructed, ...) \
    {#field, offsetof(type, field), _REFLECT_SIZE(type, field), jtype, r, NULL, item_size, constructed,\
    {.nullable = true, .serialized = true, .deserialized = true, .serialized_name = NULL, __VA_ARGS__}}

#define REFLECT_DECLARE_PTR(type, field, jtype, r) \
    {#field, offsetof(type, field), _REFLECT_SIZE(type, field), jtype, r, NULL, _REFLECT_PTR_ITEM_SIZE(type, field), true,\
    {.nullable = true, .serialized = true, .deserialized = true, .serialized_name = NULL}}
#define REFLECT_DECLARE_PTR_EX(type, field, jtype, r, ...) \
    {#field, offsetof(type, field), _REFLECT_SIZE(type, field), jtype, r, NULL, _REFLECT_PTR_ITEM_SIZE(type, field), true,\
    {.nullable = true, .serialized = true, .deserialized = true, .serialized_name = NULL, __VA_ARGS__}}

#define REFLECT_DECLARE_BASIC(type, field, jtype, r) \
    REFLECT_DECLARE(type, field, jtype, r, 0, false)
#define REFLECT_DECLARE_BASIC_EX(type, field, jtype, r, ...) \
    REFLECT_DECLARE_EX(type, field, jtype, r, 0, false, ##__VA_ARGS__)

#define REFLECT_DECLARE_ARRAY(type, field, item_count_field, r, constructed)                     \
    {#field, offsetof(type, field), _REFLECT_SIZE(type, field), REFLECT_ARRAY, r,                \
    _REFLECT_SIZE(type, item_count_field) > 0 ? #item_count_field : NULL,                        \
    _REFLECT_PTR_ITEM_SIZE(type, field), constructed,                                            \
    {.nullable = true, .serialized = true, .deserialized = true, .serialized_name = NULL}},      \
    {#item_count_field, offsetof(type, item_count_field), _REFLECT_SIZE(type, item_count_field), \
    REFLECT_INTEGER, reflect_int, NULL, 0, false, {0}}
#define REFLECT_DECLARE_ARRAY_EX(type, field, item_count_field, r, constructed, ...)             \
    {#item_count_field, offsetof(type, item_count_field), _REFLECT_SIZE(type, item_count_field), \
    REFLECT_INTEGER, reflect_int, NULL, 0, false, {0}},                                          \
    {#field, offsetof(type, field), _REFLECT_SIZE(type, field), REFLECT_ARRAY, r,                \
    _REFLECT_SIZE(type, item_count_field) > 0 ? #item_count_field : NULL,                        \
    _REFLECT_PTR_ITEM_SIZE(type, field), constructed,                                            \
    {.nullable = true, .serialized = true, .deserialized = true, .serialized_name = NULL, __VA_ARGS__}}

#define REFLECT_DECLARE_ARRAY_PTR(type, field, item_count_field, r) \
    REFLECT_DECLARE_ARRAY(type, field, item_count_field, r, true)
#define REFLECT_DECLARE_ARRAY_PTR_EX(type, field, item_count_field, r, ...) \
    REFLECT_DECLARE_ARRAY_EX(type, field, item_count_field, r, true, ##__VA_ARGS__)

#define REFLECT_DECLARE_ARRAY_PREALLOCATED(type, field, item_count_field, r) \
    REFLECT_DECLARE_ARRAY(type, field, item_count_field, r, false)
#define REFLECT_DECLARE_ARRAY_PREALLOCATED_EX(type, field, item_count_field, r, ...) \
    REFLECT_DECLARE_ARRAY_EX(type, field, item_count_field, r, false, ##__VA_ARGS__)

#define REFLECT_DECLARE_INT(type, field)              \
    REFLECT_DECLARE_BASIC(type, field, REFLECT_INTEGER, reflect_int)
#define REFLECT_DECLARE_FLOAT(type, field)            \
    REFLECT_DECLARE_BASIC(type, field, REFLECT_FLOAT, reflect_real)
#define REFLECT_DECLARE_BOOL(type, field)             \
    REFLECT_DECLARE_BASIC(type, field, REFLECT_TRUE, reflect_bool)
#define REFLECT_DECLARE_STR_PREALLOCATED(type, field) \
    REFLECT_DECLARE(type, field, REFLECT_STRING, reflect_string_preallocated, sizeof(char), false)
#define REFLECT_DECLARE_STR_PTR(type, field)          \
    REFLECT_DECLARE(type, field, REFLECT_STRING, reflect_string_ptr, sizeof(char), true)
#define REFLECT_DECLARE_STR(type, field)              \
    REFLECT_DECLARE_STR_PREALLOCATED(type, field)
#define REFLECT_DECLARE_ANY_PREALLOCATED(type, field) \
    REFLECT_DECLARE(type, field, REFLECT_ANY, reflect_string_preallocated, sizeof(char), false)
#define REFLECT_DECLARE_ANY_PTR(type, field)          \
    REFLECT_DECLARE(type, field, REFLECT_ANY, reflect_string_ptr, sizeof(char), true)
#define REFLECT_DECLARE_ANY(type, field)              \
    REFLECT_DECLARE_ANY_PREALLOCATED(type, field)
#define REFLECT_DECLARE_OBJECT(type, field, r)        \
    REFLECT_DECLARE_BASIC(type, field, REFLECT_OBJECT, r)
#define REFLECT_DECLARE_OBJECT_PTR(type, field, r)    \
    {#field, offsetof(type, field), _REFLECT_SIZE(type, field), REFLECT_OBJECT, r, NULL, \
    _REFLECT_PTR_ITEM_SIZE(type, field), true,                                           \
    {.nullable = true, .serialized = true, .deserialized = true, .serialized_name = NULL}}

#define REFLECT_DECLARE_INT_EX(type, field, ...)            \
    REFLECT_DECLARE_BASIC_EX(type, field, REFLECT_INTEGER, reflect_int, ##__VA_ARGS__)
#define REFLECT_DECLARE_FLOAT_EX(type, field, ...)          \
    REFLECT_DECLARE_BASIC_EX(type, field, REFLECT_FLOAT, reflect_float, ##__VA_ARGS__)
#define REFLECT_DECLARE_BOOL_EX(type, field, ...)           \
    REFLECT_DECLARE_BASIC_EX(type, field, REFLECT_TRUE, reflect_bool, ##__VA_ARGS__)
#define REFLECT_DECLARE_STR_EX(type, field, ...)            \
    REFLECT_DECLARE_EX(type, field, REFLECT_STRING, reflect_string_preallocated, sizeof(char), false, ##__VA_ARGS__)
#define REFLECT_DECLARE_STR_PTR_EX(type, field, ...)        \
    REFLECT_DECLARE_EX(type, field, REFLECT_STRING, reflect_string_ptr, sizeof(char), true, ##__VA_ARGS__)
#define REFLECT_DECLARE_OBJECT_EX(type, field, r, ...)      \
    REFLECT_DECLARE_BASIC_EX(type, field, REFLECT_OBJECT, r, ##__VA_ARGS__)
#define REFLECT_DECLARE_OBJECT_PTR_EX(type, field, r, ...)              \
    {#field, offsetof(type, field), _REFLECT_SIZE(type, field),         \
    REFLECT_OBJECT, r, NULL, _REFLECT_PTR_ITEM_SIZE(type, field), true, \
    {.nullable = true, .serialized = true, .deserialized = true, .serialized_name = NULL, __VA_ARGS__}}

#define REFLECT_DECLARE_ARRAY_PTR_INT(type, field, item_count_field)        \
    REFLECT_DECLARE_ARRAY_PTR(type, field, item_count_field, reflect_int)
#define REFLECT_DECLARE_ARRAY_PTR_FLOAT(type, field, item_count_field)      \
    REFLECT_DECLARE_ARRAY_PTR(type, field, item_count_field, reflect_float)
#define REFLECT_DECLARE_ARRAY_PTR_BOOL(type, field, item_count_field)       \
    REFLECT_DECLARE_ARRAY_PTR(type, field, item_count_field, reflect_bool)
#define REFLECT_DECLARE_ARRAY_PTR_STR_PTR(type, field, item_count_field)    \
    REFLECT_DECLARE_ARRAY_PTR(type, field, item_count_field, reflect_string_ptr)
#define REFLECT_DECLARE_ARRAY_PTR_OBJECT(type, field, item_count_field, r)  \
    REFLECT_DECLARE_ARRAY_PTR(type, field, item_count_field, r)

#define REFLECT_DECLARE_ARRAY_PTR_INT_EX(type, field, item_count_field, ...)        \
    REFLECT_DECLARE_ARRAY_PTR_EX(type, field, item_count_field, reflect_int, ##__VA_ARGS__)
#define REFLECT_DECLARE_ARRAY_PTR_FLOAT_EX(type, field, item_count_field, ...)      \
    REFLECT_DECLARE_ARRAY_PTR_EX(type, field, item_count_field, reflect_float, ##__VA_ARGS__)
#define REFLECT_DECLARE_ARRAY_PTR_BOOL_EX(type, field, item_count_field, ...)       \
    REFLECT_DECLARE_ARRAY_PTR_EX(type, field, item_count_field, reflect_bool, ##__VA_ARGS__)
#define REFLECT_DECLARE_ARRAY_PTR_STR_PTR_EX(type, field, item_count_field, ...)    \
    REFLECT_DECLARE_ARRAY_PTR_EX(type, field, item_count_field, reflect_string_ptr, ##__VA_ARGS__)
#define REFLECT_DECLARE_ARRAY_PTR_OBJECT_EX(type, field, item_count_field, r, ...)  \
    REFLECT_DECLARE_ARRAY_PTR_EX(type, field, item_count_field, r, ##__VA_ARGS__)

#define REFLECT_DECLARE_ARRAY_INT(type, field, item_count_field)        \
    REFLECT_DECLARE_ARRAY_PREALLOCATED(type, field, item_count_field, reflect_int)
#define REFLECT_DECLARE_ARRAY_FLOAT(type, field, item_count_field)      \
    REFLECT_DECLARE_ARRAY_PREALLOCATED(type, field, item_count_field, reflect_float)
#define REFLECT_DECLARE_ARRAY_BOOL(type, field, item_count_field)       \
    REFLECT_DECLARE_ARRAY_PREALLOCATED(type, field, item_count_field, reflect_bool)
#define REFLECT_DECLARE_ARRAY_STR(type, field, item_count_field)        \
    REFLECT_DECLARE_ARRAY_PREALLOCATED(type, field, item_count_field, reflect_string_preallocated)
#define REFLECT_DECLARE_ARRAY_STR_PTR(type, field, item_count_field)    \
    REFLECT_DECLARE_ARRAY_PREALLOCATED(type, field, item_count_field, reflect_string_ptr)
#define REFLECT_DECLARE_ARRAY_OBJECT(type, field, item_count_field, r)  \
    REFLECT_DECLARE_ARRAY_PREALLOCATED(type, field, item_count_field, r)

#define REFLECT_DECLARE_ARRAY_INT_EX(type, field, item_count_field, ...)        \
    REFLECT_DECLARE_ARRAY_PREALLOCATED_EX(type, field, item_count_field, reflect_int, ##__VA_ARGS__)
#define REFLECT_DECLARE_ARRAY_FLOAT_EX(type, field, item_count_field, ...)      \
    REFLECT_DECLARE_ARRAY_PREALLOCATED_EX(type, field, item_count_field, reflect_float, ##__VA_ARGS__)
#define REFLECT_DECLARE_ARRAY_BOOL_EX(type, field, item_count_field, ...)       \
    REFLECT_DECLARE_ARRAY_PREALLOCATED_EX(type, field, item_count_field, reflect_bool, ##__VA_ARGS__)
#define REFLECT_DECLARE_ARRAY_STR_EX(type, field, item_count_field, ...)        \
    REFLECT_DECLARE_ARRAY_PREALLOCATED_EX(type, field, item_count_field, reflect_string_preallocated, ##__VA_ARGS__)
#define REFLECT_DECLARE_ARRAY_STR_PTR_EX(type, field, item_count_field, ...)    \
    REFLECT_DECLARE_ARRAY_PREALLOCATED_EX(type, field, item_count_field, reflect_string_ptr, ##__VA_ARGS__)
#define REFLECT_DECLARE_ARRAY_OBJECT_EX(type, field, item_count_field, r, ...)  \
    REFLECT_DECLARE_ARRAY_PREALLOCATED_EX(type, field, item_count_field, r, ##__VA_ARGS__)

#define REFLECT_DECLARE_END() {NULL, 0, 0, REFLECT_NULL, NULL, NULL, 0, false, {0}}
