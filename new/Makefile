################################################################################
# Base directories
SOURCE=./src
BUILD=./build
PREFIX=~/.local


################################################################################
# Compilation flags
CXX = g++
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
	$(BUILD)/codec

install: all
	mkdir -p $(PREFIX)/lib/viua/{core,site}/{bin,lib,share,man}
	cp -v ./build/codec $(PREFIX)/lib/viua/core/bin/viua-vm

install-binfmt:
	mkdir -p $(PREFIX)/lib/binfmt.d
	cp -v ./binfmt.d/viua-exec.conf $(PREFIX)/lib/binfmt.d
	if [[ -f /proc/sys/fs/binfmt_misc/viua-exec ]]; then \
		echo -1 > /proc/sys/fs/binfmt_misc/viua-exec ; \
	fi
	cat ./binfmt.d/viua-exec.conf /proc/sys/fs/binfmt_misc/register


################################################################################
# Template rules
$(BUILD)/%: $(BUILD)/%.o
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD)/%.o: $(SOURCE)/%.cpp
	@mkdir -p $(shell dirname $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<


################################################################################
# Explicit dependencies
$(BUILD)/codec: build/arch/ops.o
