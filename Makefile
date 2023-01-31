# tested on lotos@csd.uoc.gr

.PHONY: avm
all: flex bison tcode_gen avm comp

bison: ./parser/parser.y
	bison -d -v --output=./parser/parser.c ./parser/parser.y

flex: ./lexer/lexer.l ./lexer/lexer.h
	flex --outfile=./lexer/scanner.c ./lexer/lexer.l

tcode_gen: ./target_code/target_code_gen.cpp ./target_code/target_code_gen.h ./avm/avm_instr_set.h
	g++ -std=c++11 -c -o ./target_code/tc_gen.o ./target_code/target_code_gen.cpp


avm: ./avm/avm_mem_manager.cpp ./avm/avm_mem_manager.h ./avm/avm_instr_set.h ./avm/avm_impl.cpp ./avm/dispatcher.cpp ./avm/dispatcher.h
	g++ -std=c++11 -c -o ./avm/avm_mem_manager.o ./avm/avm_mem_manager.cpp
	g++ -std=c++11 -c -o ./avm/avm_impl.o ./avm/avm_impl.cpp
	g++ -std=c++11 -c -o ./avm/avm_dispatcher.o ./avm/dispatcher.cpp
	g++ -o avm_exec ./avm/avm_mem_manager.o ./avm/avm_impl.o ./avm/avm_dispatcher.o

comp: ./lexer/scanner.c ./lexer/lexer.h ./parser/parser.h settings.h ./parser/parser.c SymTab/symtable.h ./target_code/tc_gen.o
	g++ -std=c++11 -c -o ./lexer/scanner.o ./lexer/scanner.c
	g++ -std=c++11 -c -o ./SymTab/symtab.o SymTab/symtable.cpp
	g++ -std=c++11 -c -o ./parser/parser.o ./parser/parser.c
	g++ -std=c++11 -c -o main.o main.cpp
	g++ -std=c++11 -c -o ./utils/utils.o utils/parser-utils.cpp
	g++ -o exec main.o ./lexer/scanner.o ./parser/parser.o ./SymTab/symtab.o ./utils/utils.o ./target_code/tc_gen.o

clean:
	rm -f ./lexer/scanner.c ./parser/parser.c ./parser/parser.h ./parser/parser.output ./lexer/scanner.o ./parser/parser.o main.o ./SymTab/symtab.o ./utils/utils.o ./target_code/tc_gen.o ./avm/avm_mem_manager.o ./avm/avm_impl.o ./avm/avm_dispatcher.o output.abc avm_exec exec
