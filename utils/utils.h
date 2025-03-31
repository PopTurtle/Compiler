#ifndef UTILS__H
#define UTILS__H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ast.h"

#define RESET   "\033[0m"
#define RED     "\033[0;31m"
#define BOLD    "\033[1m"

#define ERRORF(format, ...) fprintf(stderr, RED format RESET, __VA_ARGS__); exit(EXIT_FAILURE);
#define ERROR(str) ERRORF("%s", str);

#define ERRORAF(context_node, format, ...)                                                        \
    if (get_line(context_node) <= 0) {                                                            \
        fprintf(stderr, RED format RESET, __VA_ARGS__); exit(EXIT_FAILURE);                       \
    } else {                                                                                      \
        fprintf(stderr, RED BOLD "Line %d: " RESET RED format RESET, get_line(context_node), __VA_ARGS__); exit(EXIT_FAILURE);  \
    }                                                                                             \

#define ERRORA(context_node, str) ERRORAF(context_node, "%s", str);

typedef int (*cmpfunc)(const void *, const void *);
typedef size_t (*hashfunc)(const void *);

extern size_t str_hashfun(const char *s);

extern void *cralloc(size_t size);

char *mstrcpy(const char *src);

const char *b_op_to_str(binary_operator_t operator);

#endif
