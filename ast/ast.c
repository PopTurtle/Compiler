#include "ast.h"

#include <stdlib.h>
#include <stdio.h>

#define MAX_SCOPE_DEPTH 1024

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
    NODE_DO_WHILE,

    // Suite d'instructions
    NODE_FUNCTION,
    NODE_SEQUENCE,

    // Noeuds spéciaux
    NODE_SPEC_PARAMS_REASSIGN,
} ast_node_type;

struct ast_node {
    ast_node_type type;
    int line;
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
        struct { ast_node *condition; ast_node *body; } do_while;
        struct { char *function_name; ast_node *body; } function;
        struct { ast_node *first; ast_node *second; } sequence;
        struct { ast_node **parameters_expr; int params_count; } spec_params_reassign;
    };
};


static ast_node *make_spec_params_reassign(ast_node **params_expr, int pcount);


//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ----------------------------   Utilitaire   ----------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
static ast_node *cranode() {
    ast_node *node = (ast_node *) cralloc(sizeof(ast_node));
    node->line = -1;
    return node;
}

static int check_type_ignore(value_type current, value_type expected) {
    if (current == TYPE_UNKNOWN) {
        return 0;
    }
    if (current != expected) {
        return -1;
    }
    return 1;
}

static hashtable *hashtable_empty_cr() {
    hashtable *res = hashtable_empty((cmpfunc) strcmp, (hashfunc) str_hashfun);
    if (res == NULL) {
        ERROR("Could not allocate\n");
    }
    return res;
}


//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------   Inférence de types   ------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
static int g_resolving = 0;        // Résolution en cours ?
static int g_stable;               // Résolution en cours est stable ?
static algorithms_map *g_algs;     // Algorithmes de la résolution en cours


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
    if (node == NULL) { ERROR("Trying to get type of NULL ast node\n"); }

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
            ERROR("Trying to get the type of a node that has no type\n");
    }
}

static int check_type_expr(ast_node *expr, variables_map *vars, value_type expected) {
    value_type current = get_expr_known_type(expr, vars);
    int res = check_type_ignore(current, expected);
    if (res == -1) {
        ERRORAF(expr, "Expression is of type %s but type %s is expected\n", value_type_to_string(current), value_type_to_string(expected));
    }
    return res;
}

static void resolve_types_in_alg([[ maybe_unused ]] const char *alg_name, algorithm *alg) {
    resolve_types_in_ast(get_alg_tree(alg), alg, get_alg_variables(alg));
}

#define CHECK_TYPE(expr, expected, msg_prefix)                                 \
    temp = get_expr_known_type(expr, vars);                                    \
    if (temp != TYPE_UNKNOWN && temp != expected) {                            \
        ERRORAF(expr, msg_prefix " should have type %s but is of type %s\n", value_type_to_string(expected), value_type_to_string(temp));  \
    }                                                                          \
    unstable_if_unknown(temp);

static void resolve_types_in_ast(ast_node *ast, algorithm *current_alg, variables_map *vars) {
    if (ast == NULL) return;

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
        
        case NODE_CALL:
            for (int i = 0; i < ast->call.params_count; ++i) {
                resolve_types_in_ast(ast->call.parameters_expr[i], current_alg, vars);
                CHECK_TYPE(ast->call.parameters_expr[i], TYPE_INT, "Parameter of call")
            }
            break;

        case NODE_IF_STATEMENT:
            resolve_types_in_ast(ast->if_statement.condition, current_alg, vars);
            resolve_types_in_ast(ast->if_statement.then_block, current_alg, vars);
            resolve_types_in_ast(ast->if_statement.else_block, current_alg, vars);
            CHECK_TYPE(ast->if_statement.condition, TYPE_BOOL, "Condition of if statement");
            break;
        
        case NODE_DO_FOR_I:
            if (!variable_exists(vars, ast->do_for_i.var_name)) {
                create_local(vars, ast->do_for_i.var_name);
            }
            unify_variable_type(get_variable(get_alg_variables(current_alg), ast->do_for_i.var_name), TYPE_INT);
            resolve_types_in_ast(ast->do_for_i.start_expr, current_alg, vars);
            resolve_types_in_ast(ast->do_for_i.end_expr, current_alg, vars);
            resolve_types_in_ast(ast->do_for_i.body, current_alg, vars);
            CHECK_TYPE(ast->do_for_i.start_expr, TYPE_INT, "First born expression of Do for statement");
            CHECK_TYPE(ast->do_for_i.end_expr, TYPE_INT, "Second born expression of Do for statement");
            break;

        case NODE_DO_WHILE:
            resolve_types_in_ast(ast->do_while.condition, current_alg, vars);
            resolve_types_in_ast(ast->do_while.body, current_alg, vars);
            CHECK_TYPE(ast->do_while.condition, TYPE_BOOL, "Condition of while statement");
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
            if (check_type_expr(unary_op->unary_operator.operand, vars, TYPE_BOOL)) {
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
        case OP_EQUAL: // ne supporte pas bool == bool pour le moment
            if (check_binary_operation_type(binary_op, vars, TYPE_INT, TYPE_INT)) {
                binary_op->binary_operator.result_type = TYPE_BOOL;
            }
            break;
        case OP_SGT:
        case OP_EGT:
        case OP_SLT:
        case OP_ELT:
            if (check_binary_operation_type(binary_op, vars, TYPE_INT, TYPE_INT)) {
                binary_op->binary_operator.result_type = TYPE_BOOL;
            }
            break;
        default:
            ERROR("Unmanaged operator\n");
    }

    unstable_if_unknown(binary_op->binary_operator.result_type);
}

static int check_binary_operation_type(ast_node *op, variables_map *vars, value_type expected_left, value_type expected_right) {
    return check_type_expr(op->binary_operator.left, vars, expected_left)
        && check_type_expr(op->binary_operator.right, vars, expected_right);
}


//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  -------------------------   Ecriture du code   -------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
static int g_writing = 0;
static char *sbf;

static algorithms_map *g_walgs;
static algorithm *g_wcurrent;

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
        ERRORAF(cn, "Function call %s expected %d parameters but got %d\n", get_alg_name(alg), pcount, cn->call.params_count);
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
        case OP_EQUAL: EQUAL_OP(); break;
        case OP_SGT: LESS_OP(R2, R1); break; 
        case OP_EGT: LESS_EQ_OP(R2, R1); break; 
        case OP_SLT: LESS_OP(R1, R2); break; 
        case OP_ELT: LESS_EQ_OP(R1, R2); break; 
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
    if (expr == NULL) { ERROR("Expression is null\n"); }

    switch (expr->type) {
        case NODE_CONST_INT:
        case NODE_CONST_BOOL:
            CONSTINT(R1, expr->number_value);
            PUSH(R1);
            break;
        case NODE_SYMBOL:
            if (g_wcurrent == NULL) {
                ERRORAF(expr, "Tried to access symbol outside of any algorithm: '%s'\n", expr->symbol_name);
            }
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
            POP(R1);
            CMP(R1, R1);
            JMPZ(R3);
            
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
        
        case NODE_DO_WHILE:
            C("Do while loop start");
            int do_while_count = counter();

            // Début de la boucle
            TAGC("start_while_loop", do_while_count);

            // Evaluer la condition, jmp à la fin si !condition
            write_expression_code(ast->do_while.condition);
            TAGCN("end_while_loop", do_while_count, sbf);
            CONSTSTR(R3, sbf);
            POP(R1);
            CONSTINT(R2, 0);
            CMP(R1, R2);
            JMPC(R3);
            
            // Corps de la boucle
            write_instructions(ast->do_while.body);

            // Jump au début de la boucle
            TAGCN("start_while_loop", do_while_count, sbf);
            CONSTSTR(R3, sbf);
            JMP(R3);

            // Sortie de la boucle
            TAGC("end_while_loop", do_while_count)
            C("Do while loop end");
            break;

        case NODE_SPEC_PARAMS_REASSIGN:
            // Calcul des nouvelles valeurs
            variables_map *vars = get_alg_variables(g_wcurrent);
            for (int i = 0; i < params_count(vars); ++i) {
                write_expression_code(ast->spec_params_reassign.parameters_expr[i]);
            }
            // Assignations aux parametres
            for (int i = params_count(vars) - 1; i >= 0; --i) {
                LOAD_PARAM_ADDR(R1, R2, i, locals_count(vars));
                POP(R2);
                STOREW(R2, R1);
            }
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
        ERROR("There is no main call\n");
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
    g_wcurrent = NULL;
    write_start_code();
    foreach_algorithm(algs, write_instructions_pre);
    g_wcurrent = NULL;
    write_end_code(main_call);

    g_writing = 0;
}


//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------   Vérification d'AST   ------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
static const char *check_all_vars_assigned_aux(ast_node *ast, hashtable **assigned, int depth);

static int check_all_path_returns(ast_node *ast) {
    if (ast == NULL) {
        return 0;
    }

    switch (ast->type) {
        case NODE_RETURN:
            return 1;
        case NODE_SEQUENCE:
            return check_all_path_returns(ast->sequence.first)
                || check_all_path_returns(ast->sequence.second);
        case NODE_IF_STATEMENT:
            return check_all_path_returns(ast->if_statement.then_block)
                && check_all_path_returns(ast->if_statement.else_block);
        case NODE_DO_FOR_I:
            return check_all_path_returns(ast->do_for_i.body);
        case NODE_DO_WHILE:
            return check_all_path_returns(ast->do_while.body)
                || (ast->do_while.condition->type == NODE_CONST_BOOL
                    && ast->do_while.condition->number_value != 0); // Fix temporaire ?
        
        case NODE_ASSIGNEMENT:
        case NODE_FUNCTION:
        case NODE_SPEC_PARAMS_REASSIGN:
            return 0;

        default:
            ERROR("Unknown statement during return paths checking\n")
    }
}

static const char *check_all_vars_assigned_expr(ast_node *expr, hashtable *assigned) {
    if (expr == NULL) { ERROR("Expression is null (checking)\n"); }

    const char *tmp;
    switch (expr->type) {
        case NODE_SYMBOL:
            if (hashtable_search(assigned, expr->symbol_name) == NULL) {
                return expr->symbol_name;
            }
            return NULL;
        case NODE_CALL:
            for (int i = 0; i < expr->call.params_count; ++i) {
                tmp = check_all_vars_assigned_expr(expr->call.parameters_expr[i], assigned);
                if (tmp != NULL) return tmp;
            }
            return NULL;
        case NODE_UNARY_OPERATOR:
            return check_all_vars_assigned_expr(expr->unary_operator.operand, assigned);
        case NODE_BINARY_OPERATOR:
            tmp = check_all_vars_assigned_expr(expr->binary_operator.left, assigned);
            return tmp != NULL ? tmp : check_all_vars_assigned_expr(expr->binary_operator.right, assigned);

        case NODE_CONST_INT:
        case NODE_CONST_BOOL:
            return NULL;
        
        default:
            ERROR("Unknown statement during var assignement checking in expr\n");
    }
}

static hashtable **cv_add_depth(hashtable **assigned, int current_depth) {
    assigned[current_depth + 1] = hashtable_empty_cr();
    hashtable_extend(assigned[current_depth + 1], assigned[current_depth]);
    return assigned;
}

static void cv_rm_depth(hashtable **hashtables, int current_depth) {
    hashtable_dispose(&hashtables[current_depth + 1]);
}

static const char *cv_if_node(ast_node *ast, hashtable **assigned, int depth) {
    const char *tmp = check_all_vars_assigned_aux(ast->if_statement.then_block, cv_add_depth(assigned, depth), depth + 1);
    if (tmp != NULL) {
        return tmp;
    }
    hashtable *h1 = hashtable_empty_cr();
    hashtable_extend(h1, assigned[depth + 1]);
    cv_rm_depth(assigned, depth);
    tmp = check_all_vars_assigned_aux(ast->if_statement.else_block, cv_add_depth(assigned, depth), depth + 1);
    if (tmp != NULL) {
        return tmp;
    }

    int commons_count;
    char **commons = (char **) hashtable_inter(h1, assigned[depth + 1], &commons_count);
    for (int i = 0; i < commons_count; ++i) {
        hashtable_add(assigned[depth], commons[i], commons[i]);
    }

    return NULL;
}

static const char *check_all_vars_assigned_aux(ast_node *ast, hashtable **assigned, int depth) {
    if (ast == NULL) return NULL;

    const char *tmp;
    switch (ast->type) {
        case NODE_ASSIGNEMENT:
            tmp = check_all_vars_assigned_expr(ast->assignement.expr, assigned[depth]);
            if (tmp != NULL) return tmp;
            hashtable_add(assigned[depth], ast->assignement.var_name, ast->assignement.expr);
            return NULL;

        case NODE_SEQUENCE:
            tmp = check_all_vars_assigned_aux(ast->sequence.first, assigned, depth);
            return tmp != NULL ? tmp : check_all_vars_assigned_aux(ast->sequence.second, assigned, depth);

        case NODE_RETURN:
            return check_all_vars_assigned_expr(ast->inst_return.expr, assigned[depth]);

        case NODE_IF_STATEMENT:
            return cv_if_node(ast, assigned, depth);

        case NODE_DO_FOR_I:
            tmp = check_all_vars_assigned_expr(ast->do_for_i.start_expr, assigned[depth]);
            if (tmp == NULL) tmp = check_all_vars_assigned_expr(ast->do_for_i.end_expr, assigned[depth]);
            if (tmp != NULL) return tmp;
            cv_add_depth(assigned, depth);
            hashtable_add(assigned[depth + 1], ast->do_for_i.var_name, ast->do_for_i.start_expr);
            tmp = check_all_vars_assigned_aux(ast->do_for_i.body, assigned, depth + 1);
            cv_rm_depth(assigned, depth);
            return tmp;

        case NODE_DO_WHILE:
            tmp = check_all_vars_assigned_expr(ast->do_while.condition, assigned[depth]);
            if (tmp != NULL) return tmp;
            tmp = check_all_vars_assigned_aux(ast->do_while.body, cv_add_depth(assigned, depth), depth + 1);
            cv_rm_depth(assigned, depth);
            return tmp;
        
        case NODE_SPEC_PARAMS_REASSIGN:
            for (int i = 0; i < ast->spec_params_reassign.params_count; ++i) {
                tmp = check_all_vars_assigned_expr(ast->spec_params_reassign.parameters_expr[i], assigned[depth]);
                if (tmp != NULL) return tmp;
            }
            return NULL;

        default:
            ERROR("Unknown statement during var assignement checking\n");
    }
}

static const char *check_all_vars_assigned(ast_node *ast, algorithms_map *algs) {
    hashtable **assigned = malloc(MAX_SCOPE_DEPTH * sizeof *assigned);
    assigned[0] = hashtable_empty_cr();

    // Ajoute les parametres comme étant définis par défaut
    algorithm *current = get_algorithm(algs, ast->function.function_name);
    const char **pnames = get_all_param_names(get_alg_variables(current));
    for (int i = 0; pnames[i] != NULL; ++i) {
        hashtable_add(assigned[0], pnames[i], pnames[i]);
    }

    const char *result = check_all_vars_assigned_aux(ast->function.body, assigned, 0);
    hashtable_dispose(&assigned[0]);
    free(assigned);
    return result;
}

void check_ast_code(ast_node *ast, algorithms_map *algs) {
    if (ast->type != NODE_FUNCTION) { ERROR("The AST to verfify is not a function\n"); }
    if (!check_all_path_returns(ast->function.body)) {
        ERRORAF(ast, "Some paths do not return any value in function '%s'\n", ast->function.function_name);
    }

    const char *not_assigned_symbol = check_all_vars_assigned(ast, algs);
    if (not_assigned_symbol != NULL) {
        ERRORAF(ast, "Symbol '%s' might be used before being assigned in function '%s'\n", not_assigned_symbol, ast->function.function_name);
    }
}


//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------   Optimisation d'AST   ------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
static int g_odebug = 0;
static int g_ochanged;

#define O_DEBUGF(fmt, ...) if (g_odebug) { printf("-> " fmt "\n", __VA_ARGS__); }
#define O_DEBUG(str) O_DEBUGF("%s", str);

#define OC() (g_ochanged++)

#define RIGHT(expr) ((expr)->binary_operator.right)
#define LEFT(expr) ((expr)->binary_operator.left)

#define IS_INT_CONST(val) ((val)->type == NODE_CONST_INT)
#define IS_BOOL_CONST(val) ((val)->type == NODE_CONST_BOOL)

#define IS_ZERO(val) (IS_INT_CONST(val) && (val)->number_value == 0)
#define IS_ONE(val) (IS_INT_CONST(val) && (val)->number_value == 1)
#define IS_TRUE(val) (IS_BOOL_CONST(val) && (val)->number_value != 0)
#define IS_FALSE(val) (IS_BOOL_CONST(val) && (val)->number_value == 0)

#define TEST_LEFT_RIGHT(expr, test) (test((expr)->binary_operator.left) && test((expr)->binary_operator.right))

#define MKVAL(expr, op, make_new) make_new((expr)->binary_operator.left->number_value op (expr)->binary_operator.right->number_value)

#define CONCAT_2VAL(expr, op, is_const, mk_new, type_str) if (TEST_LEFT_RIGHT(expr, is_const)) { (expr) = MKVAL(expr, op, mk_new); OC(); O_DEBUG("Precalc " type_str " const expression " #op); break; }

#define CONCAT_2INT(expr, op) CONCAT_2VAL(expr, op, IS_INT_CONST, make_int, "int");
#define CONCAT_2BOOL(expr, op) CONCAT_2VAL(expr, op, IS_BOOL_CONST, make_bool, "bool");
#define CONCAT_2INT_RBOOL(expr, op) CONCAT_2VAL(expr, op, IS_INT_CONST, make_bool, "bool condition")

#define KEEP_LEFT(expr, condition) if (condition) { (expr) = (expr)->binary_operator.left; OC(); O_DEBUG("Keeping left side of operation"); break; }
#define KEEP_RIGHT(expr, condition) if (condition) { (expr) = (expr)->binary_operator.right; OC(); O_DEBUG("Keeping right side of operation"); break; }

#define IS_SYMBOL(val) ((val)->type == NODE_SYMBOL)
#define ARE_SAME_SYMBOL(s1, s2) (IS_SYMBOL(s1) && IS_SYMBOL(s2) && strcmp((s1)->symbol_name, (s2)->symbol_name) == 0)

static void optimize_expr(ast_node **expr_ptr) {
    ast_node *expr = *expr_ptr;

    switch ((*expr_ptr)->type) {
        case NODE_UNARY_OPERATOR:
            optimize_expr(&((*expr_ptr)->unary_operator.operand));
            
            switch ((*expr_ptr)->unary_operator.operator) {
                case OP_NOT:
                    // !true => false ; !false => true
                    if (IS_BOOL_CONST(expr->unary_operator.operand)) {
                        *expr_ptr = make_bool(!expr->unary_operator.operand->number_value);
                        OC(); O_DEBUG("Precalc bool expression");
                        break;
                    }

                    // !!var => var
                    if (expr->unary_operator.operand->type == NODE_UNARY_OPERATOR && expr->unary_operator.operand->unary_operator.operator == OP_NOT) {
                        *expr_ptr = expr->unary_operator.operand->unary_operator.operand;
                        OC(); O_DEBUG("Precalc bool expression");
                    }
                    break;

                default: break;
            }
             
            break;


        case NODE_BINARY_OPERATOR:
            optimize_expr(&((*expr_ptr)->binary_operator.left));
            optimize_expr(&((*expr_ptr)->binary_operator.right));
            switch ((*expr_ptr)->binary_operator.operator) {
                case OP_ADD:
                    // const + const => const
                    CONCAT_2INT(*expr_ptr, +);

                    // expr + 0 => expr || 0 + expr => expr
                    KEEP_LEFT(*expr_ptr, IS_ZERO(RIGHT(expr)));
                    KEEP_RIGHT(*expr_ptr, IS_ZERO(LEFT(expr)));
                    break;

                case OP_SUB:
                    // const - const => const
                    CONCAT_2INT(*expr_ptr, -);

                    // expr - 0 => expr
                    KEEP_LEFT(*expr_ptr, IS_ZERO(RIGHT(expr)));

                    // var - var => 0
                    if (ARE_SAME_SYMBOL(LEFT(expr), RIGHT(expr))) {
                        *expr_ptr = make_int(0);
                        OC(); O_DEBUG("Precalc expr - expr => 0");
                        break;
                    }

                    break;
                
                case OP_MUL:
                    // const * const => const
                    CONCAT_2INT(*expr_ptr, *);

                    // expr * 1 => expr || 1 * expr => expr
                    KEEP_LEFT(*expr_ptr, IS_ONE(RIGHT(expr)));
                    KEEP_RIGHT(*expr_ptr, IS_ONE(LEFT(expr)));

                    // expr * 0 => 0 || 0 * expr => 0
                    if (IS_ZERO(LEFT(expr)) || IS_ZERO(RIGHT(expr))) {
                        *expr_ptr = make_int(0);
                        OC(); O_DEBUG("Precalc expr * 0 => 0");
                        break;
                    }

                    break;

                case OP_DIV:
                    // const / const => const
                    CONCAT_2INT(*expr_ptr, /);

                    // expr / 1 => expr
                    KEEP_LEFT(*expr_ptr, IS_ONE(RIGHT(expr)))

                    // expr / 0 => ERROR
                    if (IS_ZERO(RIGHT(expr))) {
                        ERRORA(expr, "Division by zero");
                    }

                    // var / var => 1
                    if (ARE_SAME_SYMBOL(LEFT(expr), RIGHT(expr))) {
                        *expr_ptr = make_int(1);
                        OC(); O_DEBUG("Precalc expr / expr => 1");
                        break;
                    }

                    break;

                case OP_AND:
                    // const && const => const
                    CONCAT_2BOOL(*expr_ptr, &&);

                    // expr && true => expr || true && expr => expr
                    KEEP_LEFT(*expr_ptr, IS_TRUE(RIGHT(expr)))
                    KEEP_RIGHT(*expr_ptr, IS_TRUE(LEFT(expr)))

                    // expr && false => false || false && expr => false
                    if (IS_FALSE(LEFT(expr)) || IS_FALSE(RIGHT(expr))) {
                        *expr_ptr = make_bool(0);
                        OC(); O_DEBUG("Precalc expr && false => false");
                        break;
                    }

                    break;

                case OP_OR:
                    // const || const => const
                    CONCAT_2BOOL(*expr_ptr, ||);

                    // expr || false => expr || false || expr => expr
                    KEEP_LEFT(*expr_ptr, IS_FALSE(RIGHT(expr)))
                    KEEP_RIGHT(*expr_ptr, IS_FALSE(LEFT(expr)))

                    // expr || true => true || true || expr => true
                    if (IS_TRUE(LEFT(expr)) || IS_TRUE(RIGHT(expr))) {
                        *expr_ptr = make_bool(1);
                        OC(); O_DEBUG("Precalc expr || true => true");
                        break;
                    }

                    break;

                case OP_EQUAL:
                    // int const == int const => bool const
                    CONCAT_2INT_RBOOL(*expr_ptr, ==);

                    break;
                
                case OP_SGT:
                    // int const > int const => bool const
                    CONCAT_2INT_RBOOL(*expr_ptr, >);

                    break;

                case OP_EGT:
                    // int const >= int const => bool const
                    CONCAT_2INT_RBOOL(*expr_ptr, >=);

                    break;

                case OP_SLT:
                    // int const < int const => bool const
                    CONCAT_2INT_RBOOL(*expr_ptr, <);

                    break;

                case OP_ELT:
                    // int const <= int const => bool const
                    CONCAT_2INT_RBOOL(*expr_ptr, <=);

                    break;


                default: break;
            }
            break;

        default:
            break;
    }
}

static void optimize_const_expr(ast_node *ast) {
    if (ast == NULL) return;

    switch (ast->type) {
        case NODE_FUNCTION:
            optimize_const_expr(ast->function.body); break;
        case NODE_SEQUENCE:
            optimize_const_expr(ast->sequence.first);
            optimize_const_expr(ast->sequence.second); break;
        case NODE_ASSIGNEMENT:
            optimize_expr(&(ast->assignement.expr)); break;
        case NODE_RETURN:
            optimize_expr(&(ast->inst_return.expr)); break;
        case NODE_IF_STATEMENT:
            optimize_expr(&(ast->if_statement.condition));
            optimize_const_expr(ast->if_statement.then_block);
            optimize_const_expr(ast->if_statement.else_block); break;
        case NODE_DO_FOR_I:
            optimize_expr(&(ast->do_for_i.start_expr));
            optimize_expr(&(ast->do_for_i.end_expr));
            optimize_const_expr(ast->do_for_i.body); break;
        case NODE_DO_WHILE:
            optimize_expr(&(ast->do_while.condition));
            optimize_const_expr(ast->do_while.body); break;
        default:
            break;
    }
}

static void optimize_dead_blocks(ast_node **ast_ptr) {
    ast_node *ast = *ast_ptr;
    if (ast == NULL) return;

    switch (ast->type) {
        case NODE_FUNCTION:
            optimize_dead_blocks(&(ast->function.body)); break;
        case NODE_SEQUENCE: // if return delete after
            optimize_dead_blocks(&(ast->sequence.first));
            optimize_dead_blocks(&(ast->sequence.second));
            if (check_all_path_returns(ast->sequence.first)) {
                // La seconde partie de la sequence ne sera jamais executee
                OC(); O_DEBUG("Removing useless code after return statement");
                *ast_ptr = ast->sequence.first;
                break;
            }
            if (ast->sequence.first == NULL && ast->sequence.second == NULL) {
                OC(); O_DEBUG("Removed empty sequence");
                *ast_ptr = NULL;
                break;
            }
            ast_node *rplc_inst = NULL;
            if (ast->sequence.first == NULL) rplc_inst = ast->sequence.second;
            if (ast->sequence.second == NULL) rplc_inst = ast->sequence.first;
            if (rplc_inst != NULL) {
                *ast_ptr = rplc_inst;
                OC(); O_DEBUG("Removed half empty sequence");
            }
            break;
        
        case NODE_IF_STATEMENT:
            optimize_dead_blocks(&(ast->if_statement.then_block));
            optimize_dead_blocks(&(ast->if_statement.else_block));
            // Supprime les ifs dont les conditions sont constantes
            if (IS_BOOL_CONST(ast->if_statement.condition)) {
                OC(); O_DEBUG("If statement reduction, condition was bool constant");
                if (ast->if_statement.condition->number_value != 0) { // if (true)
                    *ast_ptr = ast->if_statement.then_block;
                } else { // if (false)
                    *ast_ptr = ast->if_statement.else_block;
                }
            }

            // Déplace le block ELSE dans THEN si le block THEN est vide
            if (ast->if_statement.then_block == NULL && ast->if_statement.else_block != NULL) {
                OC(); O_DEBUG("If statement switch, THEN block was empty while ELSE wasn't");
                ast->if_statement.condition = make_unary_operator(OP_NOT, ast->if_statement.condition);
                ast->if_statement.then_block = ast->if_statement.else_block;
                ast->if_statement.else_block = NULL;
            }
            break;

        case NODE_DO_FOR_I:
            optimize_dead_blocks(&(ast->do_for_i.body));
            if (IS_INT_CONST(ast->do_for_i.start_expr) && IS_INT_CONST(ast->do_for_i.end_expr)) {
                if (ast->do_for_i.start_expr->number_value > ast->do_for_i.end_expr->number_value) {
                    // La boucle ne fera aucun tour
                    OC(); O_DEBUG("Do for statement reduction, no iterations");
                    *ast_ptr = NULL;
                }
            }
            break;

        case NODE_DO_WHILE:
            optimize_dead_blocks(&(ast->do_while.body));
            if (IS_BOOL_CONST(ast->do_while.condition) && ast->do_while.condition->number_value == 0) {
                // La boucle ne fera aucun tour
                OC(); O_DEBUG("While statement reduction, no iterations");
                *ast_ptr = NULL;
            }
            break;

        default: break;
    }
}

struct derecursification_information {
    enum { DR_END, DR_IF_ELSE } dr_type;
    ast_node *func;
    union {
        struct { ast_node **return_s_ptr; } dr_end;
        struct {
            ast_node *condition;
            ast_node *terminate;
            ast_node **rec_call_body;
            ast_node **if_ptr;
            ast_node **return_s_ptr;
        } dr_if_else;
    };
};

static void optimize_tail_call_rebuild_func(const struct derecursification_information *infos) {
    ast_node *calln;
    switch (infos->dr_type) {
        case DR_END:
            calln = (*infos->dr_end.return_s_ptr)->inst_return.expr;
            *infos->dr_end.return_s_ptr = NULL;
            infos->func->function.body = make_do_while(
                make_bool(1),
                make_sequence(
                    infos->func->function.body,
                    make_spec_params_reassign(calln->call.parameters_expr, calln->call.params_count)
                )
            );
            break;
        
        case DR_IF_ELSE:
            calln = (*infos->dr_if_else.return_s_ptr)->inst_return.expr;
            *infos->dr_if_else.return_s_ptr = NULL;
            *infos->dr_if_else.if_ptr = NULL;
            ast_node *start_body = infos->func->function.body;
            infos->func->function.body = make_sequence(
                start_body,
                make_sequence(   
                    make_do_while(
                        make_unary_operator(OP_NOT, infos->dr_if_else.condition),
                        make_sequence(
                            make_sequence(
                                *infos->dr_if_else.rec_call_body,
                                make_spec_params_reassign(calln->call.parameters_expr, calln->call.params_count)
                            ),
                            start_body
                        )
                    ),
                    infos->dr_if_else.terminate
                )
            );
            break;

        default:
            ERROR("Derecursification informations are invalid\n");
    }
}

static int is_recursive_return(const char *alg_name, ast_node *return_s) {
    if (return_s->type != NODE_RETURN) return 0;
    if (return_s->inst_return.expr->type == NODE_CALL
        && strcmp(alg_name, return_s->inst_return.expr->call.function_name) == 0) {
            return 1;
    }
    return 0;
}

static ast_node **get_last_instruction(ast_node **ast_ptr) {
    ast_node *ast = *ast_ptr;
    
    if (ast == NULL) return NULL;

    switch (ast->type) {
        case NODE_SEQUENCE:
            ast_node **snd = get_last_instruction(&(ast->sequence.second));
            if (snd != NULL) return snd;
            return get_last_instruction(&(ast->sequence.first));

        default:
            return ast_ptr;
    }
}

// Au premier appel, (*ast_ptr)->type == NODE_FUNCTION
static struct derecursification_information *optimize_make_drec_infos(const char *alg_name, ast_node **ast_ptr) {
    ast_node *ast = *ast_ptr;
    struct derecursification_information *ret;

    if (ast == NULL) {
        return NULL;
    }

    switch (ast->type) {
        case NODE_FUNCTION:
            struct derecursification_information *result = optimize_make_drec_infos(alg_name, &(ast->function.body));
            if (result == NULL) {
                return NULL;
            }
            result->func = ast;
            return result;
        
        case NODE_SEQUENCE:
            return (ast->sequence.second == NULL) ?
                optimize_make_drec_infos(alg_name, &(ast->sequence.first)) :
                optimize_make_drec_infos(alg_name, &(ast->sequence.second));
        
        case NODE_RETURN:
            if (!is_recursive_return(alg_name, ast)) return NULL;
            ret = cralloc(sizeof *ret);
            ret->dr_type = DR_END;
            ret->dr_end.return_s_ptr = ast_ptr;
            return ret;

        case NODE_IF_STATEMENT:
            ret = cralloc(sizeof *ret);
            ast_node **last_inst = get_last_instruction(&(ast->if_statement.else_block));
            if (is_recursive_return(alg_name, *last_inst)) {
                ret->dr_type = DR_IF_ELSE;
                ret->dr_if_else.condition = ast->if_statement.condition;
                ret->dr_if_else.rec_call_body = &(ast->if_statement.else_block);
                ret->dr_if_else.terminate = ast->if_statement.then_block;
                ret->dr_if_else.if_ptr = ast_ptr;
                ret->dr_if_else.return_s_ptr = last_inst;
                return ret;
            }
            last_inst = get_last_instruction(&(ast->if_statement.then_block));
            if (is_recursive_return(alg_name, *last_inst)) {
                ret->dr_type = DR_IF_ELSE;
                ret->dr_if_else.condition = make_unary_operator(OP_NOT, ast->if_statement.condition);
                ret->dr_if_else.rec_call_body = &(ast->if_statement.then_block);
                ret->dr_if_else.terminate = ast->if_statement.else_block;
                ret->dr_if_else.if_ptr = ast_ptr;
                ret->dr_if_else.return_s_ptr = last_inst;
                return ret;
            }
            free(ret);
            return NULL;
        default:
            return NULL;
    }
}

static void optimize_tail_call_recursion(algorithm *alg, ast_node *ast) {
    if (ast == NULL || ast->type != NODE_FUNCTION) return;

    struct derecursification_information *infos =
        optimize_make_drec_infos(get_alg_name(alg), &ast);

    if (infos == NULL) {
        return;
    }

    optimize_tail_call_rebuild_func(infos);
    free(infos);

    OC();
    O_DEBUGF("Tail call recursion removed for function %s", get_alg_name(alg));
}

void optimize_ast(algorithms_map *algs, ast_node *ast, int debug) {
    if (ast->type != NODE_FUNCTION) {
        ERROR("Cannot optimize a non function AST\n");
    }

    g_odebug = debug;
    if (debug) printf("Optimize start for function %s\n", ast->function.function_name);
    
    do {
        g_ochanged = 0;

        optimize_const_expr(ast);
        
        optimize_dead_blocks(&ast);

        algorithm *alg = get_algorithm(algs, ast->function.function_name);
        optimize_tail_call_recursion(alg, ast);

    } while (g_ochanged > 0);

    if (debug) printf("Optimize end\n\n");
}


//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  --------------------------   Création d'AST   --------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
ast_node *make_int(int value) {
    ast_node *node = cranode();
    node->type = NODE_CONST_INT;
    node->number_value = value;
    return node;
}

ast_node *make_bool(int bool_value) {
    ast_node *node = cranode();
    node->type = NODE_CONST_BOOL;
    node->number_value = bool_value == 0 ? 0 : 1;
    return node;
}

ast_node *make_symbol(const char *symbol) {
    ast_node *node = cranode();
    node->type = NODE_SYMBOL;
    node->symbol_name = mstrcpy(symbol);
    return node;
}

ast_node *make_unary_operator(unary_operator_t operator, ast_node *operand) {
    ast_node *node = cranode();
    node->type = NODE_UNARY_OPERATOR;
    node->unary_operator.operand = operand;
    node->unary_operator.operator = operator;
    node->unary_operator.result_type = TYPE_UNKNOWN;
    return node;
}

ast_node *make_binary_operator(ast_node *left, binary_operator_t operator, ast_node *right) {
    ast_node *node = cranode();
    node->type = NODE_BINARY_OPERATOR;
    node->binary_operator.left = left;
    node->binary_operator.operator = operator;
    node->binary_operator.right = right;
    node->binary_operator.result_type = TYPE_UNKNOWN;
    return node;
}

ast_node *make_assignement(const char *var_name, ast_node *expr) {
    ast_node *node = cranode();
    node->type = NODE_ASSIGNEMENT;
    node->assignement.var_name = mstrcpy(var_name);
    node->assignement.expr = expr;
    return node;
}

ast_node *make_return(ast_node *expr) {
    ast_node *node = cranode();
    node->type = NODE_RETURN;
    node->inst_return.expr = expr;
    return node;
}

ast_node *make_call(const char *function_name, ast_node **parameters, int params_count) {
    ast_node *node = cranode();
    node->type = NODE_CALL;
    node->call.function_name = mstrcpy(function_name);
    node->call.parameters_expr = parameters;
    node->call.params_count = params_count;
    return node;
}

ast_node *make_if_statement(ast_node *condition, ast_node *then_block, ast_node *else_block) {
    ast_node *node = cranode();
    node->type = NODE_IF_STATEMENT;
    node->if_statement.condition = condition;
    node->if_statement.then_block = then_block;
    node->if_statement.else_block = else_block;
    return node;
}

ast_node *make_do_for_i(const char *var_name, ast_node *start_expr, ast_node *end_expr, ast_node *body) {
    ast_node *node = cranode();
    node->type = NODE_DO_FOR_I;
    node->do_for_i.var_name = mstrcpy(var_name);
    node->do_for_i.start_expr = start_expr;
    node->do_for_i.end_expr = end_expr;
    node->do_for_i.body = body;
    return node;
}

ast_node *make_do_while(ast_node *condition, ast_node *body) {
    ast_node *node = cranode();
    node->type = NODE_DO_WHILE;
    node->do_while.condition = condition;
    node->do_while.body = body;
    return node;
}

ast_node *make_function(const char *function_name, ast_node *body) {
    ast_node *node = cranode();
    node->type = NODE_FUNCTION;
    node->function.function_name = mstrcpy(function_name);
    node->function.body = body;
    return node;
}

ast_node *make_sequence(ast_node *first, ast_node *second) {
    ast_node *node = cranode();
    node->type = NODE_SEQUENCE;
    node->sequence.first = first;
    node->sequence.second = second;
    return node;
}

ast_node *make_spec_params_reassign(ast_node **params_expr, int pcount) {
    ast_node *node = cranode();
    node->type = NODE_SPEC_PARAMS_REASSIGN;
    node->spec_params_reassign.parameters_expr = params_expr;
    node->spec_params_reassign.params_count = pcount;
    return node;
}

extern int get_line(const ast_node *node) {
    return node->line;
}

extern void set_line(ast_node *node, int line) {
    node->line = line;
}


//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  -------------------------   Affichage d'AST   --------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
//  ------------------------------------------------------------------------  //
#define D (depth + 1)

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
            print_ast_aux(ast->unary_operator.operand, D);
            break;
        case NODE_BINARY_OPERATOR:
            printf("%sExpression, OP: %s, result in %s\n", prefix, b_op_to_str(ast->binary_operator.operator), value_type_to_string(ast->binary_operator.result_type));
            print_ast_aux(ast->binary_operator.left, D);
            print_ast_aux(ast->binary_operator.right, D);
            break;
        case NODE_ASSIGNEMENT:
            printf("%sAssignement to '%s'\n", prefix, ast->assignement.var_name);
            print_ast_aux(ast->assignement.expr, D);
            break;
        case NODE_CALL:
            printf("%sCall to %s with %d arg(s)\n", prefix, ast->call.function_name, ast->call.params_count);
            for (int i = 0; i < ast->call.params_count; ++i) {
                print_ast_aux(ast->call.parameters_expr[i], D);
            }
            break;
        case NODE_RETURN:
            printf("%sReturn value\n", prefix);
            print_ast_aux(ast->inst_return.expr, D);
            break;
        case NODE_IF_STATEMENT:
            printf("%sIf statement\n", prefix);
            print_ast_aux(ast->if_statement.condition, D);
            print_ast_aux(ast->if_statement.then_block, D);
            print_ast_aux(ast->if_statement.else_block, D);
            break;
        case NODE_DO_FOR_I:
            printf("%sDo for %s increments\n", prefix, ast->do_for_i.var_name);
            print_ast_aux(ast->do_for_i.start_expr, D);
            print_ast_aux(ast->do_for_i.end_expr, D);
            print_ast_aux(ast->do_for_i.body, D);
            break;
        case NODE_DO_WHILE:
            printf("%sDo while\n", prefix);
            print_ast_aux(ast->do_while.condition, D);
            print_ast_aux(ast->do_while.body, D);
            break;
        case NODE_FUNCTION:
            printf("%sFonction '%s'\n", prefix, ast->function.function_name);
            print_ast_aux(ast->function.body, D);
            break;
        case NODE_SEQUENCE:
            printf("%sSequence\n", prefix);
            print_ast_aux(ast->sequence.first, D);
            print_ast_aux(ast->sequence.second, D);
            break;
        case NODE_SPEC_PARAMS_REASSIGN:
            printf("%sParameters reassignement\n", prefix);
            for (int i = 0; i < ast->spec_params_reassign.params_count; ++i) {
                print_ast_aux(ast->spec_params_reassign.parameters_expr[i], D);
            }
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
