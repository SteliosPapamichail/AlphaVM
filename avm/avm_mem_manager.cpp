//
// Created by Steli on 25/5/2022.
//

#include "avm_mem_manager.h"

extern bool DEBUG_ENABLED;
extern vector<double> numConsts;
extern vector <string> strConsts;
extern vector<bool> boolConsts;
extern vector <string> libFuncs;
extern vector <userfunc> userFuncs;

avm_memcell stack[AVM_STACKSIZE];
avm_memcell ax, bx, cx, retval;
unsigned int top, topsp = AVM_STACKSIZE - 1;
unsigned int totalUserFuncs;

void avm_initstack(void) {
    for (unsigned int i = 0; i < AVM_STACKSIZE; i++) {
        AVM_WIPEOUT(stack[i]);
        stack[i].type = undef_m;
    }
}

void avm_tableinc_refcounter(avm_table *table) {
    ++table->refCounter;
}

void avm_tabledec_refcounter(avm_table *table) {
    assert(table->refCounter > 0);
    if (!--table->refCounter) avm_tabledestroy(table); //garbage collection
}

void avm_tablebuckets_init(avm_table_bucket **p) {
    for (unsigned int i = 0; i < AVM_TABLE_HASH_SIZE; i++) {
        p[i] = (avm_table_bucket *) 0;
    }
}

avm_table *avm_tablenew() {
    avm_table *table = new avm_table;
    AVM_WIPEOUT(*table);
    table->refCounter = table->total = 0;
    avm_tablebuckets_init(table->numIndexed);
    avm_tablebuckets_init(table->strIndexed);
    avm_tablebuckets_init(table->boolIndexed);
    avm_tablebuckets_init(table->userfuncIndexed);
    avm_tablebuckets_init(table->libfuncIndexed);
    return table;
}

void memclear_table(avm_memcell *m) {
    assert(m->data.tableVal);
    avm_tabledec_refcounter(m->data.tableVal);
}
typedef void (*memclear_func_t)(avm_memcell *);
memclear_func_t memclearFuncs[] = {
        0,                    //number
        0, // string
        0,                    //bool
        memclear_table,
        0,                    //userfunc
        0,                    //libfunc
        0,                    //nil
        0                    //undef
};

void avm_memcellclear(avm_memcell *m) {
    if (m->type != undef_m) {
        memclear_func_t f = memclearFuncs[m->type];
        if (f) (*f)(m);
        m->type = undef_m;
    }
}
void avm_tablebucket_destroy(avm_table_bucket **p) {
    for (unsigned int i = 0; i < AVM_TABLE_HASH_SIZE; i++) {
        for (avm_table_bucket *b = *p; b;) {
            avm_table_bucket *del = b;
            b = *b->next;
            avm_memcellclear(del->key);
            avm_memcellclear(del->value);
            delete del;
        }
        p[i] = nullptr;
    }
}

void avm_tabledestroy(avm_table *table) {
    avm_tablebucket_destroy(table->numIndexed);
    avm_tablebucket_destroy(table->strIndexed);
    avm_tablebucket_destroy(table->boolIndexed);
    avm_tablebucket_destroy(table->userfuncIndexed);
    avm_tablebucket_destroy(table->libfuncIndexed);
    delete table;
}

avm_memcell *avm_translate_operand(vmarg *arg, avm_memcell *reg) {
    if(arg->is_null) {
        if(DEBUG_ENABLED) cout << arg->type << endl;
        if(DEBUG_ENABLED) cout << "arg was null for translate_operand()" << endl;
        return reg; //dont know if its right
    }
    if (reg==nullptr) reg = new avm_memcell;
    switch (arg->type) {
        case global_a :
            if(DEBUG_ENABLED) cout<<"global_a with offset " << arg->val <<endl;
            if(DEBUG_ENABLED) cout << "top val is " << top << " and index is " << AVM_STACKSIZE-1-arg->val << endl;
            return (&stack[AVM_STACKSIZE - 1 - arg->val]);
        case local_a :
            if(DEBUG_ENABLED) cout<<"local_a"<<endl;
            return (&stack[topsp - arg->val]);
        case formal_a :
            if(DEBUG_ENABLED) cout<<"formal_a"<<endl;
            if(DEBUG_ENABLED) cout<<"topsp + AVM_STACKENV_SIZE + 1 + arg->val = "<<topsp + AVM_STACKENV_SIZE + 1 + arg->val<<endl;
            return (&stack[topsp + AVM_STACKENV_SIZE + 1 + arg->val]);
        case retval_a :
            if(DEBUG_ENABLED) cout<<"retval_a"<<endl;
            return (&retval);
        case number_a :
            if(DEBUG_ENABLED) cout<<"number_a"<<endl;
            reg->type = number_m;
            reg->data.numVal = numConsts.at(arg->val);
            return reg;
        case string_a :
            if(DEBUG_ENABLED) cout<<"string_a"<<endl;
            reg->type = string_m;
            reg->data.strVal = (char*)strConsts.at(arg->val).c_str();
            return reg;
        case bool_a :
            if(DEBUG_ENABLED) cout<<"bool_a"<<endl;
            reg->type = bool_m;
            reg->data.boolVal = boolConsts.at(arg->val);
            return reg;
        case nil_a :
            if(DEBUG_ENABLED) cout<<"nil_a"<<endl;
            reg->type = nil_m;
            return reg;
        case userfunc_a :
            if(DEBUG_ENABLED) cout<<"userfunc_a in translate with address " << avm_getfuncinfo(arg->val)->address <<endl;
            reg->type = userfunc_m;
            reg->data.funcVal = avm_getfuncinfo(arg->val)->address;
            return reg;
        case libfunc_a :
            if(DEBUG_ENABLED) cout<<"libfunc_a"<<endl;
            reg->type = libfunc_m;
            reg->data.libfuncVal = strdup(libFuncs.at(arg->val).c_str());
            return reg;
        case label_a :
            if(DEBUG_ENABLED) cout<<"label_a used with make_operand"<<endl;
        default :
            assert(0);
    }
    return reg;
}

void avm_tableIncRefCounter(avm_table *t) { ++t->refCounter; }

void avm_warning(string msg1, string id, string msg2) {
    if (!msg1.empty()) cerr << msg1 << " ";
    if (!id.empty()) cerr << id << " ";
    if (!msg2.empty()) cerr << msg2;
    cerr << endl;
}

void avm_assign(avm_memcell *lv, avm_memcell *rv) {
    if (lv == rv) return;
    if (lv->type == table_m && rv->type == table_m && lv->data.tableVal == rv->data.tableVal) return;
    if (rv->type == undef_m) avm_warning("assigning from 'undef' content !", "", "");
    avm_memcellclear(lv);
    memcpy(lv, rv, sizeof(avm_memcell)); //dispatch in cpp
    if (lv->type == string_m) lv->data.strVal = rv->data.strVal;
    else if (lv->type == table_m) avm_tableIncRefCounter(lv->data.tableVal);
}

void avm_tablesetelem(avm_table* table, avm_memcell* index, avm_memcell* value){
	assert(table);
	int indexOfTable;
	avm_memcell* elem = avm_tablegetelem(table,index);
	if(index->type == string_m){
		indexOfTable = (int) strlen(index->data.strVal) % AVM_TABLE_HASH_SIZE;
		if(elem){
			avm_assign(elem,value);
			return;
		}
        if(!table->strIndexed[indexOfTable]){
                table->strIndexed[indexOfTable] = new avm_table_bucket;
                avm_table_bucket* newBucket = new avm_table_bucket;
        		newBucket->key = index;
        		if(value->type == table_m) avm_tableIncRefCounter(value->data.tableVal);
        		newBucket->value = value;
        		table->strIndexed[indexOfTable] = newBucket ;
                table->strIndexed[indexOfTable]->next = NULL;
                table->total++;
                return;
        }
        avm_table_bucket* tmp=table->strIndexed[indexOfTable];
        while(tmp->next){ tmp=*tmp->next; }
        avm_table_bucket* newBucket = new avm_table_bucket;
        newBucket->key = index;
        if(value->type == table_m) avm_tableIncRefCounter(value->data.tableVal);
        newBucket->value = value;
        tmp->next = &newBucket;
        newBucket->next = NULL;
        table->total++;
	}else if(index->type == number_m){
		indexOfTable = (int)index->data.numVal % AVM_TABLE_HASH_SIZE;
		if(elem){
			avm_assign(elem,value);
			return;
		}
		if(!table->numIndexed[indexOfTable]){
		  	table->numIndexed[indexOfTable] = new avm_table_bucket;
            avm_table_bucket* newBucket = new avm_table_bucket;
    		newBucket->key->data.numVal = index->data.numVal;
    		if(value->type == table_m) avm_tableIncRefCounter(value->data.tableVal);
    		newBucket->value = value;
    		table->numIndexed[indexOfTable] = newBucket ;
            table->numIndexed[indexOfTable]->next = NULL;
            table->total++;
            return;
		}
		avm_table_bucket* tmp = table->numIndexed[indexOfTable];
		while(tmp->next){ tmp = *tmp->next; }
		avm_table_bucket* newBucket = new avm_table_bucket;
		newBucket->key = index;
		if(value->type == table_m) avm_tableIncRefCounter(value->data.tableVal);
		newBucket->value = value;
		tmp->next = &newBucket;
		newBucket->next = NULL;
		table->total++;
	}
}

avm_memcell *avm_tablegetelem(avm_table *table, avm_memcell *index){
	assert(table);
	int indexOfTable;
	if(index->type == string_m){
		indexOfTable = (int) strlen(index->data.strVal) % AVM_TABLE_HASH_SIZE;
		if(table->strIndexed[indexOfTable]){
			avm_table_bucket* tmp = table->strIndexed[indexOfTable];
			while(tmp){
				if( !strcmp(tmp->key->data.strVal, index->data.strVal) ) return tmp->value;
				tmp = *tmp->next;
			}
		}
	}else if(index->type == number_m){
		indexOfTable = (int) index->data.numVal % AVM_TABLE_HASH_SIZE;
		if(table->numIndexed[indexOfTable]){
			avm_table_bucket* tmp = table->numIndexed[indexOfTable];
			while(tmp){
				if(tmp->key->data.numVal == index->data.numVal) return tmp->value;
				tmp = *tmp->next;
			}
		}	
	}
	return NULL;
}
