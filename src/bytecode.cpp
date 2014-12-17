#include <vector>
#include "bytecode.h"
using namespace std;


Instruction Instruction::local(int i, int n) {
    locals[i] = n;
    return (*this);
}


Instruction Instruction::instance(Bytecode w, int o) {
    return Instruction(w, o);
}




Program& Program::push(Instruction instr) {
    instructions.push_back(instr);

    Program& self = (*this);
    return self;
}

vector<Instruction> Program::getInstructionVector() {
    return instructions;
}
