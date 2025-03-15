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
