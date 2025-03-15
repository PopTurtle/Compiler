#ifndef ALGORITHMS__H
#define ALGORITHMS__H

#include <string.h>

#include "value_type.h"
#include "hashtable.h"
#include "utils.h"

typedef struct algorithm algorithm;
typedef struct algorithms_map algorithms_map;

#include "ast.h"
#include "variables.h"

extern algorithms_map *create_algorithms_map();
extern algorithm *get_algorithm(const algorithms_map *map, const char *alg_name);

extern algorithm *create_algorithm(algorithms_map *map, const char *name);
extern void associate_tree(algorithm *alg, ast_node *tree);

extern const char *get_alg_name(const algorithm *alg);
extern ast_node *get_alg_tree(const algorithm *alg);
extern value_type get_return_type(const algorithm *alg);
extern variables_map *get_alg_variables(const algorithm *alg);

extern value_type unify_return_type(algorithm *alg, value_type return_type);

extern void foreach_algorithm(algorithms_map *map, void (*callback)(const char *alg_name, algorithm *alg));

extern void print_algorithm(const algorithm *alg);

#endif