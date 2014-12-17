CXXFLAGS=-std=c++11

.SUFFIXES: .cpp .h .o

.PHONY: clean

all: bin/tatanka


run: bin/tatanka
	./bin/tatanka


clean:
	rm -v ./build/*
	rm -v ./bin/*


bin/tatanka: src/main.cpp build/cpu.o build/bytecode.o
	${CXX} ${CXXFLAGS} -o ./bin/tatanka src/main.cpp build/cpu.o build/bytecode.o


build/cpu.o: src/cpu.h src/cpu.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/cpu.o ./src/cpu.cpp


build/bytecode.o: src/bytecode.h src/bytecode.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/bytecode.o ./src/bytecode.cpp




bin/test: test.cpp
	${CXX} ${CXXFLAGS} -o ./bin/test test.cpp

try_test: bin/test
	@./bin/test
