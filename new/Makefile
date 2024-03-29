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

PRESET?=default
ifneq ($(PRESET),default)
include Makefile.d/Preset/$(PRESET)
endif

CXXFLAGS_OPTIMISATION?=-Og
CXXFLAGS_DEBUG?=-ggdb

# Default sanitiser is for GCC.
# Remember to build without sanitisers if you want to use Valgrind.
CXXFLAGS_SANITISER?=\
					-fsanitize=leak \
					-fsanitize=address \
					-fsanitize=undefined

CXXFLAGS_OPTION?=\
				 -fstack-protector-all \
				 -fstrict-aliasing \
				 -fstrict-overflow

# Only include extra warnings if there is no BUILD_SUBMITTER ie, when the build
# is running in local environment. The CI runs with a basic set of common
# warning flags that should work on any version of GCC and Clang.
ifeq ($(BUILD_SUBMITTER),)
-include Makefile.d/Warnings_$(CXX)
endif

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
		 $(CXXFLAGS_DEBUG) \
		 $(CXXFLAGS_SANITISER) \
		 $(CXXFLAGS_OPTION) \
		 $(CXXFLAGS_WARNING) \
		 -Wall \
		 -Wextra \
		 -Wpedantic \
		 -Werror \
		 -Wfatal-errors \
		 $(CXXFLAGS_NOERROR) \
		 $(CXXFLAGS_INCLUDE)

CXXLIBS=\
		-luring


################################################################################
# High-level targets
all: dist
	@echo "Built version $(VIUAVM_VERSION_FULL)"

dist: \
	$(BUILD)/tools/exec/readelf \
	$(BUILD)/tools/exec/dis \
	$(BUILD)/tools/exec/asm \
	$(BUILD)/tools/exec/ld \
	$(BUILD)/tools/exec/vm \
	$(BUILD)/tools/exec/viua
	ln -s -f tools/exec build/bin

test: dist
	@python3 ./tests/suite.py $(TESTS)

clean-bin:
	find $(BUILD) -type f -delete 2>/dev/null || true
	find $(BUILD) -mindepth 1 -type d -delete 2>/dev/null || true

clean-test:
	find ./tests -type f -name '*.elf' -delete
	find ./tests -type f -name '*.o' -delete
	find ./tests -type f -name '*.asm~' -delete
	find ./tests -type f -name '*.log' -delete
	find ./tests -type d -name '.cache' -print0 |\
		xargs --null -I CACHE_DIR find CACHE_DIR -type f -delete
	find ./tests -type d -name '.cache' -delete

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
	make watch-impl | entr -c make -j

.PHONY: watch-impl
watch-impl:
	(\
		ls -1 Makefile ;\
		find Makefile.d -mindepth 1 ;\
		find . -mindepth 2 -name '*.asm' ;\
		find ./src -type f ;\
		find ./include -type f\
	)

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
	$(BUILD)/support/fdio.o \
	$(BUILD)/support/tty.o

$(BUILD)/tools/exec/dis: \
	$(BUILD)/tools/libs/assembler.o \
	$(BUILD)/support/errno.o \
	$(BUILD)/support/fdio.o \
	$(BUILD)/support/string.o \
	$(BUILD)/support/tty.o \
	$(BUILD)/vm/elf.o \
	$(BUILD)/arch/arch.o \
	$(BUILD)/arch/ops.o

$(BUILD)/tools/exec/asm: \
	$(BUILD)/support/errno.o \
	$(BUILD)/support/fdio.o \
	$(BUILD)/support/string.o \
	$(BUILD)/support/tty.o \
	$(BUILD)/tools/libs/errors/compile_time.o \
	$(BUILD)/tools/libs/lexer.o \
	$(BUILD)/tools/libs/stage/error.o \
	$(BUILD)/tools/libs/stage/save.o \
	$(BUILD)/tools/libs/assembler.o \
	$(BUILD)/arch/arch.o \
	$(BUILD)/arch/ops.o

$(BUILD)/tools/exec/ld: \
	$(BUILD)/arch/arch.o \
	$(BUILD)/arch/ops.o \
	$(BUILD)/vm/elf.o \
	$(BUILD)/tools/libs/errors/compile_time.o \
	$(BUILD)/tools/libs/stage/save.o \
	$(BUILD)/support/errno.o \
	$(BUILD)/support/fdio.o \
	$(BUILD)/support/string.o \
	$(BUILD)/support/tty.o

VIUA_INSTRUCTION_IMPLS=\
					   $(BUILD)/vm/ins/aa.o \
					   $(BUILD)/vm/ins/ecall.o

$(BUILD)/tools/exec/vm: \
	$(BUILD)/support/errno.o \
	$(BUILD)/support/fdio.o \
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
	$(BUILD)/support/fdio.o \
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
