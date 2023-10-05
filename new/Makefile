################################################################################
# Base directories
BUILD=./build

PREFIX=~/.local
BINDIR=$(PREFIX)/bin
LIBDIR=$(PREFIX)/lib
SHAREDIR=$(PREFIX)/share
MANDIR=$(SHAREDIR)/man

BUILD_EXEC_DIR=$(BUILD)/tools/exec
VIUA_LIB_DIR=$(LIBDIR)/viua
VIUA_CORE_DIR=$(VIUA_LIB_DIR)/viua-core

VIUAVM_VERSION=$(shell sh ./scripts/make_version.sh)
VIUAVM_VERSION_FULL=$(shell sh ./scripts/make_version.sh full)

OPTNAME=$(VIUAVM_VERSION)
VIUA_OPT_PREFIX=$(PREFIX)/opt/viua
VIUA_OPT_ROOT=$(VIUA_OPT_PREFIX)/$(OPTNAME)
VIUA_OPT_LIB_DIR=$(VIUA_OPT_ROOT)/lib
VIUA_OPT_CORE_DIR=$(VIUA_OPT_LIB_DIR)/viua-core
VIUA_OPT_ETC_DIR=$(VIUA_OPT_ROOT)/etc


################################################################################
# Compilation flags
CXX?=g++
CXXSTD?=c++23

CXXFLAGS_OPTIMISATION=-g -Og

CXXFLAGS_NOERROR=
ifeq ($(CXX),g++)
CXXFLAGS_NOERROR=\
				 -Wno-error=dangling-reference
endif

CXXFLAGS_WARNING=\
				 -Wall \
				 -Wextra \
				 -Wpedantic \
				 -Werror \
				 -Wfatal-errors \
				 $(CXXFLAGS_NOERROR)

CXXFLAGS_INCLUDE=\
				 -I./include \
				 -I./3rd-party

FEATURE_DETECT_FLAGS=\
					 $(shell CXX=$(CXX) bash ./feature-tests/test.sh ./feature-tests/strerrorname_np.c)

CXXFLAGS=\
		 -std=$(CXXSTD) \
		 -DVIUAVM_VERSION='"$(VIUAVM_VERSION)"' \
		 -DVIUAVM_VERSION_FULL='"$(VIUAVM_VERSION_FULL)"' \
		 $(FEATURE_DETECT_FLAGS) \
		 $(CXXFLAGS_OPTIMISATION) \
		 $(CXXFLAGS_WARNING) \
		 $(CXXFLAGS_INCLUDE)

CXXLIBS=\
		-luring


################################################################################
# High-level targets
all: dist

dist: \
	$(BUILD)/tools/exec/readelf \
	$(BUILD)/tools/exec/dis \
	$(BUILD)/tools/exec/asm \
	$(BUILD)/tools/exec/ld \
	$(BUILD)/tools/exec/vm \
	$(BUILD)/tools/exec/viua

test: dist
	@python3 ./tests/suite.py

clean-bin:
	find $(BUILD) -type f -delete
	find $(BUILD) -mindepth 1 -type d -delete

clean-test:
	find ./tests -type f -name '*.elf' -delete
	find ./tests -type f -name '*.o' -delete
	find ./tests -type f -name '*.asm~' -delete
	find ./tests -type f -name '*.log' -delete

clean: clean-test clean-bin

install: all
	mkdir -p $(VIUA_CORE_DIR)
	@cp -v $(BUILD_EXEC_DIR)/{asm,dis,readelf,vm,repl} $(VIUA_CORE_DIR)/
	mkdir -p $(BINDIR)
	@cp -v $(BUILD_EXEC_DIR)/viua $(BINDIR)/
	mkdir -p \
		$(MANDIR)/man1 \
		$(MANDIR)/man7
	@cp -v docs/man/*.1 $(MANDIR)/man1
	@cp -v docs/man/*.7 $(MANDIR)/man7
	@cp -v docs/man/viua.1 $(MANDIR)/man1/viua-viua.1

erase:
	rm $(BINDIR)/viua
	rm -r $(VIUA_LIB_DIR)
	rm -v $(MANDIR)/man*/viua*

install-opt:
	make PREFIX=$(VIUA_OPT_ROOT) install
	mkdir -p $(VIUA_OPT_ETC_DIR)/viua
	echo "$(VIUAVM_VERSION_FULL)" > $(VIUA_OPT_ETC_DIR)/viua/version

erase-opt:
	rm -r $(VIUA_OPT_ROOT)

install-binfmt:
	mkdir -p $(PREFIX)/lib/binfmt.d
	cp -v ./binfmt.d/viua-exec.conf $(PREFIX)/lib/binfmt.d
	if [[ -f /proc/sys/fs/binfmt_misc/viua-exec ]]; then \
		echo -1 > /proc/sys/fs/binfmt_misc/viua-exec ; \
	fi
	cat ./binfmt.d/viua-exec.conf > /proc/sys/fs/binfmt_misc/register

format:
	@(\
		find ./include -type f -name '*.h' ;\
		find ./src -type f -name '*.cpp'\
	) | xargs --max-args 1 --max-procs 0 clang-format -i
	@black tests/suite.py


################################################################################
# Development quality-of-life targets
watch:
	(\
		ls -1 Makefile ;\
		find . -name '*.asm' ;\
		find ./src -type f ;\
		find ./include -type f\
	) |\
	entr -c make -j

watch-sample:
	(\
		ls -1 Makefile ;\
		find . -name '*.asm' ;\
		find ./src -type f ;\
		find ./include -type f\
	) |\
	entr -cs 'make -j && ./build/tools/exec/asm sample.asm && VIUA_VM_TRACE_FD=2 ./build/tools/exec/vm ./a.out'

watch-sample-and-install:
	(\
		ls -1 Makefile ;\
		find . -name '*.asm' ;\
		find ./src -type f ;\
		find ./include -type f\
	) |\
	entr -cs 'make -j && make install && ./build/tools/exec/asm sample.asm && ./a.out'

watch-tests:
	(\
		find $(BUILD_EXEC_DIR) -type f ;\
		find ./tests/asm -type f ;\
		ls -1 tests/suite.py \
	) | entr -c python3 ./tests/suite.py

################################################################################
# Template rules
$(BUILD)/%: $(BUILD)/%.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(CXXLIBS)

$(BUILD)/%: ./src/%.sh
	@cp -v $^ $@

$(BUILD)/%.o: ./src/%.cpp
	@mkdir -p $(shell dirname $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<


################################################################################
# Explicit dependencies
$(BUILD)/tools/exec/readelf: \
	$(BUILD)/vm/elf.o \
	$(BUILD)/support/errno.o \
	$(BUILD)/support/tty.o

$(BUILD)/tools/exec/dis: \
	$(BUILD)/tools/libs/assembler.o \
	$(BUILD)/support/errno.o \
	$(BUILD)/support/string.o \
	$(BUILD)/support/tty.o \
	$(BUILD)/vm/elf.o \
	$(BUILD)/arch/arch.o \
	$(BUILD)/arch/ops.o

$(BUILD)/tools/exec/asm: \
	$(BUILD)/support/errno.o \
	$(BUILD)/support/string.o \
	$(BUILD)/support/tty.o \
	$(BUILD)/tools/libs/errors/compile_time.o \
	$(BUILD)/tools/libs/lexer.o \
	$(BUILD)/tools/libs/parser.o \
	$(BUILD)/tools/libs/assembler.o \
	$(BUILD)/tools/libs/stage.o \
	$(BUILD)/arch/arch.o \
	$(BUILD)/arch/ops.o

$(BUILD)/tools/exec/ld: \
	$(BUILD)/arch/arch.o \
	$(BUILD)/arch/ops.o \
	$(BUILD)/vm/elf.o \
	$(BUILD)/tools/libs/errors/compile_time.o \
	$(BUILD)/tools/libs/lexer.o \
	$(BUILD)/tools/libs/parser.o \
	$(BUILD)/tools/libs/assembler.o \
	$(BUILD)/tools/libs/stage.o \
	$(BUILD)/support/errno.o \
	$(BUILD)/support/string.o \
	$(BUILD)/support/tty.o

VIUA_INSTRUCTION_IMPLS=\
					   $(BUILD)/vm/ins/aa.o \
					   $(BUILD)/vm/ins/ecall.o

$(BUILD)/tools/exec/vm: \
	$(BUILD)/support/errno.o \
	$(BUILD)/support/tty.o \
	$(BUILD)/support/string.o \
	$(BUILD)/vm/core.o \
	$(BUILD)/vm/elf.o \
	$(BUILD)/vm/ins.o \
	$(VIUA_INSTRUCTION_IMPLS) \
	$(BUILD)/runtime/pid.o \
	$(BUILD)/arch/arch.o \
	$(BUILD)/arch/ops.o

$(BUILD)/tools/exec/repl: \
	$(BUILD)/arch/arch.o \
	$(BUILD)/arch/ops.o \
	$(BUILD)/vm/core.o \
	$(BUILD)/vm/elf.o \
	$(BUILD)/vm/ins.o \
	$(VIUA_INSTRUCTION_IMPLS) \
	$(BUILD)/support/string.o \
	$(BUILD)/support/tty.o \
	$(BUILD)/tools/libs/errors/compile_time.o \
	$(BUILD)/tools/libs/stage.o \
	$(BUILD)/tools/libs/assembler.o \
	$(BUILD)/tools/libs/lexer.o \
	$(BUILD)/tools/libs/parser.o \
	$(BUILD)/runtime/pid.o \
	3rd-party/linenoise/linenoise.o \
	3rd-party/linenoise/encodings/utf8.o
