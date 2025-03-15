#include "ast.h"

#include <stdlib.h>
#include <stdio.h>

typedef enum {
    // Expressions
    NODE_CONST_INT,
    NODE_CONST_BOOL,
    NODE_SYMBOL,
    NODE_UNARY_OPERATOR,
    NODE_BINARY_OPERATOR,
    NODE_CALL,

    // Instructions
    NODE_ASSIGNEMENT,
    NODE_RETURN,

    // Structures
    NODE_IF_STATEMENT,
    NODE_DO_FOR_I,

    // Suite d'instructions
    NODE_FUNCTION,
    NODE_SEQUENCE,
} ast_node_type;

#error TODO: Ajouter l'instruction DO_WHILE

struct ast_node {
    ast_node_type type;
    union {
        int number_value;
        char *symbol_name;
        struct { ast_node *operand; unary_operator_t operator; value_type result_type; } unary_operator;
        struct { ast_node *left; binary_operator_t operator; ast_node *right; value_type result_type; } binary_operator;
        struct { char *var_name; ast_node *expr; } assignement;
        struct { ast_node *expr; } inst_return; // ne peut pas s'appeler "return" car mot clé du langage c
        struct { char *function_name; ast_node **parameters_expr; int params_count; } call;
        struct { ast_node *condition; ast_node *then_block; ast_node *else_block; } if_statement;
        struct { const char *var_name; ast_node *start_expr; ast_node *end_expr; ast_node *body; } do_for_i;
        struct { char *function_name; ast_node *body; } function;
        struct { ast_node *first; ast_node *second; } sequence;
    };
};


//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ----------------------------   Utilitaire   ----------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
static ast_node *canode() {
    return (ast_node *) cralloc(sizeof(ast_node));
}

static void check_type(value_type current, value_type expected) {
    if (current != expected) {
        ERRORF("Type error : expected %s, got %s\n", value_type_to_string(expected), value_type_to_string(current));
    }
}

static int check_type_ignore(value_type current, value_type expected) {
    if (current == TYPE_UNKNOWN) {
        return 0;
    }
    check_type(current, expected);
    return 1;
}


//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------   Inférence de types   ------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
int g_resolving = 0;        // Résolution en cours ?
int g_stable;               // Résolution en cours est stable ?
algorithms_map *g_algs;     // Algorithmes de la résolution en cours


static void unstable_if_unknown(value_type type);
static value_type get_expr_known_type(ast_node *node, variables_map *vars);

static void resolve_types_in_alg(const char *alg_name, algorithm *alg);
static void resolve_types_in_ast(ast_node *ast, algorithm *current_alg, variables_map *vars);

static void resolve_types_in_unary_operation(ast_node *unary_op, algorithm *current_alg, variables_map *vars);
static void resolve_types_in_binary_operation(ast_node *binary_op, algorithm *current_alg, variables_map *vars);
static int check_binary_operation_type(ast_node *op, variables_map *vars, value_type expected_left, value_type expected_right);


void resolve_types(algorithms_map *algs) {
    if (g_resolving) { ERROR("Cannot call resolve_types during a type resolving\n"); }
    g_resolving = 1;

    // Inférence de types
    g_algs = algs;
    int max_iterations = 1000;
    do {
        // stable retourne à 0 si un type de variable locale ou un type
        // de retour a subit un changement pendant l'itération
        g_stable = 1;

        foreach_algorithm(algs, resolve_types_in_alg);
        
        if (--max_iterations <= 0) {
            // Exemple de programme qui pourrait faire tourner la boucle autant :
            // function f():
            //      return g()
            // function g():
            //      return f()
            // -> Impossible de déterminer les types
            break;
            ERROR("Types could not be resolved, maybe it cannot be determined\n");
        }
    } while(g_stable != 1);

    g_resolving = 0;
}

void unstable_if_unknown(value_type type) {
    if (type == TYPE_UNKNOWN) {
        g_stable = 0;
    }
}

static value_type get_expr_known_type(ast_node *node, variables_map *vars) {
    switch (node->type) {
        case NODE_CONST_INT:
            return TYPE_INT;
        case NODE_CONST_BOOL:
            return TYPE_BOOL;
        case NODE_SYMBOL:
            return get_variable_type(get_variable(vars, node->symbol_name));
        case NODE_UNARY_OPERATOR:
            return node->unary_operator.result_type;
        case NODE_BINARY_OPERATOR:
            return node->binary_operator.result_type;
        case NODE_CALL:
            return get_return_type(get_algorithm(g_algs, node->call.function_name));
        default:
            ERROR("Trying to get the type of a node that has no type - ET1\n");
    }
}

static void resolve_types_in_alg([[ maybe_unused ]] const char *alg_name, algorithm *alg) {
    resolve_types_in_ast(get_alg_tree(alg), alg, get_alg_variables(alg));
}

static void resolve_types_in_ast(ast_node *ast, algorithm *current_alg, variables_map *vars) {
    value_type temp;
    switch (ast->type) {
        case NODE_ASSIGNEMENT:
            variable *assign_to = get_variable(vars, ast->assignement.var_name);
            resolve_types_in_ast(ast->assignement.expr, current_alg, vars);
            unstable_if_unknown(unify_variable_type(assign_to, get_expr_known_type(ast->assignement.expr, vars)));
            break;

        case NODE_UNARY_OPERATOR:
            resolve_types_in_unary_operation(ast, current_alg, vars);
            break;

        case NODE_BINARY_OPERATOR:
            resolve_types_in_binary_operation(ast, current_alg, vars);
            break;
        
        case NODE_RETURN:
            resolve_types_in_ast(ast->inst_return.expr, current_alg, vars);
            unstable_if_unknown(unify_return_type(current_alg, get_expr_known_type(ast->inst_return.expr, vars)));
            break;
        
        case NODE_IF_STATEMENT:
            resolve_types_in_ast(ast->if_statement.condition, current_alg, vars);
            resolve_types_in_ast(ast->if_statement.then_block, current_alg, vars);
            resolve_types_in_ast(ast->if_statement.else_block, current_alg, vars);
            temp = get_expr_known_type(ast->if_statement.condition, vars);
            if (temp != TYPE_UNKNOWN && temp != TYPE_BOOL) {
                ERRORF("Condition of 'if statement' should have type %s but is of type %s\n", value_type_to_string(TYPE_BOOL), value_type_to_string(temp));
            }
            unstable_if_unknown(temp);
            break;
        
        case NODE_DO_FOR_I:
            unify_variable_type(get_variable(get_alg_variables(current_alg), ast->do_for_i.var_name), TYPE_INT);
            resolve_types_in_ast(ast->do_for_i.start_expr, current_alg, vars);
            resolve_types_in_ast(ast->do_for_i.end_expr, current_alg, vars);
            resolve_types_in_ast(ast->do_for_i.body, current_alg, vars);
            temp = get_expr_known_type(ast->do_for_i.start_expr, vars);
            if (temp != TYPE_UNKNOWN && temp != TYPE_INT) {
                ERRORF("First born expression of 'Do for statement' should have type %s but is of type %s\n", value_type_to_string(TYPE_INT), value_type_to_string(temp));
            }
            unstable_if_unknown(temp);
            temp = get_expr_known_type(ast->do_for_i.end_expr, vars);
            if (temp != TYPE_UNKNOWN && temp != TYPE_INT) {
                ERRORF("Second born expression of 'Do for statement' should have type %s but is of type %s\n", value_type_to_string(TYPE_INT), value_type_to_string(temp));
            }
            unstable_if_unknown(temp);
            break;

        case NODE_FUNCTION:
            resolve_types_in_ast(ast->function.body, current_alg, vars);
            break;

        case NODE_SEQUENCE:
            resolve_types_in_ast(ast->sequence.first, current_alg, vars);
            resolve_types_in_ast(ast->sequence.second, current_alg, vars);
            break;

        default:
            break;
    }
}

static void resolve_types_in_unary_operation(ast_node *unary_op, algorithm *current_alg, variables_map *vars) {
    resolve_types_in_ast(unary_op->unary_operator.operand, current_alg, vars);
    switch (unary_op->unary_operator.operator) {
        case OP_NOT:
            if (check_type_ignore(get_expr_known_type(unary_op->unary_operator.operand, vars), TYPE_BOOL)) {
                unary_op->unary_operator.result_type = TYPE_BOOL;
            }
            break;
        default:
            ERROR("Unmanaged operator\n");
    }

    unstable_if_unknown(unary_op->unary_operator.result_type);
}

static void resolve_types_in_binary_operation(ast_node *binary_op, algorithm *current_alg, variables_map *vars) {
    resolve_types_in_ast(binary_op->binary_operator.left, current_alg, vars);
    resolve_types_in_ast(binary_op->binary_operator.right, current_alg, vars);
    
    switch (binary_op->binary_operator.operator) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
            if (check_binary_operation_type(binary_op, vars, TYPE_INT, TYPE_INT)) {
                binary_op->binary_operator.result_type = TYPE_INT;
            }
            break;
        case OP_AND:
        case OP_OR:
            if (check_binary_operation_type(binary_op, vars, TYPE_BOOL, TYPE_BOOL)) {
                binary_op->binary_operator.result_type = TYPE_BOOL;
            }
            break;
        default:
            ERROR("Unmanaged operator\n");
    }

    unstable_if_unknown(binary_op->binary_operator.result_type);
}

static int check_binary_operation_type(ast_node *op, variables_map *vars, value_type expected_left, value_type expected_right) {
    return check_type_ignore(get_expr_known_type(op->binary_operator.left, vars), expected_left)
        && check_type_ignore(get_expr_known_type(op->binary_operator.right, vars), expected_right);
}


//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  -------------------------   Ecriture du code   -------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
int g_writing = 0;
char *sbf;

algorithms_map *g_walgs;
algorithm *g_wcurrent;

static void write_expression_code(ast_node *expr);

static void load_var_address(const char *reg, const char *var_name) {
    variables_map *vmap = get_alg_variables(g_wcurrent);
    variable *var = get_variable(vmap, var_name);
    CF("Loading address of variable %s into %s", var_name, reg);
    if (get_variable_semantic(var) == SEM_PARAM) {
        LOAD_PARAM_ADDR(reg, R4, get_variable_pos(var), locals_count(vmap));
    } else {
        LOAD_LOCAL_ADDR(reg, R4, get_variable_pos(var));
    }
}

static void write_call_function_code(ast_node *cn) {
    // Vérifie la cohérence
    algorithm *alg = get_algorithm(g_algs, cn->call.function_name);
    int pcount = params_count(get_alg_variables(alg));
    int lcount = locals_count(get_alg_variables(alg));
    if (pcount != cn->call.params_count) {
        ERRORF("Function call %s expected %d parameters but got %d\n", get_alg_name(alg), pcount, cn->call.params_count);
    }
    CF("Preparing to call %s (%d params, %d locals)", get_alg_name(alg), pcount, lcount);
    // Valeur de retour
    PUSH(R1);
    // Empilement des parametres
    for (int i = pcount; i > 0; --i) {
        write_expression_code(cn->call.parameters_expr[i - 1]);
    }
    // Allocation des variables locales et de bp
    for (int i = 0; i < lcount; ++i) {
        PUSH(R1);
    }
    PUSH(RBP);
    CP(RBP, RSP);
    // Appel
    CF("Calling and cleaning %s", get_alg_name(alg));
    sprintf(sbf, TAG_ALGO_PREFIX "%s", get_alg_name(alg));
    CONSTSTR(R1, sbf);
    CALL(R1);
    // Dépile bp, les variables locales et les parametres,
    // laissant la valeur de retour au sommet de la pile
    POP(RBP);
    for (int i = 0; i < pcount + lcount; ++i) {
        POP(R1);
    }
}

static void write_unary_operator_code(ast_node *op) {
    write_expression_code(op->unary_operator.operand);
    switch (op->unary_operator.operator) {
        case OP_NOT: NOT(); break;
        default:
            ERROR("Unsupported unary operator during code writing\n");
    }
}

static void write_binary_operator_code(ast_node *op) {
    write_expression_code(op->binary_operator.left);
    write_expression_code(op->binary_operator.right);
    switch (op->binary_operator.operator) {
        case OP_ADD: ADD(); break;
        case OP_SUB: SUB(); break;
        case OP_MUL: MUL(); break;
        case OP_DIV: DIV(); break;
        case OP_AND: AND(); break;
        case OP_OR: OR(); break;
        default:
            ERROR("Unsupported binary operator during code writing\n");
    }
}

static void push_symbol_code(const char *symbol) {
    load_var_address(R2, symbol);
    LOADW(R1, R2);
    PUSH(R1);
}

static void write_expression_code(ast_node *expr) {
    switch (expr->type) {
        case NODE_CONST_INT:
        case NODE_CONST_BOOL:
            CONSTINT(R1, expr->number_value);
            PUSH(R1);
            break;
        case NODE_SYMBOL:
            push_symbol_code(expr->symbol_name);
            break;
        case NODE_UNARY_OPERATOR:
            write_unary_operator_code(expr);
            break;
        case NODE_BINARY_OPERATOR:
            write_binary_operator_code(expr);
            break;
        case NODE_CALL:
            write_call_function_code(expr);
            break;
        default:
            ERROR("Node type is not an expression\n");
    }
}

static void write_assignement_code(const char *var_name, ast_node *expr) {
    write_expression_code(expr);
    load_var_address(R2, var_name);
    CF("Assigning first stack value to %s", var_name);
    POP(R1);
    STOREW(R1, R2);
}

static void write_instructions(ast_node *ast) {

    if (ast == NULL) return;

    switch (ast->type) {
        case NODE_FUNCTION:
            sprintf(sbf, TAG_ALGO_PREFIX "%s", ast->function.function_name);
            TAG(sbf);
            FUNC_START();
            write_instructions(ast->function.body);
            FUNC_END_CRASH();
            break;

        case NODE_SEQUENCE:
            write_instructions(ast->sequence.first);
            write_instructions(ast->sequence.second);
            break;

        case NODE_ASSIGNEMENT:
            write_assignement_code(ast->assignement.var_name, ast->assignement.expr);
            break;

        case NODE_RETURN:
            write_expression_code(ast->inst_return.expr);
            variables_map *vmap = get_alg_variables(g_wcurrent);
            RETURN(locals_count(vmap) + params_count(vmap));
            break;

        case NODE_IF_STATEMENT:
            write_expression_code(ast->if_statement.condition);
            int if_count = counter();
            CF("IF No %d", if_count);
            if (ast->if_statement.else_block == NULL) {
                TAGCN("endif", if_count, sbf);
            } else {
                TAGCN("else", if_count, sbf);
            }
            CONSTSTR(R3, sbf);
            
            // If
            CONSTINT(R2, 0);
            POP(R1);
            CMP(R1, R2);
            JMPC(R3);
            
            // Then
            write_instructions(ast->if_statement.then_block);
            TAGCN("endif", if_count, sbf);
            CONSTSTR(R1, sbf);
            JMP(R1);

            // Else
            if (ast->if_statement.else_block != NULL) {
                TAGC("else", if_count);
                write_instructions(ast->if_statement.else_block);
            }

            // Endif
            TAGC("endif", if_count);
            CF("ENDIF No %d", if_count);
            break;

        case NODE_DO_FOR_I:
            C("Do for loop start");
            int do_for_i_count = counter();

            write_assignement_code(ast->do_for_i.var_name, ast->do_for_i.start_expr);
            
            // Début de la boucle
            TAGC("start_for_loop", do_for_i_count);
            TAGCN("end_for_loop", do_for_i_count, sbf);
            CONSTSTR(R4, sbf);
            
            write_expression_code(ast->do_for_i.end_expr);
            POP(R1);
            load_var_address(R3, ast->do_for_i.var_name);
            LOADW(R2, R3);

            ULESS(R1, R2);
            JMPC(R4);

            write_instructions(ast->do_for_i.body);

            load_var_address(R3, ast->do_for_i.var_name);
            CONSTINT(R2, 1);
            LOADW(R1, R3);
            ADD_R(R1, R2);
            STOREW(R1, R3);
            
            TAGCN("start_for_loop", do_for_i_count, sbf);
            CONSTSTR(R3, sbf);
            JMP(R3);

            TAGC("end_for_loop", do_for_i_count);
            C("Do for loop end");
            break;

        default:
            ERROR("Unsupported operation, cannot write code\n");
    }
}

static void write_instructions_pre([[ maybe_unused ]] const char *alg_name, algorithm *alg) {
    sbf = cralloc(2048);
    g_wcurrent = alg;
    write_instructions(get_alg_tree(alg));
    free(sbf);
}

static void write_start_code() {
    CONSTSTR(R1, "start");
    JMP(R1);
    
    TAG("newline");
    printf("@string \"\\n\"\n");

    ERRORTAG(ERROR_DIVISION_BY_ZERO, "Division by zero error");
}

static void write_end_code(ast_node *main_call) {
    TAG("start");

    // Initialisation de la pile
    CONSTSTR(RBP, "pile");
    CONSTSTR(RSP, "pile");
    CONSTINT(R1, 2);
    printf("\tsub %s,%s\n", RSP, R1);

    // Appel de la fonction "main"
    C("Appel principal");
    if (main_call == NULL || main_call->type != NODE_CALL) {
        ERROR("There is no main call");
    }
    write_call_function_code(main_call);

    // Affichage et fin
    C("Ici affichage de la valeur en haut de la pile et fin du programme");
    CONSTSTR(R1, "newline");
    CP(R2, RSP);
    printf("\tcallprintfd %s\n", R2);
    printf("\tcallprintfs %s\n", R1);
    printf("\tend\n");

    TAG("pile");
    printf("@int 0\n");
}

void write_all_instructions(algorithms_map *algs, ast_node *main_call) {
    if (g_writing != 0) { ERROR("Cannot write code while code is already being written\n"); }
    g_writing = 1;

    g_walgs = algs;
    write_start_code();
    foreach_algorithm(algs, write_instructions_pre);
    write_end_code(main_call);

    g_writing = 0;
}


//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------   Optimisation d'ast   ------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
#define IS_INT_CONST(val) ((val)->type == NODE_CONST_INT)
#define IS_BOOL_CONST(val) ((val)->type == NODE_CONST_BOOL)

#define TEST_LEFT_RIGHT(expr, test) (test((expr)->binary_operator.left) && test((expr)->binary_operator.right))

#define MKVAL(expr, op, make_new) make_new((expr)->binary_operator.left->number_value op (expr)->binary_operator.right->number_value)

#define CONCAT_2INT(expr, op) if (TEST_LEFT_RIGHT(expr, IS_INT_CONST)) (expr) = MKVAL(expr, op, make_int);
#define CONCAT_2BOOL(expr, op) if (TEST_LEFT_RIGHT(expr, IS_BOOL_CONST)) (expr) = MKVAL(expr, op, make_bool);

#define UNARY_MAKE_NEW(expr, condition, make_new, op) if (condition((expr)->unary_operator.operand)) (expr) = make_new(op (expr)->unary_operator.operand->number_value)

static void optimize_expr(ast_node **expr) {
    switch ((*expr)->type) {
        case NODE_UNARY_OPERATOR:
            optimize_expr(&((*expr)->unary_operator.operand));
            switch ((*expr)->unary_operator.operator) {
                case OP_NOT: UNARY_MAKE_NEW(*expr, IS_BOOL_CONST, make_bool, !); break;
                default: break;
            }
            break;
        case NODE_BINARY_OPERATOR:
            optimize_expr(&((*expr)->binary_operator.left));
            optimize_expr(&((*expr)->binary_operator.right));
            switch ((*expr)->binary_operator.operator) {
                case OP_ADD: CONCAT_2INT(*expr, +); break;
                case OP_SUB: CONCAT_2INT(*expr, -); break;
                case OP_MUL: CONCAT_2INT(*expr, *); break;
                case OP_DIV: CONCAT_2INT(*expr, /); break;

                case OP_AND: CONCAT_2BOOL(*expr, &&); break;
                case OP_OR: CONCAT_2BOOL(*expr, ||); break;
                
                default: break;
            }
            break;
        default:
            break;
    }
}

void optimize_ast(ast_node *ast) {
    switch (ast->type) {
        case NODE_FUNCTION:
            optimize_ast(ast->function.body); break;
        case NODE_SEQUENCE:
            optimize_ast(ast->sequence.first);
            optimize_ast(ast->sequence.second); break;
        case NODE_ASSIGNEMENT:
            optimize_expr(&ast->assignement.expr); break;
        case NODE_RETURN:
            optimize_expr(&ast->inst_return.expr); break;
        case NODE_IF_STATEMENT:
            optimize_expr(&ast->if_statement.condition);
            optimize_ast(ast->if_statement.then_block);
            optimize_ast(ast->if_statement.else_block); break;
        default:
            break;
    }
}


//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  --------------------------   Création d'ast   --------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
ast_node *make_int(int value) {
    ast_node *node = canode();
    node->type = NODE_CONST_INT;
    node->number_value = value;
    return node;
}

ast_node *make_bool(int bool_value) {
    ast_node *node = canode();
    node->type = NODE_CONST_BOOL;
    node->number_value = bool_value == 0 ? 0 : 1;
    return node;
}

ast_node *make_symbol(const char *symbol) {
    ast_node *node = canode();
    node->type = NODE_SYMBOL;
    node->symbol_name = mstrcpy(symbol);
    return node;
}

ast_node *make_unary_operator(unary_operator_t operator, ast_node *operand) {
    ast_node *node = canode();
    node->type = NODE_UNARY_OPERATOR;
    node->unary_operator.operand = operand;
    node->unary_operator.operator = operator;
    node->unary_operator.result_type = TYPE_UNKNOWN;
    return node;
}

ast_node *make_binary_operator(ast_node *left, binary_operator_t operator, ast_node *right) {
    ast_node *node = canode();
    node->type = NODE_BINARY_OPERATOR;
    node->binary_operator.left = left;
    node->binary_operator.operator = operator;
    node->binary_operator.right = right;
    node->binary_operator.result_type = TYPE_UNKNOWN;
    return node;
}

ast_node *make_assignement(const char *var_name, ast_node *expr) {
    ast_node *node = canode();
    node->type = NODE_ASSIGNEMENT;
    node->assignement.var_name = mstrcpy(var_name);
    node->assignement.expr = expr;
    return node;
}

ast_node *make_return(ast_node *expr) {
    ast_node *node = canode();
    node->type = NODE_RETURN;
    node->inst_return.expr = expr;
    return node;
}

ast_node *make_call(const char *function_name, ast_node **parameters, int params_count) {
    ast_node *node = canode();
    node->type = NODE_CALL;
    node->call.function_name = mstrcpy(function_name);
    node->call.parameters_expr = parameters;
    node->call.params_count = params_count;
    return node;
}

ast_node *make_if_statement(ast_node *condition, ast_node *then_block, ast_node *else_block) {
    ast_node *node = canode();
    node->type = NODE_IF_STATEMENT;
    node->if_statement.condition = condition;
    node->if_statement.then_block = then_block;
    node->if_statement.else_block = else_block;
    return node;
}

ast_node *make_do_for_i(const char *var_name, ast_node *start_expr, ast_node *end_expr, ast_node *body) {
    ast_node *node = canode();
    node->type = NODE_DO_FOR_I;
    node->do_for_i.var_name = mstrcpy(var_name);
    node->do_for_i.start_expr = start_expr;
    node->do_for_i.end_expr = end_expr;
    node->do_for_i.body = body;
    return node;
}

ast_node *make_function(const char *function_name, ast_node *body) {
    ast_node *node = canode();
    node->type = NODE_FUNCTION;
    node->function.function_name = mstrcpy(function_name);
    node->function.body = body;
    return node;
}

ast_node *make_sequence(ast_node *first, ast_node *second) {
    ast_node *node = canode();
    node->type = NODE_SEQUENCE;
    node->sequence.first = first;
    node->sequence.second = second;
    return node;
}


//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  -------------------------   Affichage d'ast   --------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
static void print_ast_aux(const ast_node *ast, int depth) {
    char *prefix = cralloc((size_t) depth * 2 + 2);
    prefix[depth * 2] = '|';
    prefix[depth * 2 + 1] = '\0';
    for (int i = 0; i < depth * 2; ++i) {
        prefix[i] = i % 2 == 0 ? '-' : ' ';
    }

    if (ast == NULL) {
        printf("%sNULL\n", prefix);
        free(prefix);
        return;
    }

    switch (ast->type) {
        case NODE_CONST_INT:
            printf("%sConst int %d\n", prefix, ast->number_value);
            break;
        case NODE_CONST_BOOL:
            printf("%sConst bool %s\n", prefix, ast->number_value == 0 ? "false" : "true");
            break;
        case NODE_SYMBOL:
            printf("%sSymbol '%s'\n", prefix, ast->symbol_name);
            break;
        case NODE_UNARY_OPERATOR:
            printf("%sUnary expression, OP: %d, result in %s\n", prefix, ast->unary_operator.operator, value_type_to_string(ast->unary_operator.result_type));
            print_ast_aux(ast->unary_operator.operand, depth + 1);
            break;
        case NODE_BINARY_OPERATOR:
            printf("%sExpression, OP: %d, result in %s\n", prefix, ast->binary_operator.operator, value_type_to_string(ast->binary_operator.result_type));
            print_ast_aux(ast->binary_operator.left, depth + 1);
            print_ast_aux(ast->binary_operator.right, depth + 1);
            break;
        case NODE_ASSIGNEMENT:
            printf("%sAssignement to '%s'\n", prefix, ast->assignement.var_name);
            print_ast_aux(ast->assignement.expr, depth + 1);
            break;
        case NODE_CALL:
            printf("%sCall to %s with %d arg(s)\n", prefix, ast->call.function_name, ast->call.params_count);
            for (int i = 0; i < ast->call.params_count; ++i) {
                print_ast_aux(ast->call.parameters_expr[i], depth + 1);
            }
            break;
        case NODE_RETURN:
            printf("%sReturn value\n", prefix);
            print_ast_aux(ast->inst_return.expr, depth + 1);
            break;
        case NODE_IF_STATEMENT:
            printf("%sIf statement\n", prefix);
            print_ast_aux(ast->if_statement.condition, depth + 1);
            print_ast_aux(ast->if_statement.then_block, depth + 1);
            print_ast_aux(ast->if_statement.else_block, depth + 1);
            break;
        case NODE_DO_FOR_I:
            printf("%sDo for %s increments\n", prefix, ast->do_for_i.var_name);
            print_ast_aux(ast->do_for_i.start_expr, depth + 1);
            print_ast_aux(ast->do_for_i.end_expr, depth + 1);
            print_ast_aux(ast->do_for_i.body, depth + 1);
            break;
        case NODE_FUNCTION:
            printf("%sFonction %s\n", prefix, ast->function.function_name);
            print_ast_aux(ast->function.body, depth + 1);
            break;
        case NODE_SEQUENCE:
            printf("%sSequence\n", prefix);
            print_ast_aux(ast->sequence.first, depth + 1);
            print_ast_aux(ast->sequence.second, depth + 1);
            break;
        default:
            printf("%sUnknown node type\n", prefix);
            break;
    }

    free(prefix);
}

void print_ast(const ast_node *ast) {
    print_ast_aux(ast, 0);
}
