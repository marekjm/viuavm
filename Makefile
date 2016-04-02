CXXFLAGS=-std=c++11 -Wall -Wextra -Wzero-as-null-pointer-constant -Wuseless-cast -Wconversion -Winline -pedantic -Wfatal-errors -g -I./include
CXXOPTIMIZATIONFLAGS=
COPTIMIZATIONFLAGS=
DYNAMIC_SYMS=-Wl,--dynamic-list-cpp-typeinfo

VIUA_INSTR_FILES_O=build/process/instr/general.o build/process/instr/registers.o build/process/instr/calls.o build/process/instr/concurrency.o build/process/instr/linking.o build/process/instr/tcmechanism.o build/process/instr/closure.o build/process/instr/int.o build/process/instr/float.o build/process/instr/byte.o build/process/instr/str.o build/process/instr/bool.o build/process/instr/cast.o build/process/instr/vector.o build/process/instr/prototype.o build/process/instr/object.o


PREFIX=/usr
BIN_PATH=${PREFIX}/bin
LIB_PATH=${PREFIX}/lib/viua
H_PATH=${PREFIX}/include/viua

LIBDL ?= -ldl

.SUFFIXES: .cpp .h .o

.PHONY: all remake clean clean-support clean-test-compiles install compile-test test version platform


############################################################
# BASICS
all: build/bin/vm/asm build/bin/vm/cpu build/bin/vm/vdb build/bin/vm/dis build/bin/opcodes.bin platform stdlib

remake: clean all


############################################################
# CLEANING
clean: clean-support clean-test-compiles clean-stdlib
	rm -f ./build/bin/vm/*
	rm -f ./build/bin/opcodes.bin
	rm -f ./build/lib/*.o
	rm -f ./build/cpu/*.o
	rm -f ./build/process/instr/*.o
	rm -f ./build/process/*.o
	rm -f ./build/cg/assembler/*.o
	rm -f ./build/cg/disassembler/*.o
	rm -f ./build/cg/bytecode/*.o
	rm -f ./build/bin/vm/*
	rm -f ./build/platform/*.o
	rm -f ./build/test/*
	rm -f ./build/types/*.o
	rm -f ./build/*.o

clean-support:
	rm -f ./build/support/*.o

clean-test-compiles:
	rm -f ./tests/compiled/*.bin
	rm -f ./tests/compiled/*.asm
	rm -f ./tests/compiled/*.wlib
	rm -f ./misc.vlib

clean-stdlib:
	rm -f ./build/stdlib/*.o
	rm -f ./build/stdlib/*.so
	rm -f ./build/stdlib/std/*


############################################################
# DOCUMENTATION
doc/viua_virtual_machine.pdf: doc/viua_virtual_machine.lyx
	lyx --export-to pdf doc/viua_virtual_machine.pdf --force-overwrite main doc/viua_virtual_machine.lyx


############################################################
# INSTALLATION AND UNINSTALLATION
bininstall: build/bin/vm/asm build/bin/vm/cpu build/bin/vm/vdb build/bin/vm/dis
	mkdir -p ${BIN_PATH}
	cp ./build/bin/vm/asm ${BIN_PATH}/viua-asm
	chmod 755 ${BIN_PATH}/viua-asm
	cp ./build/bin/vm/cpu ${BIN_PATH}/viua-vm
	chmod 755 ${BIN_PATH}/viua-vm
	cp ./build/bin/vm/vdb ${BIN_PATH}/viua-db
	chmod 755 ${BIN_PATH}/viua-db
	cp ./build/bin/vm/dis ${BIN_PATH}/viua-dis
	chmod 755 ${BIN_PATH}/viua-dis

libinstall: stdlib
	mkdir -p ${LIB_PATH}/std
	mkdir -p ${LIB_PATH}/site
	cp ./build/stdlib/*.so ${LIB_PATH}/std
	cp ./build/stdlib/std/*.vlib ${LIB_PATH}/std

installdevel: platform
	mkdir -p ${LIB_PATH}/platform
	cp ./build/platform/*.o ${LIB_PATH}/platform

install: bininstall installdevel
	mkdir -p ${H_PATH}
	cp -R ./include/viua/. ${H_PATH}/

uninstall:
	rm -rf ${H_PATH}
	rm -rf ${LIB_PATH}
	rm -rf ${BIN_PATH}/viua-*


############################################################
# PLATFORM OBJECT FILES
platform: build/platform/exception.o build/platform/string.o build/platform/vector.o build/platform/registerset.o build/platform/support_string.o build/platform/reference.o build/platform/type.o build/platform/pointer.o

build/platform/exception.o: src/types/exception.cpp
	${CXX} -std=c++11 -fPIC -c -I./include -o ./build/platform/exception.o src/types/exception.cpp

build/platform/type.o: src/types/type.cpp
	${CXX} -std=c++11 -fPIC -c -I./include -o ./build/platform/type.o src/types/type.cpp

build/platform/pointer.o: src/types/pointer.cpp
	${CXX} -std=c++11 -fPIC -c -I./include -o ./build/platform/pointer.o src/types/pointer.cpp

build/platform/string.o: src/types/string.cpp
	${CXX} -std=c++11 -fPIC -c -I./include -o ./build/platform/string.o src/types/string.cpp

build/platform/vector.o: src/types/vector.cpp
	${CXX} -std=c++11 -fPIC -c -I./include -o ./build/platform/vector.o src/types/vector.cpp

build/platform/reference.o: src/types/reference.cpp
	${CXX} -std=c++11 -fPIC -c -I./include -o ./build/platform/reference.o src/types/reference.cpp

build/platform/registerset.o: src/cpu/registerset.cpp
	${CXX} -std=c++11 -fPIC -c -I./include -o ./build/platform/registerset.o src/cpu/registerset.cpp

build/platform/support_string.o: src/support/string.cpp
	${CXX} -std=c++11 -fPIC -c -I./include -o ./build/platform/support_string.o src/support/string.cpp


############################################################
# TESTING
build/test/World.o: sample/asm/external/World.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -fPIC -o build/test/World.o ./sample/asm/external/World.cpp

build/test/World.so: build/test/World.o
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -fPIC -shared -o build/test/World.so build/test/World.o

build/test/math.o:  sample/asm/external/math.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -fPIC -o build/test/math.o ./sample/asm/external/math.cpp

build/test/math.so: build/test/math.o build/platform/registerset.o build/platform/exception.o build/platform/type.o build/platform/pointer.o
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -fPIC -shared -o build/test/math.so build/test/math.o ./build/platform/registerset.o ./build/platform/exception.o build/platform/type.o build/platform/pointer.o

compile-test: build/test/math.so build/test/World.so

test: build/bin/vm/asm build/bin/vm/cpu build/bin/vm/dis build/test/math.so build/test/World.so stdlib standardlibrary
	VIUAPATH=./build/stdlib python3 ./tests/tests.py --verbose --catch --failfast


############################################################
# VERSION UPDATE
version:
	./scripts/update_commit_info.sh
	touch src/front/asm.cpp
	touch src/front/dis.cpp
	touch src/front/cpu.cpp
	touch src/front/wdb.cpp


############################################################
# VIRTUAL MACHINE CODE
build/asm/decode.o: src/front/asm/decode.cpp include/viua/front/asm.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/asm/gather.o: src/front/asm/gather.cpp include/viua/front/asm.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/asm/generate.o: src/front/asm/generate.cpp include/viua/front/asm.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/asm.o: src/front/asm.cpp include/viua/front/asm.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/cpu.o: src/front/cpu.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $^

build/dis.o: src/front/dis.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $^

build/wdb.o: src/front/wdb.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $^

build/cpu/opex.o: src/cpu/opex.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $^

build/operand.o: src/operand.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $^

build/assert.o: src/assert.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $^

build/front/vm.o: src/front/vm.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $^

build/bin/vm/cpu: build/cpu.o build/cpu/cpu.o build/front/vm.o build/operand.o build/assert.o build/process.o build/process/dispatch.o build/cpu/opex.o build/cpu/registserset.o build/cpu/frame.o build/loader.o build/printutils.o build/support/pointer.o build/support/string.o build/support/env.o ${VIUA_INSTR_FILES_O} build/types/vector.o build/types/function.o build/types/closure.o build/types/string.o build/types/exception.o build/types/prototype.o build/types/object.o build/types/reference.o build/types/process.o build/types/type.o build/types/pointer.o build/cg/disassembler/disassembler.o
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} ${DYNAMIC_SYMS} -o $@ $^ $(LIBDL)

build/bin/vm/vdb: build/wdb.o build/lib/linenoise.o build/cpu/cpu.o build/front/vm.o build/operand.o build/assert.o build/process.o build/process/dispatch.o build/cpu/opex.o build/cpu/registserset.o build/cpu/frame.o build/loader.o build/cg/disassembler/disassembler.o build/printutils.o build/support/pointer.o build/support/string.o build/support/env.o ${VIUA_INSTR_FILES_O} build/types/vector.o build/types/function.o build/types/closure.o build/types/string.o build/types/exception.o build/types/prototype.o build/types/object.o build/types/reference.o build/types/process.o build/types/type.o build/types/pointer.o
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} ${DYNAMIC_SYMS} -o $@ $^ $(LIBDL)

build/bin/vm/asm: build/asm.o build/asm/generate.o build/asm/gather.o build/asm/decode.o build/program.o build/programinstructions.o build/cg/tokenizer/tokenize.o build/cg/assembler/operands.o build/cg/assembler/ce.o build/cg/assembler/verify.o build/cg/bytecode/instructions.o build/loader.o build/support/string.o build/support/env.o
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} ${DYNAMIC_SYMS} -o $@ $^

build/bin/vm/dis: build/dis.o build/loader.o build/cg/disassembler/disassembler.o build/support/pointer.o build/support/string.o build/support/env.o
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} ${DYNAMIC_SYMS} -o $@ $^


############################################################
# OBJECTS COMMON FOR DEBUGGER AND CPU COMPILATION
build/cpu/cpu.o: src/cpu/cpu.cpp include/viua/cpu/cpu.h include/viua/bytecode/opcodes.h include/viua/cpu/frame.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/cpu/registserset.o: src/cpu/registerset.cpp include/viua/cpu/registerset.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/cpu/frame.o: src/cpu/frame.cpp include/viua/cpu/frame.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


############################################################
# STANDARD LIBRARY
standardlibrary: build/bin/vm/asm build/stdlib/std/vector.vlib build/stdlib/std/functional.vlib build/stdlib/std/misc.vlib

stdlib: build/bin/vm/asm standardlibrary
	${MAKE} build/stdlib/std/string.vlib build/stdlib/typesystem.so build/stdlib/io.so build/stdlib/random.so

build/stdlib/std/vector.vlib: src/stdlib/viua/vector.asm build/bin/vm/asm
	./build/bin/vm/asm --lib -o $@ $<

build/stdlib/std/functional.vlib: src/stdlib/viua/functional.asm build/bin/vm/asm
	./build/bin/vm/asm --lib -o $@ $<

build/stdlib/std/string.vlib: src/stdlib/viua/string.asm
	./build/bin/vm/asm --lib -o $@ $<

build/stdlib/std/misc.vlib: src/stdlib/viua/misc.asm
	./build/bin/vm/asm --lib -o $@ $<

build/stdlib/typesystem.o: src/stdlib/typesystem.cpp
	${CXX} -std=c++11 -fPIC -c -I./include -o $@ $<

build/stdlib/io.o: src/stdlib/io.cpp
	${CXX} -std=c++11 -fPIC -c -I./include -o $@ $<

build/stdlib/random.o: src/stdlib/random.cpp
	${CXX} -std=c++11 -fPIC -c -I./include -o $@ $<

build/stdlib/kitchensink.o: src/stdlib/kitchensink.cpp
	${CXX} -std=c++11 -fPIC -c -I./include -o $@ $<

build/stdlib/typesystem.so: build/stdlib/typesystem.o build/platform/exception.o build/platform/vector.o build/platform/registerset.o build/platform/support_string.o build/platform/string.o build/platform/type.o build/platform/pointer.o
	${CXX} -std=c++11 -fPIC -shared -o $@ $^

build/stdlib/io.so: build/stdlib/io.o build/platform/exception.o build/platform/vector.o build/platform/registerset.o build/platform/support_string.o build/platform/string.o build/platform/type.o build/platform/pointer.o
	${CXX} -std=c++11 -fPIC -shared -o $@ $^

build/stdlib/random.so: build/stdlib/random.o build/platform/exception.o build/platform/vector.o build/platform/registerset.o build/platform/support_string.o build/platform/string.o build/platform/type.o build/platform/pointer.o
	${CXX} -std=c++11 -fPIC -shared -o $@ $^

build/stdlib/kitchensink.so: build/stdlib/kitchensink.o build/platform/exception.o build/platform/vector.o build/platform/registerset.o build/platform/support_string.o build/platform/string.o build/platform/type.o build/platform/pointer.o
	${CXX} -std=c++11 -fPIC -shared -o $@ $^


############################################################
# OPCODE LISTER PROGRAM
build/bin/opcodes.bin: src/bytecode/opcd.cpp include/viua/bytecode/opcodes.h include/viua/bytecode/maps.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -o $@ $<


############################################################
# CODE GENERATION
build/cg/disassembler/disassembler.o: src/cg/disassembler/disassembler.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/cg/tokenizer/tokenize.o: src/cg/tokenizer/tokenize.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


############################################################
# TYPE MODULES
build/types/vector.o: src/types/vector.cpp include/viua/types/vector.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/types/closure.o: src/types/closure.cpp include/viua/types/closure.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/types/function.o: src/types/function.cpp include/viua/types/function.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/types/string.o: src/types/string.cpp include/viua/types/string.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/types/exception.o: src/types/exception.cpp include/viua/types/exception.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/types/prototype.o: src/types/prototype.cpp include/viua/types/prototype.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/types/object.o: src/types/object.cpp include/viua/types/object.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/types/type.o: src/types/type.cpp include/viua/types/type.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/types/pointer.o: src/types/pointer.cpp include/viua/types/pointer.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/types/reference.o: src/types/reference.cpp include/viua/types/reference.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/types/process.o: src/types/process.cpp include/viua/types/process.h
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


############################################################
# CPU, INSTRUCTION DISPATCH AND IMPLEMENTATION MODULES
build/process.o: src/process.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/dispatch.o: src/process/dispatch.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/general.o: src/process/instr/general.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/registers.o: src/process/instr/registers.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/calls.o: src/process/instr/calls.cpp build/process.o
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/concurrency.o: src/process/instr/concurrency.cpp build/process.o
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/linking.o: src/process/instr/linking.cpp build/process.o
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/tcmechanism.o: src/process/instr/tcmechanism.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/closure.o: src/process/instr/closure.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/int.o: src/process/instr/int.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/float.o: src/process/instr/float.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/byte.o: src/process/instr/byte.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/str.o: src/process/instr/str.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/bool.o: src/process/instr/bool.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/cast.o: src/process/instr/cast.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/vector.o: src/process/instr/vector.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/prototype.o: src/process/instr/prototype.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/process/instr/object.o: src/process/instr/object.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


############################################################
# UTILITY MODULES
build/printutils.o: src/printutils.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/support/string.o: src/support/string.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/support/pointer.o: src/support/pointer.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/support/env.o: src/support/env.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


############################################################
# CODE AND BYTECODE GENERATION
build/program.o: src/program.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/programinstructions.o: src/programinstructions.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


build/cg/assembler/operands.o: src/cg/assembler/operands.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/cg/assembler/ce.o: src/cg/assembler/codeextract.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<

build/cg/assembler/verify.o: src/cg/assembler/verify.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


build/cg/bytecode/instructions.o: src/cg/bytecode/instructions.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


############################################################
# MISC MODULES
build/loader.o: src/loader.cpp
	${CXX} ${CXXFLAGS} ${CXXOPTIMIZATIONFLAGS} -c -o $@ $<


############################################################
# DEPENDENCY LIBRARIES
build/lib/linenoise.o: lib/linenoise/linenoise.c lib/linenoise/linenoise.h
	${CC} ${CFLAGS} ${COPTIMIZATIONFLAGS} -c -o $@ $<
