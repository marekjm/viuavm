CXXFLAGS=-std=c++11

.SUFFIXES: .cpp .h .o

.PHONY: clean run try_test sample_ifbranching

all: bin/tatanka


run: bin/tatanka
	./bin/tatanka


clean:
	rm -v ./build/*
	rm -v ./bin/*


bin/tatanka: src/bytecode.h src/main.cpp build/cpu.o build/program.o
	${CXX} ${CXXFLAGS} -o ./bin/tatanka src/main.cpp build/cpu.o build/program.o


build/cpu.o: src/bytecode.h src/cpu.h src/cpu.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/cpu.o ./src/cpu.cpp


build/program.o: src/bytecode.h src/program.h src/program.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/program.o ./src/program.cpp


bin/test: test.cpp
	${CXX} ${CXXFLAGS} -o ./bin/test test.cpp


bin/sample/sample: src/bytecode.h src/sample/sample.cpp build/cpu.o build/program.o
	${CXX} ${CXXFLAGS} -o ./bin/sample/main src/sample/sample.cpp build/cpu.o build/program.o
	./bin/sample/main


bin/sample/ifbranching: src/bytecode.h src/sample/ifbranching.cpp build/cpu.o build/program.o
	${CXX} ${CXXFLAGS} -o ./bin/sample/ifbranching src/sample/ifbranching.cpp build/cpu.o build/program.o

sample_ifbranching: bin/sample/ifbranching
	@./bin/sample/ifbranching


try_test: bin/test
	@./bin/test
