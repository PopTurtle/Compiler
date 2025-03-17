%{
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
    #include "ast.h"
    #include "algorithms.h"
    #include "variables.h"
    #include "compiler.h"
	#include "utils.h"

    static algorithms_map *g_algs_map;
    static algorithm *g_current_alg;
    static variables_map *g_current_vars;
	static ast_node *first_call;

	#define ARGS_DATA_BUF_INIT 2
	#define ARGS_DATA_BUF_MUL 2

	// Structures
	struct args_data {
		ast_node **params;
		int params_count;
		int __buff_size;
	};

	// Prototypes
	int yylex();
	void yyerror(char const *);

	struct args_data *args_data_empty();
	void args_data_add(struct args_data *args, ast_node *expr);
%}

%define parse.error verbose

%union {
	int int_value;
	int bool_value;

    struct ast_node* node_type;
	struct args_data* args;

	char symbol[256];
}

%token START
%token END

%token<int_value> INT_VALUE
%token<bool_value> BOOL_VALUE;
%token<symbol> SYMBOL

%token INST_CALL;

%token INST_SET
%token INST_RETURN
%token INST_IF
%token INST_ELSE
%token INST_ENDIF
%token INST_DOWHILE
%token INST_DOFORI
%token INST_OD

%token I_OP_AND
%token I_OP_OR
%token I_OP_EQUAL
%token I_OP_SGT
%token I_OP_EGT
%token I_OP_SLT
%token I_OP_ELT

%token I_OP_NOT

%type<node_type> BLOCK
%type<node_type> STATEMENT
%type<node_type> EXPR
%type<node_type> FUNCTION_CALL
%type<args> ARGS_LIST

%left '+' '-'
%left '*' '/'
%left ','
%left I_OP_NOT
%left I_OP_AND
%left I_OP_OR
%left I_OP_EQUAL I_OP_SGT I_OP_EGT I_OP_SLT I_OP_ELT

%start FULL_CODE
%%


// Fichier de code complet
FULL_CODE:
	ALGOS FUNCTION_CALL {
		first_call = $2;
	}
;

ALGOS:
      ALGO
	| ALGOS ALGO
;

// Définition d'un algorithme (Fonction)
ALGO:
	 START '{' NEW_FUNC_NAME '}' '{' PARAMS_LIST '}' BLOCK END {
        associate_tree(g_current_alg, make_function(get_alg_name(g_current_alg), $8));
	}
;

NEW_FUNC_NAME:
	 SYMBOL {
        g_current_alg = create_algorithm(g_algs_map, $1);
        g_current_vars = get_alg_variables(g_current_alg);
	}
;

PARAMS_LIST:
	| SYMBOL	 						{ create_parameter(g_current_vars, $1); }
	| PARAMS_LIST ',' PARAMS_LIST	 	{ ; }
;

// Définition de morceaux de code
BLOCK:
	 STATEMENT				{ $$ = $1; }
	| STATEMENT BLOCK		{ $$ = make_sequence($1, $2); }
;

STATEMENT:
      INST_SET '{' SYMBOL '}' '{' EXPR '}' {
        $$ = make_assignement($3, $6);
        if (!variable_exists(g_current_vars, $3)) {
            create_local(g_current_vars, $3);
        }
      }
	| INST_RETURN '{' EXPR '}' {
        $$ = make_return($3);
	  }
	| INST_IF '{' EXPR '}' BLOCK INST_ENDIF {
		$$ = make_if_statement($3, $5, NULL);
	}
	| INST_IF '{' EXPR '}' BLOCK INST_ELSE BLOCK INST_ENDIF {
		$$ = make_if_statement($3, $5, $7);
	}
	| INST_DOWHILE '{' EXPR '}' BLOCK INST_OD {
		$$ = make_do_while($3, $5);
	}
	| INST_DOFORI '{' SYMBOL '}' '{' EXPR '}' '{' EXPR '}' BLOCK INST_OD {
		$$ = make_do_for_i($3, $6, $9, $11);
	}
;

EXPR:
	  '(' EXPR ')'				{ $$ = $2; }
	| EXPR '+' EXPR				{ $$ = make_binary_operator($1, OP_ADD, $3); }
	| EXPR '-' EXPR				{ $$ = make_binary_operator($1, OP_SUB, $3); }
	| EXPR '*' EXPR				{ $$ = make_binary_operator($1, OP_MUL, $3); }
	| EXPR '/' EXPR				{ $$ = make_binary_operator($1, OP_DIV, $3); }
	| EXPR I_OP_AND EXPR		{ $$ = make_binary_operator($1, OP_AND, $3); }
	| EXPR I_OP_OR EXPR			{ $$ = make_binary_operator($1, OP_OR, $3); }
	| EXPR I_OP_EQUAL EXPR		{ $$ = make_binary_operator($1, OP_EQUAL, $3); }
	| EXPR I_OP_SGT EXPR		{ $$ = make_binary_operator($1, OP_SGT, $3); }
	| EXPR I_OP_EGT EXPR		{ $$ = make_binary_operator($1, OP_EGT, $3); }
	| EXPR I_OP_SLT EXPR		{ $$ = make_binary_operator($1, OP_SLT, $3); }
	| EXPR I_OP_ELT EXPR		{ $$ = make_binary_operator($1, OP_ELT, $3); }
	| I_OP_NOT EXPR				{ $$ = make_unary_operator(OP_NOT, $2); }
	| INT_VALUE					{ $$ = make_int($1); }
	| BOOL_VALUE				{ $$ = make_bool($1); }
	| SYMBOL                    {
        $$ = make_symbol($1);
    }
	| FUNCTION_CALL {
		$$ = $1;
	}

FUNCTION_CALL:
	  INST_CALL '{' SYMBOL '}' '{' ARGS_LIST '}' {
		$$ = make_call($3, $6->params, $6->params_count);
	}

ARGS_LIST:
	  { $$ = args_data_empty(); }
	| EXPR {
		$$ = args_data_empty();
		args_data_add($$, $1);
	}
	| ARGS_LIST ',' EXPR {
		$$ = $1;
		args_data_add($$, $3);
	}

%%

void yyerror(char const *s) {
	fprintf(stderr, "%s\n", s);
}

int main(int argc, char *argv[]) {
    g_algs_map = create_algorithms_map();
    g_current_alg = NULL;
    g_current_vars = NULL;

	yyparse();

    //print_algorithm(g_current_alg);
    compile_code(argc, argv, g_algs_map, first_call);

	return EXIT_SUCCESS;
}


struct args_data *args_data_empty() {
	struct args_data *ret = malloc(sizeof *ret);
	if (ret == NULL) {
		ERROR("Could not allocate\n");
	}
	ret->params = malloc(ARGS_DATA_BUF_INIT * sizeof *ret->params);
	if (ret->params == NULL) {
		ERROR("Could not allocate\n");
	}
	ret->__buff_size = ARGS_DATA_BUF_INIT;
	ret->params_count = 0;
	return ret;
}

void args_data_add(struct args_data *args, ast_node *expr) {
	if (args->params_count == args->__buff_size) {
		args->__buff_size *= ARGS_DATA_BUF_MUL;
		args->params = realloc(args->params, (size_t)args->__buff_size * sizeof *args->params);
		if (args->params == NULL) {
			ERROR("Could not allocate\n");
		}
	}
	args->params[args->params_count++] = expr;
}
