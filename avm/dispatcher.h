#ifndef CS_340_COMPILER_DISPATCHER_H
#define CS_340_COMPILER_DISPATCHER_H
#include "avm_mem_manager.h"

#define AVM_MAX_INSTRUCTIONS    nop_v
#define AVM_NUMACTUALS_OFFSET    4
#define AVM_SAVEDPC_OFFSET       3
#define AVM_SAVEDTOP_OFFSET      2
#define AVM_SAVEDTOPSP_OFFSET    1
#define AVM_TABLE_HASHSIZE	100

#define execute_add execute_arithmetic
#define execute_sub execute_arithmetic
#define execute_mul execute_arithmetic
#define execute_div execute_arithmetic
#define execute_mod execute_arithmetic

#define execute_jle execute_comparison
#define execute_jge execute_comparison
#define execute_jgt execute_comparison
#define execute_jlt execute_comparison

extern bool executionFinished;
extern unsigned int codeSize;

typedef void (*execute_func_t)(instruction *);

userfunc *avm_getfuncinfo(unsigned int address);

void execute_cycle();

void execute_assign(instruction *instr);

void execute_add(instruction *instr);

void execute_sub(instruction *instr);

void execute_mul(instruction *instr);

void execute_div(instruction *instr);

void execute_mod(instruction *instr);

void execute_uminus(instruction *instr);

void execute_and(instruction *instr);

void execute_or(instruction *instr);

void execute_not(instruction *instr);

void execute_jeq(instruction *instr);

void execute_jne(instruction *instr);

void execute_jle(instruction *instr);

void execute_jge(instruction *instr);

void execute_jlt(instruction *instr);

void execute_jgt(instruction *instr);

void execute_call(instruction *instr);

void execute_pusharg(instruction *instr);

void execute_funcenter(instruction *instr);

void execute_funcexit(instruction *instr);

void execute_newtable(instruction *instr);

void execute_tablegetelem(instruction *instr);

void execute_tablesetelem(instruction *instr);

void execute_nop(instruction *instr);

void execute_jump(instruction *instr);

void execute_arithmetic(instruction *instr);

void execute_comparison(instruction *instr);

void execute_cycle();

void avm_initialize();

#endif //CS_340_COMPILER_DISPATCHER_H