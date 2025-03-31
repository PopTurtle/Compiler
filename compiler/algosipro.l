%{
	#include <stdio.h>
	#include <stdlib.h>
	#include <limits.h>
  #include <string.h>
	#include "algosipro.tab.h"


  #define YYLLOC_DEFAULT(yylval, yylloc, lineno) yylloc.first_line = yylloc.last_line = lineno;
  #define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;

	void string_to_int(int *r, const char *s);
%}

%option yylineno
%option noyywrap
entier	0|-?[1-9][0-9]*
boolean "true"|"false"
symbol  [a-zA-Z_][a-zA-Z0-9_]{0,254}

%%


"\\begin{algo}"         { return START; }
"\\end{algo}"           { return END; }

"\\CALL"                { return INST_CALL; }

"\\SET"                 { return INST_SET; }
"\\RETURN"              { return INST_RETURN; }

"\\INCR"                { return INST_INCR; }
"\\DECR"                { return INST_DECR; }

"\\IF"                  { return INST_IF; }
"\\ELSE"                { return INST_ELSE; }
"\\FI"                  { return INST_ENDIF; }

"\\DOWHILE"             { return INST_DOWHILE; }
"\\DOFORI"              { return INST_DOFORI; }
"\\OD"                  { return INST_OD; }

{entier}						                    { string_to_int(&yylval.int_value, yytext); return INT_VALUE; }
{boolean}                               { yylval.bool_value = strcmp(yytext, "false"); return BOOL_VALUE; }
{symbol}                                { strcpy(yylval.symbol, yytext); return SYMBOL; }
"+"|"-"|"*"|"/"|"{"|"}"|","|"("|")"     { return yytext[0]; }

"&&"   { return I_OP_AND; }
"||"   { return I_OP_OR; }
"!"    { return I_OP_NOT; }
"=="   { return I_OP_EQUAL; }
">"    { return I_OP_SGT; }
">="   { return I_OP_EGT; }
"<"    { return I_OP_SLT; }
"<="   { return I_OP_ELT; }


.								                        { }
\n								                      { }

%%

void string_to_int(int *r, const char *s) {
  char *p;
  long v;
  errno = 0;
  v = strtol(s, &p, 10);
  if ( ( *p != '\0' || ( errno == ERANGE 
                     && ( v == LONG_MIN || v == LONG_MAX ) ) ) 
       || ( v < INT_MIN || v > INT_MAX ) ) {
    fprintf(stderr, "Error converting string to int\n");
    exit(EXIT_FAILURE);
  } 
  *r = (int) v;
}
