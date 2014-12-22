CXXFLAGS=-std=c++11

.SUFFIXES: .cpp .h .o

.PHONY: clean run try_test sample_ifbranching

all: bin/lib/asm bin/lib/runner


clean:
	rm -v ./build/*
	rm -v ./bin/lib/*
	rm -v ./bin/sample/*


bin/lib/runner: src/bytecode.h src/lib/runner.cpp build/cpu.o
	${CXX} ${CXXFLAGS} -o ./bin/lib/runner src/lib/runner.cpp build/cpu.o

bin/lib/asm: src/bytecode.h src/lib/asm.cpp build/program.o
	${CXX} ${CXXFLAGS} -o ./bin/lib/asm src/lib/asm.cpp build/program.o


build/cpu.o: src/bytecode.h src/cpu.h src/cpu.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/cpu.o ./src/cpu.cpp


build/program.o: src/bytecode.h src/program.h src/program.cpp
	${CXX} ${CXXFLAGS} -c -o ./build/program.o ./src/program.cpp


bin/test: test.cpp
	${CXX} ${CXXFLAGS} -o ./bin/test test.cpp


bin/sample/sample: src/bytecode.h src/sample/sample.cpp build/cpu.o build/program.o
	${CXX} ${CXXFLAGS} -o ./bin/sample/main src/sample/sample.cpp build/cpu.o build/program.o
	./bin/sample/main


bin/sample/ifbranching: src/bytecode.h src/sample/ifbranching.cpp build/cpu.o build/program.o
	${CXX} ${CXXFLAGS} -o ./bin/sample/ifbranching src/sample/ifbranching.cpp build/cpu.o build/program.o

sample_ifbranching: bin/sample/ifbranching
	@./bin/sample/ifbranching


bin/sample/looping: src/bytecode.h src/sample/looping.cpp build/cpu.o build/program.o
	${CXX} ${CXXFLAGS} -o ./bin/sample/looping src/sample/looping.cpp build/cpu.o build/program.o

sample_looping: bin/sample/looping
	@./bin/sample/looping



try_test: bin/test
	@./bin/test
