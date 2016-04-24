CXXFLAGS=-std=c++11 -Wall -Wextra -Wzero-as-null-pointer-constant -Wuseless-cast -Wconversion -Winline -pedantic -Wfatal-errors -g -I../../include

PREFIX=/usr/local
BIN_PATH=$(PREFIX)/bin
VIUA_LIB_PATH=$(PREFIX)/lib/viua
VIUA_SITE_LIB_PATH=$(VIUA_LIB_PATH)/lib/site
VIUA_PLATFORM_PATH=../../build/platform
H_PATH=../../include

.PHONY: all install uninstall remake

all: build/example.so

build/example.o: src/example.cpp
	$(CXX) $(CXXFLAGS) -c -fPIC -o build/example.o src/example.cpp

build/example.so: build/example.o
	$(CXX) $(CXXFLAGS) -fPIC -shared -o build/example.so build/example.o $(VIUA_PLATFORM_PATH)/type.o $(VIUA_PLATFORM_PATH)/exception.o $(VIUA_PLATFORM_PATH)/registerset.o

remake: clean build/example.so

clean:
	rm -f ./build/*
	rm -f *.out

install: build/example.so
	mkdir -p $(VIUA_SITE_LIB_PATH)
	cp ./build/example.so $(VIUA_SITE_LIB_PATH)/

uninstall:
	rm -f $(VIUA_SITE_LIB_PATH)/example.so
