hashtable_dir = ../hashtable/
ast_dir = ../ast/
data_dir = ../data/
utils_dir = ../utils/
assembly_dir = ../assembly/

SHELL=/bin/sh
LEX=flex
YACC=bison
CC = gcc
CFLAGS = -std=c2x \
  -Wall -Wconversion -Wextra -Wpedantic -Wwrite-strings \
  -Og -g \
  -I$(hashtable_dir) -I$(ast_dir) -I$(data_dir) -I$(utils_dir) -I$(assembly_dir) \
  -DHASHTABLE_STATS=0

vpath %.c $(hashtable_dir) $(ast_dir) $(data_dir) $(utils_dir) $(assembly_dir)
vpath %.h $(hashtable_dir) $(ast_dir) $(data_dir) $(utils_dir) $(assembly_dir)
objects = compiler.o hashtable.o ast.o value_type.o algorithms.o variables.o utils.o instructions.o

# --nounput: ne genere pas la fonction yyunput() inutile
# --DYY_NO_INPUT: ne prend pas en compte la fonction input() inutile
# -D_POSIX_SOURCE: declare la fonction fileno()
LEXOPTS=-D_POSIX_SOURCE -DYY_NO_INPUT --nounput
YACCOPTS=

executable = algosipro
makefile_indicator = .\#makefile\#

.PHONY: all clean

all: $(executable)

clean:
	-rm lex.yy.* $(executable).tab.* *.err *.output *.out *.dot
	$(RM) $(objects) $(executable)
	@$(RM) $(makefile_indicator)

$(executable): lex.yy.o $(executable).tab.o $(objects)
	$(CC) $+ -o $@ $(LDFLAGS)


lex.yy.c: $(executable).l $(executable).tab.h
	$(LEX) $(LEXOPTS) $<

lex.yy.h: $(executable).l
	$(LEX) $(LEXOPTS) --header-file=$@ $<

$(executable).tab.c $(executable).tab.h: $(executable).y lex.yy.h
	$(YACC) $(YACCOPTS) $< -d -v --graph


compiler.o: compiler.c ast.h algorithms.h variables.h
hashtable.o: hashtable.c hashtable.h
ast.o: ast.c ast.h algorithms.h value_type.h utils.h instructions.h
value_type.o: value_type.c value_type.h
algorithms.o: algorithms.c algorithms.h hashtable.h ast.h value_type.h variables.h utils.h
variables.o: variables.c hashtable.h value_type.h variables.h utils.h
utils.o: utils.c utils.h
instructions.o: instructions.c instructions.h

include $(makefile_indicator)

$(makefile_indicator): makefile
	@touch $@
	@$(RM) $(objects) $(executable)
