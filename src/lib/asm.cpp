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


bool DEBUG = false;


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
    { "branch",     1 + sizeof(int) },
    { "branchif",   1 + sizeof(bool) + 3*sizeof(int) },
    { "ret",        1 + 1*sizeof(bool) + sizeof(int) },
    { "end",        1 },
    { "halt",       1 },
    { "pass",       1 },
};


int_op getint_op(const string& s) {
    bool ref = s[0] == '@';
    return tuple<bool, int>(ref, stoi(ref ? str::sub(s, 1) : s));
}

byte_op getbyte_op(const string& s) {
    bool ref = s[0] == '@';
    return tuple<bool, char>(ref, (char)stoi(ref ? str::sub(s, 1) : s));
}


vector<string> getilines(const vector<string>& lines) {
    /*  Clears code from empty lines and comments.
     */
    vector<string> ilines;
    string line;

    for (int i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (!line.size() or line[0] == ';') continue;
        ilines.push_back(line);
    }

    return ilines;
}


uint16_t countBytes(const vector<string>& lines, const string& filename) {
    /*  First, we must decide how much memory (how big byte array) we need to hold the program.
     *  This is done by iterating over instruction lines and
     *  increasing bytes size.
     */
    uint16_t bytes = 0;
    int inc = 0;
    istringstream iss;
    string instr, line;

    for (int i = 0; i < lines.size(); ++i) {
        inc = 0;
        instr = "";
        line = str::lstrip(lines[i]);

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

        bytes += inc;
    }

    return bytes;
}


void assemble(Program& program, const vector<string>& lines) {
    /** Assemble the instructions in lines into bytecode, using
     *  Bytecode Programming API.
     *
     *  :params:
     *
     *  program     - program object which will be used for assembling
     *  lines       - lines with instructions
     */
    int inc = 0;
    string line;

    for (int i = 0; i < lines.size(); ++i) {
        line = lines[i];

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
            line = str::lstrip(str::sub(line, 4));
            string rega_chnk, regb_chnk, regr_chnk;
            // get chunk for first-op register
            rega_chnk = str::chunk(line);
            line = str::lstrip(str::sub(line, rega_chnk.size()));
            // get chunk for second-op register
            regb_chnk = str::chunk(line);
            line = str::lstrip(str::sub(line, regb_chnk.size()));
            // get chunk for result register
            regr_chnk = str::chunk(line);
            // feed chunks into Bytecode Programming API
            program.iadd(getint_op(rega_chnk), getint_op(regb_chnk), getint_op(regr_chnk));
        } else if (str::startswith(line, "isub")) {
            line = str::lstrip(str::sub(line, 4));
            string rega_chnk, regb_chnk, regr_chnk;
            // get chunk for first-op register
            rega_chnk = str::chunk(line);
            line = str::lstrip(str::sub(line, rega_chnk.size()));
            // get chunk for second-op register
            regb_chnk = str::chunk(line);
            line = str::lstrip(str::sub(line, regb_chnk.size()));
            // get chunk for result register
            regr_chnk = str::chunk(line);
            // feed chunks into Bytecode Programming API
            program.isub(getint_op(rega_chnk), getint_op(regb_chnk), getint_op(regr_chnk));
        } else if (str::startswith(line, "imul")) {
            inc = 1 + 3*sizeof(int);
        } else if (str::startswith(line, "idiv")) {
            inc = 1 + 3*sizeof(int);
        } else if (str::startswith(line, "ilt")) {
            line = str::lstrip(str::sub(line, 3));

            string rega, regb, regresult;

            rega = str::chunk(line);
            line = str::lstrip(str::sub(line, rega.size()));

            regb = str::chunk(line);
            line = str::sub(line, regb.size());

            regresult = str::chunk(line);

            program.ilt(getint_op(rega), getint_op(regb), getint_op(regresult));
        } else if (str::startswith(line, "ilte")) {
            inc = 1 + 3*sizeof(int);
        } else if (str::startswith(line, "igt")) {
            inc = 1 + 3*sizeof(int);
        } else if (str::startswith(line, "igte")) {
            inc = 1 + 3*sizeof(int);
        } else if (str::startswith(line, "ieq")) {
            inc = 1 + 3*sizeof(int);
        } else if (str::startswith(line, "iinc")) {
            line = str::lstrip(str::sub(line, 4));
            string regno_chnk;
            regno_chnk = str::chunk(line);
            program.iinc(getint_op(regno_chnk));
        } else if (str::startswith(line, "idec")) {
            inc = 1 + sizeof(int);
        } else if (str::startswith(line, "bstore")) {
            line = str::lstrip(str::sub(line, 6));
            string regno_chnk, byte_chnk;
            regno_chnk = str::chunk(line);
            line = str::sub(line, regno_chnk.size());
            byte_chnk = str::chunk(line);
            program.bstore(getint_op(regno_chnk), getbyte_op(byte_chnk));
        } else if (str::startswith(line, "print")) {
            line = str::lstrip(str::sub(line, 6));
            string regno_chnk;
            regno_chnk = str::chunk(line);
            program.print(getint_op(regno_chnk));
        } else if (str::startswith(line, "echo")) {
            line = str::lstrip(str::sub(line, 4));
            string regno_chnk;
            regno_chnk = str::chunk(line);
            program.echo(getint_op(regno_chnk));
        } else if (str::startswith(line, "branchif")) {
            line = str::lstrip(str::sub(line, 8));

            string regcond, t, f;

            regcond = str::chunk(line);
            line = str::lstrip(str::sub(line, regcond.size()));

            t = str::chunk(line);
            line = str::sub(line, t.size());

            f = str::chunk(line);

            int addrt, addrf;
            addrt = stoi(t);
            addrf = stoi(f);

            program.branchif(getint_op(regcond), addrt, addrf);
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
        string filename, compilename = "";
        if (args[1] == "--debug") {
            DEBUG = true;
            if (argc > 2) {
                filename = args[2];
            } else {
                cout << "fatal: filename required" << endl;
                return 1;
            }
        } else {
            filename = args[1];
        }

        if (DEBUG and argc >= 4) {
            compilename = args[3];
        } else if (!DEBUG and argc >= 3) {
            compilename = args[2];
        }
        if (compilename.size() == 0) {
            compilename = "out.bin";
        }

        if (DEBUG) {
            cout << "assembling \"" << filename << "\" to \"" << compilename << "\"" << endl;
        }

        if (!filename.size()) {
            cout << "fatal: no file to assemble" << endl;
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
        ilines = getilines(lines);

        uint16_t bytes = 0;
        uint16_t starting_instruction = 0;  // the bytecode offset to first executable instruction

        bytes = countBytes(ilines, filename);

        if (DEBUG) { cout << "total required bytes: "; }
        if (DEBUG) { cout << bytes << endl; }

        if (DEBUG) { cout << "executable offset: " << starting_instruction << endl; }

        Program program(bytes);
        try {
            assemble(program.setdebug(DEBUG), ilines);
        } catch (const char*& e) {
            cout << "fatal: error during assembling: " << e << endl;
            return 1;
        }

        if (DEBUG) { cout << "branches: "; }
        try {
            program.calculateBranches();
        } catch (const char*& e) {
            cout << "fatal: branch calculation failed: " << e << endl;
            return 1;
        }
        if (DEBUG) { cout << "OK" << endl; }

        byte* bytecode = program.bytecode();

        ofstream out(compilename, ios::out | ios::binary);
        out.write((const char*)&bytes, 16);
        out.write((const char*)&starting_instruction, 16);
        out.write(bytecode, bytes);
        out.close();

        delete[] bytecode;
    } else {
        cout << "fatal: no input file" << endl;
        return 1;
    }

    return ret_code;
}
