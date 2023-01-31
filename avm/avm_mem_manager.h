//
// Created by Steli on 25/5/2022.
//

#ifndef CS_340_COMPILER_AVM_MEM_MANAGER_H
#define CS_340_COMPILER_AVM_MEM_MANAGER_H

#include <string>
#include <cstring>
#include "avm_instr_set.h"

using namespace std;

#define AVM_STACKSIZE 4096
#define AVM_STACKENV_SIZE 4
#define AVM_TABLE_HASH_SIZE 211
#define AVM_WIPEOUT(m) memset(&(m),0,sizeof(m))

enum avm_memcell_t {
    number_m = 0,
    string_m = 1,
    bool_m = 2,
    table_m = 3,
    userfunc_m = 4,
    libfunc_m = 5,
    nil_m = 6,
    undef_m = 7
};

typedef struct avm_memcell avm_memcell;

typedef struct avm_table_bucket {
    avm_memcell *key;
    avm_memcell *value;
    avm_table_bucket **next;
} avm_table_bucket;

// dynamic relational table section
typedef struct avm_table {
    unsigned int refCounter;
    //note: each bucket could be replaced with a dequeue<dequeue<>>
    avm_table_bucket *strIndexed[AVM_TABLE_HASH_SIZE];
    avm_table_bucket *numIndexed[AVM_TABLE_HASH_SIZE];
    avm_table_bucket *boolIndexed[AVM_TABLE_HASH_SIZE];
    avm_table_bucket *userfuncIndexed[AVM_TABLE_HASH_SIZE];
    avm_table_bucket *libfuncIndexed[AVM_TABLE_HASH_SIZE];
    unsigned int total;
} avm_table;

typedef struct avm_memcell {
    avm_memcell_t type;
    union {
        double numVal;
        char *strVal;
        bool boolVal;
        avm_table *tableVal;
        unsigned int funcVal; //todo:sp may need to be changed into funcval * struct
        char *libfuncVal;
    } data;
} avm_memcell;

extern avm_memcell stack[AVM_STACKSIZE];
extern avm_memcell ax, bx, cx, retval;
extern unsigned int top, topsp;
extern unsigned int totalUserFuncs;

void avm_initstack(void);

/**
 * When a cell is cleared, it destroys all
 * dynamic data content or reset its reference
 * to a table
 * @param m The memory cell to clear
 */

void avm_tableinc_refcounter(avm_table *table);

void avm_tabledec_refcounter(avm_table *table);

void avm_tablebuckets_init(avm_table_bucket **p);

void avm_tablebucket_destroy(avm_table_bucket *p);

avm_table *avm_tablenew();

void avm_tabledestroy(avm_table *table);

avm_memcell *avm_tablegetelem(avm_memcell *key);

void avm_tablesetelem(avm_memcell *key, avm_memcell *value);

avm_memcell *avm_translate_operand(vmarg *arg, avm_memcell *reg);

void avm_tablesetelem(avm_table* table, avm_memcell* index, avm_memcell* value);

avm_memcell *avm_tablegetelem(avm_table *table, avm_memcell *index);

void avm_memcellclear(avm_memcell *m);

void avm_tableIncRefCounter(avm_table *t);

void avm_warning(string msg1, string id, string msg2);

void avm_assign(avm_memcell *lv, avm_memcell *rv);

void avm_tablesetelem(avm_table* table, avm_memcell* index, avm_memcell* value);

avm_memcell *avm_tablegetelem(avm_table *table, avm_memcell *index);

extern userfunc *avm_getfuncinfo(unsigned int address);
#endif //CS_340_COMPILER_AVM_MEM_MANAGER_H
