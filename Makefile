# Clang 4.0 does not recognise -std=c++17 option, and
# needs -std=c++1z.
# Clang 5.0 and later recognises -std=c++17
# See http://clang.llvm.org/cxx_status.html for details.
CXX_STANDARD=c++17


# By default, try to catch everything.
# This may be painful at first but it pays off in the long run.
GENERIC_SANITISER_FLAGS=-fstack-protector-strong \
						-fsanitize=undefined \
						-fsanitize=leak \
						-fsanitize=address
CLANG_SANITISER_FLAGS=  -fstack-protector-strong \
						-fsanitize=undefined \
						-fsanitize=leak \
						-fsanitize=address
# No -fsanitize=address for GCC because of too many false positives.
# No -fsanitize=leak for GCC because it broke on my Arch Linux after
# last update.
GCC_SANITISER_FLAGS=    -fstack-protector-strong \
						-fsanitize=undefined


# These are generic flags that should be used for compiling Viua VM.
# Additional flags:
# 	-Wpadded            -- maybe, but throws errors on current code
#	-Wsuggest-override  -- definitely, but it is a matter of style so not now
#	-Wsuggest-final-types	-- ditto
#	-Wsuggest-final-methods	-- ditto
#	-Wfloat-equal       -- maybe, floating-point comparison is a tricky subject
#	-Winline			-- fails if GCC tries to inline calls that are unlikely and
#							the code size would grow
# 	-Wdisabled-optimization	-- some source files are too big
GENERIC_CXXFLAGS=-Wall \
				 -Wextra \
				 -Wctor-dtor-privacy \
				 -Wnon-virtual-dtor \
				 -Wreorder \
				 -Woverloaded-virtual \
				 -Wundef \
				 -Wstrict-overflow=2 \
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
				 -Wsign-conversion \
				 -Wrestrict \
				 -Wstack-protector \
				 -Werror \
				 -Wfatal-errors \
				 -Wpedantic \
				 -g \
				 -I./include
# -Weverything						-- in theory enables *everything*
# -Wdeprecated-implementations		-- makes sense for C++?
# -Wpadded							-- breaks existing code
# -Wexit-time-destructors 			-- breaks existing code
# -Wglobal-constructors				-- breaks existing code
# -Wmissing-variable-declarations	-- breaks existing code
# -Wcovered-switch-default 			-- breaks existing code
# -Wunused-template					-- breaks existing code
# -Wfloat-equal 					-- breaks existing code
# -Wlifetime						-- would be useful
CLANG_CXXFLAGS=-Wall \
			   -Wextra \
			   -Wabsolute-value \
			   -Wabstract-vbase-init \
			   -Warray-bounds-pointer-arithmetic \
			   -Wassign-enum \
			   -Wbad-function-cast \
			   -Wbitfield-enum-conversion \
			   -Wcast-align \
			   -Wcast-qual \
			   -Wchar-subscripts \
			   -Wcomma \
			   -Wconditional-uninitialized \
			   -Wconversion \
			   -Wconsumed \
			   -Wdate-time \
			   -Wdelete-non-virtual-dtor \
			   -Wdeprecated \
			   -Wdeprecated-implementations \
			   -Wdirect-ivar-access \
			   -Wdiv-by-zero \
			   -Wdouble-promotion \
			   -Wduplicate-enum \
			   -Wduplicate-method-arg \
			   -Wduplicate-method-match \
			   -Wfloat-conversion \
			   -Wint-to-void-pointer-cast \
			   -Wfor-loop-analysis \
			   -Wformat-nonliteral \
			   -Wformat-pedantic \
			   -Wfour-char-constants \
			   -Wheader-hygiene \
			   -Widiomatic-parentheses \
			   -Winfinite-recursion \
			   -Wkeyword-macro \
			   -Wmain \
			   -Wmissing-braces \
			   -Wmissing-field-initializers \
			   -Wmissing-include-dirs \
			   -Wmissing-prototypes \
			   -Wmissing-noreturn \
			   -Wnon-virtual-dtor \
			   -Wnull-pointer-arithmetic \
			   -Wold-style-cast \
			   -Wover-aligned \
			   -Woverlength-strings \
			   -Woverloaded-virtual \
			   -Woverriding-method-mismatch \
			   -Wpacked \
			   -Wpessimizing-move \
			   -Wpointer-arith \
			   -Wrange-loop-analysis \
			   -Wredundant-move \
			   -Wredundant-parens \
			   -Wreorder \
			   -Wretained-language-linkage \
			   -Wself-assign \
			   -Wself-move \
			   -Wsemicolon-before-method-body \
			   -Wshadow-all \
			   -Wshift-sign-overflow \
			   -Wshorten-64-to-32 \
			   -Wsign-compare \
			   -Wsign-conversion \
			   -Wsometimes-uninitialized \
			   -Wstatic-in-inline \
			   -Wstrict-prototypes \
			   -Wstring-conversion \
			   -Wsuper-class-method-mismatch \
			   -Wswitch-enum \
			   -Wtautological-overlap-compare \
			   -Wthread-safety \
			   -Wundef \
			   -Wundefined-func-template \
			   -Wundefined-internal-type \
			   -Wundefined-reinterpret-cast \
			   -Wuninitialized \
			   -Wunneeded-internal-declaration \
			   -Wunneeded-member-function \
			   -Wunreachable-code-aggressive \
			   -Wunused-const-variable \
			   -Wunused-function \
			   -Wunused-label \
			   -Wunused-lambda-capture \
			   -Wunused-local-typedef \
			   -Wunused-macros \
			   -Wunused-member-function \
			   -Wunused-parameter \
			   -Wunused-private-field \
			   -Wunused-variable \
			   -Wused-but-marked-unused \
			   -Wuser-defined-literals \
			   -Wvector-conversion \
			   -Wvla \
			   -Wweak-template-vtables \
			   -Wweak-vtables \
			   -Wzero-as-null-pointer-constant \
			   -Wzero-length-array \
			   -Wc++2a-compat \
			   -Werror \
			   -Wno-weak-vtables \
			   -Wfatal-errors \
			   -pedantic \
			   -g \
			   -I./include
GCC_CXXFLAGS=$(GENERIC_CXXFLAGS)


# For different compilers (and for TravisCI) compiler flags should be overridden, because
# of throwing too many false positives or being unsupported.
# If any compiler that should be treated specially is detected, CXXFLAGS is adjusted for its (the
# compiler's) needs; otherwise, generic CXXFLAGS are used.
#
# The process of setting CXXFLAGS in a multi-phase one.
# There are several things that need to be decided on:
# - the C++ standard to be used
# - the set of warning flags
# - the set of sanitiser flags
# - optimisation level
#
# After all this parts are set, Make will compose the final CXXFLAGS.

# By default, use generic flags.
SANITISER_FLAGS=$(GENERIC_SANITISER_FLAGS)
COMPILER_FLAGS=$(GENERIC_CXXFLAGS)

ifeq ($(CXX), g++)
COMPILER_FLAGS=$(GENERIC_CXXFLAGS) \
			   --param max-gcse-memory=134217728
			   #--param max-gcse-memory=67108864
SANITISER_FLAGS=$(GCC_SANITISER_FLAGS)
else ifeq ($(CXX), g++-7)
COMPILER_FLAGS=-Wall \
			   -Wextra \
			   -Wzero-as-null-pointer-constant \
			   -Wuseless-cast \
			   -Wconversion \
			   -Wshadow \
			   -Wswitch-default \
			   -Wredundant-decls \
			   -Wlogical-op \
			   -Wmissing-include-dirs \
			   -Wcast-align \
			   -Wold-style-cast \
			   -Werror \
			   -Wfatal-errors \
			   -pedantic \
			   -g \
			   -I./include
SANITISER_FLAGS=-fsanitize=undefined
else ifeq ($(CXX), clang++)
COMPILER_FLAGS=$(CLANG_CXXFLAGS)
else ifeq ($(CXX), clang++-5.0)
COMPILER_FLAGS=$(CLANG_CXXFLAGS)
SANITISER_FLAGS=-fsanitize=undefined \
				-fstack-protector-strong \
				-fsanitize=address
endif

# Combine compiler and sanitiser flags, and used C++ standard into final CXXFLAGS.
# CXX_EXTRA_FLAGS are meant to be supplied on the command line.
#
# Build information includes commit from which the VM was built and a hash of
# the code (this is for development builds as commit and version stays the same
# for them). These two pieces of information stay constant and depend only on
# the code.
# If we were to include build date in build information embedded in Viua VM
# executables it should be presented this way: date --rfc-3339 ns
CXXFLAGS=\
		 -std=$(CXX_STANDARD) \
		 -DVIUA_VM_COMMIT="\"$(shell ./scripts/get_head_commit.sh)\"" \
		 -DVIUA_VM_CODE_HASH="\"$(shell ./scripts/get_code_fingerprint.sh)\"" \
		 $(COMPILER_FLAGS) \
		 $(SANITISER_FLAGS) \
		 $(CXX_EXTRA_FLAGS)

# By default, the VM is compiled using no optimisations.
# This makes for shorter compile times, but prevents speed-testing the VM.
# You should run the VM with -O3 every once in a while to see how it's doing.
CXXOPTIMIZATIONFLAGS=-O0
COPTIMIZATIONFLAGS=

# Expose symbols in the VM kernel binary to the shared libraries that are linked at runtime.
# It is done this way to avoid making every library carry its own copy of functions used to access
# registers, types, the kernel, etc.
# I don't yet know how this will affect library compatibility. Runtime crashes because a symbol has
# been moved? Shouldn't happen because the symbol will be located dynamically.
# I guess we'll see.
DYNAMIC_SYMS=-Wl,--dynamic-list-cpp-typeinfo -rdynamic


PREFIX=/usr/local
BIN_PATH=$(PREFIX)/bin
LIB_PATH=$(PREFIX)/lib/viua
H_PATH=$(PREFIX)/include/viua
LDLIBS=-ldl -lpthread

.SUFFIXES: .cpp .h .o

.PHONY: all remake clean clean-support clean-test-compiles install compile-test test version platform


############################################################
# BASICS
all: build/bin/vm/asm \
	build/bin/vm/kernel \
	build/bin/vm/dis \
	build/bin/vm/lex \
	build/bin/vm/parser \
	build/bin/opcodes.bin \
	platform \
	stdlib \
	standardlibrary \
	compile-test \
	tooling

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
	find . -name '*.module' | xargs -n 1 $(RM)

clean-test-compiles:
	find ./tests/compiled -name '*.asm' | xargs -n 1 $(RM)


############################################################
# INSTALLATION AND UNINSTALLATION
bininstall: build/bin/vm/asm \
	build/bin/vm/kernel \
	build/bin/vm/dis
	mkdir -p $(BIN_PATH)
	cp ./build/bin/vm/asm $(BIN_PATH)/viua-asm
	chmod 755 $(BIN_PATH)/viua-asm
	cp ./build/bin/vm/kernel $(BIN_PATH)/viua-vm
	chmod 755 $(BIN_PATH)/viua-vm
	cp ./build/bin/vm/dis $(BIN_PATH)/viua-dis
	chmod 755 $(BIN_PATH)/viua-dis

libinstall: stdlib
	mkdir -p $(LIB_PATH)/std
	mkdir -p $(LIB_PATH)/std/posix
	cp ./build/stdlib/std/*.so $(LIB_PATH)/std
	cp ./build/stdlib/std/posix/*.so $(LIB_PATH)/std/posix

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
platform: build/platform/types/exception.o \
	build/platform/types/value.o \
	build/platform/types/pointer.o \
	build/platform/types/number.o \
	build/platform/types/integer.o \
	build/platform/types/io.o \
	build/platform/types/bits.o \
	build/platform/types/float.o \
	build/platform/types/string.o \
	build/platform/types/atom.o \
	build/platform/types/struct.o \
	build/platform/types/text.o \
	build/platform/types/vector.o \
	build/platform/types/reference.o \
	build/platform/kernel/registerset.o \
	build/platform/support/string.o

build/platform/kernel/registerset.o: src/kernel/registerset.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o $@ $<

build/platform/support/string.o: src/support/string.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o $@ $<


############################################################
# TESTING
build/test/printer.so: build/test/printer.o \
	build/platform/kernel/registerset.o \
	build/platform/types/value.o \
	build/platform/types/exception.o

build/test/sleeper.so: build/test/sleeper.o \
	build/platform/kernel/registerset.o \
	build/platform/types/value.o \
	build/platform/types/exception.o

build/test/math.so: build/test/math.o \
	build/platform/kernel/registerset.o \
	build/platform/types/exception.o \
	build/platform/types/value.o \
	build/platform/types/pointer.o \
	build/platform/types/integer.o \
	build/platform/types/float.o \
	build/platform/types/number.o

build/test/throwing.so: build/test/throwing.o \
	build/platform/kernel/registerset.o \
	build/platform/types/exception.o \
	build/platform/types/value.o \
	build/platform/types/pointer.o \
	build/platform/types/integer.o \
	build/platform/types/number.o

compile-test: build/test/math.so \
	build/test/World.so \
	build/test/throwing.so \
	build/test/printer.so \
	build/test/sleeper.so

test: build/bin/vm/asm \
	build/bin/vm/kernel \
	build/bin/vm/dis \
	compile-test \
	stdlib \
	standardlibrary
	VIUA_LIBRARY_PATH=./build/stdlib:./build/test python3 ./tests/tests.py --verbose --catch --failfast


############################################################
# VERSION UPDATE
version:
	./scripts/update_commit_info.sh
	touch src/front/asm.cpp
	touch src/front/dis.cpp
	touch src/front/kernel.cpp


############################################################
# TOOLS
build/bin/tools/log-shortener: ./tools/log-shortener.cpp
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -o $@ $<

tools: build/bin/tools/log-shortener


############################################################
# RULES
build/platform/types/%.o: src/types/%.cpp include/viua/types/%.h
	$(CXX) $(CXXFLAGS) -fPIC -c -o $@ $<

build/platform/%.o: src/%.cpp
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

build/stdlib/std/%.module: src/stdlib/viua/%.asm build/bin/vm/asm
	./build/bin/vm/asm --lib -o $@ $<

build/stdlib/std/%.o: src/stdlib/%.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -I./include -o $@ $<

build/stdlib/std/%.so: build/stdlib/std/%.o
	$(CXX) $(CXXFLAGS) -fPIC -shared -o $@ $^
	# $(CXX) $(CXXFLAGS) -Wl,--no-undefined -fPIC -shared -o $@ $^

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
VIUA_INSTR_FILES_O=\
				   build/process/instr/arithmetic.o \
				   build/process/instr/atom.o \
				   build/process/instr/bits.o \
				   build/process/instr/bool.o \
				   build/process/instr/calls.o \
				   build/process/instr/cast.o \
				   build/process/instr/closure.o \
				   build/process/instr/concurrency.o \
				   build/process/instr/float.o \
				   build/process/instr/general.o \
				   build/process/instr/int.o \
				   build/process/instr/io.o \
				   build/process/instr/linking.o \
				   build/process/instr/registers.o \
				   build/process/instr/str.o \
				   build/process/instr/struct.o \
				   build/process/instr/tcmechanism.o \
				   build/process/instr/text.o \
				   build/process/instr/vector.o

VIUA_TYPES_FILES_O=build/types/atom.o \
				   build/types/bits.o \
				   build/types/boolean.o \
				   build/types/closure.o \
				   build/types/exception.o \
				   build/types/float.o \
				   build/types/function.o \
				   build/types/integer.o \
				   build/types/io.o \
				   build/types/number.o \
				   build/types/pointer.o \
				   build/types/process.o \
				   build/types/reference.o \
				   build/types/string.o \
				   build/types/struct.o \
				   build/types/text.o \
				   build/types/value.o \
				   build/types/vector.o

build/bin/vm/kernel: \
	build/front/kernel.o \
	build/kernel/kernel.o \
	build/scheduler/process.o \
	build/front/vm.o \
	build/runtime/imports.o \
	build/assert.o \
	build/process.o \
	build/process/stack.o \
	build/bytecode/codec/main/decoder.o \
	build/pid.o \
	build/process/dispatch.o \
	build/scheduler/ffi/request.o \
	build/scheduler/ffi/scheduler.o \
	build/scheduler/io/request.o \
	build/scheduler/io/scheduler.o \
	build/kernel/registerset.o \
	build/kernel/frame.o \
	build/loader.o \
	build/printutils.o \
	build/support/pointer.o \
	build/support/string.o \
	build/support/env.o \
	build/util/string/ops.o \
	$(VIUA_INSTR_FILES_O) \
	$(VIUA_TYPES_FILES_O) \
	build/cg/disassembler/disassembler.o \
	build/assembler/util/pretty_printer.o \
	build/cg/lex.o \
	build/cg/lex/reduce_fns.o \
	build/cg/lex/cook.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -o $@ $^ $(LDLIBS)

OP_ASSEMBLERS= \
	build/assembler/backend/op_assemblers/assemble_op_bits.o \
	build/assembler/backend/op_assemblers/assemble_op_bitset.o \
	build/assembler/backend/op_assemblers/assemble_op_call.o \
	build/assembler/backend/op_assemblers/assemble_op_float.o \
	build/assembler/backend/op_assemblers/assemble_op_frame.o \
	build/assembler/backend/op_assemblers/assemble_op_if.o \
	build/assembler/backend/op_assemblers/assemble_op_integer.o \
	build/assembler/backend/op_assemblers/assemble_op_io_wait.o \
	build/assembler/backend/op_assemblers/assemble_op_join.o \
	build/assembler/backend/op_assemblers/assemble_op_jump.o \
	build/assembler/backend/op_assemblers/assemble_op_process.o \
	build/assembler/backend/op_assemblers/assemble_op_receive.o \
	build/assembler/backend/op_assemblers/assemble_op_string.o \
	build/assembler/backend/op_assemblers/assemble_op_structremove.o \
	build/assembler/backend/op_assemblers/assemble_op_structat.o \
	build/assembler/backend/op_assemblers/assemble_op_join.o \
	build/assembler/backend/op_assemblers/assemble_op_text.o \
	build/assembler/backend/op_assemblers/assemble_op_vector.o \
	build/assembler/backend/op_assemblers/assemble_op_vinsert.o \
	build/assembler/backend/op_assemblers/assemble_op_vpop.o

build/bin/vm/asm: build/front/asm.o \
	build/front/asm/generate.o \
	$(OP_ASSEMBLERS) \
	build/front/asm/assemble_instruction.o \
	build/front/asm/decode.o \
	build/runtime/imports.o \
	build/program.o \
	build/programinstructions.o \
	build/runtime/imports.o \
	build/cg/tokenizer/tokenize.o \
	build/cg/assembler/operands.o \
	build/cg/assembler/codeextract.o \
	build/cg/lex.o \
	build/cg/lex/reduce_fns.o \
	build/cg/lex/cook.o \
	build/cg/tools.o \
	build/cg/assembler/verify.o \
	build/cg/assembler/static_analysis.o \
	build/cg/assembler/utils.o \
	build/bytecode/codec/main/encoder.o \
	build/loader.o \
	build/support/string.o \
	build/support/env.o \
	build/cg/assembler/binary_literals.o \
	build/assembler/frontend/parser.o \
	build/assembler/frontend/gather.o \
	build/assembler/frontend/static_analyser/verifier.o \
	build/assembler/frontend/static_analyser/register_usage.o \
	build/assembler/frontend/static_analyser/checkers/check_closure_instantiations.o \
	build/assembler/frontend/static_analyser/checkers/check_for_unused_registers.o \
	build/assembler/frontend/static_analyser/checkers/check_for_unused_values.o \
	build/assembler/frontend/static_analyser/checkers/check_op_allocate_registers.o \
	build/assembler/frontend/static_analyser/checkers/check_op_arithmetic.o \
	build/assembler/frontend/static_analyser/checkers/check_op_atomeq.o \
	build/assembler/frontend/static_analyser/checkers/check_op_atom.o \
	build/assembler/frontend/static_analyser/checkers/check_op_binary_logic.o \
	build/assembler/frontend/static_analyser/checkers/check_op_bit_arithmetic.o \
	build/assembler/frontend/static_analyser/checkers/check_op_bitat.o \
	build/assembler/frontend/static_analyser/checkers/check_op_bit_increment.o \
	build/assembler/frontend/static_analyser/checkers/check_op_bitnot.o \
	build/assembler/frontend/static_analyser/checkers/check_op_bit_rotates.o \
	build/assembler/frontend/static_analyser/checkers/check_op_bitset.o \
	build/assembler/frontend/static_analyser/checkers/check_op_bit_shifts.o \
	build/assembler/frontend/static_analyser/checkers/check_op_bits.o \
	build/assembler/frontend/static_analyser/checkers/check_op_bits_of_integer.o \
	build/assembler/frontend/static_analyser/checkers/check_op_boolean_and_or.o \
	build/assembler/frontend/static_analyser/checkers/check_op_call.o \
	build/assembler/frontend/static_analyser/checkers/check_op_capturecopy.o \
	build/assembler/frontend/static_analyser/checkers/check_op_capturemove.o \
	build/assembler/frontend/static_analyser/checkers/check_op_capture.o \
	build/assembler/frontend/static_analyser/checkers/check_op_closure.o \
	build/assembler/frontend/static_analyser/checkers/check_op_compare.o \
	build/assembler/frontend/static_analyser/checkers/check_op_copy.o \
	build/assembler/frontend/static_analyser/checkers/check_op_defer.o \
	build/assembler/frontend/static_analyser/checkers/check_op_delete.o \
	build/assembler/frontend/static_analyser/checkers/check_op_draw.o \
	build/assembler/frontend/static_analyser/checkers/check_op_enter.o \
	build/assembler/frontend/static_analyser/checkers/check_op_exception.o \
	build/assembler/frontend/static_analyser/checkers/check_op_exception_tag.o \
	build/assembler/frontend/static_analyser/checkers/check_op_exception_value.o \
	build/assembler/frontend/static_analyser/checkers/check_op_float.o \
	build/assembler/frontend/static_analyser/checkers/check_op_frame.o \
	build/assembler/frontend/static_analyser/checkers/check_op_ftoi.o \
	build/assembler/frontend/static_analyser/checkers/check_op_function.o \
	build/assembler/frontend/static_analyser/checkers/check_op_if.o \
	build/assembler/frontend/static_analyser/checkers/check_op_iinc.o \
	build/assembler/frontend/static_analyser/checkers/check_op_integer.o \
	build/assembler/frontend/static_analyser/checkers/check_op_integer_of_bits.o \
	build/assembler/frontend/static_analyser/checkers/check_op_io_cancel.o \
	build/assembler/frontend/static_analyser/checkers/check_op_io_close.o \
	build/assembler/frontend/static_analyser/checkers/check_op_io_read.o \
	build/assembler/frontend/static_analyser/checkers/check_op_io_wait.o \
	build/assembler/frontend/static_analyser/checkers/check_op_io_write.o \
	build/assembler/frontend/static_analyser/checkers/check_op_isnull.o \
	build/assembler/frontend/static_analyser/checkers/check_op_itof.o \
	build/assembler/frontend/static_analyser/checkers/check_op_izero.o \
	build/assembler/frontend/static_analyser/checkers/check_op_join.o \
	build/assembler/frontend/static_analyser/checkers/check_op_jump.o \
	build/assembler/frontend/static_analyser/checkers/check_op_move.o \
	build/assembler/frontend/static_analyser/checkers/check_op_not.o \
	build/assembler/frontend/static_analyser/checkers/check_op_pideq.o \
	build/assembler/frontend/static_analyser/checkers/check_op_print.o \
	build/assembler/frontend/static_analyser/checkers/check_op_process.o \
	build/assembler/frontend/static_analyser/checkers/check_op_ptrlive.o \
	build/assembler/frontend/static_analyser/checkers/check_op_ptr.o \
	build/assembler/frontend/static_analyser/checkers/check_op_receive.o \
	build/assembler/frontend/static_analyser/checkers/check_op_self.o \
	build/assembler/frontend/static_analyser/checkers/check_op_send.o \
	build/assembler/frontend/static_analyser/checkers/check_op_stof.o \
	build/assembler/frontend/static_analyser/checkers/check_op_stoi.o \
	build/assembler/frontend/static_analyser/checkers/check_op_streq.o \
	build/assembler/frontend/static_analyser/checkers/check_op_string.o \
	build/assembler/frontend/static_analyser/checkers/check_op_structat.o \
	build/assembler/frontend/static_analyser/checkers/check_op_structinsert.o \
	build/assembler/frontend/static_analyser/checkers/check_op_structkeys.o \
	build/assembler/frontend/static_analyser/checkers/check_op_struct.o \
	build/assembler/frontend/static_analyser/checkers/check_op_structremove.o \
	build/assembler/frontend/static_analyser/checkers/check_op_swap.o \
	build/assembler/frontend/static_analyser/checkers/check_op_tailcall.o \
	build/assembler/frontend/static_analyser/checkers/check_op_textat.o \
	build/assembler/frontend/static_analyser/checkers/check_op_textcommonprefix.o \
	build/assembler/frontend/static_analyser/checkers/check_op_textcommonsuffix.o \
	build/assembler/frontend/static_analyser/checkers/check_op_textconcat.o \
	build/assembler/frontend/static_analyser/checkers/check_op_texteq.o \
	build/assembler/frontend/static_analyser/checkers/check_op_textlength.o \
	build/assembler/frontend/static_analyser/checkers/check_op_text.o \
	build/assembler/frontend/static_analyser/checkers/check_op_textsub.o \
	build/assembler/frontend/static_analyser/checkers/check_op_throw.o \
	build/assembler/frontend/static_analyser/checkers/check_op_vat.o \
	build/assembler/frontend/static_analyser/checkers/check_op_vector.o \
	build/assembler/frontend/static_analyser/checkers/check_op_vinsert.o \
	build/assembler/frontend/static_analyser/checkers/check_op_vlen.o \
	build/assembler/frontend/static_analyser/checkers/check_op_vpop.o \
	build/assembler/frontend/static_analyser/checkers/check_op_vpush.o \
	build/assembler/frontend/static_analyser/checkers/check_op_watchdog.o \
	build/assembler/frontend/static_analyser/checkers/utils.o \
	build/assembler/frontend/static_analyser/Register.o \
	build/assembler/frontend/static_analyser/Closure.o \
	build/assembler/frontend/static_analyser/Register_usage_profile.o \
	build/assembler/util/pretty_printer.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -o $@ $^

build/bin/vm/lex: build/front/lexer.o \
	build/cg/lex.o \
	build/cg/lex/reduce_fns.o \
	build/cg/lex/cook.o \
	build/cg/tools.o \
	build/support/string.o \
	build/support/env.o \
	build/cg/assembler/binary_literals.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -o $@ $^

build/bin/vm/parser: build/front/parser.o \
	build/cg/lex.o \
	build/cg/lex/reduce_fns.o \
	build/cg/lex/cook.o \
	build/cg/tools.o \
	build/support/string.o \
	build/support/env.o \
	build/cg/assembler/binary_literals.o \
	build/cg/assembler/utils.o \
	build/assembler/frontend/parser.o \
	build/assembler/frontend/static_analyser/verifier.o \
	build/assembler/util/pretty_printer.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -o $@ $^

build/bin/vm/dis: build/front/dis.o \
	build/loader.o \
	build/cg/disassembler/disassembler.o \
	build/bytecode/codec/main/decoder.o \
	build/support/pointer.o \
	build/support/string.o \
	build/support/env.o \
	build/cg/assembler/utils.o \
	build/assembler/util/pretty_printer.o \
	build/cg/lex.o
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) $(DYNAMIC_SYMS) -o $@ $^


############################################################
# OBJECTS COMMON FOR DEBUGGER AND KERNEL COMPILATION
build/kernel/kernel.o: src/kernel/kernel.cpp \
	include/viua/kernel/kernel.h \
	include/viua/bytecode/opcodes.h \
	include/viua/kernel/frame.h
build/kernel/registerset.o: src/kernel/registerset.cpp \
	include/viua/kernel/registerset.h
build/kernel/frame.o: src/kernel/frame.cpp \
	include/viua/kernel/frame.h


############################################################
# STANDARD LIBRARY
standardlibrary: build/bin/vm/asm \
	build/stdlib/std/vector.module \
	build/stdlib/std/functional.module \
	build/stdlib/std/misc.module

build/stdlib/Std/Os.so: build/stdlib/std/os.so
	mkdir -p build/stdlib/Std
	cp -v $^ $@

build/stdlib/Std/Io.so: build/stdlib/std/io.so
	mkdir -p build/stdlib/Std
	cp -v $^ $@

build/stdlib/Std/Posix/Network.so: build/stdlib/std/posix/network.so
	mkdir -p build/stdlib/Std/Posix
	cp -v $^ $@

build/stdlib/Std/Posix/Io.so: build/stdlib/std/posix/io.so
	mkdir -p build/stdlib/Std/Posix
	cp -v $^ $@

build/stdlib/Std/Random.so: build/stdlib/std/random.so
	mkdir -p build/stdlib/Std
	cp -v $^ $@

stdlib: build/bin/vm/asm \
	standardlibrary \
	build/stdlib/std/typesystem.so \
	build/stdlib/std/os.so \
	build/stdlib/Std/Os.so \
	build/stdlib/std/io.so \
	build/stdlib/Std/Io.so \
	build/stdlib/std/posix/network.so \
	build/stdlib/Std/Posix/Network.so \
	build/stdlib/std/posix/io.so \
	build/stdlib/Std/Posix/Io.so \
	build/stdlib/Std/Random.so \
	build/stdlib/std/kitchensink.so

####
#### Fix for Travis CI.
#### Once GNU Make is upgraded to 4.2 or later, these targets will not
#### be needed.
####
build/stdlib/std/typesystem.o: src/stdlib/typesystem.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -I./include -o $@ $<

build/stdlib/std/os.o: src/stdlib/os.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -I./include -o $@ $<

build/stdlib/std/io.o: src/stdlib/io.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -I./include -o $@ $<

build/stdlib/std/random.o: src/stdlib/random.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -I./include -o $@ $<

build/stdlib/std/kitchensink.o: src/stdlib/kitchensink.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -I./include -o $@ $<
####
#### End of fix for Travis CI.
####

build/stdlib/std/typesystem.so: build/stdlib/std/typesystem.o

build/stdlib/std/os.so: build/stdlib/std/os.o

build/stdlib/std/io.so: build/stdlib/std/io.o

build/stdlib/std/posix/network.so: build/stdlib/std/posix/network.o
build/stdlib/std/posix/io.so: build/stdlib/std/posix/io.o

build/stdlib/std/random.so: build/stdlib/std/random.o

build/stdlib/std/kitchensink.so: build/stdlib/std/kitchensink.o


############################################################
# OPCODE LISTER PROGRAM
build/bin/opcodes.bin: src/bytecode/opcd.cpp \
	include/viua/bytecode/opcodes.h \
	include/viua/bytecode/maps.h
	$(CXX) $(CXXFLAGS) $(CXXOPTIMIZATIONFLAGS) -o $@ $<


############################################################
# DEPENDENCY LIBRARIES
build/lib/linenoise.o: lib/linenoise/linenoise.c lib/linenoise/linenoise.h
	$(CC) $(CFLAGS) $(COPTIMIZATIONFLAGS) -c -o $@ $<


#######################################################################
# TOOLING
tooling: build/tooling/exec/assembler.bin

build/tooling/libs/lexer/tokenise.o: build/tooling/libs/lexer/normaliser.o
build/tooling/exec/assembler.bin: \
	build/tooling/exec/assembler/main.o \
	build/tooling/errors/compile_time.o \
	build/util/string/escape_sequences.o \
	build/util/string/ops.o \
	build/util/filesystem.o \
	build/tooling/libs/lexer/tokenise.o \
	build/tooling/libs/lexer/classifier.o \
	build/tooling/libs/lexer/normaliser.o \
	build/tooling/libs/parser/parse.o \
	build/tooling/libs/static_analyser/static_analyser.o \
	build/tooling/libs/static_analyser/values.o \
	build/tooling/libs/static_analyser/function_state.o \
	build/tooling/errors/compile_time/Error.o \
	build/tooling/errors/compile_time/Error_wrapper.o
	$(CXX) $(CXXFLAGS) -o $@ $^
