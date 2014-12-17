CXXFLAGS=-std=c++11

.SUFFIXES: .cpp .h .o

.PHONY: clean

all: bin/tatanka


run: bin/tatanka
	./bin/tatanka


clean:
	rm -v ./build/*
	rm -v ./bin/*


bin/tatanka: src/bytecode.h src/main.cpp build/cpu.o
	${CXX} ${CXXFLAGS} -o ./bin/tatanka src/main.cpp build/cpu.o


build/cpu.o: src/cpu.h src/cpu.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/cpu.o ./src/cpu.cpp
