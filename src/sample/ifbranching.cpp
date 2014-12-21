#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "../version.h"
#include "../cpu.h"
#include "../bytecode.h"
#include "../program.h"
using namespace std;

typedef char byte;

int main(int argc, char* argv[]) {
    cout << "tatanka VM, version " << VERSION << endl;

    int return_code = 0;

    //Program sample(1048576);    // 1MB
    Program sample(1024);    // 1KB

    sample.setAddressPtr(-1);
    sample.halt();
    sample.setAddressPtr();

    sample.istore(1, 4)
          .istore(2, 12)
          .print(1)
          .print(2)
          .iadd(1, 2, 3)
          .isub(1, 2, 4)    // 5
          .imul(1, 2, 5)
          .idiv(1, 2, 6)
          .ilt(1, 2, 7)
          .ilte(2, 3, 8)
          .igt(3, 4, 9)     // 10
          .igte(4, 5, 10)
          .ieq(5, 6, 11)
          .branchif(7, 19, 14)  // if register 7. is true, branch to 19. instruction
          .print(7)
          .print(8)         // 15
          .print(9)
          .print(10)
          .print(11)
          .branch(-1)
          ;

    cout << "total instructions:  " << sample.instructionCount() << endl;
    cout << "calculating branches..." << endl;
    sample.calculateBranches();
    cout << "total instructions:  " << sample.instructionCount() << endl;
    cout << "total bytecode size: " << sample.size() << endl;

    CPU cpu(64);
    cpu.load(sample.bytecode()).bytes(sample.size());
    return_code = cpu.run();

    return return_code;
}
