#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include "../version.h"
#include "../program.h"
using namespace std;


typedef char byte;


bool startswith(string s, string w) {
    return (s.compare(0, w.length(), w) == 0);
}


int main(int argc, char* argv[]) {
    // setup command line arguments vector
    vector<string> args;
    for (int i = 0; i < argc; ++i) { args.push_back(argv[i]); }

    int ret_code = 0;

    if (argc > 1 and args[1] == "--help") {
        cout << "tatanka VM assembler, version " << VERSION << endl;
        cout << args[0] << " <infile> [<outfile>]" << endl;
        return 0;
    }

    // run code
    if (argc > 1) {
        string filename = args[1];

        if (!filename.size()) {
            cout << "fatal: no file to run" << endl;
            return 1;
        }

        ifstream in(filename, ios::in | ios::binary);

        if (!in) {
            cout << "fatal: file could not be opened" << endl;
            return 1;
        }

        vector<string> lines;
        vector<string> ilines;  // instruction lines
        string line;

        while (getline(in, line)) { lines.push_back(line); }

        uint16_t bytes = 0;
        unsigned instructions = 0;

        int inc = 0;
        for (int i = 0; i < lines.size(); ++i) {
            line = lines[i];
            if (!line.size()) continue;

            //cout << filename << ":" << i+1 << ": '" << line << "' = ";

            if (startswith(line, "istore")) {
                inc = 1 + 2*sizeof(int);
            } else if (startswith(line, "iadd")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "isub")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "imul")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "idiv")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "ilt")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "ilte")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "igt")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "igte")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "ieq")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "iinc")) {
                inc = 1 + sizeof(int);
            } else if (startswith(line, "idec")) {
                inc = 1 + sizeof(int);
            } else if (startswith(line, "print")) {
                inc = 1 + sizeof(int);
            } else if (startswith(line, "branchif")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "branch")) {
                inc = 1 + sizeof(int);
            } else if (startswith(line, "pass")) {
                inc = 1;
            } else if (startswith(line, "halt")) {
                inc = 1;
            }

            if (inc == 0) {
                cout << filename << ":" << i+1 << ": '" << line << "'" << endl;
                cout << "fail: line is not empty and requires 0 bytes: ";
                cout << "possibly an unrecognised instruction" << endl;
                exit(1);
            }

            //cout << inc << " bytes" << endl;

            ilines.push_back(line);
            instructions++;
            bytes += inc;
            inc = 0;
        }

        Program program(bytes);

        for (int i = 0; i < ilines.size(); ++i) {
            line = lines[i];

            string instr;
            istringstream iss(line);

            if (startswith(line, "istore")) {
                int regno, number;
                iss >> instr >> regno >> number;
                program.istore(regno, number);
            } else if (startswith(line, "iadd")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "isub")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "imul")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "idiv")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "ilt")) {
                int rega, regb, regresult;
                iss >> instr >> rega >> regb >> regresult;
                program.ilt(rega, regb, regresult);
            } else if (startswith(line, "ilte")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "igt")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "igte")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "ieq")) {
                inc = 1 + 3*sizeof(int);
            } else if (startswith(line, "iinc")) {
                int regno;
                iss >> instr >> regno;
                program.iinc(regno);
            } else if (startswith(line, "idec")) {
                inc = 1 + sizeof(int);
            } else if (startswith(line, "print")) {
                int regno;
                iss >> instr >> regno;
                program.print(regno);
            } else if (startswith(line, "branchif")) {
                int condition, if_true, if_false;
                iss >> instr >> condition >> if_true >> if_false;
                program.branchif(condition, if_true, if_false);
            } else if (startswith(line, "branch")) {
                int addr;
                iss >> instr >> addr;
                program.branch(addr);
            } else if (startswith(line, "pass")) {
                program.pass();
            } else if (startswith(line, "halt")) {
                program.halt();
            }
        }

        program.calculateBranches();

        byte* bytecode = program.bytecode();

        ofstream out(((args.size() >= 3) ? args[2] : "out.bin"), ios::out | ios::binary);
        out.write((const char*)&bytes, 16);
        out.write(bytecode, bytes);
        out.close();

        delete[] bytecode;
    } else {
        cout << "fatal: no input file" << endl;
        return 1;
    }

    return ret_code;
}
