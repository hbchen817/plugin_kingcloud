#include "any.h"
#include "error.h"
#include "yaml.h"
#include <stdio.h>

IMPLEMENT_MAP(any, _MAP_COMPARE_STRING, _MAP_COPY_STRING, _MAP_FREE_STRING, char *, any_t)

bool any_is_null(const any_t *val) {
    return val && (val->type == kNull);
}

bool any_is_bool(const any_t *val) {
    return val && ((val->type & 0xFFFFFFFE) == kFalse);
}

bool any_is_true(const any_t *val) {
    return val && (val->type == kTrue);
}

bool any_is_false(const any_t *val) {
    return val && (val->type == kFalse);
}

bool any_is_float(const any_t *val) {
    return val && ((val->type & 0xFFFFFFFC) == kFloat);
}

bool any_is_single(const any_t *val) {
    return val && (val->type == kSingle);
}

bool any_is_double(const any_t *val) {
    return val && (val->type == kDouble);
}

bool any_is_integer(const any_t *val) {
    return val && ((val->type & 0xFFFFFFF8) == kInteger);
}

bool any_is_signed(const any_t *val) {
    return val && ((val->type & 0xFFFFFFFFC) == kInteger);
}

bool any_is_number(const any_t *val) {
    return any_is_float(val) || any_is_integer(val);
}

bool any_is_custom(const any_t *val) {
    return val && (val->type == kCustom);
}

bool any_is_plain(const any_t *val) {
    return val && (val->type < 0x01000000);
}

bool any_is_string(const any_t *val) {
    return val && ((val->type & 0xFE000000) == kString);
}

bool any_is_valid_string(const any_t *val) {
    return val && ((val->type & 0xFE000000) == kString) && val->u.sval != NULL;
}

bool any_is_const_string(const any_t *val) {
    return val && ((val->type & 0xFF000000) == kConstString);
}

bool any_is_sequence(const any_t *val) {
    return val && ((val->type & 0xFC000000) == kSequence);
}

bool any_is_map(const any_t *val) {
    return val && ((val->type & 0xF8000000) == kMap);
}

int any_length(const any_t *val) {
    if (!val) {
        return -1;
    }
    if (any_is_plain(val)) {
        return 0;
    }
    return val->type & 0x00FFFFFF;
}

void any_set_integer(any_t *val, long long i) {
    any_free(val);
    val->type   = kInteger;
    val->u.ival = i;
}

void any_set_string(any_t *val, const char *str) {
    any_free(val);
    if (str) {
        val->type   = kString + strlen(str);
        val->u.sval = strdup(str);
    } else {
        val->type = kString;
    }
}

void any_set_string_n(any_t *val, const char *str, size_t len) {
    any_free(val);
    if (str && len) {
        val->u.sval = malloc(len + 1);
        if (val->u.sval) {
            memcpy(val->u.sval, str, len);
            val->u.sval[len] = '\0';
            val->type        = kString + len;
        }
    } else {
        val->type = kString;
    }
}

void any_set_const_string(any_t *val, const char *str) {
    any_free(val);
    if (str) {
        val->type   = kConstString + strlen(str);
        val->u.sval = (char *)str;
    } else {
        val->type = kConstString;
    }
}

void any_append(any_t *container, any_t val) {
    if (any_is_sequence(container)) {
        if (!container->u.aval) {
            container->u.aval = malloc(sizeof(any_t));
            if (container->u.aval) {
                container->u.aval[0] = val;
                container->type++;
            }
        } else {
            int    orig_len      = any_length(container);
            any_t *new_container = malloc((orig_len + 1) * sizeof(any_t));
            if (new_container) {
                memcpy(new_container, container->u.aval, orig_len * sizeof(any_t));
                new_container[orig_len] = val;
                free(container->u.aval);
                container->u.aval = new_container;
                container->type++;
            }
        }
    } else if (any_is_null(container)) {
        *container = val;
    } else {
        any_t *new_container = malloc(2 * sizeof(any_t));
        new_container[0]     = *container;
        new_container[1]     = val;
        container->type      = kSequence + 2;
        container->u.aval    = new_container;
    }
}
any_t any_duplicate(const any_t *val) {
    any_t dest = {.type = kUnknown};
    if (any_is_plain(val)) {
        dest = *val;
    } else if (any_is_string(val)) {
        dest.type = val->type & 0xFEFFFFFF;
        int size  = val->type & 0x00FFFFFF;
        if (val->u.sval && size > 0) {
            dest.u.sval = malloc(size + 1);
            if (!dest.u.sval) {
                dest.type = kUnknown;
                return dest;
            }
            memcpy(dest.u.sval, val->u.sval, size + 1);
        } else {
            dest.u.sval = NULL;
        }
    } else if (any_is_sequence(val)) {
        dest.type = val->type;
        int size  = val->type & 0x00FFFFFF;
        if (val->u.aval && size > 0) {
            dest.u.aval = malloc(size * sizeof(any_t));
            if (!dest.u.aval) {
                dest.type = kUnknown;
                return dest;
            }
            for (int i = 0; i < size; i++) {
                dest.u.aval[i] = any_duplicate(&val->u.aval[i]);
            }
        } else {
            dest.u.aval = NULL;
        }
    } else if (any_is_map(val)) {
        dest.type = val->type;
        if (val->u.mval) {
            dest.u.mval = map_any_create();
            for (map_any_iterator *iter = map_any_first(val->u.mval); iter != NULL; iter = map_any_next(iter)) {
                map_any_insert(dest.u.mval, iter->key, any_duplicate(&iter->val0));
            }
        } else {
            dest.u.mval = NULL;
        }
    }
    return dest;
}

bool any_is_equal(const any_t *val1, const any_t *val2) {
    if (val1->type != val2->type) {
        return false;
    }
    if (val1->type == kSingle) {
        return val1->u.fval == val2->u.fval;
    }
    if (val1->type == kDouble) {
        return val1->u.dval == val2->u.dval;
    }
    if (any_is_integer(val1)) {
        return val1->u.ival == val2->u.ival;
    }
    if (any_is_string(val1)) {
        return strncmp(val1->u.sval, val2->u.sval, any_length(val1)) == 0;
    }
    if (any_is_sequence(val1)) {
        for (int i = 0; i < any_length(val1); i++) {
            if (!any_is_equal(&val1->u.aval[i], &val2->u.aval[i])) {
                return false;
            }
        }
        return true;
    }
    if (any_is_map(val1)) {
        int count = 0;
        for (map_any_iterator *iter = map_any_first(val1->u.mval); iter != NULL; iter = map_any_next(iter), count++) {
            map_any_iterator *it = map_any_find(val2->u.mval, iter->key);
            if (it == NULL || !any_is_equal(&iter->val0, &it->val0)) {
                return false;
            }
        }
        for (map_any_iterator *iter = map_any_first(val2->u.mval); iter != NULL; iter = map_any_next(iter), count--)
            ;
        return count == 0;
    }
    return true;
}

bool any_is_equal_str(const any_t *val, const char *str) {
    return val && any_is_string(val) && strcmp(val->u.sval, str) == 0;
}

void any_free(any_t *val) {
    if (!val || val->type == kUnknown) {
        return;
    }
    if (val->type <= 0x00FFFFFF) {
        val->type = kUnknown;
        return;
    }
    if (any_is_string(val)) {
        if (!any_is_const_string(val)) {
            free(val->u.sval);
        }
    } else if (any_is_sequence(val)) {
        for (int i = 0; i < any_length(val); i++) {
            any_free(&val->u.aval[i]);
        }
        if (val->u.aval) {
            free(val->u.aval);
        }
    } else if (any_is_map(val)) {
        if (val->u.mval) {
            map_any_destroy_ex(val->u.mval);
        }
    }
    val->type = kUnknown;
}

static int estimate_length(const any_t *val) {
    if (any_is_null(val)) {
        return 4;
    } else if (any_is_true(val)) {
        return 4;
    } else if (any_is_false(val)) {
        return 5;
    } else if (any_is_float(val)) {
        return 24;
    } else if (any_is_integer(val)) {
        return 20;
    } else if (any_is_string(val)) {
        return any_length(val) + 2;
    } else if (any_is_sequence(val)) {
        int len  = any_length(val);
        int size = len > 0 ? len + 1 : 2;
        for (int i = 0; i < len; i++) {
            size += estimate_length(&val->u.aval[i]);
        }
        return size;
    } else if (any_is_map(val)) {
        int size = 3;
        for (map_any_iterator *iter = map_any_first(val->u.mval); iter != NULL; iter = map_any_next(iter)) {
            size += strlen(iter->key) + 4 + estimate_length(&iter->val0);
        }
        return size;
    } else {
        return 0;
    }
}

static bool string_has_truthy_value(const char *scalar) {
    unsigned long long value  = 0;
    int                length = strlen(scalar);
    if (length > 8) {
        return false;
    }
    memcpy(&value, scalar, length);
    return value == 0x79          // "y"
           || value == 0x59       // "Y"
           || value == 0x736579   // "yes"
           || value == 0x736559   // "Yes"
           || value == 0x534559   // "YES"
           || value == 0x65757274 // "true"
           || value == 0x65757254 // "True"
           || value == 0x45555254 // "TRUE"
           || value == 0x6E6F     // "on"
           || value == 0x6E4F     // "On"
           || value == 0x4E4F;    // "ON"
}

static bool string_has_falsy_value(const char *scalar) {
    unsigned long long value  = 0;
    int                length = strlen(scalar);
    if (length > 8) {
        return false;
    }
    memcpy(&value, scalar, length);
    return value == 0x6E            // "n"
           || value == 0x4E         // "N"
           || value == 0x6F6E       // "no"
           || value == 0x6F4E       // "No"
           || value == 0x4F4E       // "NO"
           || value == 0x65736C6166 // "false"
           || value == 0x65736C6146 // "False"
           || value == 0x45534C4146 // "FALSE"
           || value == 0x66666F     // "off"
           || value == 0x66664F     // "Off"
           || value == 0x46464F;    // "OFF"
}

any_t any_create_null() {
    any_t result = {.type = kNull};
    return result;
}

any_t any_create_sequence() {
    any_t result = {.type = kSequence};
    return result;
}

any_t any_create_map() {
    any_t result = {.type = kMap};
    return result;
}

void any_map_add(any_t *val, const char *key, any_t value) {
    if (val && any_is_map(val)) {
        map_any_iterator *iter = NULL;
        if (val->u.mval == NULL) {
            val->u.mval = map_any_create();
        } else {
            iter = map_any_find(val->u.mval, key);
        }
        if (iter == NULL) {
            map_any_insert(val->u.mval, key, value);
        } else {
            any_free(&iter->val0);
            iter->val0 = value;
        }
    }
}

any_t any_from_bool(bool val) {
    any_t result = {.type = val ? kTrue : kFalse};
    return result;
}

any_t any_from_int(long long val) {
    any_t result = {.type = kInteger, .u = {.ival = val}};
    return result;
}

any_t any_from_float(double val) {
    any_t result = {.type = kDouble, .u = {.dval = val}};
    return result;
}

any_t any_from_string(const char *buf) {
    any_t result = {.type = kUnknown};
    any_set_string(&result, buf);
    return result;
}

any_t any_from_const_string(const char *buf) {
    any_t result = {.type = kUnknown};
    any_set_const_string(&result, buf);
    return result;
}

static any_t any_from_yaml_node(yaml_document_t *doc, yaml_node_t *node) {
    any_t result = {.type = kUnknown};
    switch (node->type) {
    case YAML_NO_NODE:
        result.type = kNull;
        break;
    case YAML_SCALAR_NODE:
        if (node->data.scalar.value[0] == '\0') {
            result.type = kString;
        } else if (node->data.scalar.style == YAML_SINGLE_QUOTED_SCALAR_STYLE || node->data.scalar.style == YAML_DOUBLE_QUOTED_SCALAR_STYLE) {
            any_set_string_n(&result, (char *)node->data.scalar.value, node->data.scalar.length);
        } else {
            char  *end;
            double number = strtod((char *)node->data.scalar.value, &end);
            if (*end == '\0') {
                bool is_integer = true;
                for (char *c = (char *)node->data.scalar.value; is_integer && c != end; c++) {
                    if (*c < '0' || *c > '9') {
                        is_integer = false;
                    }
                }
                if (is_integer && (long long)number == number) {
                    result.type   = kInteger;
                    result.u.ival = (long long)number;
                } else {
                    result.type   = kDouble;
                    result.u.dval = number;
                }
            } else if (string_has_truthy_value((char *)node->data.scalar.value)) {
                result.type = kTrue;
            } else if (string_has_falsy_value((char *)node->data.scalar.value)) {
                result.type = kFalse;
            } else {
                any_set_string(&result, (char *)node->data.scalar.value);
            }
        }
        break;
    case YAML_SEQUENCE_NODE:
        if (node->data.sequence.items.top == node->data.sequence.items.start) {
            result.type   = kSequence;
            result.u.aval = NULL;
        } else {
            int size      = node->data.sequence.items.top - node->data.sequence.items.start;
            result.u.aval = malloc(size * sizeof(any_t));
            if (!result.u.aval) {
                return result;
            }
            result.type = kSequence + size;
            for (int i = 0; i < size; i++) {
                result.u.aval[i] = any_from_yaml_node(doc, yaml_document_get_node(doc, node->data.sequence.items.start[i]));
                if (result.u.aval[i].type == kUnknown) {
                }
            }
        }
        break;
    case YAML_MAPPING_NODE:
        result.type   = kMap;
        result.u.mval = map_any_create();
        for (yaml_node_pair_t *pair = node->data.mapping.pairs.start; pair < node->data.mapping.pairs.top; pair++) {
            yaml_node_t *key = yaml_document_get_node(doc, pair->key);
            if (key && key->type == YAML_SCALAR_NODE) {
                any_t val = any_from_yaml_node(doc, yaml_document_get_node(doc, pair->value));
                map_any_insert(result.u.mval, (char *)key->data.scalar.value, val);
            }
        }
        break;
    }
    return result;
}

any_t any_from_yaml_buffer(const char *buf, size_t len) {
    any_t         result = {.type = kUnknown};
    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        yaml_parser_delete(&parser);
        return result;
    }
    yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);
    yaml_parser_set_input_string(&parser, (const unsigned char *)buf, len);

    yaml_document_t document;
    if (!yaml_parser_load(&parser, &document)) {
        yaml_document_delete(&document);
        yaml_parser_delete(&parser);
        return result;
    }
    yaml_node_t *root = yaml_document_get_root_node(&document);
    if (!root) {
        result.type = kNull;
    } else {
        result = any_from_yaml_node(&document, root);
    }
    yaml_document_delete(&document);
    yaml_parser_delete(&parser);
    return result;
}

any_t any_from_yaml_file(const char *filename) {
    any_t         result = {.type = kUnknown};
    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        yaml_parser_delete(&parser);
        return result;
    }
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        yaml_parser_delete(&parser);
        return result;
    }
    yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);
    yaml_parser_set_input_file(&parser, file);

    yaml_document_t document;
    if (!yaml_parser_load(&parser, &document)) {
        yaml_document_delete(&document);
        yaml_parser_delete(&parser);
        fclose(file);
        return result;
    }
    yaml_node_t *root = yaml_document_get_root_node(&document);
    if (!root) {
        result.type = kNull;
    } else {
        result = any_from_yaml_node(&document, root);
    }
    yaml_document_delete(&document);
    yaml_parser_delete(&parser);
    fclose(file);
    return result;
}

int any_to_str_buffer(char *buf, const any_t *val) {
    if (any_is_null(val)) {
        strcpy(buf, "null");
        return 4;
    } else if (any_is_true(val)) {
        strcpy(buf, "true");
        return 4;
    } else if (any_is_false(val)) {
        strcpy(buf, "false");
        return 5;
    } else if (any_is_single(val)) {
        return sprintf(buf, "%f", val->u.fval);
    } else if (any_is_double(val)) {
        return sprintf(buf, "%lf", val->u.dval);
    } else if (any_is_integer(val)) {
        if (any_is_signed(val)) {
            return sprintf(buf, "%lld", val->u.ival);
        } else {
            return sprintf(buf, "%llu", val->u.ival);
        }
    } else if (any_is_string(val)) {
        int len = val->type & 0x00FFFFFF;
        buf[0]  = '\"';
        memcpy(buf + 1, val->u.sval, len);
        buf[len + 1] = '\"';
        buf[len + 2] = '\0';
        return len + 2;
    } else if (any_is_sequence(val)) {
        buf[0]    = '[';
        int count = val->type & 0x00FFFFFF;
        int len   = 1;
        for (int i = 0; i < count; i++) {
            if (i > 0) {
                buf[len++] = ',';
            }
            len += any_to_str_buffer(buf + len, &val->u.aval[i]);
        }
        buf[len++] = ']';
        buf[len]   = '\0';
        return len;
    } else if (any_is_map(val)) {
        int len = 1;
        buf[0]  = '{';
        if (val->u.mval) {
            int i = 0;
            for (map_any_iterator *iter = map_any_first(val->u.mval); iter != NULL; iter = map_any_next(iter), i++) {
                if (i > 0) {
                    buf[len++] = ',';
                }
                buf[len++] = '"';
                strcpy(buf + len, iter->key);
                len += strlen(iter->key);
                buf[len++] = '"';
                buf[len++] = ':';
                len += any_to_str_buffer(buf + len, &iter->val0);
            }
        }
        buf[len++] = '}';
        buf[len]   = '\0';
        return len;
    } else {
        return 0;
    }
}

char *any_to_json_str(const any_t *val) {
    int len = estimate_length(val);
    if (len == 0) {
        return NULL;
    }
    char *str = malloc(len + 1);
    if (!str) {
        return NULL;
    }
    any_to_str_buffer(str, val);
    return str;
}

static int any_to_yaml(const any_t *val, yaml_document_t *doc, yaml_scalar_style_t style) {
    if (!val) {
        return 0;
    }
    if (any_is_null(val)) {
        return yaml_document_add_scalar(doc, NULL, (const yaml_char_t *)"null", -1, YAML_PLAIN_SCALAR_STYLE);
    } else if (any_is_false(val)) {
        return yaml_document_add_scalar(doc, NULL, (const yaml_char_t *)"false", -1, YAML_PLAIN_SCALAR_STYLE);
    } else if (any_is_true(val)) {
        return yaml_document_add_scalar(doc, NULL, (const yaml_char_t *)"true", -1, YAML_PLAIN_SCALAR_STYLE);
    } else if (any_is_float(val)) {
        char buffer[256];
        sprintf(buffer, "%f", val->u.fval);
        return yaml_document_add_scalar(doc, NULL, (const yaml_char_t *)buffer, -1, YAML_PLAIN_SCALAR_STYLE);
    } else if (any_is_double(val)) {
        char buffer[256];
        sprintf(buffer, "%lf", val->u.dval);
        return yaml_document_add_scalar(doc, NULL, (const yaml_char_t *)buffer, -1, YAML_PLAIN_SCALAR_STYLE);
    } else if (any_is_signed(val)) {
        char buffer[32];
        sprintf(buffer, "%lld", val->u.ival);
        return yaml_document_add_scalar(doc, NULL, (const yaml_char_t *)buffer, -1, YAML_PLAIN_SCALAR_STYLE);
    } else if (any_is_integer(val)) {
        char buffer[32];
        sprintf(buffer, "%llu", val->u.ival);
        return yaml_document_add_scalar(doc, NULL, (const yaml_char_t *)buffer, -1, YAML_PLAIN_SCALAR_STYLE);
    } else if (any_is_string(val)) {
        if (val->u.sval) {
            return yaml_document_add_scalar(doc, NULL, (const yaml_char_t *)val->u.sval, any_length(val), style);
        } else {
            return yaml_document_add_scalar(doc, NULL, (const yaml_char_t *)"", 0, style ? style : YAML_DOUBLE_QUOTED_SCALAR_STYLE);
        }
    } else if (any_is_map(val)) {
        int obj = yaml_document_add_mapping(doc, NULL, YAML_ANY_MAPPING_STYLE);
        if (obj > 0) {
            for (map_any_iterator *iter = map_any_first(val->u.mval); iter != NULL; iter = map_any_next(iter)) {
                any_t key = any_from_const_string(iter->key);
                int   k   = any_to_yaml(&key, doc, YAML_PLAIN_SCALAR_STYLE);
                int   v   = any_to_yaml(&iter->val0, doc, style);
                if (k > 0 && v > 0) {
                    yaml_document_append_mapping_pair(doc, obj, k, v);
                }
            }
        }
        return obj;
    } else if (any_is_sequence(val)) {
        int arr = yaml_document_add_sequence(doc, NULL, YAML_ANY_SEQUENCE_STYLE);
        if (arr > 0) {
            for (int i = 0; i < any_length(val); i++) {
                int item = any_to_yaml(&val->u.aval[i], doc, style);
                if (item > 0) {
                    yaml_document_append_sequence_item(doc, arr, item);
                }
            }
        }
        return arr;
    } else {
        return 0;
    }
}

int any_to_yaml_file(const any_t *val, const char *filename) {
    yaml_document_t doc;
    memset(&doc, 0, sizeof(doc));
    if (!yaml_document_initialize(&doc, NULL, NULL, NULL, 0, 0)) {
        return ERR_CONFIG_CREATE_YAML;
    }
    int id = any_to_yaml(val, &doc, YAML_ANY_SCALAR_STYLE);
    if (id == 0) {
        yaml_document_delete(&doc);
        return ERR_CONFIG_MALFORMED;
    }
    yaml_emitter_t emitter;
    memset(&emitter, 0, sizeof(emitter));
    if (!yaml_emitter_initialize(&emitter)) {
        yaml_document_delete(&doc);
        return ERR_CONFIG_CREATE_EMITTER;
    }
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        yaml_document_delete(&doc);
        yaml_emitter_delete(&emitter);
        return errno;
    }
    yaml_emitter_set_output_file(&emitter, fp);
    yaml_emitter_set_canonical(&emitter, 0);
    yaml_emitter_set_unicode(&emitter, 1);
    if (!yaml_emitter_open(&emitter)) {
        yaml_emitter_delete(&emitter);
        yaml_document_delete(&doc);
        fclose(fp);
        return ERR_CONFIG_OPEN_EMITTER;
    }
    int rc = yaml_emitter_dump(&emitter, &doc);
    yaml_emitter_close(&emitter);
    yaml_emitter_delete(&emitter);
    yaml_document_delete(&doc);
    fclose(fp);
    return rc ? ERR_SUCCESS : ERR_CONFIG_DUMP_EMITTER;
}

map_any_iterator *map_any_insert_ex(map_any *m, const char *key, any_t value) {
    map_any_iterator *iter = map_any_find(m, key);
    if (iter == NULL) {
        return map_any_insert(m, key, value);
    }
    any_free(&iter->val0);
    iter->val0 = value;
    return iter;
}

void map_any_destroy_ex(map_any *m) {
    for (map_any_iterator *iter = map_any_first(m); iter != NULL; iter = map_any_next(iter)) {
        any_free(&iter->val0);
    }
    map_any_destroy(m);
}