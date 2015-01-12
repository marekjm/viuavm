#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include "../bytecode/maps.h"
#include "../support/string.h"
#include "../version.h"
#include "../program.h"
using namespace std;


bool DEBUG = false;


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

    for (unsigned i = 0; i < lines.size(); ++i) {
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

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);

        if (str::startswith(line, ".mark:") or str::startswith(line, ".name:")) {
            /*  Markers and name instructions must be skipped here or they would cause the code below to
             *  throw exceptions.
             */
            continue;
        }

        instr = "";
        inc = 0;

        instr = str::chunk(line);
        try {
            inc = OP_SIZES.at(instr);
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
     * When referring to a mark in code, you should use: `jump :<name>`.
     *
     * The colon before name of the marker is placed here to make it possible to use numeric markers
     * which would otherwise be treated as instruction indexes.
     */
    map<string, int> marks;
    string line, mark;
    int instruction = 0;  // we need separate instruction counter because number of lines is not exactly number of instructions
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = lines[i];
        if (str::startswith(line, ".name:")) {
            continue;
        }
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

map<string, int> getnames(const vector<string>& lines) {
    /** This function will pass over all instructions and
     *  gather "names", i.e. `.name: <register> <name>` instructions which may be used by
     *  as substitutes for register indexes to more easily remember what is stored where.
     *
     *  Example name instruction: `.name: 1 base`.
     *  This allows to access first register with name `base` instead of its index.
     *
     *  Example (which also uses marks) name reference could be: `branch if_equals_0 :finish`.
     */
    map<string, int> names;
    string line, reg, name;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = lines[i];
        if (!str::startswith(line, ".name:")) {
            continue;
        }

        line = str::lstrip(str::sub(line, 6));
        reg = str::chunk(line);
        line = str::lstrip(str::sub(line, reg.size()));
        name = str::chunk(line);

        if (DEBUG) { cout << " *  name: `" << name << "` -> " << reg << endl; }
        try {
            names[name] = stoi(reg);
        } catch (const std::invalid_argument& e) {
            throw "invalid register index in .name instruction";
        }
    }
    return names;
}


int resolvejump(string jmp, const map<string, int>& marks) {
    /*  This function is used to resolve jumps in `jump` and `branch` instructions.
     */
    int addr = 0;
    if (str::isnum(jmp)) {
        addr = stoi(jmp);
    } else {
        jmp = str::sub(jmp, 1);
        try {
            addr = marks.at(jmp);
        } catch (const std::out_of_range& e) {
            throw ("jump to unrecognised marker: " + jmp);
        }
    }
    return addr;
}

string resolveregister(string reg, const map<string, int>& names) {
    /*  This function is used to register numbers when a register is accessed, e.g.
     *  in `istore` instruction or in `branch` in condition operand.
     *
     *  This function MUST return string as teh result is further passed to getint_op() function which *expects* string.
     */
    ostringstream out;
    if (str::isnum(reg)) {
        /*  Basic case - the register is accessed as real index, everything is nice and simple.
         */
        out.str(reg);
    } else if (reg[0] == '@' and str::isnum(str::sub(reg, 1))) {
        /*  Basic case - the register index is taken from another register, everything is still nice and simple.
         */
        out.str(reg);
    } else {
        /*  Case is no longer basic - it seems that a register is being accessed by name.
         *  Names must be checked to see if the one used was declared.
         */
        if (reg[0] == '@') {
            out << '@';
            reg = str::sub(reg, 1);
        }
        try {
            out << names.at(reg);
        } catch (const std::out_of_range& e) {
            // Jinkies! This name was not declared.
            throw ("undeclared name: " + reg);
        }
    }
    return out.str();
}


void assemble(Program& program, const vector<string>& lines, const string& filename) {
    /** Assemble the instructions in lines into bytecode, using
     *  Bytecode Programming API.
     *
     *  :params:
     *
     *  program     - program object which will be used for assembling
     *  lines       - lines with instructions
     */
    string line;
    int instruction = 0;  // instruction counter

    if (DEBUG) { cout << "gathering markers:" << '\n'; }
    map<string, int> marks = getmarks(lines);
    if (DEBUG) { cout << endl; }

    if (DEBUG) { cout << "gathering names:" << '\n'; }
    map<string, int> names = getnames(lines);
    if (DEBUG) { cout << endl; }

    if (DEBUG) { cout << "assembling:" << '\n'; }
    for (unsigned i = 0; i < lines.size(); ++i) {
        /*  This is main assembly loop.
         *  It iterates over lines with instructions and
         *  uses Bytecode Programming API to fill a program with instructions and
         *  from them generate the bytecode.
         */
        line = lines[i];

        if (str::startswith(line, ".mark:") or str::startswith(line, ".name:")) {
            /*  Lines beginning with `.mark:` are just markers placed in code and
             *  are do not produce any bytecode.
             *  Lines beginning with `.name:` are asm instructions that assign human-rememberable names to
             *  registers.
             *
             *  Assembler instructions are discarded by the assembler during the bytecode-generation phase
             *  so they can be skipped in this step as fast as possible
             *  to avoid complicating code that appears later and
             *  deals with assembling CPU instructions.
             */
            if (DEBUG) { cout << " -  skip asm: " << filename << ':' << i << ":+" << instruction << ": " << line << '\n'; }
            continue;
        }

        string instr;
        string operands;
        istringstream iss(line);

        instr = str::chunk(line);
        operands = str::lstrip(str::sub(line, instr.size()));

        if (DEBUG) { cout << " *  assemble: " << filename << ':' << i << ":+" << instruction << ": " << instr << '\n'; }

        if (str::startswith(line, "istore")) {
            string regno_chnk, number_chnk;
            regno_chnk = str::chunk(operands);
            operands = str::sub(operands, regno_chnk.size());
            number_chnk = str::chunk(operands);
            program.istore(getint_op(resolveregister(regno_chnk, names)), getint_op(resolveregister(number_chnk, names)));
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

            rega_chnk = resolveregister(rega_chnk, names);
            regb_chnk = resolveregister(regb_chnk, names);
            regr_chnk = resolveregister(regr_chnk, names);
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

            rega_chnk = resolveregister(rega_chnk, names);
            regb_chnk = resolveregister(regb_chnk, names);
            regr_chnk = resolveregister(regr_chnk, names);
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

            rega_chnk = resolveregister(rega_chnk, names);
            regb_chnk = resolveregister(regb_chnk, names);
            regr_chnk = resolveregister(regr_chnk, names);
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

            rega_chnk = resolveregister(rega_chnk, names);
            regb_chnk = resolveregister(regb_chnk, names);
            regr_chnk = resolveregister(regr_chnk, names);
            // feed chunks into Bytecode Programming API
            program.idiv(getint_op(rega_chnk), getint_op(regb_chnk), getint_op(regr_chnk));
        } else if (str::startswithchunk(line, "ilt")) {
            string rega, regb, regr;

            rega = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega.size()));

            regb = str::chunk(operands);
            operands = str::sub(operands, regb.size());

            regr = str::chunk(operands);

            rega = resolveregister(rega, names);
            regb = resolveregister(regb, names);
            regr = resolveregister(regr, names);
            program.ilt(getint_op(rega), getint_op(regb), getint_op(regr));
        } else if (str::startswithchunk(line, "ilte")) {
            string rega, regb, regr;

            rega = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega.size()));

            regb = str::chunk(operands);
            operands = str::sub(operands, regb.size());

            regr = str::chunk(operands);

            rega = resolveregister(rega, names);
            regb = resolveregister(regb, names);
            regr = resolveregister(regr, names);
            program.ilte(getint_op(rega), getint_op(regb), getint_op(regr));
        } else if (str::startswith(line, "igte")) {
            string rega, regb, regr;

            rega = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega.size()));

            regb = str::chunk(operands);
            operands = str::sub(operands, regb.size());

            regr = str::chunk(operands);

            rega = resolveregister(rega, names);
            regb = resolveregister(regb, names);
            regr = resolveregister(regr, names);
            program.igte(getint_op(rega), getint_op(regb), getint_op(regr));
        } else if (str::startswith(line, "igt")) {
            string rega, regb, regr;

            rega = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega.size()));

            regb = str::chunk(operands);
            operands = str::sub(operands, regb.size());

            regr = str::chunk(operands);

            rega = resolveregister(rega, names);
            regb = resolveregister(regb, names);
            regr = resolveregister(regr, names);
            program.igt(getint_op(rega), getint_op(regb), getint_op(regr));
        } else if (str::startswith(line, "ieq")) {
            string rega, regb, regr;

            rega = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega.size()));

            regb = str::chunk(operands);
            operands = str::sub(operands, regb.size());

            regr = str::chunk(operands);

            rega = resolveregister(rega, names);
            regb = resolveregister(regb, names);
            regr = resolveregister(regr, names);
            program.ieq(getint_op(rega), getint_op(regb), getint_op(regr));
        } else if (str::startswith(line, "iinc")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.iinc(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "idec")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.idec(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "bstore")) {
            string regno_chnk, byte_chnk;
            regno_chnk = str::chunk(operands);
            operands = str::sub(operands, regno_chnk.size());
            byte_chnk = str::chunk(operands);
            program.bstore(getint_op(resolveregister(regno_chnk, names)), getbyte_op(resolveregister(byte_chnk, names)));
        } else if (str::startswith(line, "not")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.lognot(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "and")) {
            string rega, regb, regr;

            rega = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega.size()));

            regb = str::chunk(operands);
            operands = str::sub(operands, regb.size());

            regr = str::chunk(operands);

            rega = resolveregister(rega, names);
            regb = resolveregister(regb, names);
            regr = resolveregister(regr, names);
            program.logand(getint_op(rega), getint_op(regb), getint_op(regr));
        } else if (str::startswith(line, "or")) {
            string rega, regb, regr;

            rega = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, rega.size()));

            regb = str::chunk(operands);
            operands = str::sub(operands, regb.size());

            regr = str::chunk(operands);

            rega = resolveregister(rega, names);
            regb = resolveregister(regb, names);
            regr = resolveregister(regr, names);
            program.logor(getint_op(rega), getint_op(regb), getint_op(regr));
        } else if (str::startswith(line, "move")) {
            string a_chnk, b_chnk;
            a_chnk = str::chunk(operands);
            operands = str::sub(operands, a_chnk.size());
            b_chnk = str::chunk(operands);
            program.move(getint_op(resolveregister(a_chnk, names)), getint_op(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "copy")) {
            string a_chnk, b_chnk;
            a_chnk = str::chunk(operands);
            operands = str::sub(operands, a_chnk.size());
            b_chnk = str::chunk(operands);
            program.copy(getint_op(resolveregister(a_chnk, names)), getint_op(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "ref")) {
            string a_chnk, b_chnk;
            a_chnk = str::chunk(operands);
            operands = str::sub(operands, a_chnk.size());
            b_chnk = str::chunk(operands);
            program.ref(getint_op(resolveregister(a_chnk, names)), getint_op(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "print")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.print(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "echo")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.echo(getint_op(resolveregister(regno_chnk, names)));
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

            program.branch(getint_op(resolveregister(regcond, names)), addrt, addrf);
        } else if (str::startswith(line, "jump")) {
            /*  Jump instruction can be written in two forms:
             *
             *      * `jump <index>`
             *      * `jump :<marker>`
             *
             *  Assembler must distinguish between these two forms, and so it does.
             *  Here, we use a function from string support lib to determine
             *  if the jump is numeric, and thus an index, or
             *  a string - in which case we consider it a marker jump.
             *
             *  If it is a marker jump, assembler will look the marker up in a map and
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
            assemble(program.setdebug(DEBUG), ilines, filename);
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
        out.write((const char*)bytecode, bytes);
        out.close();

        delete[] bytecode;
    } else {
        cout << "fatal: no input file" << endl;
        return 1;
    }

    return ret_code;
}
