CXXFLAGS=-std=c++11

.SUFFIXES: .cpp .h .o

.PHONY: clean run try_test

all: bin/tatanka


run: bin/tatanka
	./bin/tatanka


clean:
	rm -v ./build/*
	rm -v ./bin/*


bin/tatanka: src/bytecode.h src/main.cpp build/cpu.o build/program.o
	${CXX} ${CXXFLAGS} -o ./bin/tatanka src/main.cpp build/cpu.o build/program.o


build/cpu.o: src/bytecode.h src/cpu.h src/cpu.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/cpu.o ./src/cpu.cpp


build/program.o: src/bytecode.h src/program.h src/program.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/program.o ./src/program.cpp


bin/test: test.cpp
	${CXX} ${CXXFLAGS} -o ./bin/test test.cpp


try_test: bin/test
	@./bin/test
