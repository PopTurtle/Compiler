#ifndef UTILS__H
#define UTILS__H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ERRORF(format, ...) fprintf(stderr, format, __VA_ARGS__); exit(EXIT_FAILURE);
#define ERROR(str) ERRORF("%s", str);

typedef int (*cmpfunc)(const void *, const void *);
typedef size_t (*hashfunc)(const void *);

extern size_t str_hashfun(const char *s);

extern void *cralloc(size_t size);

char *mstrcpy(const char *src);

#endif
