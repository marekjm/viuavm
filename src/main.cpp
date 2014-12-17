#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "version.h"
#include "cpu.h"
#include "bytecode.h"
using namespace std;


int main(int argc, char* argv[]) {
    cout << "tatanka VM, version " << VERSION << endl;

    /*
    vector<Instruction> program_halt;
    program_halt.push_back(Instruction(BRANCH, 3));  // 2
    program_halt.push_back(Instruction(HALT));       // 7
    program_halt.push_back(Instruction(BRANCH, 1));  // 3
    program_halt.push_back(Instruction(BRANCH, 2));  // 5
    program_halt.push_back(Instruction(HALT));       // 7


    vector<Instruction> program_iadd;
    Instruction istore1 = Instruction(ISTORE, 2);
    istore1.local(0, 1);
    istore1.local(1, 4);

    Instruction istore2 = Instruction(ISTORE, 2);
    istore2.local(0, 2);
    istore2.local(1, 4);

    Instruction iadd = Instruction(IADD, 3);
    iadd.local(0, 1);
    iadd.local(1, 2);
    iadd.local(2, 3);

    Instruction print_i = Instruction(PRINT_I, 1);
    print_i.local(0, 3);

    program_iadd.push_back(istore1);
    program_iadd.push_back(istore2);
    program_iadd.push_back(iadd);
    program_iadd.push_back(print_i);
    program_iadd.push_back(Instruction(HALT));
    */

    Program program;

    program.push( Instruction(ISTORE, 2).local(0, 1).local(1, 4) )
           .push( Instruction(ISTORE, 2).local(0, 2).local(1, 8) )
           .push( Instruction(PRINT_I).local(0, 1) )
           .push( Instruction(PRINT_I).local(0, 2) )
           ;

    CPU cpu = CPU();

    cpu.setRegisterCount(1024);
    cpu.load(program.getInstructionVector());

    int return_code;
    return_code = cpu.run();

    //cpu.load(program_iadd);
    //cpu.run();

    return return_code;
}
