# Clang 4.0 does not recognise -std=c++17 option, and
# needs -std=c++1z.
# Clang 5.0 and later recognises -std=c++17
# See http://clang.llvm.org/cxx_status.html for details.
CXX_STANDARD=c++17


GENERIC_SANITISER_FLAGS=-fstack-protector-strong \
						-fsanitize=undefined \
						-fsanitize=leak \
						-fsanitize=address
CLANG_SANITISER_FLAGS=  -fstack-protector-strong \
						-fsanitize=undefined \
						-fsanitize=leak \
						-fsanitize=address
# No -fsanitize=address for GCC because of too many false positives.
GCC_SANITISER_FLAGS=    -fstack-protector-strong \
						-fsanitize=undefined \
						-fsanitize=leak


# These are generic flags that should be used for compiling Viua VM.
# Additional flags:
# 	-Wpadded            -- maybe, but throws errors on current code
#	-Wsuggest-override  -- definitely, but it is a matter of style so not now
#	-Wfloat-equal       -- maybe, floating-point comparison is a tricky subject
GENERIC_CXXFLAGS=-Wall \
				 -Wextra \
				 -Wctor-dtor-privacy \
				 -Wnon-virtual-dtor \
				 -Wreorder \
				 -Woverloaded-virtual \
				 -Wundef \
				 -Wstrict-overflow=2 \
				 -Wdisabled-optimization \
				 -Winit-self \
				 -Wzero-as-null-pointer-constant \
				 -Wuseless-cast \
				 -Wconversion \
				 -Wshadow \
				 -Wswitch-default \
				 -Wswitch-enum \
				 -Wredundant-decls \
				 -Wlogical-op \
				 -Wmissing-include-dirs \
				 -Wmissing-declarations \
				 -Wcast-align \
				 -Wcast-qual \
				 -Wold-style-cast \
				 -Walloc-zero \
				 -Wdouble-promotion \
				 -Wunused-const-variable=2 \
				 -Wduplicated-branches \
				 -Wduplicated-cond \
				 -Wsuggest-final-types \
				 -Wsuggest-final-methods \
				 -Wconversion \
				 -Wsign-conversion \
				 -Wrestrict \
				 -Winline \
				 -Wstack-protector \
				 -Werror \
				 -Wfatal-errors \
				 -Wpedantic \
				 -g \
				 -I./include
CLANG_CXXFLAGS=-Wall \
			   -Wextra \
			   -Wint-to-void-pointer-cast \
			   -Wconversion \
			   -Wshadow \
			   -Wswitch-default \
			   -Wmissing-include-dirs \
			   -Wcast-align \
			   -Wold-style-cast \
			   -Werror \
			   -Wfatal-errors \
			   -pedantic \
			   -g \
			   -I./include
GCC_CXXFLAGS=$(GENERIC_CXXFLAGS)


SANITISER_FLAGS=$(GENERIC_SANITISER_FLAGS)
COMPILER_FLAGS=$(GENERIC_CXXFLAGS)


# For different compilers (and for TravisCI) compiler flags should be overridden, because
# of throwing too many false positives or being unsupported.
# If any compiler that should be treated specially is detected CXXFLAGS is adjusted for its (compiler's) needs;
# otherwise, generic CXXFLAGS are used.
ifeq ($(CXX), g++)
COMPILER_FLAGS=$(GENERIC_CXXFLAGS)
SANITISER_FLAGS=$(GCC_SANITISER_FLAGS)
else ifeq ($(CXX), g++-7)
COMPILER_FLAGS=-Wall -Wextra -Wzero-as-null-pointer-constant -Wuseless-cast -Wconversion -Wshadow \
			   -Wswitch-default -Wredundant-decls -Wlogical-op -Wmissing-include-dirs -Wcast-align \
			   -Wold-style-cast -Werror -Wfatal-errors -pedantic -g -I./include
SANITISER_FLAGS=-fsanitize=undefined
else ifeq ($(CXX), clang++)
COMPILER_FLAGS=$(CLANG_CXXFLAGS)
else ifeq ($(CXX), clang++-5.0)
COMPILER_FLAGS=$(CLANG_CXXFLAGS)
SANITISER_FLAGS=-fsanitize=undefined -fstack-protector-strong -fsanitize=address
endif

# Combine compiler and sanitiser flags, and used C++ standard into final CXXFLAGS.
CXXFLAGS=-std=$(CXX_STANDARD) $(COMPILER_FLAGS) $(SANITISER_FLAGS) $(CXX_EXTRA_FLAGS)

CXXOPTIMIZATIONFLAGS=-O0
COPTIMIZATIONFLAGS=
DYNAMIC_SYMS=-Wl,--dynamic-list-cpp-typeinfo

VIUA_INSTR_FILES_O=build/process/instr/general.o build/process/instr/registers.o build/process/instr/calls.o \
				   build/process/instr/concurrency.o build/process/instr/linking.o \
				   build/process/instr/tcmechanism.o build/process/instr/closure.o build/process/instr/int.o \
				   build/process/instr/float.o build/process/instr/arithmetic.o build/process/instr/str.o \
				   build/process/instr/text.o build/process/instr/bool.o build/process/instr/bits.o \
				   build/process/instr/cast.o build/process/instr/vector.o build/process/instr/prototype.o \
				   build/process/instr/object.o build/process/instr/struct.o build/process/instr/atom.o


PREFIX=/usr/local
BIN_PATH=$(PREFIX)/bin
LIB_PATH=$(PREFIX)/lib/viua
H_PATH=$(PREFIX)/include/viua
LDLIBS=-ldl -lpthread

.SUFFIXES: .cpp .h .o

.PHONY: all remake clean clean-support clean-test-compiles install compile-test test version platform


############################################################
# BASICS
all: build/bin/vm/asm build/bin/vm/kernel build/bin/vm/dis build/bin/vm/lex build/bin/vm/parser \
	build/bin/opcodes.bin platform stdlib standardlibrary compile-test

what:
	@echo "compiler:  $(CXX)"
	@echo "standard:  $(CXX_STANDARD)"
	@echo "sanitiser: $(SANITISER_FLAGS)"
	@echo "flags:     $(CXXFLAGS)"

remake: clean all


############################################################
# CLEANING
clean: clean-test-compiles
	find ./build -type f | grep -Pv '.gitkeep' | xargs -n 1 $(RM)
	find ./src -name '*.d' | xargs -n 1 $(RM)
	find . -name '*.o' | xargs -n 1 $(RM)
	find . -name '*.so' | xargs -n 1 $(RM)
	find . -name '*.bin' | xargs -n 1 $(RM)
	find . -name '*.vlib' | xargs -n 1 $(RM)

clean-test-compiles:
	find ./tests/compiled -name '*.asm' | xargs -n 1 $(RM)


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
platform: build/platform/types/exception.o build/platform/types/value.o build/platform/types/pointer.o \
	build/platform/types/number.o build/platform/types/integer.o build/platform/types/bits.o \
	build/platform/types/float.o build/platform/types/string.o build/platform/types/text.o \
	build/platform/types/vector.o build/platform/types/reference.o \
	build/platform/kernel/registerset.o \
	build/platform/support/string.o

build/platform/kernel/registerset.o: src/kernel/registerset.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o $@ $<

build/platform/support/string.o: src/support/string.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o $@ $<


############################################################
# TESTING
build/test/printer.so: build/test/printer.o build/platform/kernel/registerset.o build/platform/types/value.o \
	build/platform/types/exception.o

build/test/sleeper.so: build/test/sleeper.o build/platform/kernel/registerset.o build/platform/types/value.o \
	build/platform/types/exception.o

build/test/math.so: build/test/math.o build/platform/kernel/registerset.o build/platform/types/exception.o \
	build/platform/types/value.o build/platform/types/pointer.o build/platform/types/integer.o \
	build/platform/types/float.o build/platform/types/number.o

build/test/throwing.so: build/test/throwing.o build/platform/kernel/registerset.o \
	build/platform/types/exception.o build/platform/types/value.o build/platform/types/pointer.o \
	build/platform/types/integer.o build/platform/types/number.o

compile-test: build/test/math.so build/test/World.so build/test/throwing.so build/test/printer.so \
	build/test/sleeper.so

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
# RULES
build/platform/types/%.o: src/types/%.cpp include/viua/types/%.h
	$(CXX) $(CXXFLAGS) -fPIC -c -o $@ $<

build/test/%.o: sample/asm/external/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -fPIC -o $@ $<

build/test/%.so: build/test/%.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -fPIC -shared -o $@ $^

build/front/asm/%.o: src/front/asm/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/front/%.o: src/front/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/scheduler/%.o: src/scheduler/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $^

build/kernel/%.o: src/kernel/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/stdlib/std/%.vlib: src/stdlib/viua/%.asm build/bin/vm/asm
	./build/bin/vm/asm --lib -o $@ $<

build/stdlib/%.o: src/stdlib/%.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -I./include -o $@ $<

build/stdlib/%.so: build/stdlib/%.o
	$(CXX) $(CXXFLAGS) -fPIC -shared -o $@ $^

build/types/%.o: src/types/%.cpp include/viua/types/%.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/process/instr/%.o: src/process/instr/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/support/%.o: src/support/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/assembler/%.o: src/assembler/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<

build/cg/%.o: src/cg/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -c -o $@ $<


############################################################
# VIRTUAL MACHINE CODE
build/bin/vm/kernel: build/front/kernel.o build/kernel/kernel.o build/scheduler/vps.o build/front/vm.o \
	build/assert.o build/process.o build/process/stack.o build/pid.o build/process/dispatch.o \
	build/scheduler/ffi/request.o build/scheduler/ffi/scheduler.o build/kernel/registerset.o \
	build/kernel/frame.o build/loader.o build/machine.o build/printutils.o build/support/pointer.o \
	build/support/string.o build/support/env.o $(VIUA_INSTR_FILES_O) build/bytecode/decoder/operands.o \
	build/types/vector.o build/types/boolean.o build/types/function.o build/types/closure.o \
	build/types/string.o build/types/text.o build/types/atom.o build/types/struct.o build/types/number.o \
	build/types/integer.o build/types/bits.o build/types/float.o build/types/exception.o \
	build/types/prototype.o build/types/object.o build/types/reference.o build/types/process.o \
	build/types/value.o build/types/pointer.o build/cg/disassembler/disassembler.o \
	build/assembler/util/pretty_printer.o build/cg/lex.o build/cg/lex/reduce_fns.o build/cg/lex/cook.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -o $@ $^ $(LDLIBS)

build/bin/vm/vdb: build/front/wdb.o build/lib/linenoise.o build/kernel/kernel.o build/scheduler/vps.o \
	build/front/vm.o build/assert.o build/process.o build/process/stack.o build/pid.o \
	build/process/dispatch.o build/scheduler/ffi/request.o build/scheduler/ffi/scheduler.o \
	build/kernel/registerset.o build/kernel/frame.o build/loader.o build/machine.o \
	build/cg/disassembler/disassembler.o build/printutils.o build/support/pointer.o build/support/string.o \
	build/support/env.o $(VIUA_INSTR_FILES_O) build/types/vector.o build/types/boolean.o \
	build/types/function.o build/types/closure.o build/types/string.o build/types/text.o build/types/atom.o \
	build/types/struct.o build/types/number.o build/types/integer.o build/types/bits.o build/types/float.o \
	build/types/exception.o build/types/prototype.o build/types/object.o build/types/reference.o \
	build/types/process.o build/types/value.o build/types/pointer.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -o $@ $^ $(LDLIBS)

build/bin/vm/asm: build/front/asm.o \
	build/front/asm/generate.o \
	build/front/asm/assemble_instruction.o \
	build/front/asm/gather.o \
	build/front/asm/decode.o \
	build/program.o \
	build/programinstructions.o \
	build/cg/tokenizer/tokenize.o \
	build/cg/assembler/operands.o build/cg/assembler/codeextract.o \
	build/cg/lex.o \
	build/cg/lex/reduce_fns.o \
	build/cg/lex/cook.o \
	build/cg/tools.o \
	build/cg/assembler/verify.o \
	build/cg/assembler/static_analysis.o \
	build/cg/assembler/utils.o build/cg/bytecode/instructions.o build/loader.o build/machine.o \
	build/support/string.o build/support/env.o build/cg/assembler/binary_literals.o \
	build/assembler/frontend/parser.o build/assembler/frontend/static_analyser/verifier.o \
	build/assembler/frontend/static_analyser/register_usage.o \
	build/assembler/frontend/static_analyser/checkers/check_closure_instantiations.o \
	build/assembler/frontend/static_analyser/checkers/check_for_unused_registers.o \
	build/assembler/frontend/static_analyser/checkers/check_op_argc.o \
	build/assembler/frontend/static_analyser/checkers/check_op_arithmetic.o \
	build/assembler/frontend/static_analyser/checkers/check_op_bit_arithmetic.o \
	build/assembler/frontend/static_analyser/checkers/check_op_bit_increment.o \
	build/assembler/frontend/static_analyser/checkers/check_op_compare.o \
	build/assembler/frontend/static_analyser/checkers/check_op_float.o \
	build/assembler/frontend/static_analyser/checkers/check_op_ftoi.o \
	build/assembler/frontend/static_analyser/checkers/check_op_iinc.o \
	build/assembler/frontend/static_analyser/checkers/check_op_integer.o \
	build/assembler/frontend/static_analyser/checkers/check_op_itof.o \
	build/assembler/frontend/static_analyser/checkers/check_op_izero.o \
	build/assembler/frontend/static_analyser/checkers/check_op_stof.o \
	build/assembler/frontend/static_analyser/checkers/check_op_stoi.o \
	build/assembler/frontend/static_analyser/checkers/check_op_streq.o \
	build/assembler/frontend/static_analyser/checkers/check_op_string.o \
	build/assembler/frontend/static_analyser/checkers/check_op_textat.o \
	build/assembler/frontend/static_analyser/checkers/check_op_textcommonprefix.o \
	build/assembler/frontend/static_analyser/checkers/check_op_textcommonsuffix.o \
	build/assembler/frontend/static_analyser/checkers/check_op_textconcat.o \
	build/assembler/frontend/static_analyser/checkers/check_op_text.o \
	build/assembler/frontend/static_analyser/checkers/check_op_texteq.o \
	build/assembler/frontend/static_analyser/checkers/check_op_textlength.o \
	build/assembler/frontend/static_analyser/checkers/check_op_textsub.o \
	build/assembler/frontend/static_analyser/Register.o \
	build/assembler/frontend/static_analyser/Closure.o \
	build/assembler/frontend/static_analyser/Register_usage_profile.o \
	build/assembler/util/pretty_printer.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -o $@ $^

build/bin/vm/lex: build/front/lexer.o build/cg/lex.o build/cg/lex/reduce_fns.o build/cg/lex/cook.o build/cg/tools.o build/support/string.o \
	build/support/env.o build/cg/assembler/binary_literals.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -o $@ $^

build/bin/vm/parser: build/front/parser.o build/cg/lex.o build/cg/lex/reduce_fns.o build/cg/lex/cook.o build/cg/tools.o build/support/string.o \
	build/support/env.o build/cg/assembler/binary_literals.o build/cg/assembler/utils.o \
	build/assembler/frontend/parser.o build/assembler/frontend/static_analyser/verifier.o \
	build/assembler/util/pretty_printer.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -o $@ $^

build/bin/vm/dis: build/front/dis.o build/loader.o build/machine.o build/cg/disassembler/disassembler.o \
	build/support/pointer.o build/support/string.o build/support/env.o build/cg/assembler/utils.o \
	build/assembler/util/pretty_printer.o build/cg/lex.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -o $@ $^


############################################################
# OBJECTS COMMON FOR DEBUGGER AND KERNEL COMPILATION
build/kernel/kernel.o: src/kernel/kernel.cpp include/viua/kernel/kernel.h include/viua/bytecode/opcodes.h \
	include/viua/kernel/frame.h build/scheduler/vps.o
build/kernel/registerset.o: src/kernel/registerset.cpp include/viua/kernel/registerset.h
build/kernel/frame.o: src/kernel/frame.cpp include/viua/kernel/frame.h


############################################################
# STANDARD LIBRARY
standardlibrary: build/bin/vm/asm build/stdlib/std/vector.vlib build/stdlib/std/functional.vlib \
	build/stdlib/std/misc.vlib

stdlib: build/bin/vm/asm standardlibrary build/stdlib/typesystem.so build/stdlib/io.so \
	build/stdlib/random.so build/stdlib/kitchensink.so

####
#### Fix for Travis CI.
#### Once GNU Make is upgraded to 4.2 or later, these targets will not
#### be needed.
####
build/stdlib/typesystem.o: src/stdlib/typesystem.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -I./include -o $@ $<

build/stdlib/io.o: src/stdlib/io.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -I./include -o $@ $<

build/stdlib/random.o: src/stdlib/random.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -I./include -o $@ $<

build/stdlib/kitchensink.o: src/stdlib/kitchensink.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -I./include -o $@ $<
####
#### End of fix for Travis CI.
####

build/stdlib/typesystem.so: build/stdlib/typesystem.o build/platform/types/exception.o \
	build/platform/types/vector.o build/platform/types/string.o build/platform/types/value.o \
	build/platform/types/pointer.o build/platform/types/integer.o build/platform/types/bits.o \
	build/platform/types/number.o build/platform/kernel/registerset.o build/platform/support/string.o

build/stdlib/io.so: build/stdlib/io.o build/platform/types/exception.o build/platform/types/vector.o \
	build/platform/types/string.o build/platform/types/value.o build/platform/types/pointer.o \
	build/platform/types/integer.o build/platform/kernel/registerset.o build/platform/support/string.o

build/stdlib/random.so: build/stdlib/random.o build/platform/types/exception.o build/platform/types/vector.o \
	build/platform/types/string.o build/platform/types/value.o build/platform/types/pointer.o \
	build/platform/kernel/registerset.o build/platform/support/string.o

build/stdlib/kitchensink.so: build/stdlib/kitchensink.o build/platform/types/exception.o \
	build/platform/types/vector.o build/platform/types/string.o build/platform/types/value.o \
	build/platform/types/pointer.o build/platform/kernel/registerset.o build/platform/support/string.o


############################################################
# OPCODE LISTER PROGRAM
build/bin/opcodes.bin: src/bytecode/opcd.cpp include/viua/bytecode/opcodes.h include/viua/bytecode/maps.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -o $@ $<


############################################################
# DEPENDENCY LIBRARIES
build/lib/linenoise.o: lib/linenoise/linenoise.c lib/linenoise/linenoise.h
	$(CC) $(CFLAGS) $(COPTIMIZATIONFLAGS) -c -o $@ $<
