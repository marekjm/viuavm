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
# Targets
all: \
	$(BUILD)/codec


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
