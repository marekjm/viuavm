#include "bytecode.h"


void Instruction::local(int i, int n) {
    locals[i] = n;
}
