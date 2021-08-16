################################################################################
# Base directories
BUILD=./build
PREFIX=~/.local

BUILD_EXEC_DIR=$(BUILD)/tools/exec
CORE_DIR=$(PREFIX)/lib/viua/core
CORE_BIN_DIR=$(CORE_DIR)/bin
CORE_LIB_DIR=$(CORE_DIR)/lib
CORE_MAN_DIR=$(CORE_DIR)/man


################################################################################
# Compilation flags
CXX=g++
CXXSTD=c++20

CXXFLAGS_OPTIMISATION=-g

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
		 $(CXXFLAGS_OPTIMISATION) \
		 $(CXXFLAGS_WARNING) \
		 $(CXXFLAGS_INCLUDE)


################################################################################
# High-level targets
all: \
	$(BUILD)/tools/exec/asm \
	$(BUILD)/tools/exec/vm

clean:
	find $(BUILD) -type f -delete
	find $(BUILD) -mindepth 1 -type d -delete

install: all
	mkdir -p $(PREFIX)/lib/viua/{core,site}/{bin,lib,share,man}
	@cp -v $(BUILD_EXEC_DIR)/asm $(CORE_BIN_DIR)/viua-asm
	@cp -v $(BUILD_EXEC_DIR)/vm $(CORE_BIN_DIR)/viua-vm

install-binfmt:
	mkdir -p $(PREFIX)/lib/binfmt.d
	cp -v ./binfmt.d/viua-exec.conf $(PREFIX)/lib/binfmt.d
	if [[ -f /proc/sys/fs/binfmt_misc/viua-exec ]]; then \
		echo -1 > /proc/sys/fs/binfmt_misc/viua-exec ; \
	fi
	cat ./binfmt.d/viua-exec.conf > /proc/sys/fs/binfmt_misc/register


################################################################################
# Template rules
$(BUILD)/%: $(BUILD)/%.o
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD)/%.o: ./src/%.cpp
	@mkdir -p $(shell dirname $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<


################################################################################
# Explicit dependencies
$(BUILD)/tools/exec/asm: \
	$(BUILD)/support/string.o \
	$(BUILD)/support/tty.o \
	$(BUILD)/tools/libs/errors/compile_time.o \
	$(BUILD)/tools/libs/lexer.o \
	$(BUILD)/arch/arch.o \
	$(BUILD)/arch/ops.o

$(BUILD)/tools/exec/asm.o: \
	./include/viua/libs/errors/compile_time.h \
	./include/viua/libs/lexer.h \
	./include/viua/support/tty.h \
	./include/viua/support/vector.h

$(BUILD)/tools/exec/vm: \
	$(BUILD)/support/string.o \
	$(BUILD)/arch/arch.o \
	$(BUILD)/arch/ops.o

$(BUILD)/arch/ops.o: \
	./include/viua/arch/arch.h \
	./include/viua/arch/ops.h

$(BUILD)/tools/libs/lexer.o: \
	./include/viua/libs/lexer.h

$(BUILD)/tools/libs/errors/compile_time.o: \
	./include/viua/libs/errors/compile_time.h
