.PHONY: avm
all: avm

avm: ./SymTab/symtable.h ./avm/avm_mem_manager.cpp ./avm/avm_mem_manager.h ./avm/avm_instr_set.h ./avm/avm_impl.cpp ./avm/dispatcher.cpp ./avm/dispatcher.h
	g++ -std=c++11 -c -o ./SymTab/symtab.o SymTab/symtable.cpp
	g++ -std=c++11 -c -o ./avm/avm_mem_manager.o ./avm/avm_mem_manager.cpp
	g++ -std=c++11 -c -o ./avm/avm_impl.o ./avm/avm_impl.cpp
	g++ -std=c++11 -c -o ./avm/avm_dispatcher.o ./avm/dispatcher.cpp
	g++ -o alphavm ./avm/avm_mem_manager.o ./avm/avm_impl.o ./avm/avm_dispatcher.o

clean:
	rm -f ./SymTab/symtab.o ./avm/avm_mem_manager.o ./avm/avm_impl.o ./avm/avm_dispatcher.o output.abc alphavm
