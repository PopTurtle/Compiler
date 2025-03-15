#include "variables.h"

struct variable {
    char *name;
    int position;
    variable_semantic semantic;
    value_type type;
};

struct variables_map {
    hashtable *map;
    int params_count;
    int locals_count;
};

int params_count(const variables_map *map) {
    return map->params_count;
}

int locals_count(const variables_map *map) {
    return map->locals_count;
}

variables_map *create_variables_map() {
    variables_map *r = cralloc(sizeof *r);
    r->map = hashtable_empty((cmpfunc) strcmp, (hashfunc) str_hashfun);
    if (r->map == NULL) {
        ERROR("Could not allocate variables map\n");
    }
    r->params_count = 0;
    r->locals_count = 0;
    return r;
}

variable *get_variable(const variables_map *map, const char *var_name) {
    variable *var = hashtable_search(map->map, var_name);
    if (var == NULL) {
        ERRORF("Variable name '%s' does not exist in this context\n", var_name);
    }
    return var;
}

static variable *create_var(variables_map *map, const char *var_name, value_type var_type) {
    variable *var = cralloc(sizeof *var);
    var->name = cralloc(strlen(var_name) + 1);
    strcpy(var->name, var_name);
    var->type = var_type;
    
    hashtable_add(map->map, var_name, var);
    return var;
}

variable *create_local(variables_map *map, const char *var_name, value_type var_type) {
    variable *var = create_var(map, var_name, var_type);
    var->semantic = SEM_LOCAL;
    var->position = map->locals_count++;
    return var;
}

variable *create_parameter(variables_map *map, const char *var_name) {
    variable *var = create_var(map, var_name, TYPE_INT);
    var->semantic = SEM_PARAM;
    var->position = map->params_count++;
    return var;
}

const char *get_variable_name(const variable *var) {
    return var->name;
}

value_type get_variable_type(const variable *var) {
    return var->type;
}

variable_semantic get_variable_semantic(const variable *var) {
    return var->semantic;
}

int get_variable_pos(const variable *var) {
    return var->position;
}

value_type unify_variable_type(variable *var, value_type new_type) {
    if (new_type == TYPE_UNKNOWN) {
        return var->type;
    }
    if (var->type == TYPE_UNKNOWN) {
        var->type = new_type;
    }
    if (var->type != new_type) {
        ERRORF("Variable '%s' is used as a %s but is of type %s\n", var->name, value_type_to_string(new_type), value_type_to_string(var->type));
    }
    return new_type;
}

void foreach_variable(variables_map *map, void (*callback)(const char *var_name, variable *var)) {
    hashtable_foreach(map->map, (void (*)(const void *, const void *)) callback);
}