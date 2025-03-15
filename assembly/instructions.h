#ifndef INSTRUCTIONS__H
#define INSTRUCTIONS__H


#define TAG_ALGO_PREFIX "algo__"

// Tag
#define TAG(tag_name) printf(":%s\n", tag_name);

// Utilitaire
#define TAGC(tag_name, count) printf(":%s__%d\n", tag_name, count);
#define TAGCN(tag_name, count, buff) sprintf(buff, "%s__%d", tag_name, count)

// Registres
#define R1 "ax"
#define R2 "bx"
#define R3 "cx"
#define R4 "dx"
#define RBP "bp"
#define RSP "sp"

// Instructions
#define CF(fmt, ...) printf("; " fmt "\n", __VA_ARGS__);
#define C(str) CF("%s", str);

#define CONSTSTR(reg, val) printf("\tconst %s,%s\n", reg, val);
#define CONSTINT(reg, val) printf("\tconst %s,%d\n", reg, val);

#define PUSH(reg) printf("\tpush %s\n", reg);
#define POP(reg) printf("\tpop %s\n", reg);

#define CP(reg1, reg2) printf("\tcp %s,%s\n", reg1, reg2);

#define CMP(reg1, reg2) printf("\tcmp %s,%s\n", reg1, reg2);
#define ULESS(reg1, reg2) printf("\tuless %s,%s\n", reg1, reg2);
#define SLESS(reg1, reg2) printf("\tsless %s,%s\n", reg1, reg2);

#define JMP(reg) printf("\tjmp %s\n", reg);
#define JMPE(reg) printf("\tjmpe %s\n", reg);
#define JMPC(reg) printf("\tjmpc %s\n", reg);
#define JMPZ(reg) printf("\tjmpz %s\n", reg);

#define LOADW(val_reg, addr_reg) printf("\tloadw %s,%s\n", val_reg, addr_reg);
#define STOREW(val_reg, addr_reg) printf("\tstorew %s,%s\n", val_reg, addr_reg);

#define CALL(reg) printf("\tcall %s\n", reg);

// Gestion des erreurs
#define ERRORTAG(tag_name, message)                                            \
    TAG("msg__" tag_name);                                                     \
    printf("@string \"" message "\"\n");                                       \
    TAG(tag_name);                                                             \
    CONSTSTR(R1, "msg__" tag_name);                                            \
    printf("\tcallprintfs %s\n", R1);

#define LOAD_ERROR_ADDR(reg, error_tag) CONSTSTR(reg, error_tag);

#define ERROR_DIVISION_BY_ZERO "error_zero_division"

// Morceaux de code
#define FUNC_START() PUSH(R1); PUSH(R2); PUSH(R3); PUSH(R4);
#define FUNC_END() POP(R4); POP(R3); POP(R2); POP(R1);
#define FUNC_END_CRASH() ;

#define LOAD_ADDR(reg, tmp_reg, offset)                                        \
    PUSH(tmp_reg);                                                             \
    CP(reg, RBP);                                                              \
    CONSTINT(tmp_reg, (offset) * 2);                                           \
    printf("\tsub %s,%s\n", reg, tmp_reg);                                     \
    POP(tmp_reg);

#define LOAD_LOCAL_ADDR(reg, tmp_reg, pos) LOAD_ADDR(reg, tmp_reg, 1 + pos);

#define LOAD_PARAM_ADDR(reg, tmp_reg, pos, lcount) LOAD_ADDR(reg, tmp_reg, 1 + lcount + pos);

#define LOAD_RETURN_ADDR(reg, tmp_reg, var_count) LOAD_ADDR(reg, tmp_reg, 1 + var_count);

// Renvoie la valeur au sommet de la pile, contient ret (termine l'appel)
#define RETURN(var_count)                                                      \
    C("Returning first stack value");                                          \
    LOAD_RETURN_ADDR(R2, R1, var_count);                                       \
    POP(R1); STOREW(R1, R2)                                                    \
    FUNC_END(); printf("\tret\n");

// Opérations
#define ADD_R(reg1, reg2) printf("\tadd %s,%s\n", reg1, reg2);

#define ADD() C("OP Add"); POP(R2); POP(R1); printf("\tadd %s,%s\n", R1, R2); PUSH(R1);
#define SUB() C("OP Sub"); POP(R2); POP(R1); printf("\tsub %s,%s\n", R1, R2); PUSH(R1);
#define MUL() C("OP Mul"); POP(R2); POP(R1); printf("\tmul %s,%s\n", R1, R2); PUSH(R1);
#define DIV() C("OP Div"); POP(R2); POP(R1);                                   \
    LOAD_ERROR_ADDR(R3, ERROR_DIVISION_BY_ZERO);                               \
    printf("\tdiv %s,%s\n", R1, R2); JMPE(R3);                                 \
    PUSH(R1);

#define AND() C("OP And"); POP(R2); POP(R1); printf("\tand %s,%s\n", R1, R2); PUSH(R1);
#define OR() C("OP Or"); POP(R2); POP(R1); printf("\tor %s,%s\n", R1, R2); PUSH(R1);

#define NOT() C("OP Not");                                                     \
    POP(R1); PUSH(R2); CONSTINT(R2, 2)                                         \
    printf("\tnot %s\n\tadd %s,%s\n", R1, R1, R2);                             \
    POP(R2); PUSH(R1);


#define BOOL_OP(operation)                                                     \
        {                                                                      \
            int op_counter = counter();                                        \
            printf("\tconst %s,op__true__%d\n", R4, op_counter);               \
            printf("\t" #operation " %s,%s\n", R1, R2);                        \
            JMPC(R4);                                                          \
            CONSTINT(R3, 0);                                                   \
            printf("\tconst %s,op__end__%d\n", R4, op_counter);                \
            JMP(R4);                                                           \
            printf(":op__true__%d\n", op_counter);                             \
            CONSTINT(R3, 1);                                                   \
            printf(":op__end__%d\n", op_counter);                              \
        }


#define EQUAL() BOOL_OP(cmp);
#define LESS() BOOL_OP(uless);

#define LESS_EQ()                                                               \
        {                                                                       \
            int op_counter = counter();                                         \
            LESS();                                                             \
            printf("\tconst %s,op__equal__%d\n", R4, op_counter);               \
            CMP(R3, R3);                                                        \
            JMPZ(R4);                                                           \
            printf("\tconst %s,op__end__%d\n", R4, op_counter);                 \
            JMP(R4);                                                            \
            printf(":op__equal__%d\n", op_counter);                             \
            EQUAL();                                                            \
            printf(":op__end__%d\n", op_counter);                               \
        }

#define EQUAL_OP() POP(R2); POP(R1); EQUAL(); PUSH(R3);
#define LESS_OP(reg1, reg2) POP(reg2); POP(reg1); LESS(); PUSH(R3);
#define LESS_EQ_OP(reg1, reg2) POP(reg2); POP(reg1); LESS_EQ(); PUSH(R3);

extern int counter();


#endif