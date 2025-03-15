#include "value_type.h"

#define STR_NON_EXISTANT "Non existant type";

#define STR_TYPE_UNKNOWN "Unknown"
#define STR_TYPE_INT "Int"
#define STR_TYPE_BOOL "Bool"

const char *value_type_to_string(value_type v) {
    switch (v) {
        case TYPE_UNKNOWN:
            return STR_TYPE_UNKNOWN;
        case TYPE_INT:
            return STR_TYPE_INT;
        case TYPE_BOOL:
            return STR_TYPE_BOOL;
        default:
            return STR_NON_EXISTANT;
    }
}