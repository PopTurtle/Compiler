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

    const char **param_names;
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
    r->param_names = cralloc((MAX_PARAMS_COUNT + 1) * sizeof *(r->param_names));
    r->param_names[0] = NULL;
    return r;
}

variable *get_variable(const variables_map *map, const char *var_name) {
    variable *var = hashtable_search(map->map, var_name);
    if (var == NULL) {
        ERRORF("Variable name '%s' does not exist in this context\n", var_name);
    }
    return var;
}

int variable_exists(const variables_map *map, const char *var_name) {
    variable *var = hashtable_search(map->map, var_name);
    return var != NULL;
}

static variable *create_var(variables_map *map, const char *var_name, value_type var_type) {
    variable *var = cralloc(sizeof *var);
    var->name = cralloc(strlen(var_name) + 1);
    strcpy(var->name, var_name);
    var->type = var_type;

    hashtable_add(map->map, var->name, var);
    return var;
}

variable *create_local(variables_map *map, const char *var_name) {
    if (hashtable_search(map->map, var_name) != NULL) {
        ERRORF("Cannot create local, var name '%s' is already used\n", var_name);
    }

    variable *var = create_var(map, var_name, TYPE_UNKNOWN);
    var->semantic = SEM_LOCAL;
    var->position = map->locals_count++;
    return var;
}

variable *create_parameter(variables_map *map, const char *var_name) {
    if (hashtable_search(map->map, var_name) != NULL) {
        ERRORF("Cannot create parameter, var name '%s' is already used\n", var_name);
    }

    if (map->params_count == MAX_PARAMS_COUNT) {
        ERROR("Exceeded maximum parameters count\n");
    }

    variable *var = create_var(map, var_name, TYPE_INT);
    var->semantic = SEM_PARAM;
    var->position = map->params_count++;
    
    const char *pname = mstrcpy(var_name);
    map->param_names[map->params_count - 1] = pname;
    map->param_names[map->params_count] = NULL;

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

const char **get_all_param_names(const variables_map *map) {
    return map->param_names;
}

void foreach_variable(variables_map *map, void (*callback)(const char *var_name, variable *var)) {
    hashtable_foreach(map->map, (void (*)(const void *, const void *)) callback);
}