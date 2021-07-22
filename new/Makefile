SOURCE=./src
BUILD=./build
PREFIX=~/.local

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
				 -I$(SOURCE)/include

CXXFLAGS=\
		 -std=$(CXXSTD) \
		 $(CXXFLAGS_OPTIMISATION) \
		 $(CXXFLAGS_WARNING) \
		 $(CXXFLAGS_INCLUDE)

all: \
	$(BUILD)/codec

$(BUILD)/%: $(BUILD)/%.o
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD)/%.o: $(SOURCE)/%.cpp
	@mkdir -p $(shell dirname $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
