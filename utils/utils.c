#include "utils.h"

size_t str_hashfun(const char *s) {
    size_t h = 0;
    for (const unsigned char *p = (const unsigned char *) s; *p != '\0'; ++p) {
        h = 37 * h + *p;
    }
    return h;
}

void *cralloc(size_t size) {
    void *result = malloc(size);
    if (result == NULL) {
        fprintf(stderr, "Could not allocate\n");
        exit(EXIT_FAILURE);
    }
    return result;
}

char *mstrcpy(const char *src) {
    char *res = cralloc(strlen(src) + 1);
    strcpy(res, src);
    return res;
}

const char *b_op_to_str(binary_operator_t operator) {
    switch (operator) {
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_AND: return "&&";
        case OP_OR: return "||";
        case OP_EQUAL: return "==";
        case OP_SGT: return ">";
        case OP_EGT: return ">=";
        case OP_SLT: return "<";
        case OP_ELT: return "<=";
        default: return "Unknwon operator";
    }
}