#include "dispatcher.h"
#include <cmath>

bool executionFinished = false;
unsigned int pc = 0;
unsigned int currLine = 0;
unsigned int codeSize = 0;
unsigned int totalActuals = 0;
unsigned int code = 0;
int avm_infunc = 0;
extern unsigned int NUM_OF_GLOBALS;
extern bool DEBUG_ENABLED;
#define AVM_ENDING_PC   codeSize

// defined in avm_impl.cpp. Should be populated during
// binary reading
extern vector <instruction> instructions;
extern vector<binding *> funcStack;
extern vector<double> numConsts;
extern vector <string> strConsts;
extern vector<bool> boolConsts;
extern vector <string> libFuncs;
extern vector <userfunc> userFuncs;

execute_func_t executeFuncs[] = {
        execute_assign,
        execute_add,
        execute_sub,
        execute_mul,
        execute_div,
        execute_mod,
        execute_uminus,
        execute_and,
        execute_or,
        execute_not,
        execute_jump,
        execute_jeq,
        execute_jne,
        execute_jle,
        execute_jge,
        execute_jlt,
        execute_jgt,
        execute_call,
        execute_pusharg,
        execute_funcenter,
        execute_funcexit,
        execute_newtable,
        execute_tablegetelem,
        execute_tablesetelem,
        execute_nop
};

char* typeStrings[] = {
        "number",
        "string",
        "bool",
        "table",
        "userfunc",
        "libfunc",
        "nil",
        "undef"
};

void avm_error(string msg1, string id, string msg2) {
    if (!msg1.empty()) cerr << msg1 << " ";
    if (!id.empty()) cerr << id << " ";
    if (!msg2.empty()) cerr << msg2;
    cerr << endl;
}

typedef char*(*tostring_func_t)(avm_memcell *);

char* number_tostring(avm_memcell *m) { return (char*)to_string(m->data.numVal).c_str(); }

char* string_tostring(avm_memcell *m) { return m->data.strVal; }

char* bool_tostring(avm_memcell *m) { return m->data.boolVal ? (char*)"true" : (char*)"false"; }

//todo:sp ???
char* table_tostring(avm_memcell *m) {
    return "table_tostring";
}

//todo:sp probably need to also return the func id
char* userfunc_tostring(avm_memcell *m) { return (char*)to_string(m->data.funcVal).c_str(); }

char* libfunc_tostring(avm_memcell *m) { return m->data.libfuncVal; }

char* nil_tostring(avm_memcell *m) { return (char*)"nil"; }

char* undef_tostring(avm_memcell *m) {
    assert(0);
    return (char*)"";
}

tostring_func_t tostringFuncs[] = {
        number_tostring,
        string_tostring,
        bool_tostring,
        table_tostring,
        userfunc_tostring,
        libfunc_tostring,
        nil_tostring,
        undef_tostring
};

typedef void (*library_func_t)(void);


void execute_cycle() {
    if (executionFinished==true) return;
    if (pc == AVM_ENDING_PC) {
        executionFinished = true;
        return;
    } else {
        assert(pc < AVM_ENDING_PC);
        instruction instr = instructions.at(code + pc);
        if(DEBUG_ENABLED) cout << "instruction " << code+pc << endl;
        assert(instr.opcode >= 0 && instr.opcode <= AVM_MAX_INSTRUCTIONS);
        if (instr.srcLine) currLine = instr.srcLine;
        unsigned int oldPC = pc;
        (*executeFuncs[instr.opcode])(&instr);
        if(instr.opcode == jump_v) {
            if(DEBUG_ENABLED) cout << "jump with oldPc = " << oldPC << " and pc = " << pc << endl;
            if(pc == oldPC) if(DEBUG_ENABLED) cout << "equal" << endl;
        }
        if (pc == oldPC) ++pc;
    }
}

unsigned char number_tobool(avm_memcell *m) { return m->data.numVal != 0; }

unsigned char string_tobool(avm_memcell *m) { return m->data.strVal[0] != 0; }

unsigned char bool_tobool(avm_memcell *m) { return m->data.boolVal; }

unsigned char table_tobool(avm_memcell *m) { return 1; }

unsigned char userfunc_tobool(avm_memcell *m) { return 1; }

unsigned char libfunc_tobool(avm_memcell *m) { return 1; }

unsigned char nil_tobool(avm_memcell *m) { return 0; }

unsigned char undef_tobool(avm_memcell *m) {
    assert(0);
    return 0;
}

typedef unsigned char (*tobool_func_t)(avm_memcell *);

tobool_func_t toboolFuncs[] = {
        number_tobool,
        string_tobool,
        bool_tobool,
        table_tobool,
        userfunc_tobool,
        libfunc_tobool,
        nil_tobool,
        undef_tobool
};

unsigned char avm_tobool(avm_memcell *m) {
    assert(m->type >= 0 && m->type < undef_m);
    return ((*toboolFuncs[m->type])(m));
}

void avm_dec_top(void) {
    if (!top) {
        avm_error("Stack Overflow", "", "");
        executionFinished = true;
    } else --top;
}

unsigned int avm_get_envvalue(unsigned int i) {
    if(DEBUG_ENABLED) cout<<"avm_get_envvalue called"<<endl;
    assert(stack[i].type == number_m);
    unsigned int val = (unsigned int) stack[i].data.numVal;
    assert(stack[i].data.numVal == (double) val);
    return val;
}

void avm_push_envValue(unsigned int val) {
    if(DEBUG_ENABLED) cout << "top is " << top << endl;
    if(DEBUG_ENABLED) cout << "before type" << endl;
    stack[top].type = number_m;
    if(DEBUG_ENABLED) cout << "before numval" << endl;
    stack[top].data.numVal = val;
    if(DEBUG_ENABLED) cout << "before dec" << endl;
    avm_dec_top();
}

void avm_callSaveEnvironment() {
    avm_push_envValue(totalActuals);
    if(DEBUG_ENABLED) cout << "saved total actuals" << endl;
    avm_push_envValue(pc + 1);
    if(DEBUG_ENABLED) cout << "saved pc" << endl;
    avm_push_envValue(top + totalActuals + 2);
    if(DEBUG_ENABLED) cout << "saved actuals" << endl;
    avm_push_envValue(topsp);
    if(DEBUG_ENABLED) cout << "saved topsp" << endl;
}

unsigned int avm_totalactuals(void) { return avm_get_envvalue(topsp + AVM_NUMACTUALS_OFFSET); }

avm_memcell *avm_getactual(unsigned int i) {
    assert(i < avm_totalactuals());
    return &stack[topsp + AVM_STACKENV_SIZE + 1 + i];
}

char* avm_tostring(avm_memcell *m) {
    assert(m->type >= 0 && m->type <= undef_m);
    return (*tostringFuncs[m->type])(m);
}
/**
 * Implementation of the library function 'print'.
 * It outputs every argument to the console
 */
void libfunc_print() {
    if(DEBUG_ENABLED) cout<<"print called"<<endl;
    unsigned int n = avm_totalactuals();
    unsigned int i = 0;
    for (i = 0; i < n; ++i) {
        string s = avm_tostring(avm_getactual(i));
        puts(s.c_str());
    }
}

void libfunc_input(){
	char* userInput;
	cin>>userInput;
	char* strPure = strtok(userInput,"\"");
	retval.type = string_m;
	retval.data.strVal = strPure;
}

void libfunc_totalarguments() {
    unsigned int p_topsp = avm_get_envvalue(topsp + AVM_SAVEDTOPSP_OFFSET);
    avm_memcellclear(&retval);
    if (!p_topsp) {
        avm_error("'totalarguments' called outside a function!", "", "");
        retval.type = nil_m;
        executionFinished = true;
    } else {
        retval.type = number_m;
        retval.data.numVal = avm_get_envvalue(p_topsp + AVM_NUMACTUALS_OFFSET);
    }
}

void libfunc_objectmemberkeys(){
    unsigned int n = avm_totalactuals();
	if(n != 1){
		char *nName = NULL;
    	asprintf(&nName, "%d",n);
		avm_error( "One argument and not(", nName , ") expected in 'libfunc objecttotalmembers' !" );
		executionFinished = true;
		return;
	}
	if(avm_getactual(0)->type != table_m){
		avm_error("libfunc objecttotalmembers ",0,", gets only arguments, of type table");
		executionFinished = true;
		return;
	}
	if(avm_getactual(0)->data.tableVal){
		avm_memcellclear(&retval);
		retval.type = table_m;
		retval.data.tableVal = avm_getactual(0)->data.tableVal;
	}
}

void libfunc_objecttotalmembers() {
    avm_memcell* tm = avm_getactual(0);
    int j = tm->data.tableVal->total;
    retval.data.numVal = j;
}

void libfunc_objectcopy(){

}

void libfunc_argument() {
    if(DEBUG_ENABLED) cout<<"libfunc_argument called"<<endl;
    unsigned int n = avm_totalactuals();
	if(n!=1){
		char *nName = NULL;
    	asprintf(&nName, "%d",n);
		avm_error( "One argument and not(", nName , ") expected in 'libfunc argument' !" );
		executionFinished = true;
	}else{
		if(avm_getactual(0)->type != number_m) {
			avm_error("libfunc argument ",0,", gets only arguments, of type number");
			executionFinished = true;
		}else{
			avm_memcellclear(&retval);
			memcpy(&retval,avm_getactual(0),sizeof(avm_memcell));
		}
	}
}

void libfunc_typeof() {
    unsigned n = avm_totalactuals();
    if (n > 1){
        retval.type = nil_m;
        cerr<<"ERROR: Too many arguments for function typeof."<<endl;
    }else retval.data.strVal = typeStrings[avm_getactual(0)->type];
}

void libfunc_strtonum() {
    char* i = avm_getactual(0)->data.strVal;
    if (avm_getactual(0)->type != string_m || (i != 0 && atof(i) == 0)) {
        retval.type = nil_m;
    } else {
        retval.data.numVal = atof(i);
        retval.type = number_m;
    }
}

void libfunc_sqrt() {
    double x = avm_getactual(0)->data.numVal;
    if (x < 0) retval.type = nil_m;
    else retval.data.numVal = sqrt(x);
    
}

void libfunc_cos() {
    double x = avm_getactual(0)->data.numVal;
    double rads = atan(1)*4 * x / 180;
    retval.data.numVal = cos(rads);
}

void libfunc_sin() {
    double x = avm_getactual(0)->data.numVal;
    double rads = atan(1)*4 * x / 180;
    retval.data.numVal = sin(rads);
}

library_func_t avm_getlibraryfunc(string id){
        if(DEBUG_ENABLED) cout<<"avm_getlibraryfunc called"<<endl;
		if (id=="print") return libfunc_print;
		if (id=="input") return libfunc_input;
		if (id=="objectmemberkeys") return libfunc_objectmemberkeys;
		if (id=="libfunc_objecttotalmembers") return libfunc_objecttotalmembers;	
		if (id=="objectcopy") return libfunc_objectcopy;
		if (id=="totalarguments") return libfunc_totalarguments;
		if (id=="argument") return libfunc_argument;
		if (id=="typeof") return libfunc_typeof;
		if (id=="strtonum") return libfunc_strtonum;
		if (id=="sqrt") return libfunc_sqrt;
		if (id=="cos") return libfunc_cos;
		if (id=="sin") return libfunc_sin;
		return 0;					
}

void avm_callLibFunc(string id) {
    if(DEBUG_ENABLED) cout<<"avm_callLibFunc caleld"<<endl;
    library_func_t f = avm_getlibraryfunc(id);
    if (!f) {
        avm_error("unsupported libfunc \'", id, "\' called!");
        executionFinished = true;
    } else {
        /*
         * calling enter/exit function parts manually
         */
        topsp = top; // enter function sequence. no stack locals
        avm_infunc++;
        totalActuals = 0;
        (*f)(); // call library function
        if (executionFinished==false) execute_funcexit((instruction *) 0); // return sequence if no error occured
    }
}

double div_impl(double x, double y) {
    if (y != 0) return (x / y);
    avm_error("Devision by 0 is forbidden", "", "");
    executionFinished = true;
    return -1.0;
}

double mod_impl(double x, double y) {
    if (y != 0) return (unsigned int) x % (unsigned int) y;
    avm_error("Devision by 0 is forbidden","","");
    executionFinished = true;
    return -1.0;
}

void execute_arithmetic(instruction *instr) {
    if(DEBUG_ENABLED) cout<<"execute_arithmetic called"<<endl;
    avm_memcell *lv = avm_translate_operand(&instr->result, (avm_memcell *) 0);
    avm_memcell *rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell *rv2 = avm_translate_operand(&instr->arg2, &bx);
    assert(lv && (&stack[AVM_STACKSIZE - 1] >= lv && lv > &stack[top] || lv == &retval));
    assert(rv1 && rv2);
    if (rv1->type != number_m || rv2->type != number_m) {
        avm_error("Not a number in arithmetic!", "", "");
        executionFinished = true;
    } else {
        avm_memcellclear(lv);
        lv->type = number_m;
        if(instr->opcode==sub_v) lv->data.numVal = rv1->data.numVal - rv2->data.numVal;
        if(instr->opcode==mul_v) lv->data.numVal = rv1->data.numVal * rv2->data.numVal;
        if(instr->opcode==div_v) lv->data.numVal = div_impl(rv1->data.numVal,rv2->data.numVal);
        if(instr->opcode==mod_v) lv->data.numVal = mod_impl(rv1->data.numVal,rv2->data.numVal);
    }
}

void execute_comparison(instruction* instr){
    if(DEBUG_ENABLED) cout<<"execute_comparison called"<<endl;
	avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
	avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);
	assert(rv1 && rv2);
	if(rv1->type != number_m || rv2->type != number_m){
		avm_error("Not a number in comparison!","","");
		executionFinished = true;
	}else{
        unsigned char result;
		if (instr->opcode==jle_v) result = rv1->data.numVal <= rv2->data.numVal;
        if (instr->opcode==jge_v) result = rv1->data.numVal >= rv2->data.numVal;
        if (instr->opcode==jlt_v) result = rv1->data.numVal < rv2->data.numVal;
        if (instr->opcode==jgt_v) result = rv1->data.numVal > rv2->data.numVal;
		if(executionFinished==false && result) pc = instr->result.val;
	}
}

userfunc *avm_getfuncinfo(unsigned int address) {
    assert(address >= 0 && address < userFuncs.size());
    return &userFuncs[address];
}

void avm_initialize() {
    avm_initstack();
    // init registers
    ax.type = undef_m;
    bx.type = undef_m;
    cx.type = undef_m;
    retval.type = undef_m;
    top = topsp = AVM_STACKSIZE - 1;
    top = top - NUM_OF_GLOBALS;
    if(DEBUG_ENABLED) cout << "after initialization top is : " << top << endl;
}

//********************execute funcs********************************
void execute_assign(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, nullptr);
    avm_memcell *rv = avm_translate_operand(&instr->arg1, &ax);
    assert(lv && (&stack[top] < lv && lv <= &stack[AVM_STACKSIZE-1] || lv == &retval));
    assert(rv);
    avm_assign(lv, rv);
}

void execute_nop(instruction *instr) { executionFinished = true; }

void execute_jump(instruction *instr) {
    if(DEBUG_ENABLED) cout<<"execute_jump called"<<endl;
    assert(instr->result.type == label_a);
    if(DEBUG_ENABLED) cout << "jump called with val " << instr->result.val << endl;
    if (executionFinished==false) pc = instr->result.val;
}

void execute_jeq(instruction *instr) {
    if(DEBUG_ENABLED) cout<<"execute_jeq called"<<endl;
    assert(instr->result.type == label_a);
    avm_memcell *rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell *rv2 = avm_translate_operand(&instr->arg2, &bx);
    unsigned char result = 0;
    if (rv1->type == undef_m || rv2->type == undef_m) {
        avm_error("'undef' involved in equality", "", "");
        executionFinished = true;
    } else if (rv1->type == nil_m || rv2->type == nil_m) {
        result = (rv1->type == nil_m) && (rv2->type == nil_m);
    } else if (rv1->type == bool_m || rv2->type == bool_m) {
        result = (avm_tobool(rv1) == avm_tobool(rv2));
    } else if (rv1->type != rv2->type) {
        avm_error("Equality check between different types is illegal", "", "");
        executionFinished = true;
    } else {
        if (rv1->type == number_m) result = rv1->data.numVal == rv2->data.numVal;
        else if (rv1->type == string_m) result = !strcmp(rv1->data.strVal,rv2->data.strVal);
        else result = (avm_tobool(rv1) == avm_tobool(rv2));
    }
    if (executionFinished==false && result) pc = instr->result.val;
}

void execute_jne(instruction *instr) {
    if(DEBUG_ENABLED) cout<<"execute_jne called"<<endl;
    assert(instr->result.type == label_a);
    avm_memcell *rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell *rv2 = avm_translate_operand(&instr->arg2, &bx);
    unsigned char result = 0;
    if (rv1->type == undef_m || rv2->type == undef_m) {
        avm_error("'undef' involved in equality", "", "");
        executionFinished = true;
    } else if (rv1->type == nil_m || rv2->type == nil_m) {
        result = (rv1->type == nil_m) && (rv2->type == nil_m);
    } else if (rv1->type == bool_m || rv2->type == bool_m) {
        result = (avm_tobool(rv1) == avm_tobool(rv2));
    } else if (rv1->type != rv2->type) {
        avm_error("Equality check between different types is illegal", "", "");
        executionFinished = true;
    } else {
        if (rv1->type == number_m) result = rv1->data.numVal == rv2->data.numVal;
        else if (rv1->type == string_m) result = !strcmp(rv1->data.strVal,rv2->data.strVal);
        else result = (avm_tobool(rv1) == avm_tobool(rv2));
    }
    if (executionFinished==false && !result) pc = instr->result.val;
}

void execute_call(instruction *instr) {
    if(DEBUG_ENABLED) cout<<"execute_call called"<<endl;
    avm_memcell *func = avm_translate_operand(&instr->result, &ax);
    if(DEBUG_ENABLED) cout << "passed translation" << endl;
    assert(func);
    avm_callSaveEnvironment();
    if(DEBUG_ENABLED) cout << "saved env" << endl;
    switch (func->type) {
        case userfunc_m:
            pc = func->data.funcVal;
            assert(pc < AVM_ENDING_PC);
            if(DEBUG_ENABLED) cout << "userfunc_m pc = " << pc << endl;
            if(DEBUG_ENABLED) cout << "opcode at pc = " << vmopcodeStrings[instructions[pc].opcode] << endl;
            assert(instructions[pc].opcode == funcenter_v);
            break;
        case string_m:
            avm_callLibFunc(func->data.strVal);
            break;
        case libfunc_m:
            avm_callLibFunc(func->data.libfuncVal);
            break;
        default:
            string s = avm_tostring(func);
            avm_error("Call : cannot bind '", s, "' to function!");
            executionFinished = true;
    }
}

void execute_pusharg(instruction *instr) {
    if(DEBUG_ENABLED) cout<<"execute_pusharg called"<<endl;
    avm_memcell *arg = avm_translate_operand(&instr->result, &ax);
    assert(arg);
    assert(top<AVM_STACKSIZE);
    avm_assign(&stack[top], arg);
    ++totalActuals;
    avm_dec_top();
}

void execute_funcenter(instruction *instr) {
    if(DEBUG_ENABLED) cout<<"execute_funcenter called"<<endl;
    avm_memcell *func = avm_translate_operand(&instr->result, &ax);
    assert(func);
    if(DEBUG_ENABLED) cout << "pc was " << pc << " & funcval = " << func->data.funcVal << endl;
    assert(pc == func->data.funcVal);
    avm_infunc++;
    // callee actions
    totalActuals = 0;
    userfunc *funcInfo = avm_getfuncinfo(pc);
    topsp = top;
    top = top - funcInfo->total_locals;
}

void execute_funcexit(instruction *instr) { //todo:sp why is the instr unused?
    if(DEBUG_ENABLED) cout<<"execute_funcexit called"<<endl;
    unsigned int oldTop = top;
    top = avm_get_envvalue(topsp + AVM_SAVEDTOP_OFFSET);
    pc = avm_get_envvalue(topsp + AVM_SAVEDPC_OFFSET);
    topsp = avm_get_envvalue(topsp + AVM_SAVEDTOPSP_OFFSET);
    // garbage collection for activation record
    // intentionally skipping first
    while (++oldTop <= top) avm_memcellclear(&stack[oldTop]);
    avm_infunc--;
}

void execute_newtable(instruction *instr) {
    avm_memcell *lv = avm_translate_operand(&instr->result, nullptr);
    if(DEBUG_ENABLED) cout << "lv: " << lv << " | stack[top] : " << &stack[top] << " | stack[0] : " << &stack[0] <<  endl;
    assert(lv && (lv <= &stack[AVM_STACKSIZE-1] && lv > &stack[top] || lv == &retval));
    avm_memcellclear(lv);
    lv->type = table_m;
    lv->data.tableVal = avm_tablenew();
    avm_tableIncRefCounter(lv->data.tableVal);
}

void execute_tablegetelem(instruction *instr) {
    if(DEBUG_ENABLED) cout<<"execute_tablegetelem called"<<endl;
    avm_memcell *lv = avm_translate_operand(&instr->result, nullptr);
    avm_memcell *t = avm_translate_operand(&instr->arg1, nullptr);
    avm_memcell *i = avm_translate_operand(&instr->arg2, &ax);
    if (lv) if(DEBUG_ENABLED) cout<<"lv pass"<<endl;
    else if(DEBUG_ENABLED) cout<<"lv failed"<<endl;
    if (&stack[0] <= lv) if(DEBUG_ENABLED) cout<<"&stack[0] <= lv pass"<<endl;
    else if(DEBUG_ENABLED) cout<<"&stack[0] <= lv failed"<<endl;
    if (&stack[top]>lv) if(DEBUG_ENABLED) cout<<"&stack[top]>lv pass"<<endl;
    else if(DEBUG_ENABLED) cout<<"&stack[top]>lv failed"<<endl;
    if (lv == &retval) if(DEBUG_ENABLED) cout<<"lv == &retval pass"<<endl;
    else if(DEBUG_ENABLED) cout<<"lv == &retval failed"<<endl;
    assert(lv && (&stack[AVM_STACKSIZE - 1] >= lv && lv > &stack[top] || lv == &retval));
    assert(t && (&stack[AVM_STACKSIZE - 1] >= t && t > &stack[top] || t == &retval));
    assert(i);
    avm_memcellclear(lv);
    lv->type = nil_m;
    if (t->type != table_m) {
        avm_error("Illegal use of type ", typeStrings[t->type], " as table!");
        executionFinished = true;
    } else {
        avm_memcell *content = avm_tablegetelem(t->data.tableVal, i);
        if (content) avm_assign(lv, content);
        else {
            string ts = avm_tostring(t);
            string is = avm_tostring(i);
            avm_warning("Index not found! Table: ", ts, is);
        }
    }
}

void execute_uminus(instruction *instr){}

void execute_and(instruction *instr){}

void execute_or(instruction *instr){}

void execute_not(instruction *instr){}

void execute_tablesetelem(instruction *instr) {
    avm_memcell *t = avm_translate_operand(&instr->result, nullptr);
    avm_memcell *i = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell *c = avm_translate_operand(&instr->arg2, &bx);
    assert(t && &stack[AVM_STACKSIZE-1] >= t && t > &stack[top]);
    assert(i && c);
    if(DEBUG_ENABLED) cout<<t->type<<" type"<<endl;
    if (t->type != table_m) {
        avm_error("Illegal use of type ", typeStrings[t->type], " as table!");
        executionFinished = true;
    } else avm_tablesetelem(t->data.tableVal, i, c);
}