#include "ast.h"
#include "algorithms.h"
#include "variables.h"

#define ARG_NONE 0
#define ARG_HELP 1
#define ARG_DEBUG 2
#define ARG_NO_CODE 3
#define ARG_NO_OPTIMIZATION 4

#define ARG_HELP_STR "-h"
#define ARG_DEBUG_STR "-d"
#define ARG_NO_CODE_STR "-c"
#define ARG_NO_OPTIMIZATION_STR "-o"

void print_help_and_exit();
void analyze_arg(const char *argstr);
void print_alg(const char *alg_name, algorithm *alg);

void optimize_alg(const char *alg_name, algorithm *alg);
void check_code(const char *alg_name, algorithm *alg);

void debug_print_part(algorithms_map *algs, int should_print_part_title, const char *part_title);

static int g_debug = 0;
static int g_no_code = 0;
static int g_no_optimization = 0;

const char *g_exec_name;

algorithms_map *g_algs_map;

int main(int argc, char *argv[]) {
    g_exec_name = argv[0];
    for (int i = 1; i < argc; ++i) {
        analyze_arg(argv[i]);
    }

    // Parse
    // -------------------------------------------------------------------------
    //// Création des algos
    //algorithms_map *algs_map = create_algorithms_map();
    //algorithm *alg1 = create_algorithm(algs_map, "Algo1");
    //variables_map *algv1 = get_alg_variables(alg1);
    //algorithm *alg2 = create_algorithm(algs_map, "Algo2");
    //variables_map *algv2 = get_alg_variables(alg2);
//
    //// VARS
    //create_parameter(algv1, "n");
    //create_local(algv1, "a");
//
    //// Arbre de l'algorithme Algo1
    //ast_node *params1[] = { make_binary_operator(make_symbol("n"), OP_SUB, make_int(1)) };
    //ast_node *node1 = 
    //make_function(get_alg_name(alg1),
    //    make_return(make_binary_operator(make_symbol("n"), OP_SUB, make_int(1)))
    //);
//
    //// Arbre de l'algorithme Algo2
    //ast_node *params2_3[] = { make_int(5) };
    //ast_node *params2_2[] = { make_call("Algo1", params2_3, 1) };
    //ast_node *params2_1[] = { make_call("Algo1", params2_2, 1) };
//
    //ast_node *node2 = 
    //make_function(get_alg_name(alg2),
    //    make_return(make_call("Algo1", params2_1, 1))
    //);
//
    //// Association
    //associate_tree(alg1, node1);
    //associate_tree(alg2, node2);


    // ALGO PUISSANCE REC
    algorithms_map *algs_map = create_algorithms_map();
    algorithm *alg1 = create_algorithm(algs_map, "Puissance");
    variables_map *algv1 = get_alg_variables(alg1);
    algorithm *alg2 = create_algorithm(algs_map, "Algo2");
    variables_map *algv2 = get_alg_variables(alg2);
    
    // VARS
    create_parameter(algv1, "a");
    create_parameter(algv1, "b");
    create_parameter(algv1, "acc");

    // Arbre de l'algorithme Puissance
    ast_node *params1[] = { make_symbol("a"), make_binary_operator(make_symbol("b"), OP_SUB, make_int(1)), make_binary_operator(make_symbol("acc"), OP_MUL, make_symbol("a")) };
    ast_node *node1 = 
    make_function(get_alg_name(alg1),
        make_if_statement(
            make_binary_operator(make_symbol("b"), OP_EQUAL, make_int(0)),
            make_return(make_symbol("acc")),
            make_return(make_call("Puissance", params1, 3))
        )
    );
    
    // Arbre de l'algorithme Algo2
    ast_node *params2[] = { make_int(4), make_int(5), make_int(1) };
    
    ast_node *node2 = 
    make_function(get_alg_name(alg2),
        make_return(make_call("Puissance", params2, 3))
    );
    
    // Association
    associate_tree(alg1, node1);
    associate_tree(alg2, node2);

    // -------------------------------------------------------------------------
    
    g_algs_map = algs_map;

    debug_print_part(algs_map, 1, "Type resolving");
    resolve_types(algs_map);

    if (!g_no_optimization) {
        debug_print_part(algs_map, 1, "Optimizing code");
        foreach_algorithm(algs_map, optimize_alg);
    }

    debug_print_part(algs_map, 1, "Code checking");
    foreach_algorithm(algs_map, check_code);

    debug_print_part(algs_map, !g_no_code, "Output code");
    if (!g_no_code) {
        write_all_instructions(algs_map, make_call("Algo2", NULL, 0));
    }

    return 0;
}


void print_help_and_exit() {
    printf("Usage: %s\n", g_exec_name);
    printf("\t" ARG_DEBUG_STR ": Show debug information on standard output\n");
    printf("\t" ARG_NO_CODE_STR ": Do not print output code, useful to debug\n");
    printf("\t" ARG_NO_OPTIMIZATION_STR ": Do not run any optimization code, compile as code is written\n");
    printf("\t" ARG_HELP_STR ": Show help\n");
    printf("\tTo compile to a file, redirect: %s < input.algo > output.asipro\n", g_exec_name);
    exit(0);
}

void analyze_arg(const char *argstr) {
    int arg = ARG_NONE;
    
    if (strcmp(argstr, ARG_DEBUG_STR) == 0) {
        arg = ARG_DEBUG;
    } else if (strcmp(argstr, ARG_NO_CODE_STR) == 0) {
        arg = ARG_NO_CODE;
    } else if (strcmp(argstr, ARG_HELP_STR) == 0) {
        arg = ARG_HELP;
    } else if (strcmp(argstr, ARG_NO_OPTIMIZATION_STR) == 0) {
        arg = ARG_NO_OPTIMIZATION;
    }
    
    switch (arg) {
        case ARG_DEBUG:
            g_debug = 1;
            break;
        case ARG_NO_CODE:
            g_no_code = 1;
            break;
        case ARG_NO_OPTIMIZATION:
            g_no_optimization = 1;
            break;
        case ARG_HELP:
            print_help_and_exit();
            break; // Useless
        default:
            break;
    }
}

void print_alg([[ maybe_unused ]] const char *alg_name, algorithm *alg) {
    print_algorithm(alg);
    printf("\n");
}

void optimize_alg([[ maybe_unused ]] const char *alg_name, algorithm *alg) {
    optimize_ast(g_algs_map, get_alg_tree(alg), g_debug);
}

void check_code([[ maybe_unused ]] const char *alg_name, algorithm *alg) {
    check_ast_code(get_alg_tree(alg), g_algs_map);
}


void debug_print_part(algorithms_map *algs, int should_print_part_title, const char *part_title) {
    if (!g_debug) return;
    foreach_algorithm(algs, print_alg);

    if (should_print_part_title) {
        size_t tlen = strlen(part_title) + 4 + 1;
        char *line_buff = cralloc(tlen);
        char *text_buff = cralloc(tlen);
        
        line_buff[tlen - 1] = '\0';
        for (size_t i = 0; i < tlen - 1; ++i) {
            line_buff[i] = '-';
        }

        sprintf(text_buff, "| %s |", part_title);

        printf("\n%s\n%s\n%s\n\n\n", line_buff, text_buff, line_buff);
        
        free(line_buff);
        free(text_buff);
    }
}