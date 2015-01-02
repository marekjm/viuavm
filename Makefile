CXXFLAGS=-std=c++11

VM_ASM=bin/vm/asm
VM_RUN=bin/vm/run

.SUFFIXES: .cpp .h .o

.PHONY: clean run try_test sample_ifbranching

all: ${VM_ASM} ${VM_RUN}


clean:
	rm -v ./bin/vm/*
	rm -v ./build/support/*.o
	rm -v ./build/*.o


${VM_RUN}: src/bytecode.h src/lib/run.cpp build/cpu/cpu.o build/support/pointer.o
	${CXX} ${CXXFLAGS} -o ${VM_RUN} src/lib/run.cpp build/cpu/cpu.o build/support/pointer.o

${VM_ASM}: src/bytecode.h src/lib/asm.cpp build/program.o build/support/string.o
	${CXX} ${CXXFLAGS} -o ${VM_ASM} src/lib/asm.cpp build/program.o build/support/string.o


build/cpu/cpu.o: src/bytecode.h src/cpu/cpu.h src/cpu/cpu.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/cpu/cpu.o ./src/cpu/cpu.cpp

build/program.o: src/bytecode.h src/program.h src/program.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/program.o ./src/program.cpp


build/support/string.o: src/support/string.h src/support/string.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/support/string.o ./src/support/string.cpp

build/support/pointer.o: src/support/pointer.h src/support/pointer.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/support/pointer.o ./src/support/pointer.cpp
