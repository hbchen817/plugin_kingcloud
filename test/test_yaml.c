#include "yaml.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#define YAML_EVENT_BASED_PARSE    0
#define YAML_DOCUMENT_BASED_PARSE 1

static const int FoldingDepth = 4;

void handle_parser_error(yaml_parser_t *p) {
    switch (p->error) {
    case YAML_MEMORY_ERROR:
        fprintf(stderr, "[E] Memory error: Not enough memory for parsing\n");
        break;
    case YAML_READER_ERROR:
        if (p->problem_value != -1) {
            fprintf(stderr, "[R]: %s: #%X at %ld\n", p->problem, p->problem_value, p->problem_offset);
        } else {
            fprintf(stderr, "[R]: %s at %ld\n", p->problem, p->problem_offset);
        }
        break;
    case YAML_SCANNER_ERROR:
        if (p->context) {
            fprintf(stderr, "[S]: %s at line %" PRIdPTR ", column %" PRIdPTR "\n%s at line %" PRIdPTR ", column %" PRIdPTR "\n", p->context,
                    p->context_mark.line + 1, p->context_mark.column + 1, p->problem, p->problem_mark.line + 1, p->problem_mark.column + 1);
        } else {
            fprintf(stderr, "[S]: %s at line %" PRIdPTR ", column %" PRIdPTR "\n", p->problem, p->problem_mark.line + 1, p->problem_mark.column + 1);
        }
        break;
    case YAML_PARSER_ERROR:
        if (p->context) {
            fprintf(stderr, "[P]: %s at line %" PRIdPTR ", column %" PRIdPTR "\n%s at line %" PRIdPTR ", column %" PRIdPTR "\n", p->context,
                    p->context_mark.line + 1, p->context_mark.column + 1, p->problem, p->problem_mark.line + 1, p->problem_mark.column + 1);
        } else {
            fprintf(stderr, "[P]: %s at line %" PRIdPTR ", colum %" PRIdPTR "\n", p->problem, p->problem_mark.line + 1, p->problem_mark.column + 1);
        }
        break;
    default:
        fprintf(stderr, "[E] Internal error\n");
        break;
    }
}

bool string_has_truthy_value(const char *scalar) {
    uint64_t value  = 0;
    int      length = strlen(scalar);
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

bool string_has_falsy_value(const char *scalar) {
    uint64_t value  = 0;
    int      length = strlen(scalar);
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

bool string_has_bool_value(const char *scalar, bool *value) {
    if (string_has_truthy_value(scalar)) {
        if (value) {
            *value = true;
        }
        return true;
    }
    if (string_has_falsy_value(scalar)) {
        if (value) {
            *value = false;
        }
        return true;
    }
    return false;
}

bool node_is_bool(yaml_node_t *node, bool *value) {
    if (node->data.scalar.style == YAML_PLAIN_SCALAR_STYLE) {
        return string_has_bool_value((char *)node->data.scalar.value, value);
    } else {
        return false;
    }
}

bool scalar_needs_quote(yaml_node_t *node) {
    if (node->type != YAML_SCALAR_NODE) {
        return false;
    }
    char *scalar = (char *)node->data.scalar.value;
    char *end;
    strtod(scalar, &end);
    bool is_string = end == scalar || *end;
    return is_string && !node_is_bool(node, NULL);
}

size_t escape_quoted_string(const char *str, size_t length) {
    size_t bytes_out = 0;
    for (size_t i = 0; i < length; i++, str++) {
        switch (*str) {
        case '\\':
            bytes_out += printf("\\\\");
            break;
        case '\0':
            bytes_out += printf("\\0");
            break;
        case '\b':
            bytes_out += printf("\\b");
            break;
        case '\n':
            bytes_out += printf("\\n");
            break;
        case '\r':
            bytes_out += printf("\\r");
            break;
        case '\t':
            bytes_out += printf("\\t");
            break;
        case '"':
            bytes_out += printf("\\\"");
            break;
        default:
            bytes_out += printf("%c", *str);
            break;
        }
    }
    return bytes_out;
}

size_t output_data(yaml_document_t *document, yaml_node_t *node, yaml_node_type_t type, int depth) {
    size_t bytes_out = 0;
    switch (node->type) {
    case YAML_SCALAR_NODE: {
        bool needs_quotes = scalar_needs_quote(node);
        if (needs_quotes) {
            bytes_out += printf("\"");
        }
        bytes_out += escape_quoted_string((char *)node->data.scalar.value, strlen((char *)node->data.scalar.value));
        if (needs_quotes) {
            bytes_out += printf("\"");
        }
    } break;
    case YAML_SEQUENCE_NODE: {
        bytes_out += printf("[");
        int node_max   = node->data.sequence.items.top - node->data.sequence.items.start;
        int node_count = 0;
        for (yaml_node_item_t *item_i = node->data.sequence.items.start; item_i < node->data.sequence.items.top; item_i++) {
            yaml_node_t *node_item   = yaml_document_get_node(document, *item_i);
            int          child_level = node_item->type == YAML_MAPPING_NODE ? depth + 1 : depth - 1;
            bytes_out += output_data(document, node_item, YAML_SEQUENCE_NODE, child_level);
            node_count++;
            if (node_count < node_max) {
                bytes_out += printf(", ");
            }
        }
        bytes_out += printf("]");
    } break;
    case YAML_MAPPING_NODE: {
        bool is_seq        = depth > 0 && type == YAML_SEQUENCE_NODE;
        bool needs_folding = depth > FoldingDepth;
        int  node_count    = 0;
        bytes_out += printf("{");
        if (!needs_folding) {
            bytes_out += printf("\n");
            for (int i = 0; i <= depth; i++) {
                bytes_out += printf("\t");
            }
        }
        int node_max = node->data.mapping.pairs.top - node->data.mapping.pairs.start;
        for (yaml_node_pair_t *pair_i = node->data.mapping.pairs.start; pair_i < node->data.mapping.pairs.top; pair_i++) {
            yaml_node_t *key   = yaml_document_get_node(document, pair_i->key);
            yaml_node_t *value = yaml_document_get_node(document, pair_i->value);
            if (key->type != YAML_SCALAR_NODE) {
                fprintf(stderr, "[E] Mapping key is not scalar (line %" PRIuPTR ").", key->start_mark.line);
                continue;
            }
            bytes_out += printf("\"%s\": ", key->data.scalar.value);
            bytes_out += output_data(document, value, YAML_MAPPING_NODE, depth + 1);
            node_count++;
            if (node_count < node_max) {
                if (!needs_folding) {
                    bytes_out += printf(",\n");
                    for (int i = 0; i <= depth; i++) {
                        bytes_out += printf("\t");
                    }
                } else {
                    bytes_out += printf(", ");
                }
            }
        }
        if (!needs_folding) {
            bytes_out += printf("\n");
            for (int i = is_seq ? depth - 1 : depth; i > 0; i--) {
                bytes_out += printf("\t");
            }
        }
        bytes_out += printf("}");
    } break;
    case YAML_NO_NODE:
        break;
    default:
        fprintf(stderr, "[E] Unknown node type (line %" PRIuPTR ").", node->start_mark.line);
        break;
    }
    return bytes_out;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <yaml file>\n", argv[0]);
        return 0;
    }

    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) {
        fprintf(stderr, "[E] Open file %s failed!\n", argv[1]);
        return 1;
    }

    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        fprintf(stderr, "[E] Initialize yaml parser failed!\n");
        handle_parser_error(&parser);
        yaml_parser_delete(&parser);
        fclose(file);
        return 1;
    }
    yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);
    yaml_parser_set_input_file(&parser, file);

#if YAML_EVENT_BASED_PARSE
    yaml_event_t event;
    bool         done  = false;
    int          count = 0;
    while (!done) {
        if (!yaml_parser_parse(&parser, &event)) {
            fprintf(stderr, "[E] parse yaml file failed!\n");
            handle_parser_error(&parser);
            yaml_parser_delete(&parser);
            fclose(file);
            return 1;
        }
        done = event.type == YAML_STREAM_END_EVENT;
        yaml_event_delete(&event);
        count++;
    }
    printf("%d events\n", count);
#endif
#if YAML_DOCUMENT_BASED_PARSE
    yaml_document_t document;
    if (!yaml_parser_load(&parser, &document)) {
        fprintf(stderr, "[E] parse yaml file failed!\n");
        handle_parser_error(&parser);
        yaml_document_delete(&document);
        yaml_parser_delete(&parser);
        fclose(file);
        return 1;
    }
    yaml_node_t *root = yaml_document_get_root_node(&document);
    if (!root) {
        fprintf(stderr, "[E] No document defined.\n");
    } else {
        output_data(&document, root, YAML_NO_NODE, 0);
    }
    yaml_document_delete(&document);
#endif
    yaml_parser_delete(&parser);
    fclose(file);
    return 0;
}