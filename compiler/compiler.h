#ifndef COMPILER__H
#define COMPILER__H

#include "ast.h"
#include "algorithms.h"
#include "variables.h"

int compile_code(int argc, char *argv[], algorithms_map *algs_map, ast_node *first_call);

#endif