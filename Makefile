CXX=g++
CXXFLAGS=-std=c++11 -Wall -pedantic -Wfatal-errors -g
CXXOPTIMIZATIONFLAGS=

VM_ASM=bin/vm/asm
VM_CPU=bin/vm/cpu
VM_WDB=bin/vm/wdb

VIUA_CPU_INSTR_FILES_CPP=src/cpu/instr/general.cpp src/cpu/instr/closure.cpp src/cpu/instr/int.cpp src/cpu/instr/float.cpp src/cpu/instr/byte.cpp src/cpu/instr/str.cpp src/cpu/instr/bool.cpp src/cpu/instr/cast.cpp src/cpu/instr/vector.cpp
VIUA_CPU_INSTR_FILES_O=build/cpu/instr/general.o build/cpu/instr/closure.o build/cpu/instr/int.o build/cpu/instr/float.o build/cpu/instr/byte.o build/cpu/instr/str.o build/cpu/instr/bool.o build/cpu/instr/cast.o build/cpu/instr/vector.o

PREFIX=~/.local
BIN_PATH=${PREFIX}/bin
LIB_PATH=${PREFIX}/lib/viua


.SUFFIXES: .cpp .h .o

.PHONY: all remake clean clean-support clean-test-compiles install test stdlib


all: ${VM_ASM} ${VM_CPU} ${VM_WDB} bin/opcodes.bin

remake: clean all

doc/viua_virtual_machine.pdf: doc/viua_virtual_machine.lyx
	lyx --export-to pdf doc/viua_virtual_machine.pdf --force-overwrite main doc/viua_virtual_machine.lyx


clean: clean-support clean-stdlib
	rm -f ./build/lib/*.o
	rm -f ./build/cpu/instr/*.o
	rm -f ./build/cpu/*.o
	rm -f ./build/*.o
	rm -f ./bin/vm/*

clean-stdlib:
	rm -f ./build/stdlib/*.o
	rm -f ./build/stdlib/lib/*.so

clean-support:
	rm -f ./build/support/*.o

clean-test-compiles:
	rm -f ./tests/compiled/*.bin


install: ${VM_ASM} ${VM_CPU} ${VM_WDB} stdlib
	mkdir -p ${BIN_PATH}
	cp ${VM_ASM} ${BIN_PATH}/viua-asm
	chmod 755 ${BIN_PATH}/viua-asm
	cp ${VM_CPU} ${BIN_PATH}/viua-cpu
	chmod 755 ${BIN_PATH}/viua-cpu
	cp ${VM_WDB} ${BIN_PATH}/viua-db
	chmod 755 ${BIN_PATH}/viua-db
	mkdir -p ${LIB_PATH}
	cp -v ./build/stdlib/lib/*.so ${LIB_PATH}/


test: ${VM_CPU} ${VM_ASM} clean-test-compiles
	python3 ./tests/tests.py --verbose --catch --failfast


${VM_CPU}: src/front/cpu.cpp build/cpu/cpu.o build/cpu/registserset.o build/loader.o build/printutils.o build/support/pointer.o build/support/string.o ${VIUA_CPU_INSTR_FILES_O} build/types/vector.o build/types/closure.o build/types/string.o build/types/exception.o
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -ldl -o ${VM_CPU} $^

${VM_WDB}: src/front/wdb.cpp build/lib/linenoise.o build/cpu/cpu.o build/cpu/registserset.o build/loader.o build/printutils.o build/support/pointer.o build/support/string.o ${VIUA_CPU_INSTR_FILES_O} build/types/vector.o build/types/closure.o build/types/string.o build/types/exception.o
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -ldl -o ${VM_WDB} $^

${VM_ASM}: src/front/asm.cpp build/program.o build/programinstructions.o build/assembler/operands.o build/assembler/ce.o build/assembler/verify.o build/loader.o build/support/string.o
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -o ${VM_ASM} $^


bin/opcodes.bin: src/bytecode/opcd.cpp src/bytecode/opcodes.h src/bytecode/maps.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -o $@ $<


# compile all standard library shared modules
stdlib: build/stdlib/lib/typesystem.so

# compile prerequisites for all further modules
build/stdlib/exception.o: src/types/exception.cpp src/types/exception.h
	${CXX} ${CXXFLAGS} -fPIC -c -o $@ src/types/exception.cpp

build/stdlib/registerset.o: src/cpu/registerset.cpp src/types/exception.h
	${CXX} ${CXXFLAGS} -fPIC -c -o $@ src/cpu/registerset.cpp


build/stdlib/lib/typesystem.o: src/stdlib/typesystem.cpp
	${CXX} ${CXXFLAGS} -fPIC -c -o ./build/stdlib/lib/typesystem.o src/stdlib/typesystem.cpp

build/stdlib/lib/typesystem.so: build/stdlib/lib/typesystem.o build/stdlib/exception.o build/stdlib/registerset.o
	${CXX} ${CXXFLAGS} -shared -Wl,-soname,$(notdir $@) -o $@ $< -lc


build/types/vector.o: src/types/vector.cpp src/types/vector.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/types/closure.o: src/types/closure.cpp src/types/closure.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/types/string.o: src/types/string.cpp src/types/string.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/types/exception.o: src/types/exception.cpp src/types/exception.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


build/cpu/cpu.o: src/cpu/cpu.cpp src/cpu/cpu.h src/bytecode/opcodes.h src/cpu/frame.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/cpu/registserset.o: src/cpu/registerset.cpp src/cpu/registerset.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


build/cpu/instr/general.o: src/cpu/instr/general.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/cpu/instr/closure.o: src/cpu/instr/closure.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/cpu/instr/int.o: src/cpu/instr/int.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/cpu/instr/float.o: src/cpu/instr/float.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/cpu/instr/byte.o: src/cpu/instr/byte.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/cpu/instr/str.o: src/cpu/instr/str.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/cpu/instr/bool.o: src/cpu/instr/bool.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/cpu/instr/cast.o: src/cpu/instr/cast.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/cpu/instr/vector.o: src/cpu/instr/vector.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


build/program.o: src/program.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/programinstructions.o: src/programinstructions.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


build/printutils.o: src/printutils.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


build/assembler/operands.o: src/assembler/operands.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/assembler/ce.o: src/assembler/codeextract.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/assembler/verify.o: src/assembler/verify.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


build/loader.o: src/loader.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


build/support/string.o: src/support/string.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/support/pointer.o: src/support/pointer.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


build/lib/linenoise.o: lib/linenoise/linenoise.c lib/linenoise/linenoise.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<
