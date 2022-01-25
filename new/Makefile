################################################################################
# Base directories
BUILD=./build
PREFIX=~/.local

BUILD_EXEC_DIR=$(BUILD)/tools/exec
CORE_DIR=$(PREFIX)/lib/viua/core
CORE_BIN_DIR=$(CORE_DIR)/bin
CORE_LIB_DIR=$(CORE_DIR)/lib
CORE_MAN_DIR=$(CORE_DIR)/man


VIUAVM_VERSION=$(shell sh ./scripts/make_version.sh)
VIUAVM_VERSION_FULL=$(shell sh ./scripts/make_version.sh full)


################################################################################
# Compilation flags
CXX=g++
CXXSTD=c++20

CXXFLAGS_OPTIMISATION=-g -Og

CXXFLAGS_WARNING=\
				 -Wall \
				 -Wextra \
				 -Wpedantic \
				 -Werror \
				 -Wfatal-errors

CXXFLAGS_INCLUDE=\
				 -I./include

CXXFLAGS=\
		 -std=$(CXXSTD) \
		 -DVIUAVM_VERSION='"$(VIUAVM_VERSION)"' \
		 -DVIUAVM_VERSION_FULL='"$(VIUAVM_VERSION_FULL)"' \
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
	$(BUILD)/tools/exec/vm \
	$(BUILD)/tools/exec/viua

test: dist
	@python3 ./tests/suite.py

clean:
	find $(BUILD) -type f -delete
	find $(BUILD) -mindepth 1 -type d -delete
	find ./tests -type f -name '*.bin' -delete

install: all
	mkdir -p $(PREFIX)/lib/viua/{core,site}/{bin,lib,share,man}
	@cp -v $(BUILD_EXEC_DIR)/{asm,dis,readelf,vm} $(CORE_BIN_DIR)/
	@cp -v $(BUILD_EXEC_DIR)/viua $(PREFIX)/bin/

install-binfmt:
	mkdir -p $(PREFIX)/lib/binfmt.d
	cp -v ./binfmt.d/viua-exec.conf $(PREFIX)/lib/binfmt.d
	if [[ -f /proc/sys/fs/binfmt_misc/viua-exec ]]; then \
		echo -1 > /proc/sys/fs/binfmt_misc/viua-exec ; \
	fi
	cat ./binfmt.d/viua-exec.conf > /proc/sys/fs/binfmt_misc/register

format:
	(find ./include -type f ; find ./src -type f) | xargs --max-args 1 --max-procs 0 --verbose clang-format -i


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
	$(BUILD)/support/tty.o

$(BUILD)/tools/exec/dis: \
	$(BUILD)/tools/libs/assembler.o \
	$(BUILD)/support/tty.o \
	$(BUILD)/vm/elf.o \
	$(BUILD)/arch/arch.o \
	$(BUILD)/arch/ops.o

$(BUILD)/tools/exec/asm: \
	$(BUILD)/support/string.o \
	$(BUILD)/support/tty.o \
	$(BUILD)/tools/libs/errors/compile_time.o \
	$(BUILD)/tools/libs/lexer.o \
	$(BUILD)/tools/libs/parser.o \
	$(BUILD)/tools/libs/assembler.o \
	$(BUILD)/arch/arch.o \
	$(BUILD)/arch/ops.o

$(BUILD)/tools/exec/vm: \
	$(BUILD)/support/tty.o \
	$(BUILD)/support/string.o \
	$(BUILD)/vm/core.o \
	$(BUILD)/vm/elf.o \
	$(BUILD)/vm/ins.o \
	$(BUILD)/vm/types.o \
	$(BUILD)/vm/types/traits.o \
	$(BUILD)/arch/arch.o \
	$(BUILD)/arch/ops.o
