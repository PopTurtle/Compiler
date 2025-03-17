#ifndef VARIABLE__H
#define VARIABLE__H

#include <string.h>

#define MAX_PARAMS_COUNT 16

typedef enum {
    SEM_LOCAL,
    SEM_PARAM,
} variable_semantic;

typedef struct variable variable;
typedef struct variables_map variables_map;

#include "value_type.h"
#include "hashtable.h"
#include "utils.h"

extern int params_count(const variables_map *map);
extern int locals_count(const variables_map *map);

extern variables_map *create_variables_map();
extern variable *get_variable(const variables_map *map, const char *var_name);
extern int variable_exists(const variables_map *map, const char *var_name);
extern variable *create_local(variables_map *map, const char *var_name);
extern variable *create_parameter(variables_map *map, const char *var_name);

extern const char *get_variable_name(const variable *var);
extern value_type get_variable_type(const variable *var);
extern variable_semantic get_variable_semantic(const variable *var);
extern int get_variable_pos(const variable *var);
extern value_type unify_variable_type(variable *var, value_type new_type); // Crash si incohérent

extern const char **get_all_param_names(const variables_map *map);
extern void foreach_variable(variables_map *map, void (*callback)(const char *var_name, variable *var));

#endif