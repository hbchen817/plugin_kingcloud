#pragma once
#include "map.h"
#include <stdbool.h>

const static int kUnknown     = 0x00;
const static int kNull        = 0x01;
const static int kFalse       = 0x02;
const static int kTrue        = 0x03;
const static int kFloat       = 0x04;
const static int kDouble      = 0x04;
const static int kSingle      = 0x05;
const static int kInteger     = 0x08;
const static int kInt64       = 0x08;
const static int kInt32       = 0x09;
const static int kInt16       = 0x0A;
const static int kInt8        = 0x0B;
const static int kUint64      = 0x0C;
const static int kUint32      = 0x0D;
const static int kUint16      = 0x0E;
const static int kUint8       = 0x0F;
const static int kCustom      = 0x00010000;
const static int kString      = 0x02000000;
const static int kConstString = 0x03000000;
const static int kSequence    = 0x04000000;
const static int kMap         = 0x08000000;

struct _map_any;

typedef struct any {
    int type;
    union {
        long long        ival;
        float            fval;
        double           dval;
        char            *sval;
        void            *cval;
        struct _map_any *mval;
        struct any      *aval;
    } u;
} any_t;

bool any_is_null(const any_t *val);
bool any_is_bool(const any_t *val);
bool any_is_true(const any_t *val);
bool any_is_false(const any_t *val);
bool any_is_float(const any_t *val);
bool any_is_single(const any_t *val);
bool any_is_double(const any_t *val);
bool any_is_integer(const any_t *val);
bool any_is_signed(const any_t *val);
bool any_is_number(const any_t *val);
bool any_is_custom(const any_t *val);
bool any_is_plain(const any_t *val);
bool any_is_string(const any_t *val);
bool any_is_valid_string(const any_t *val);
bool any_is_const_string(const any_t *val);
bool any_is_sequence(const any_t *val);
bool any_is_map(const any_t *val);
int  any_length(const any_t *val);

void any_set_integer(any_t *val, long long i);
void any_set_string(any_t *val, const char *str);
void any_set_string_n(any_t *val, const char *str, size_t len);
void any_set_const_string(any_t *val, const char *str);
void any_append(any_t *container, any_t val);

any_t any_duplicate(const any_t *val);
bool  any_is_equal(const any_t *val1, const any_t *val2);
bool  any_is_equal_str(const any_t *val, const char *str);

any_t any_create_null();
any_t any_create_sequence();
any_t any_create_map();
void  any_map_add(any_t *val, const char *key, any_t value);
any_t any_from_bool(bool val);
any_t any_from_int(long long val);
any_t any_from_float(double val);
any_t any_from_string(const char *buf);
any_t any_from_const_string(const char *buf);
any_t any_from_yaml_buffer(const char *buf, size_t len);
any_t any_from_yaml_file(const char *filename);
void  any_free(any_t *val);
int   any_to_str_buffer(char *buf, const any_t *val);
char *any_to_json_str(const any_t *val);
int   any_to_yaml_file(const any_t *val, const char *filename);

DECLARE_MAP(any, char *, any_t)
map_any_iterator *map_any_insert_ex(map_any *m, const char *key, any_t value);
void              map_any_destroy_ex(map_any *m);
