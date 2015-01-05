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
    { "not",        1 + sizeof(bool) + sizeof(int) },
    { "and",        1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "or",         1 + 3*sizeof(bool) + 3*sizeof(int) },
    { "print",      1 + 1*sizeof(bool) + sizeof(int) },
    { "echo",       1 + 1*sizeof(bool) + sizeof(int) },
    { "jump",       1 + sizeof(int) },
    { "branch",     1 + sizeof(bool) + 3*sizeof(int) },
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
    string instr, line;

    for (int i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);

        if (str::startswith(line, ".mark:")) {
            /*  Markers must be skipped here or they would cause the code below to
             *  throw exceptions.
             */
            continue;
        }

        instr = "";
        inc = 0;

        instr = str::chunk(line);
        try {
            inc = INSTRUCTION_SIZE.at(instr);   // and get its size
        } catch (const std::out_of_range &e) {
            cout << "fatal: unrecognised instruction: `" << instr << '`' << endl;
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


map<string, int> getmarks(const vector<string>& lines) {
    /** This function will pass over all instructions and
     * gather "marks", i.e. `.mark: <name>` directives which may be used by
     * `jump` and `branch` instructions.
     *
     * When referring to a mark in code, you should use: `jump <name>`.
     */
    map<string, int> marks;
    string line, mark;
    int instruction = 0;  // we need separate instruction counter because number of lines is not exactly number of instructions
    for (int i = 0; i < lines.size(); ++i) {
        line = lines[i];
        if (!str::startswith(line, ".mark:")) {
            ++instruction;
            continue;
        }

        line = str::lstrip(str::sub(line, 6));
        mark = str::chunk(line);

        if (DEBUG) { cout << " *  marker: `" << mark << "` -> " << instruction << endl; }
        marks[mark] = instruction;
    }
    return marks;
}


int resolvejump(string jmp, const map<string, int>& marks) {
    /*  This function is used to resolve jumps in `jump` and `branch` instructions.
     */
    int addr = 0;
    if (str::isnum(jmp)) {
        addr = stoi(jmp);
    } else {
        try {
            addr = marks.at(jmp);
        } catch (const std::out_of_range& e) {
            throw ("jump to unrecognised marker: " + jmp);
        }
    }
    return addr;
}


void assemble(Program& program, const vector<string>& lines, bool debug) {
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
    int instruction = 0;  // instruction counter

    if (DEBUG) { cout << "gathering markers:" << '\n'; }
    map<string, int> marks = getmarks(lines);
    if (DEBUG) { cout << endl; }

    if (DEBUG) { cout << "assembling:" << '\n'; }
    for (int i = 0; i < lines.size(); ++i) {
        /*  This is main assembly loop.
         *  It iterates over lines with instructions and
         *  uses Bytecode Programming API to fill a program with instructions and
         *  from them generate the bytecode.
         */
        line = lines[i];

        if (str::startswith(line, ".mark:")) {
            /*  Lines beginning with `.mark:` are just markers placed in code and
             *  are do not produce any bytecode.
             *  As such, they are discarded by the assembler so we just skip them as fast as we can
             *  to avoid complicating code that appears later and
             *  deals with assembling real instructions.
             */
            continue;
        }

        string instr;
        string operands;
        istringstream iss(line);

        instr = str::chunk(line);
        operands = str::lstrip(str::sub(line, instr.size()));

        if (debug) { cout << " *  assemble: " << instr << '\n'; }

        if (str::startswith(line, "istore")) {
            string regno_chnk, number_chnk;
            regno_chnk = str::chunk(operands);
            operands = str::sub(operands, regno_chnk.size());
            number_chnk = str::chunk(operands);
            program.istore(getint_op(regno_chnk), getint_op(number_chnk));
        } else if (str::startswith(line, "iadd")) {
            string rega_chnk, regb_chnk, regr_chnk;
            // get chunk for first-op register
            rega_chnk = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega_chnk.size()));
            // get chunk for second-op register
            regb_chnk = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, regb_chnk.size()));
            // get chunk for result register
            regr_chnk = (operands.size() ? str::chunk(operands) : rega_chnk);
            // feed chunks into Bytecode Programming API
            program.iadd(getint_op(rega_chnk), getint_op(regb_chnk), getint_op(regr_chnk));
        } else if (str::startswith(line, "isub")) {
            string rega_chnk, regb_chnk, regr_chnk;
            // get chunk for first-op register
            rega_chnk = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega_chnk.size()));
            // get chunk for second-op register
            regb_chnk = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, regb_chnk.size()));
            // get chunk for result register
            regr_chnk = (operands.size() ? str::chunk(operands) : rega_chnk);
            // feed chunks into Bytecode Programming API
            program.isub(getint_op(rega_chnk), getint_op(regb_chnk), getint_op(regr_chnk));
        } else if (str::startswith(line, "imul")) {
            string rega_chnk, regb_chnk, regr_chnk;
            // get chunk for first-op register
            rega_chnk = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega_chnk.size()));
            // get chunk for second-op register
            regb_chnk = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, regb_chnk.size()));
            // get chunk for result register
            regr_chnk = (operands.size() ? str::chunk(operands) : rega_chnk);
            // feed chunks into Bytecode Programming API
            program.imul(getint_op(rega_chnk), getint_op(regb_chnk), getint_op(regr_chnk));
        } else if (str::startswith(line, "idiv")) {
            string rega_chnk, regb_chnk, regr_chnk;
            // get chunk for first-op register
            rega_chnk = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega_chnk.size()));
            // get chunk for second-op register
            regb_chnk = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, regb_chnk.size()));
            // get chunk for result register
            regr_chnk = (operands.size() ? str::chunk(operands) : rega_chnk);
            // feed chunks into Bytecode Programming API
            program.idiv(getint_op(rega_chnk), getint_op(regb_chnk), getint_op(regr_chnk));
        } else if (str::startswithchunk(line, "ilt")) {
            string rega, regb, regresult;

            rega = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega.size()));

            regb = str::chunk(operands);
            operands = str::sub(operands, regb.size());

            regresult = str::chunk(operands);

            program.ilt(getint_op(rega), getint_op(regb), getint_op(regresult));
        } else if (str::startswithchunk(line, "ilte")) {
            string rega, regb, regresult;

            rega = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega.size()));

            regb = str::chunk(operands);
            operands = str::sub(operands, regb.size());

            regresult = str::chunk(operands);

            program.ilte(getint_op(rega), getint_op(regb), getint_op(regresult));
        } else if (str::startswith(line, "igte")) {
            string rega, regb, regresult;

            rega = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega.size()));

            regb = str::chunk(operands);
            operands = str::sub(operands, regb.size());

            regresult = str::chunk(operands);

            program.igte(getint_op(rega), getint_op(regb), getint_op(regresult));
        } else if (str::startswith(line, "igt")) {
            string rega, regb, regresult;

            rega = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega.size()));

            regb = str::chunk(operands);
            operands = str::sub(operands, regb.size());

            regresult = str::chunk(operands);

            program.igt(getint_op(rega), getint_op(regb), getint_op(regresult));
        } else if (str::startswith(line, "ieq")) {
            string rega, regb, regresult;

            rega = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega.size()));

            regb = str::chunk(operands);
            operands = str::sub(operands, regb.size());

            regresult = str::chunk(operands);

            program.ieq(getint_op(rega), getint_op(regb), getint_op(regresult));
        } else if (str::startswith(line, "iinc")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.iinc(getint_op(regno_chnk));
        } else if (str::startswith(line, "idec")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.idec(getint_op(regno_chnk));
        } else if (str::startswith(line, "bstore")) {
            string regno_chnk, byte_chnk;
            regno_chnk = str::chunk(operands);
            operands = str::sub(operands, regno_chnk.size());
            byte_chnk = str::chunk(operands);
            program.bstore(getint_op(regno_chnk), getbyte_op(byte_chnk));
        } else if (str::startswith(line, "not")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.lognot(getint_op(regno_chnk));
        } else if (str::startswith(line, "and")) {
            string rega, regb, regresult;

            rega = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega.size()));

            regb = str::chunk(operands);
            operands = str::sub(operands, regb.size());

            regresult = str::chunk(operands);

            program.logand(getint_op(rega), getint_op(regb), getint_op(regresult));
        } else if (str::startswith(line, "or")) {
            string rega, regb, regresult;

            rega = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega.size()));

            regb = str::chunk(operands);
            operands = str::sub(operands, regb.size());

            regresult = str::chunk(operands);

            program.logor(getint_op(rega), getint_op(regb), getint_op(regresult));
        } else if (str::startswith(line, "print")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.print(getint_op(regno_chnk));
        } else if (str::startswith(line, "echo")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.echo(getint_op(regno_chnk));
        } else if (str::startswith(line, "branch")) {
            string regcond, t, f;

            regcond = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, regcond.size()));

            t = str::chunk(operands);
            operands = str::sub(operands, t.size());

            f = str::chunk(operands);

            int addrt, addrf;
            addrt = resolvejump(t, marks);
            /*  If f has non-zero size it means that the instruction has been given three operands.
             *  Otherwise, it is short-form instruction and we should adjust third operand accordingly,
             *  as the spec (a.k.a. "The Code") says:
             *
             *  In case of short-form `branch` instruction:
             *
             *      * first operand is index of the register to check,
             *      * second operand is the address to which to jump if register is true,
             *      * third operand is assumed to be the *next instruction*, i.e. instruction after the branch instruction,
             *
             *  In long (with three operands) form of `branch` instruction:
             *
             *      * third operands is the address to which to jump if register is false,
             */
            addrf = (f.size() ? resolvejump(f, marks) : instruction+1);

            program.branch(getint_op(regcond), addrt, addrf);
        } else if (str::startswith(line, "jump")) {
            /*  Jump instruction can be written in two forms:
             *
             *      * `jump <index>`
             *      * `jump <marker>`
             *
             *  Assembler must distinguish between these two forms, and so it does.
             *  Here, we use a function from string support lib to determine
             *  if the jump is numeric, and thus an index, or
             *  a string - in which case we consider it a marker jump.
             *
             *  If it is a marker jump, assembler will look the marker in a map and
             *  if it is not found throw an exception about unrecognised marker being used.
             */
            program.jump(resolvejump(operands, marks));
        } else if (str::startswith(line, "pass")) {
            program.pass();
        } else if (str::startswith(line, "halt")) {
            program.halt();
        }

        ++instruction;
    }
    if (DEBUG) { cout << endl; }
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
            assemble(program.setdebug(DEBUG), ilines, DEBUG);
        } catch (const string& e) {
            cout << "fatal: error during assembling: " << e << endl;
            return 1;
        } catch (const char*& e) {
            cout << "fatal: error during assembling: " << e << endl;
            return 1;
        } catch (const std::invalid_argument& e) {
            cout << "fatal: error during assembling: " << e.what() << endl;
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
