#include "instructions.h"

int __counter = 0;

int counter() {
    return __counter++;
}