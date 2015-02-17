CXXFLAGS=-std=c++11 -Wall -pedantic -Wfatal-errors

VM_ASM=bin/vm/asm
VM_CPU=bin/vm/cpu

WUDOO_CPU_INSTR_FILES_CPP=src/cpu/instr/general.cpp src/cpu/instr/int.cpp src/cpu/instr/float.cpp src/cpu/instr/byte.cpp src/cpu/instr/bool.cpp src/cpu/instr/cast.cpp
WUDOO_CPU_INSTR_FILES_O=build/cpu/instr/general.o build/cpu/instr/int.o build/cpu/instr/float.o build/cpu/instr/byte.o build/cpu/instr/bool.o build/cpu/instr/cast.o

BIN_PATH=/usr/local/bin


.SUFFIXES: .cpp .h .o

.PHONY: all remake clean clean-support clean-test-compiles install test


all: ${VM_ASM} ${VM_CPU} bin/vm/analyze bin/opcodes.bin

remake: clean all


clean: clean-support
	touch ./build/cpu/instr/dummy.o
	rm -v ./build/cpu/instr/*.o
	touch ./build/cpu/dummy.o
	rm -v ./build/cpu/*.o
	touch ./build/dummy.o
	rm -v ./build/*.o
	touch ./bin/vm/dummy.o
	rm -v ./bin/vm/*

clean-support:
	touch ./build/support/dummy.o
	rm -v ./build/support/*.o

clean-test-compiles:
	touch ./tests/compiled/a.bin
	rm ./tests/compiled/*.bin


install: ${VM_ASM} ${VM_CPU}
	mkdir -p ${BIN_PATH}
	cp ${VM_ASM} ${BIN_PATH}/wudoo-asm
	chmod 755 ${BIN_PATH}/wudoo-asm
	cp ${VM_CPU} ${BIN_PATH}/wudoo-cpu
	chmod 755 ${BIN_PATH}/wudoo-cpu


test: ${VM_CPU} ${VM_ASM} clean-test-compiles
	python3 ./tests/tests.py --verbose --catch --failfast


${VM_CPU}: src/bytecode/opcodes.h src/front/cpu.cpp build/cpu/cpu.o build/support/pointer.o build/support/string.o ${WUDOO_CPU_INSTR_FILES_O}
	${CXX} ${CXXFLAGS} -o ${VM_CPU} src/front/cpu.cpp build/cpu/cpu.o build/support/pointer.o build/support/string.o ${WUDOO_CPU_INSTR_FILES_O}

${VM_ASM}: src/bytecode/opcodes.h src/front/asm.cpp build/program.o build/support/string.o
	${CXX} ${CXXFLAGS} -o ${VM_ASM} src/front/asm.cpp build/program.o build/support/string.o

bin/vm/analyze: src/bytecode/opcodes.h src/front/analyze.cpp build/program.o build/support/string.o
	${CXX} ${CXXFLAGS} -o bin/vm/analyze src/front/analyze.cpp build/program.o build/support/string.o


bin/opcodes.bin: src/bytecode/opcodes.h src/bytecode/maps.h src/bytecode/opcd.cpp
	${CXX} ${CXXFLAGS} -o bin/opcodes.bin src/bytecode/opcd.cpp


build/cpu/cpu.o: src/bytecode/opcodes.h src/cpu/frame.h src/cpu/cpu.h src/cpu/cpu.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/cpu/cpu.cpp

build/cpu/instr/general.o: src/cpu/instr/general.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/cpu/instr/general.cpp

build/cpu/instr/int.o: src/cpu/instr/int.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/cpu/instr/int.cpp

build/cpu/instr/float.o: src/cpu/instr/float.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/cpu/instr/float.cpp

build/cpu/instr/byte.o: src/cpu/instr/byte.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/cpu/instr/byte.cpp

build/cpu/instr/bool.o: src/cpu/instr/bool.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/cpu/instr/bool.cpp

build/cpu/instr/cast.o: src/cpu/instr/cast.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/cpu/instr/cast.cpp


build/program.o: src/program.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/program.cpp


build/support/string.o: src/support/string.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/support/string.cpp

build/support/pointer.o: src/support/pointer.cpp
	${CXX} ${CXXFLAGS} -c -o $@ ./src/support/pointer.cpp
