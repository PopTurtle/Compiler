#include "ast.h"
#include "algorithms.h"
#include "variables.h"

#define ARG_NONE 0
#define ARG_HELP 1
#define ARG_DEBUG 2
#define ARG_NO_CODE 3

void print_help_and_exit();
void analyze_arg(const char *argstr, int *debug, int *no_code);
void print_alg(const char *alg_name, algorithm *alg);

void optimize_alg(const char *alg_name, algorithm *alg);
void check_code(const char *alg_name, algorithm *alg);

void debug_print_part(algorithms_map *algs, int should_print_part_title, const char *part_title);

static int g_debug = 0;
static int g_no_code = 0;

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        analyze_arg(argv[i], &g_debug, &g_no_code);
    }

    // Parse
    // -------------------------------------------------------------------------
    // Création des algos
    algorithms_map *algs_map = create_algorithms_map();
    algorithm *alg1 = create_algorithm(algs_map, "Algo1");
    variables_map *algv1 = get_alg_variables(alg1);
    algorithm *alg2 = create_algorithm(algs_map, "Algo2");
    variables_map *algv2 = get_alg_variables(alg2);

    // VARS
    create_parameter(algv1, "a");
    create_parameter(algv1, "b");
    create_local(algv1, "p", TYPE_UNKNOWN);
    create_local(algv1, "k", TYPE_UNKNOWN);

    // Arbre de l'algorithme Algo1
    ast_node *node1 = 
    make_function(get_alg_name(alg1),
        make_sequence(
            make_assignement("p", make_int(1)),
            make_sequence(
                make_do_for_i(
                    "k", make_int(1), make_symbol("b"),
                    make_assignement("p", make_binary_operator(make_symbol("p"), OP_MUL, make_symbol("a")))
                ),
                make_return(make_symbol("p"))
            )
        )
    );

    // Arbre de l'algorithme Algo2
    ast_node *params2[] = { make_int(3), make_int(4) };
    ast_node *node2 = 
    make_function(get_alg_name(alg2),
        make_return(make_call("Algo1", params2, 2))
    );

    //// Remplissage des variables de Algo1
    //create_parameter(algv1, "a");
    //create_parameter(algv1, "b");
    //create_local(algv1, "c", TYPE_UNKNOWN);
//
    //// Arbre de l'algorithme Algo1
    //ast_node *params1[] = { make_symbol("b") };
//
    //ast_node *node1 = 
    //make_function(get_alg_name(alg1),
    //    make_sequence(
    //        make_assignement(
    //            "c", make_binary_operator(make_symbol("a"), OP_ADD, make_call(
    //                "Algo2", params1, 1)
    //            )
    //        ),
    //        make_return(make_symbol("c"))
    //    )
    //);
    //
    //// Remplissage des variables de Algo2
    //create_parameter(algv2, "param");
//
    //// Arbre de l'algorithme Algo2
    //ast_node *node2 = make_function(get_alg_name(alg2),
    //    make_return(make_binary_operator(
    //        make_symbol("param"), OP_ADD, make_int(5)
    //    ))
    //);
    
    // Association
    associate_tree(alg1, node1);
    associate_tree(alg2, node2);
    // -------------------------------------------------------------------------

    debug_print_part(algs_map, 1, "Type resolving");
    resolve_types(algs_map);

    debug_print_part(algs_map, 1, "Optimizing code");
    foreach_algorithm(algs_map, optimize_alg);

    debug_print_part(algs_map, 1, "Code checking");
    foreach_algorithm(algs_map, check_code);

    debug_print_part(algs_map, !g_no_code, "Output code");
    if (!g_no_code) {
        write_all_instructions(algs_map, make_call("Algo2", NULL, 0));
    }

    return 0;
}


void print_help_and_exit() {
    printf("Usage: ./compile\n");
    printf("\t-d: Show g_debug information on standard output\n");
    printf("\t-h: Show help\n");
    printf("\tTo compile to a file, redirect: ./compile < input.algo > output.asipro\n");
    exit(0);
}

void analyze_arg(const char *argstr, int *debug, int *no_code) {
    int arg = ARG_NONE;
    
    if (strcmp(argstr, "-d") == 0) {
        arg = ARG_DEBUG;
    } else if (strcmp(argstr, "-c") == 0) {
        arg = ARG_NO_CODE;
    } else if (strcmp(argstr, "-h") == 0) {
        arg = ARG_HELP;
    }
    
    switch (arg) {
        case ARG_DEBUG:
            *debug = 1;
            break;
        case ARG_NO_CODE:
            *no_code = 1;
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
    optimize_ast(get_alg_tree(alg), g_debug);
}

void check_code([[ maybe_unused ]] const char *alg_name, algorithm *alg) {
    check_ast_code(get_alg_tree(alg));
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