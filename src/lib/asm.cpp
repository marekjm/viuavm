#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include "../support/string.h"
#include "../version.h"
#include "../program.h"
using namespace std;


typedef char byte;


bool DEBUG = true;


const map<string, int> INSTRUCTION_SIZE = {
    { "istore",     1 + 2*sizeof(bool) + 2*sizeof(int) },
    { "iadd",       1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "isub",       1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "imul",       1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "idiv",       1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "iinc",       1 + 1*sizeof(bool) + sizeof(int) },
    { "idec",       1 + 1*sizeof(bool) + sizeof(int) },
    { "ilt",        1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "ilte",       1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "igt",        1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "igte",       1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "ieq",        1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "bstore",     1 + 2*sizeof(bool) + sizeof(int) + sizeof(char) },
    { "badd",       1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "bsub",       1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "binc",       1 + 1*sizeof(bool) + sizeof(int) },
    { "bdec",       1 + 1*sizeof(bool) + sizeof(int) },
    { "blt",        1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "blte",       1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "bgt",        1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "bgte",       1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "beq",        1 + 3*sizeof(bool) + sizeof(int) },
    { "print",      1 + 1*sizeof(bool) + sizeof(int) },
    { "echo",       1 + 1*sizeof(bool) + sizeof(int) },
    { "branch",     1 + 1*sizeof(bool) + 1*sizeof(int) },
    { "branchif",   1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "ret",        1 + 1*sizeof(bool) + sizeof(int) },
    { "end",        1 },
    { "halt",       1 },
    { "pass",       1 },
};


int_op getint_op(const string& s) {
    bool ref = s[0] == '@';
    return tuple<bool, int>(ref, stoi(ref ? str::sub(s, 1) : s));
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

        /*  First, we must decide how much memory (how big byte array) we need to hold the program.
         *  This is done by iterating over instruction lines and
         *  increasing bytes size.
         */
        int inc = 0;
        istringstream iss;
        string instr;
        for (int i = 0; i < lines.size(); ++i) {
            instr = "";
            line = lines[i];
            if (!line.size()) continue;

            iss.str(line);
            iss >> instr;                       // extract the instruction
            try {
                inc = INSTRUCTION_SIZE.at(instr);   // and get its size
            } catch (const std::out_of_range &e) {
                cout << "fatal: unrecognised instruction" << endl;
                cout << filename << ":" << i+1 << ": " << line << endl;
                exit(1);
            }

            if (inc == 0) {
                cout << filename << ":" << i+1 << ": '" << line << "'" << endl;
                cout << "fail: line is not empty and requires 0 bytes: ";
                cout << "possibly an unrecognised instruction" << endl;
                exit(1);
            }

            ilines.push_back(line);
            bytes += inc;
            inc = 0;
        }

        if (DEBUG) {
            cout << "total required bytes: " << bytes << endl;
        }

        Program program(bytes);

        for (int i = 0; i < ilines.size(); ++i) {
            line = ilines[i];

            string instr;
            istringstream iss(line);

            if (str::startswith(line, "istore")) {
                line = str::lstrip(str::sub(line, 6));
                string regno_chnk, number_chnk;
                regno_chnk = str::chunk(line);
                line = str::sub(line, regno_chnk.size());
                number_chnk = str::chunk(line);
                program.istore(getint_op(regno_chnk), getint_op(number_chnk));
            } else if (str::startswith(line, "iadd")) {
                inc = 1 + 3*sizeof(int);
            } else if (str::startswith(line, "isub")) {
                inc = 1 + 3*sizeof(int);
            } else if (str::startswith(line, "imul")) {
                inc = 1 + 3*sizeof(int);
            } else if (str::startswith(line, "idiv")) {
                inc = 1 + 3*sizeof(int);
            } else if (str::startswith(line, "ilt")) {
                int rega, regb, regresult;
                iss >> instr >> rega >> regb >> regresult;
                program.ilt(rega, regb, regresult);
            } else if (str::startswith(line, "ilte")) {
                inc = 1 + 3*sizeof(int);
            } else if (str::startswith(line, "igt")) {
                inc = 1 + 3*sizeof(int);
            } else if (str::startswith(line, "igte")) {
                inc = 1 + 3*sizeof(int);
            } else if (str::startswith(line, "ieq")) {
                inc = 1 + 3*sizeof(int);
            } else if (str::startswith(line, "iinc")) {
                int regno;
                iss >> instr >> regno;
                program.iinc(regno);
            } else if (str::startswith(line, "idec")) {
                inc = 1 + sizeof(int);
            } else if (str::startswith(line, "bstore")) {
                int regno;
                short bt;
                iss >> instr >> regno >> bt;
                program.bstore(regno, (char)bt);
            } else if (str::startswith(line, "print")) {
                line = str::lstrip(str::sub(line, 6));
                string regno_chnk;
                regno_chnk = str::chunk(line);
                program.print(getint_op(regno_chnk));
                //int regno;
                //iss >> instr >> regno;
                //program.print(regno);
            } else if (str::startswith(line, "echo")) {
                //int regno;
                //iss >> instr >> regno;
                //program.echo(regno);
            } else if (str::startswith(line, "branchif")) {
                int condition, if_true, if_false;
                iss >> instr >> condition >> if_true >> if_false;
                program.branchif(condition, if_true, if_false);
            } else if (str::startswith(line, "branch")) {
                int addr;
                iss >> instr >> addr;
                program.branch(addr);
            } else if (str::startswith(line, "pass")) {
                program.pass();
            } else if (str::startswith(line, "halt")) {
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
