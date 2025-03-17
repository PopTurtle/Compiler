#ifndef AST__H
#define AST__H

#include "value_type.h"
#include "instructions.h"

typedef enum {
    OP_NOT,
} unary_operator_t;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,

    OP_AND,
    OP_OR,

    OP_EQUAL,
    OP_SGT,             // Strictly Greater Than
    OP_EGT,             // Equal or Greater Than
    OP_SLT,             // Strictly Less Than
    OP_ELT              // Equal or Less Than
} binary_operator_t;

typedef struct ast_node ast_node;

#include "utils.h"
#include "algorithms.h"

extern void resolve_types(algorithms_map *algs);
//extern void type_resolve(ast_node *node, const char *current_alg, algorithms_map *algs);
//extern void instruct(ast_node *node);

extern ast_node *make_int(int value);
extern ast_node *make_bool(int bool_value);
extern ast_node *make_symbol(const char *symbol);
extern ast_node *make_unary_operator(unary_operator_t operator, ast_node *operand);
extern ast_node *make_binary_operator(ast_node *left, binary_operator_t operator, ast_node *right);
extern ast_node *make_assignement(const char *var_name, ast_node *expr);

extern ast_node *make_return(ast_node *expr);
extern ast_node *make_call(const char *function_name, ast_node **parameters, int params_count);

extern ast_node *make_if_statement(ast_node *condition, ast_node *then_block, ast_node *else_block);
extern ast_node *make_do_for_i(const char *var_name, ast_node *start_expr, ast_node *end_expr, ast_node *body);
extern ast_node *make_do_while(ast_node *condition, ast_node *body);

extern ast_node *make_function(const char *function_name, ast_node *body);
extern ast_node *make_sequence(ast_node *first, ast_node *second);

extern void optimize_ast(algorithms_map *algs, ast_node *ast, int debug);
extern void check_ast_code(ast_node *ast, algorithms_map *algs);
extern void write_all_instructions(algorithms_map *algs, ast_node *main_call);

extern void print_ast(const ast_node *ast);

#endif
