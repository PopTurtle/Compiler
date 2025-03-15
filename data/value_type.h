#ifndef VALUE_TYPE__H
#define VALUE_TYPE__H

typedef enum {
    TYPE_UNKNOWN,
    TYPE_INT,
    TYPE_BOOL,
} value_type;

extern const char *value_type_to_string(value_type v);

#endif