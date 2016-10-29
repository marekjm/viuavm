CXX_STANDARD=c++14

ifeq ($(CXX), g++)
CXXFLAGS=-std=$(CXX_STANDARD) -Wall -Wextra -Wctor-dtor-privacy -Wnon-virtual-dtor -Wreorder -Woverloaded-virtual -Wundef -Wstrict-overflow=5 -Wdisabled-optimization -Winit-self -Wzero-as-null-pointer-constant -Wuseless-cast -Wconversion -Winline -Wshadow -Wswitch-default -Wredundant-decls -Wlogical-op -Wmissing-include-dirs -Wmissing-declarations -Wcast-align -Wcast-qual -Wold-style-cast -Werror -Wfatal-errors -pedantic -g -I./include
else ifeq ($(CXX), g++-5)
# make TravisCI happy
CXXFLAGS=-std=$(CXX_STANDARD) -Wall -Wextra -Wzero-as-null-pointer-constant -Wuseless-cast -Wconversion -Winline -Wshadow -Wswitch-default -Wredundant-decls -Wlogical-op -Wmissing-include-dirs -Wcast-align -Wold-style-cast -Werror -Wfatal-errors -pedantic -g -I./include
else ifeq ($(CXX), clang++)
CXXFLAGS=-std=$(CXX_STANDARD) -Wall -Wextra -Wint-to-void-pointer-cast -Wconversion -Winline -Wshadow -Wswitch-default -Wmissing-include-dirs -Wcast-align -Wold-style-cast -Werror -Wfatal-errors -pedantic -g -I./include
endif

CXXOPTIMIZATIONFLAGS=
COPTIMIZATIONFLAGS=
DYNAMIC_SYMS=-Wl,--dynamic-list-cpp-typeinfo

VIUA_INSTR_FILES_O=build/process/instr/general.o build/process/instr/registers.o build/process/instr/calls.o build/process/instr/concurrency.o build/process/instr/linking.o build/process/instr/tcmechanism.o build/process/instr/closure.o build/process/instr/int.o build/process/instr/float.o build/process/instr/str.o build/process/instr/bool.o build/process/instr/cast.o build/process/instr/vector.o build/process/instr/prototype.o build/process/instr/object.o


PREFIX=/usr/local
BIN_PATH=$(PREFIX)/bin
LIB_PATH=$(PREFIX)/lib/viua
H_PATH=$(PREFIX)/include/viua

LIBDL ?= -ldl

.SUFFIXES: .cpp .h .o

.PHONY: all remake clean clean-support clean-test-compiles install compile-test test version platform


############################################################
# BASICS
all: build/bin/vm/asm build/bin/vm/kernel build/bin/vm/dis build/bin/vm/lex build/bin/opcodes.bin platform stdlib

remake: clean all


############################################################
# CLEANING
clean: clean-test-compiles
	find . -name '*.o' | xargs -n 1 rm -fv
	find . -name '*.so' | xargs -n 1 rm -fv
	find . -name '*.bin' | xargs -n 1 rm -fv
	find . -name '*.vlib' | xargs -n 1 rm -fv

clean-test-compiles:
	find ./tests/compiled -name '*.asm' | xargs rm -fv


############################################################
# DOCUMENTATION
doc/viua_virtual_machine.pdf: doc/viua_virtual_machine.lyx
	lyx --export-to pdf doc/viua_virtual_machine.pdf --force-overwrite main doc/viua_virtual_machine.lyx


############################################################
# INSTALLATION AND UNINSTALLATION
bininstall: build/bin/vm/asm build/bin/vm/kernel build/bin/vm/dis
	mkdir -p $(BIN_PATH)
	cp ./build/bin/vm/asm $(BIN_PATH)/viua-asm
	chmod 755 $(BIN_PATH)/viua-asm
	cp ./build/bin/vm/kernel $(BIN_PATH)/viua-vm
	chmod 755 $(BIN_PATH)/viua-vm
	cp ./build/bin/vm/dis $(BIN_PATH)/viua-dis
	chmod 755 $(BIN_PATH)/viua-dis

libinstall: stdlib
	mkdir -p $(LIB_PATH)/std
	mkdir -p $(LIB_PATH)/site
	cp ./build/stdlib/*.so $(LIB_PATH)/std
	cp ./build/stdlib/std/*.vlib $(LIB_PATH)/std

installdevel: platform
	mkdir -p $(LIB_PATH)/platform
	cp ./build/platform/*.o $(LIB_PATH)/platform

install: bininstall libinstall
	mkdir -p $(H_PATH)
	cp -R ./include/viua/. $(H_PATH)/

uninstall:
	rm -rf $(H_PATH)
	rm -rf $(LIB_PATH)
	rm -rf $(BIN_PATH)/viua-*


############################################################
# PLATFORM OBJECT FILES
platform: build/platform/exception.o build/platform/string.o build/platform/vector.o build/platform/registerset.o build/platform/support_string.o build/platform/reference.o build/platform/type.o build/platform/pointer.o

build/platform/exception.o: src/types/exception.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o ./build/platform/exception.o src/types/exception.cpp

build/platform/type.o: src/types/type.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o ./build/platform/type.o src/types/type.cpp

build/platform/pointer.o: src/types/pointer.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o ./build/platform/pointer.o src/types/pointer.cpp

build/platform/integer.o: src/types/integer.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o ./build/platform/integer.o src/types/integer.cpp

build/platform/float.o: src/types/float.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o ./build/platform/float.o src/types/float.cpp

build/platform/string.o: src/types/string.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o ./build/platform/string.o src/types/string.cpp

build/platform/vector.o: src/types/vector.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o ./build/platform/vector.o src/types/vector.cpp

build/platform/reference.o: src/types/reference.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o ./build/platform/reference.o src/types/reference.cpp

build/platform/registerset.o: src/kernel/registerset.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o ./build/platform/registerset.o src/kernel/registerset.cpp

build/platform/support_string.o: src/support/string.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o ./build/platform/support_string.o src/support/string.cpp


############################################################
# TESTING
build/test/World.o: sample/asm/external/World.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -fPIC -o build/test/World.o ./sample/asm/external/World.cpp

build/test/World.so: build/test/World.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -fPIC -shared -o build/test/World.so build/test/World.o

build/test/printer.o: sample/asm/external/printer.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -fPIC -o build/test/printer.o ./sample/asm/external/printer.cpp

build/test/printer.so: build/test/printer.o build/platform/registerset.o build/platform/type.o build/platform/exception.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -fPIC -shared -o $@ $^

build/test/sleeper.o: sample/asm/external/sleeper.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -fPIC -o build/test/sleeper.o ./sample/asm/external/sleeper.cpp

build/test/sleeper.so: build/test/sleeper.o build/platform/registerset.o build/platform/type.o build/platform/exception.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -fPIC -shared -o $@ $^

build/test/math.o:  sample/asm/external/math.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -fPIC -o build/test/math.o ./sample/asm/external/math.cpp

build/test/math.so: build/test/math.o build/platform/registerset.o build/platform/exception.o build/platform/type.o build/platform/pointer.o build/platform/integer.o build/platform/float.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -fPIC -shared -o $@ $^

build/test/throwing.o:  sample/asm/external/throwing.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -fPIC -o $@ $^

build/test/throwing.so: build/test/throwing.o build/platform/registerset.o build/platform/exception.o build/platform/type.o build/platform/pointer.o build/platform/integer.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -fPIC -shared -o $@ $^

compile-test: build/test/math.so build/test/World.so build/test/throwing.so build/test/printer.so build/test/sleeper.so

test: build/bin/vm/asm build/bin/vm/kernel build/bin/vm/dis compile-test stdlib standardlibrary
	VIUAPATH=./build/stdlib python3 ./tests/tests.py --verbose --catch --failfast


############################################################
# VERSION UPDATE
version:
	./scripts/update_commit_info.sh
	touch src/front/asm.cpp
	touch src/front/dis.cpp
	touch src/front/kernel.cpp
	touch src/front/wdb.cpp


############################################################
# TOOLS
build/bin/tools/log-shortener: ./tools/log-shortener.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -o $@ $<

tools: build/bin/tools/log-shortener


############################################################
# VIRTUAL MACHINE CODE
build/asm/decode.o: src/front/asm/decode.cpp include/viua/front/asm.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/asm/gather.o: src/front/asm/gather.cpp include/viua/front/asm.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/asm/generate.o: src/front/asm/generate.cpp include/viua/front/asm.h include/viua/machine.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/asm.o: src/front/asm.cpp build/cg/assembler/verify.o include/viua/front/asm.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/kernel.o: src/front/kernel.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $^

build/dis.o: src/front/dis.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $^

build/wdb.o: src/front/wdb.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $^

build/kernel/ffi/request.o: src/kernel/ffi/request.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $^

build/kernel/ffi/scheduler.o: src/kernel/ffi/scheduler.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $^

build/assert.o: src/assert.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $^

build/front/vm.o: src/front/vm.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $^

build/machine.o: src/machine.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $^

build/bin/vm/kernel: build/kernel.o build/kernel/kernel.o build/scheduler/vps.o build/front/vm.o build/assert.o build/process.o build/process/dispatch.o build/kernel/ffi/request.o build/kernel/ffi/scheduler.o build/kernel/registserset.o build/kernel/frame.o build/loader.o build/machine.o build/printutils.o build/support/pointer.o build/support/string.o build/support/env.o $(VIUA_INSTR_FILES_O) build/bytecode/decoder/operands.o build/types/vector.o build/types/function.o build/types/closure.o build/types/string.o build/types/integer.o build/types/float.o build/types/exception.o build/types/prototype.o build/types/object.o build/types/reference.o build/types/process.o build/types/type.o build/types/pointer.o build/cg/disassembler/disassembler.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -lpthread -o $@ $^ $(LIBDL)

build/bin/vm/vdb: build/wdb.o build/lib/linenoise.o build/kernel/kernel.o build/scheduler/vps.o build/front/vm.o build/assert.o build/process.o build/process/dispatch.o build/kernel/ffi/request.o build/kernel/ffi/scheduler.o build/kernel/registserset.o build/kernel/frame.o build/loader.o build/machine.o build/cg/disassembler/disassembler.o build/printutils.o build/support/pointer.o build/support/string.o build/support/env.o $(VIUA_INSTR_FILES_O) build/types/vector.o build/types/function.o build/types/closure.o build/types/string.o build/types/integer.o build/types/float.o build/types/exception.o build/types/prototype.o build/types/object.o build/types/reference.o build/types/process.o build/types/type.o build/types/pointer.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -lpthread -o $@ $^ $(LIBDL)

build/bin/vm/asm: build/asm.o build/asm/generate.o build/asm/gather.o build/asm/decode.o build/program.o build/programinstructions.o build/cg/tokenizer/tokenize.o build/cg/assembler/operands.o build/cg/assembler/ce.o build/cg/lex.o build/cg/tools.o build/cg/assembler/verify.o build/cg/assembler/static_analysis.o build/cg/assembler/utils.o build/cg/bytecode/instructions.o build/loader.o build/machine.o build/support/string.o build/support/env.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -o $@ $^

build/bin/vm/lex: src/front/lexer.cpp build/cg/lex.o build/cg/tools.o build/support/string.o build/support/env.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -o $@ $^

build/bin/vm/dis: build/dis.o build/loader.o build/machine.o build/cg/disassembler/disassembler.o build/support/pointer.o build/support/string.o build/support/env.o build/cg/assembler/utils.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -o $@ $^


############################################################
# OBJECTS COMMON FOR DEBUGGER AND KERNEL COMPILATION
build/scheduler/vps.o: src/scheduler/vps.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/kernel/kernel.o: src/kernel/kernel.cpp include/viua/kernel/kernel.h include/viua/bytecode/opcodes.h include/viua/kernel/frame.h build/scheduler/vps.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/kernel/registserset.o: src/kernel/registerset.cpp include/viua/kernel/registerset.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/kernel/frame.o: src/kernel/frame.cpp include/viua/kernel/frame.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/bytecode/decoder/operands.o: src/bytecode/decoder/operands.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<


############################################################
# STANDARD LIBRARY
standardlibrary: build/bin/vm/asm build/stdlib/std/vector.vlib build/stdlib/std/functional.vlib build/stdlib/std/misc.vlib

stdlib: build/bin/vm/asm standardlibrary
	$(MAKE) build/stdlib/std/string.vlib build/stdlib/typesystem.so build/stdlib/io.so build/stdlib/random.so build/stdlib/kitchensink.so

build/stdlib/std/vector.vlib: src/stdlib/viua/vector.asm build/bin/vm/asm
	./build/bin/vm/asm --lib -o $@ $<

build/stdlib/std/functional.vlib: src/stdlib/viua/functional.asm build/bin/vm/asm
	./build/bin/vm/asm --lib -o $@ $<

build/stdlib/std/string.vlib: src/stdlib/viua/string.asm build/bin/vm/asm
	./build/bin/vm/asm --lib -o $@ $<

build/stdlib/std/misc.vlib: src/stdlib/viua/misc.asm build/bin/vm/asm
	./build/bin/vm/asm --lib -o $@ $<

build/stdlib/typesystem.o: src/stdlib/typesystem.cpp
	$(CXX) -std=$(CXX_STANDARD) -fPIC -c -I./include -o $@ $<

build/stdlib/io.o: src/stdlib/io.cpp
	$(CXX) -std=$(CXX_STANDARD) -fPIC -c -I./include -o $@ $<

build/stdlib/random.o: src/stdlib/random.cpp
	$(CXX) -std=$(CXX_STANDARD) -fPIC -c -I./include -o $@ $<

build/stdlib/kitchensink.o: src/stdlib/kitchensink.cpp
	$(CXX) -std=$(CXX_STANDARD) -fPIC -c -I./include -o $@ $<

build/stdlib/typesystem.so: build/stdlib/typesystem.o build/platform/exception.o build/platform/vector.o build/platform/registerset.o build/platform/support_string.o build/platform/string.o build/platform/type.o build/platform/pointer.o build/platform/integer.o
	$(CXX) -std=$(CXX_STANDARD) -fPIC -shared -o $@ $^

build/stdlib/io.so: build/stdlib/io.o build/platform/exception.o build/platform/vector.o build/platform/registerset.o build/platform/support_string.o build/platform/string.o build/platform/type.o build/platform/pointer.o
	$(CXX) -std=$(CXX_STANDARD) -fPIC -shared -o $@ $^

build/stdlib/random.so: build/stdlib/random.o build/platform/exception.o build/platform/vector.o build/platform/registerset.o build/platform/support_string.o build/platform/string.o build/platform/type.o build/platform/pointer.o
	$(CXX) -std=$(CXX_STANDARD) -fPIC -shared -o $@ $^

build/stdlib/kitchensink.so: build/stdlib/kitchensink.o build/platform/exception.o build/platform/vector.o build/platform/registerset.o build/platform/support_string.o build/platform/string.o build/platform/type.o build/platform/pointer.o
	$(CXX) -std=$(CXX_STANDARD) -fPIC -shared -o $@ $^


############################################################
# OPCODE LISTER PROGRAM
build/bin/opcodes.bin: src/bytecode/opcd.cpp include/viua/bytecode/opcodes.h include/viua/bytecode/maps.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -o $@ $<


############################################################
# CODE GENERATION
build/cg/disassembler/disassembler.o: src/cg/disassembler/disassembler.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/cg/tokenizer/tokenize.o: src/cg/tokenizer/tokenize.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/cg/lex.o: src/cg/lex.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/cg/tools.o: src/cg/tools.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<


############################################################
# TYPE MODULES
build/types/vector.o: src/types/vector.cpp include/viua/types/vector.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/types/closure.o: src/types/closure.cpp include/viua/types/closure.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/types/function.o: src/types/function.cpp include/viua/types/function.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/types/string.o: src/types/string.cpp include/viua/types/string.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/types/integer.o: src/types/integer.cpp include/viua/types/integer.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/types/float.o: src/types/float.cpp include/viua/types/float.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/types/exception.o: src/types/exception.cpp include/viua/types/exception.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/types/prototype.o: src/types/prototype.cpp include/viua/types/prototype.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/types/object.o: src/types/object.cpp include/viua/types/object.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/types/type.o: src/types/type.cpp include/viua/types/type.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/types/pointer.o: src/types/pointer.cpp include/viua/types/pointer.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/types/reference.o: src/types/reference.cpp include/viua/types/reference.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/types/process.o: src/types/process.cpp include/viua/types/process.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<


############################################################
# KERNEL, INSTRUCTION DISPATCH AND IMPLEMENTATION MODULES
build/process.o: src/process.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/dispatch.o: src/process/dispatch.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/general.o: src/process/instr/general.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/registers.o: src/process/instr/registers.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/calls.o: src/process/instr/calls.cpp build/process.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/concurrency.o: src/process/instr/concurrency.cpp build/process.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/linking.o: src/process/instr/linking.cpp build/process.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/tcmechanism.o: src/process/instr/tcmechanism.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/closure.o: src/process/instr/closure.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/int.o: src/process/instr/int.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/float.o: src/process/instr/float.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/str.o: src/process/instr/str.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/bool.o: src/process/instr/bool.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/cast.o: src/process/instr/cast.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/vector.o: src/process/instr/vector.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/prototype.o: src/process/instr/prototype.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/object.o: src/process/instr/object.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<


############################################################
# UTILITY MODULES
build/printutils.o: src/printutils.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/support/string.o: src/support/string.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/support/pointer.o: src/support/pointer.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/support/env.o: src/support/env.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<


############################################################
# CODE AND BYTECODE GENERATION
build/program.o: src/program.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/programinstructions.o: src/programinstructions.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<


build/cg/assembler/operands.o: src/cg/assembler/operands.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/cg/assembler/ce.o: src/cg/assembler/codeextract.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/cg/assembler/verify.o: src/cg/assembler/verify.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/cg/assembler/static_analysis.o: src/cg/assembler/static_analysis.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/cg/assembler/utils.o: src/cg/assembler/utils.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<


build/cg/bytecode/instructions.o: src/cg/bytecode/instructions.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<


############################################################
# MISC MODULES
build/loader.o: src/loader.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<


############################################################
# DEPENDENCY LIBRARIES
build/lib/linenoise.o: lib/linenoise/linenoise.c lib/linenoise/linenoise.h
	$(CC) $(CFLAGS) $(COPTIMIZATIONFLAGS) -c -o $@ $<
