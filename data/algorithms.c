#include "algorithms.h"


#define PRINT_LINE_LEN 75
#define PRINT_START " Debut "
#define PRINT_END " Fin "


struct algorithm {
    char *name;
    variables_map *variables;
    value_type return_type;
    ast_node *associated_ast;
};

struct algorithms_map {
    hashtable *map;
};


algorithms_map *create_algorithms_map() {
    algorithms_map *m = cralloc(sizeof *m);
    m->map = hashtable_empty((cmpfunc) strcmp, (hashfunc) str_hashfun);
    if (m->map == NULL) {
        ERROR("Could not allocate algorithms map\n");
    }
    return m;
}

algorithm *get_algorithm(const algorithms_map *map, const char *alg_name) {
    algorithm *alg = hashtable_search(map->map, alg_name);
    if (alg == NULL) {
        ERRORF("Algorithm '%s' does not exist", alg_name);
    }
    return alg;
}

algorithm *create_algorithm(algorithms_map *map, const char *name) {
    algorithm *alg = cralloc(sizeof *alg);
    alg->name = cralloc(strlen(name) + 1);
    strcpy(alg->name, name);
    alg->variables = create_variables_map();
    alg->return_type = TYPE_UNKNOWN;
    alg->associated_ast = NULL;

    hashtable_add(map->map, name, alg);
    return alg;
}

void associate_tree(algorithm *alg, ast_node *tree) {
    alg->associated_ast = tree;
}

const char *get_alg_name(const algorithm *alg) {
    return (const char *) alg->name;
}

ast_node *get_alg_tree(const algorithm *alg) {
    return alg->associated_ast;
}

value_type get_return_type(const algorithm *alg) {
    return alg->return_type;
}

variables_map *get_alg_variables(const algorithm *alg) {
    return alg->variables;
}

value_type unify_return_type(algorithm *alg, value_type return_type) {
    if (return_type == TYPE_UNKNOWN) {
        return alg->return_type;
    }
    if (alg->return_type == TYPE_UNKNOWN) {
        alg->return_type = return_type;
    }
    if (alg->return_type != return_type) {
        ERRORF("Function %s has two different return types in its usage: %s and %s\n", alg->name, value_type_to_string(alg->return_type), value_type_to_string(return_type));
    }
    return alg->return_type;
}

static void print_var(const char *var_name, variable *var) {
    printf("\t%s, %s, pos: %d\n", var_name, value_type_to_string(get_variable_type(var)), get_variable_pos(var));
}

void foreach_algorithm(algorithms_map *map, void (*callback)(const char *alg_name, algorithm *alg)) {
    hashtable_foreach(map->map, (void (*)(const void *, const void *)) callback);
}

void print_algorithm(const algorithm *alg) {
    char *buff = cralloc(PRINT_LINE_LEN + 1);
    char *start = cralloc(strlen(PRINT_START) + strlen(alg->name) + 3);
    char *end = cralloc(strlen(PRINT_END) + strlen(alg->name) + 3);

    sprintf(start, PRINT_START "%s ", alg->name);
    sprintf(end, PRINT_END "%s ", alg->name);

    for (size_t i = 0; i < PRINT_LINE_LEN; ++i) {
        buff[i] = '/';
    }

    for (size_t i = PRINT_LINE_LEN / 2; i < PRINT_LINE_LEN && i < PRINT_LINE_LEN / 2 + strlen(start); ++i) {
        buff[i] = start[i - PRINT_LINE_LEN / 2];
    }

    printf("%s\n", buff);
    
    for (size_t i = PRINT_LINE_LEN / 2; i < PRINT_LINE_LEN && i < PRINT_LINE_LEN / 2 + strlen(end); ++i) {
        buff[i] = end[i - PRINT_LINE_LEN / 2];
    }

    for (size_t i = PRINT_LINE_LEN / 2 + strlen(end); i < PRINT_LINE_LEN / 2 + strlen(start); ++i) {
        buff[i] = '/';
    }

    // Affichage des infos
    printf("Algorithme: %s\n", alg->name);
    printf("Return type: %s\n", value_type_to_string(alg->return_type));

    // Affichage des variables
    printf("Variables: \n");
    foreach_variable(alg->variables, print_var);
    printf("\n");

    // Affichage de l'ast associé
    printf("AST de l'algorithme: \n");
    print_ast(alg->associated_ast);

    printf("%s\n", buff);
    free(buff);
    free(start);
    free(end);
}