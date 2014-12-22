CXXFLAGS=-std=c++11

.SUFFIXES: .cpp .h .o

.PHONY: clean run try_test sample_ifbranching

all: bin/lib/asm bin/lib/run


clean:
	rm -v ./build/*
	rm -v ./bin/lib/*
	rm -v ./bin/sample/*


bin/lib/run: src/bytecode.h src/lib/run.cpp build/cpu.o
	${CXX} ${CXXFLAGS} -o ./bin/lib/run src/lib/run.cpp build/cpu.o

bin/lib/asm: src/bytecode.h src/lib/asm.cpp build/program.o
	${CXX} ${CXXFLAGS} -o ./bin/lib/asm src/lib/asm.cpp build/program.o


build/cpu.o: src/bytecode.h src/cpu.h src/cpu.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/cpu.o ./src/cpu.cpp


build/program.o: src/bytecode.h src/program.h src/program.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/program.o ./src/program.cpp
