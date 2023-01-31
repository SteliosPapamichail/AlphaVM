//
// Created by Steli on 25/5/2022.
//

#include "avm_instr_set.h"
#include "dispatcher.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>

unsigned int MAGIC_NUMBER = 340;
unsigned int NUM_OF_GLOBALS = 0;
bool DEBUG_ENABLED = false;
int num = 0;
vector <instruction> instructions;
vector<binding *> funcStack;
vector<double> numConsts;
vector<bool> boolConsts;
vector <string> strConsts;
vector <string> libFuncs;
vector <userfunc> userFuncs;

void parse_binary(string filepath) {
    unsigned int i = 0;
    unsigned long int str_len;
    unsigned int numOfNumberConsts,
            numOfStrConsts,
            numOfBoolConsts,
            numOfLibFuncs,
            numOfUsrFuncs,
            numOfInstr,
            magic_num = 0;

    FILE *bytecode_file = fopen(filepath.c_str(), "r");

    // read magic number
    fread(&magic_num, sizeof(unsigned int), 1, bytecode_file);
    //printf("magic: %u\n\n", magic);
    if (magic_num != MAGIC_NUMBER) {
        //avm_error("invalid bytecode file");
        cout << "INVALID FILE" << endl;
        exit(EXIT_FAILURE);
    }

    // globals count
    fread(&NUM_OF_GLOBALS, sizeof(unsigned int), 1, bytecode_file);

    // read numbers table
    fread(&numOfNumberConsts, sizeof(unsigned int), 1, bytecode_file);

    numConsts.resize(numOfNumberConsts);
    for (i = 0; i < numOfNumberConsts; i++) {
        fread(&numConsts[i], sizeof(double), 1, bytecode_file);
    }

    // read strings table
    fread(&numOfStrConsts, sizeof(unsigned int), 1, bytecode_file);
    strConsts.resize(numOfStrConsts);

    for (i = 0; i < numOfStrConsts; i++) {
        fread(&str_len, sizeof(unsigned long int), 1, bytecode_file);
        char str[str_len];
        fread(&str[0], sizeof(char), str_len + 1, bytecode_file);
        string s(str); // convert char* to string
        strConsts.insert(strConsts.begin() + i, s);
    }

    // read bools table
    fread(&numOfBoolConsts, sizeof(unsigned int), 1, bytecode_file);

    boolConsts.resize(numOfBoolConsts);
    for (i = 0; i < numOfBoolConsts; i++) {
        int boolAsInt;
        fread(&boolAsInt, sizeof(unsigned int), 1, bytecode_file);
        bool intAsBool = boolAsInt ? true : false;
        boolConsts.insert(boolConsts.begin() + i, intAsBool);
    }

    // library funcs table
    fread(&numOfLibFuncs, sizeof(unsigned int), 1, bytecode_file);
    libFuncs.resize(numOfLibFuncs);

    for (i = 0; i < numOfLibFuncs; i++) {
        fread(&str_len, sizeof(unsigned long int), 1, bytecode_file);
        char str[str_len];
        fread(&str[0], sizeof(char), str_len + 1, bytecode_file);
        string s(str); // convert char* to string
        libFuncs.insert(libFuncs.begin() + i, s);
    }

    // user funcs table
    fread(&numOfUsrFuncs, sizeof(unsigned int), 1, bytecode_file);
    userFuncs.resize(numOfLibFuncs);

    for (i = 0; i < numOfUsrFuncs; i++) {
        userfunc temp;
        fread(&temp.address, sizeof(unsigned int), 1, bytecode_file);
        fread(&temp.total_locals, sizeof(unsigned int), 1, bytecode_file);

        fread(&str_len, sizeof(unsigned long int), 1, bytecode_file);
        char str[str_len];
        fread(&str[0], sizeof(char), str_len + 1, bytecode_file);
        string s(str); // convert char* to string
        temp.id = s;
        userFuncs.insert(userFuncs.begin() + i, temp);
    }

    fread(&numOfInstr, sizeof(unsigned int), 1, bytecode_file);
    instructions.resize(numOfInstr);

    for (i = 0; i < numOfInstr; i++) {
        instruction tmp;
        fread(&(tmp.opcode), sizeof(vmopcode), 1, bytecode_file);
        // read result
        vmarg result;
        unsigned int isResultNull;
        fread(&isResultNull, sizeof(unsigned int), 1, bytecode_file);
        result.is_null = isResultNull;
        fread(&result.type, sizeof(vmarg_t), 1, bytecode_file);
        fread(&result.val, sizeof(unsigned int), 1, bytecode_file);
        // read arg1
        vmarg arg1;
        unsigned int isArg1Null;
        fread(&isArg1Null, sizeof(unsigned int), 1, bytecode_file);
        arg1.is_null = isArg1Null;
        if (!isArg1Null) {
            fread(&arg1.type, sizeof(vmarg_t), 1, bytecode_file);
            fread(&arg1.val, sizeof(unsigned int), 1, bytecode_file);
        }
        // read arg2
        vmarg arg2;
        unsigned int isArg2Null;
        fread(&isArg2Null, sizeof(unsigned int), 1, bytecode_file);
        arg2.is_null = isArg2Null;
        if (!isArg2Null) {
            fread(&arg2.type, sizeof(vmarg_t), 1, bytecode_file);
            fread(&arg2.val, sizeof(unsigned int), 1, bytecode_file);
        }
        fread(&(tmp.srcLine), sizeof(unsigned int), 1, bytecode_file);
        tmp.result = result;
        tmp.arg1 = arg1;
        tmp.arg2 = arg2;
        //cout << "adding instruction " << i << " to position : " << instructions.begin()+i << endl;
        instructions.insert(instructions.begin() + i, tmp);
        //instructions[i] = tmp;
    }
    codeSize = numOfInstr;

    if (DEBUG_ENABLED) {
        // print other data
        cout << "# of globals " << NUM_OF_GLOBALS << endl;

        // print number consts
        cout << "# of number consts " << numOfNumberConsts << endl;
        for (i = 0; i < numOfNumberConsts; i++) {
            cout << numConsts.at(i) << endl;
        }

        // print string consts
        cout << "# of string consts " << numOfStrConsts << endl;
        for (i = 0; i < numOfStrConsts; i++) {
            cout << strConsts.at(i) << endl;
        }

        // print bool consts
        cout << "# of bool consts " << numOfBoolConsts << endl;
        for (i = 0; i < numOfBoolConsts; i++) {
            cout << boolConsts.at(i) << endl;
        }

        // print library functions
        cout << "# of library functions " << numOfLibFuncs << endl;
        for (i = 0; i < numOfLibFuncs; i++) {
            cout << libFuncs.at(i) << endl;
        }

        // print user functions
        cout << "# of user functions " << numOfUsrFuncs << endl;
        for (i = 0; i < numOfUsrFuncs; i++) {
            userfunc t = userFuncs.at(i);
            cout << t.id << " [addr] = " << t.address << " | [localsSize] = " << t.total_locals << endl;
        }

        // print instructions
        cout << "# of instructions " << numOfInstr << endl;
        for (i = 0; i < numOfInstr; i++) {
            instruction in = instructions.at(i);
            cout << "#" << i << " " << vmopcodeStrings[in.opcode] << " | " <<
                 vmargStrings[in.result.type] << " | " << in.result.val << endl;
            if (!in.arg1.is_null) {
                cout << "arg1: " << vmargStrings[in.arg1.type] << " | " << in.arg1.val << endl;
            }
            if (!in.arg2.is_null) {
                cout << "arg2: " << vmargStrings[in.arg2.type] << " | " << in.arg2.val << endl;
            }
        }
    }

    fclose(bytecode_file);
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        cout << "Usage: ./avm [binary_file_path].asc [optional -d]" << endl;
        return -1;
    } else if(argc == 2) {
        parse_binary(argv[1]);
    } else if(argc == 3) {
        if(strcmp(argv[2],"-d") == 0) DEBUG_ENABLED = true;
        parse_binary(argv[1]);
    } else {
        cout << "Too many arguments." << endl;
        cout << "Usage: ./avm [binary_file_path].asc [optional -d]" << endl;
        return -1;
    }
    avm_initialize();
	while(executionFinished == false){
		execute_cycle();
	}
    cout<<"Execution finished successfully!"<<endl;
}