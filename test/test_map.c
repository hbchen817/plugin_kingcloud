#include "map.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DECLARE_MAP(int, int, const char *, float, bool)
IMPLEMENT_MAP(int, _MAP_COMPARE_ARITHMATIC, _MAP_COPY_ASSIGNMENT, _MAP_FREE_PLAIN, int, const char *, float, bool)

DECLARE_MAP(string, char *, const char *)
IMPLEMENT_MAP(string, _MAP_COMPARE_STRING, _MAP_COPY_STRING, _MAP_FREE_STRING, char *, const char *)

enum value_type {
    kNull,
    kTrue,
    kFalse,
    kInteger,
    kDouble,
    kString,
    kSequence,
    kMap,
};

struct _map_any;

typedef struct any {
    enum value_type type;
    union {
        long long        ival;
        double           fval;
        char            *sval;
        struct _map_any *mval;
        struct {
            int         count;
            struct any *val;
        } s;
    } u;
} any_t;

DECLARE_MAP(any, char *, any_t)
IMPLEMENT_MAP(any, _MAP_COMPARE_STRING, _MAP_COPY_STRING, _MAP_FREE_STRING, char *, any_t)

int main() {
    map_string *m = map_string_create();

    const char *key  = "hello";
    const char *word = "world";
    map_string_insert(m, key, word);

    const char *key1  = "hello 1";
    const char *word1 = "world 1";
    map_string_insert(m, key1, word1);

    const char *key2  = "hello 1";
    const char *word2 = "world 2 change";
    map_string_insert(m, key2, word2);

    map_string_iterator *iter1 = map_string_find(m, "hello 1");
    if (iter1 != NULL) {
        printf("%s\n", (iter1->val0));
    }

    map_string_iterator *node;
    for (node = map_string_first(m); node; node = map_string_next(node)) {
        printf("{\"%s\":\"%s\"}\n", node->key, node->val0);
    }
    while (map_string_first(m) != NULL) {
        map_string_erase(map_string_first(m));
    }
    map_string_destroy(m);

    map_any *m1 = map_any_create();
    any_t    v  = {.type = kInteger, .u = {.ival = 1}};
    map_any_insert(m1, "data", v);
    map_any_iterator *i1 = map_any_find(m1, "data1");
    if (i1 != NULL) {
        fprintf(stderr, "[ERR] map_any_find");
    }
    i1 = map_any_find(m1, "data");
    if (i1 == NULL) {
        fprintf(stderr, "[ERR] map_any_find(\"data\")");
    } else {
        printf("%d, %lld\n", i1->val0.type, i1->val0.u.ival);
    }
    map_any_destroy(m1);
    return 0;
}