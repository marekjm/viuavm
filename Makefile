CXXFLAGS=-std=c++11

.SUFFIXES: .cpp .h .o

.PHONY: clean run try_test sample_ifbranching

all: bin/lib/asm bin/lib/run


clean:
	rm -v ./bin/lib/*
	rm -v ./build/support/*.o
	rm -v ./build/*.o


bin/lib/run: src/bytecode.h src/lib/run.cpp build/cpu.o build/support/pointer.o
	${CXX} ${CXXFLAGS} -o ./bin/lib/run src/lib/run.cpp build/cpu.o build/support/pointer.o

bin/lib/asm: src/bytecode.h src/lib/asm.cpp build/program.o build/support/string.o
	${CXX} ${CXXFLAGS} -o ./bin/lib/asm src/lib/asm.cpp build/program.o build/support/string.o


build/cpu.o: src/bytecode.h src/cpu.h src/cpu.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/cpu.o ./src/cpu.cpp

build/program.o: src/bytecode.h src/program.h src/program.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/program.o ./src/program.cpp


build/support/string.o: src/support/string.h src/support/string.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/support/string.o ./src/support/string.cpp

build/support/pointer.o: src/support/pointer.h src/support/pointer.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/support/pointer.o ./src/support/pointer.cpp
