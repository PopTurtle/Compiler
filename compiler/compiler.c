#include "compiler.h"

#define ARG_NONE 0
#define ARG_HELP 1
#define ARG_DEBUG 2
#define ARG_NO_CODE 3
#define ARG_NO_OPTIMIZATION 4

#define ARG_HELP_STR "-h"
#define ARG_DEBUG_STR "-d"
#define ARG_NO_CODE_STR "-c"
#define ARG_NO_OPTIMIZATION_STR "-o"

static void print_help_and_exit();
static void analyze_arg(const char *argstr);
static void print_alg(const char *alg_name, algorithm *alg);

static void optimize_alg(const char *alg_name, algorithm *alg);
static void check_code(const char *alg_name, algorithm *alg);

static void debug_print_part(algorithms_map *algs, int should_print_part_title, const char *part_title);

static int g_debug = 0;
static int g_no_code = 0;
static int g_no_optimization = 0;

static const char *g_exec_name;

static algorithms_map *g_algs_map;

int compile_code(int argc, char *argv[], algorithms_map *algs_map, ast_node *first_call) {
    g_exec_name = argv[0];
    for (int i = 1; i < argc; ++i) {
        analyze_arg(argv[i]);
    }

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
        write_all_instructions(algs_map, first_call);
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