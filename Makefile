CXXFLAGS=-std=c++11 -Wall -pedantic

VM_ASM=bin/vm/asm
VM_RUN=bin/vm/run

WUDOO_CPU_INSTR_FILES_CPP=src/cpu/instr/general.cpp src/cpu/instr/int.cpp src/cpu/instr/byte.cpp src/cpu/instr/bool.cpp
WUDOO_CPU_INSTR_FILES_O=build/cpu/instr/general.o build/cpu/instr/int.o build/cpu/instr/byte.o build/cpu/instr/bool.o


.SUFFIXES: .cpp .h .o

.PHONY: all


all: ${VM_ASM} ${VM_RUN}


clean: clean-support
	rm -v ./build/cpu/instr/*.o
	rm -v ./build/cpu/*.o
	rm -v ./build/*.o
	rm -v ./bin/vm/*

clean-support:
	rm -v ./build/support/*.o


${VM_RUN}: src/bytecode.h src/front/run.cpp build/cpu/cpu.o build/support/pointer.o build/support/string.o ${WUDOO_CPU_INSTR_FILES_O}
	${CXX} ${CXXFLAGS} -o ${VM_RUN} src/front/run.cpp build/cpu/cpu.o build/support/pointer.o build/support/string.o ${WUDOO_CPU_INSTR_FILES_O}

${VM_ASM}: src/bytecode.h src/front/asm.cpp build/program.o build/support/string.o
	${CXX} ${CXXFLAGS} -o ${VM_ASM} src/front/asm.cpp build/program.o build/support/string.o


build/cpu/cpu.o: src/bytecode.h src/cpu/cpu.h src/cpu/cpu.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/cpu/cpu.cpp

build/cpu/instr/general.o: src/cpu/instr/general.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/cpu/instr/general.cpp

build/cpu/instr/int.o: src/cpu/instr/int.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/cpu/instr/int.cpp

build/cpu/instr/byte.o: src/cpu/instr/byte.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/cpu/instr/byte.cpp

build/cpu/instr/bool.o: src/cpu/instr/bool.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/cpu/instr/bool.cpp


build/program.o: src/program.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/program.cpp


build/support/string.o: src/support/string.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/support/string.cpp

build/support/pointer.o: src/support/pointer.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/support/pointer.cpp
