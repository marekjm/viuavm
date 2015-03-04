#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <algorithm>
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


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;

// are we assembling a library?
bool AS_LIB = false;

bool VERBOSE = false;
bool DEBUG = false;

bool WARNING_ALL = false;
bool ERROR_ALL = false;


// WARNINGS
bool WARNING_MISSING_END = false;
bool WARNING_EMPTY_FUNCTION_BODY = false;
bool WARNING_OPERANDLESS_FRAME = false;


// ERRORS
bool ERROR_MISSING_END = false;
bool ERROR_EMPTY_FUNCTION_BODY = false;
bool ERROR_OPERANDLESS_FRAME = false;


// ASSEMBLY CONSTANTS
const string ENTRY_FUNCTION_NAME = "__entry";


// LOGIC
int_op getint_op(const string& s) {
    bool ref = s[0] == '@';
    return tuple<bool, int>(ref, stoi(ref ? str::sub(s, 1) : s));
}

byte_op getbyte_op(const string& s) {
    bool ref = s[0] == '@';
    return tuple<bool, char>(ref, (char)stoi(ref ? str::sub(s, 1) : s));
}

float_op getfloat_op(const string& s) {
    bool ref = s[0] == '@';
    return tuple<bool, float>(ref, stof(ref ? str::sub(s, 1) : s));
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
        if (str::startswith(line, ".name:") or str::startswith(line, ".link:")) {
            // names and links can be safely skipped as they are not CPU instructions
            continue;
        }
        if (str::startswith(line, ".def:")) {
            // instructions in functions are counted separately so they are
            // not included here
            while (!str::startswith(lines[i], ".end")) { ++i; }
            continue;
        }
        if (!str::startswith(line, ".mark:")) {
            // if all previous checks were false, then this line must be either .mark: directive or
            // an instruction
            // if this check is true - then it is an instruction
            ++instruction;
            continue;
        }

        // get mark name
        line = str::lstrip(str::sub(line, 6));
        mark = str::chunk(line);

        if (DEBUG) { cout << " *  marker: `" << mark << "` -> " << instruction << endl; }
        // create mark for current instruction
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

vector<string> getlinks(const vector<string>& lines) {
    /** This function will pass over all instructions and
     * gather .link: assembler instructions.
     */
    vector<string> links;
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = lines[i];
        if (str::startswith(line, ".link:")) {
            line = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            links.push_back(line);
        }
    }
    return links;
}

vector<string> getFunctionNames(const vector<string>& lines) {
    vector<string> names;

    string line, holdline;
    for (unsigned i = 0; i < lines.size(); ++i) {
        holdline = line = lines[i];
        if (!str::startswith(line, ".def:") and !str::startswith(line, ".dec:")) { continue; }

        if (str::startswith(line, ".def:")) {
            for (int j = i+1; lines[j] != ".end"; ++j, ++i) {}
        }

        line = str::lstrip(str::sub(line, str::chunk(line).size()));
        string name = str::chunk(line);
        line = str::lstrip(str::sub(line, name.size()));
        string ret_sign = str::chunk(line);
        bool returns;   // for now it is unused but is here for the future - when checking is more strict
        if (ret_sign == "true" or ret_sign == "1") {
            returns = true;
        } else if (ret_sign == "false" or ret_sign == "0") {
            returns = false;
        } else {
            throw ("invalid function signature: illegal return declaration: " + holdline);
        }

        names.push_back(name);
    }

    return names;
}
map<string, pair<bool, vector<string> > > getFunctions(const vector<string>& lines) {
    map<string, pair<bool, vector<string> > > functions;

    string line, holdline;
    for (unsigned i = 0; i < lines.size(); ++i) {
        holdline = line = lines[i];
        if (!str::startswith(line, ".def:")) { continue; }

        vector<string> flines;
        for (int j = i+1; lines[j] != ".end"; ++j, ++i) { flines.push_back(lines[j]); }

        line = str::lstrip(str::sub(line, 5));
        string name = str::chunk(line);
        line = str::lstrip(str::sub(line, name.size()));
        string ret_sign = str::chunk(line);
        bool returns;
        if (ret_sign == "true" or ret_sign == "1") {
            returns = true;
        } else if (ret_sign == "false" or ret_sign == "0") {
            returns = false;
        } else {
            throw ("invalid function signature: illegal return declaration: " + holdline);
        }

        if (flines.size() == 0) {
            if (ERROR_EMPTY_FUNCTION_BODY or ERROR_ALL) {
                throw ("function '" + name + "' is empty");
            } else if (WARNING_EMPTY_FUNCTION_BODY or WARNING_ALL) {
                cout << ("warning: function '" + name + "' is empty\n");
            }
        }
        if ((flines.size() == 0 or flines.back() != "end") and (name != "main" and flines.back() != "halt")) {
            if (ERROR_MISSING_END or ERROR_ALL) {
                throw ("missing 'end' at the end of function '" + name + "'");
            } else if (WARNING_MISSING_END or WARNING_ALL) {
                cout << ("warning: missing 'end' at the end of function '" + name + "'\n");
            } else {
                flines.push_back("end");
            }
        }

        functions[name] = pair<bool, vector<string> >(returns, flines);
    }

    return functions;
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
            // first, check if the name is non-empty
            if (reg != "") {
                // Jinkies! This name was not declared.
                throw ("undeclared name: " + reg);
            } else {
                throw "not enough operands";
            }
        }
    }
    return out.str();
}


tuple<string, string> get2operands(string s) {
    /** Returns tuple of two strings - two operands chunked from the `s` string.
     */
    string op_a, op_b;
    op_a = str::chunk(s);
    s = str::sub(s, op_a.size());
    op_b = str::chunk(s);
    return tuple<string, string>(op_a, op_b);
}

tuple<string, string, string> get3operands(string s, bool fill_third = true) {
    string op_a, op_b, op_c;

    op_a = str::chunk(s);
    s = str::lstrip(str::sub(s, op_a.size()));

    op_b = str::chunk(s);
    s = str::lstrip(str::sub(s, op_b.size()));

    /* If s is empty and fill_third is true, use first operand as a filler.
     * In any other case, use the chunk of s.
     * The chunk of empty string will give us empty string and
     * it is a valid (and sometimes wanted) value to return.
     */
    op_c = (s.size() == 0 and fill_third ? op_a : str::chunk(s));

    return tuple<string, string, string>(op_a, op_b, op_c);
}


/*  This is a mapping of instructions to their assembly functions.
 *  Used in the assembly() function.
 *
 *  It is suitable for all instructions which use three, simple register-index operands.
 *
 *  BE WARNED!
 *  This mapping (and the assemble_three_intop_instruction() function) *greatly* reduce the amount of code repetition
 *  in the assembler but are kinda black voodoo magic...
 *
 *  NOTE TO FUTURE SELF:
 *  If you feel comfortable with taking pointers of member functions and calling such things - go on.
 *  Otherwise, it may be better to leave this alone until your have refreshed your memory.
 *  Here is isocpp.org's FAQ about pointers to members (2015-01-17): https://isocpp.org/wiki/faq/pointers-to-members
 */
typedef Program& (Program::*ThreeIntopAssemblerFunction)(int_op, int_op, int_op);
const map<string, ThreeIntopAssemblerFunction> THREE_INTOP_ASM_FUNCTIONS = {
    { "iadd", &Program::iadd },
    { "isub", &Program::isub },
    { "imul", &Program::imul },
    { "idiv", &Program::idiv },
    { "ilt",  &Program::ilt },
    { "ilte", &Program::ilte },
    { "igt",  &Program::igt },
    { "igte", &Program::igte },
    { "ieq",  &Program::ieq },

    { "fadd", &Program::fadd },
    { "fsub", &Program::fsub },
    { "fmul", &Program::fmul },
    { "fdiv", &Program::fdiv },
    { "flt",  &Program::flt },
    { "flte", &Program::flte },
    { "fgt",  &Program::fgt },
    { "fgte", &Program::fgte },
    { "feq",  &Program::feq },

    { "and",  &Program::logand },
    { "or",   &Program::logor },
};

void assemble_three_intop_instruction(Program& program, map<string, int>& names, const string& instr, const string& operands) {
    string rega, regb, regr;
    tie(rega, regb, regr) = get3operands(operands);
    rega = resolveregister(rega, names);
    regb = resolveregister(regb, names);
    regr = resolveregister(regr, names);

    // feed chunks into Bytecode Programming API
    try {
        (program.*THREE_INTOP_ASM_FUNCTIONS.at(instr))(getint_op(rega), getint_op(regb), getint_op(regr));
    } catch (const std::out_of_range& e) {
        throw ("instruction is not present in THREE_INTOP_ASM_FUNCTIONS map but it should be: " + instr);
    }
}


vector<string> filter(const vector<string>& lines) {
    vector<string> filtered;

    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = lines[i];
        if (str::startswith(line, ".mark:") or str::startswith(line, ".name:") or str::startswith(line, ".main:") or str::startswith(line, ".link:")) {
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
            continue;
        }

        if (str::startswith(line, ".def:")) {
            // just skip function lines
            while (lines[++i] != ".end");
            continue;
        }

        filtered.push_back(line);
    }

    return filtered;
}

Program& compile(Program& program, const vector<string>& lines, map<string, int>& marks, map<string, int>& names, const vector<string>& function_names) {
    /** Compile instructions into bytecode using bytecode generation API.
     *
     */
    vector<string> ilines = filter(lines);

    string line;
    int instruction = 0;  // instruction counter
    for (unsigned i = 0; i < ilines.size(); ++i) {
        /*  This is main assembly loop.
         *  It iterates over lines with instructions and
         *  uses bytecode generation API to fill the program with instructions and
         *  from them generate the bytecode.
         */
        line = ilines[i];

        string instr;
        string operands;
        istringstream iss(line);

        instr = str::chunk(line);
        operands = str::lstrip(str::sub(line, instr.size()));

        if (str::startswith(line, "izero")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.izero(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "istore")) {
            string regno_chnk, number_chnk;
            tie(regno_chnk, number_chnk) = get2operands(operands);
            program.istore(getint_op(resolveregister(regno_chnk, names)), getint_op(resolveregister(number_chnk, names)));
        } else if (str::startswith(line, "iadd")) {
            assemble_three_intop_instruction(program, names, "iadd", operands);
        } else if (str::startswith(line, "isub")) {
            assemble_three_intop_instruction(program, names, "isub", operands);
        } else if (str::startswith(line, "imul")) {
            assemble_three_intop_instruction(program, names, "imul", operands);
        } else if (str::startswith(line, "idiv")) {
            assemble_three_intop_instruction(program, names, "idiv", operands);
        } else if (str::startswithchunk(line, "ilt")) {
            assemble_three_intop_instruction(program, names, "ilt", operands);
        } else if (str::startswithchunk(line, "ilte")) {
            assemble_three_intop_instruction(program, names, "ilte", operands);
        } else if (str::startswith(line, "igte")) {
            assemble_three_intop_instruction(program, names, "igte", operands);
        } else if (str::startswith(line, "igt")) {
            assemble_three_intop_instruction(program, names, "igt", operands);
        } else if (str::startswith(line, "ieq")) {
            assemble_three_intop_instruction(program, names, "ieq", operands);
        } else if (str::startswith(line, "iinc")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.iinc(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "idec")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.idec(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "fstore")) {
            string regno_chnk, float_chnk;
            tie(regno_chnk, float_chnk) = get2operands(operands);
            program.fstore(getint_op(resolveregister(regno_chnk, names)), stod(float_chnk));
        } else if (str::startswith(line, "fadd")) {
            assemble_three_intop_instruction(program, names, "fadd", operands);
        } else if (str::startswith(line, "fsub")) {
            assemble_three_intop_instruction(program, names, "fsub", operands);
        } else if (str::startswith(line, "fmul")) {
            assemble_three_intop_instruction(program, names, "fmul", operands);
        } else if (str::startswith(line, "fdiv")) {
            assemble_three_intop_instruction(program, names, "fdiv", operands);
        } else if (str::startswithchunk(line, "flt")) {
            assemble_three_intop_instruction(program, names, "flt", operands);
        } else if (str::startswithchunk(line, "flte")) {
            assemble_three_intop_instruction(program, names, "flte", operands);
        } else if (str::startswithchunk(line, "fgt")) {
            assemble_three_intop_instruction(program, names, "fgt", operands);
        } else if (str::startswithchunk(line, "fgte")) {
            assemble_three_intop_instruction(program, names, "fgte", operands);
        } else if (str::startswith(line, "feq")) {
            assemble_three_intop_instruction(program, names, "feq", operands);
        } else if (str::startswith(line, "bstore")) {
            string regno_chnk, byte_chnk;
            tie(regno_chnk, byte_chnk) = get2operands(operands);
            program.bstore(getint_op(resolveregister(regno_chnk, names)), getbyte_op(resolveregister(byte_chnk, names)));
        } else if (str::startswith(line, "itof")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = get2operands(operands);
            if (b_chnk.size() == 0) { b_chnk = a_chnk; }
            program.itof(getint_op(resolveregister(a_chnk, names)), getint_op(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "ftoi")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = get2operands(operands);
            if (b_chnk.size() == 0) { b_chnk = a_chnk; }
            program.ftoi(getint_op(resolveregister(a_chnk, names)), getint_op(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "strstore")) {
            string reg_chnk, str_chnk;
            reg_chnk = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, reg_chnk.size()));
            str_chnk = str::extract(operands);
            program.strstore(getint_op(resolveregister(reg_chnk, names)), str_chnk);
        } else if (str::startswith(line, "not")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.lognot(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "and")) {
            assemble_three_intop_instruction(program, names, "and", operands);
        } else if (str::startswith(line, "or")) {
            assemble_three_intop_instruction(program, names, "or", operands);
        } else if (str::startswith(line, "move")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = get2operands(operands);
            program.move(getint_op(resolveregister(a_chnk, names)), getint_op(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "copy")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = get2operands(operands);
            program.copy(getint_op(resolveregister(a_chnk, names)), getint_op(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "ref")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = get2operands(operands);
            program.ref(getint_op(resolveregister(a_chnk, names)), getint_op(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "swap")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = get2operands(operands);
            program.swap(getint_op(resolveregister(a_chnk, names)), getint_op(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "free")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.free(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "empty")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.empty(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "isnull")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = get2operands(operands);
            program.isnull(getint_op(resolveregister(a_chnk, names)), getint_op(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "ress")) {
            vector<string> legal_register_sets = {
                "global",   // global register set
                "local",    // local register set for function
                "static",   // static register set
                "temp",     // temporary register set
            };
            if (find(legal_register_sets.begin(), legal_register_sets.end(), operands) == legal_register_sets.end()) {
                throw ("illegal register set name in ress instruction: '" + operands + "'");
            }
            program.ress(operands);
        } else if (str::startswith(line, "tmpri")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.tmpri(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "tmpro")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.tmpro(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "print")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.print(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "echo")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.echo(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "frame")) {
            if (operands.size() == 0) {
                if (ERROR_OPERANDLESS_FRAME or ERROR_ALL) {
                    throw "frame instruction without operand";
                } else if (WARNING_OPERANDLESS_FRAME or WARNING_ALL) {
                    cout << "warning: frame instruction without operand: inserting zero" << endl;
                }
            }
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = get2operands(operands);
            if (a_chnk.size() == 0) { a_chnk = "0"; }
            if (b_chnk.size() == 0) { b_chnk = "16"; }  // default number of local registers
            program.frame(getint_op(resolveregister(a_chnk, names)), getint_op(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "param")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = get2operands(operands);
            program.param(getint_op(resolveregister(a_chnk, names)), getint_op(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "paref")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = get2operands(operands);
            program.paref(getint_op(resolveregister(a_chnk, names)), getint_op(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "arg")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = get2operands(operands);
            program.arg(getint_op(resolveregister(a_chnk, names)), getint_op(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "call")) {
            /*  Full form of call instruction has two operands: instruction index and register index.
             *  If call is given only one operand - it means it is the instruction index and returned value is discarded.
             *  To explicitly state that return value should be discarder, 0 can be supplied as second operand.
             *
             *  Note that when writing assembly code, first operand of `call` instruction is function name and
             *  not an integer.
             *
             */
            string fn_name, reg;
            tie(fn_name, reg) = get2operands(operands);

            if (find(function_names.begin(), function_names.end(), fn_name) == function_names.end()) {
                throw ("call to an undefined function: '" + fn_name + "'");
            }

            /*  Why is the instruction_index index of the function name in the vector with function names and
             *  not a real instruction index?
             *
             *  Because functions are written into bytecode in the order in which their names appear in this vector.
             *  This allows to actually insert a *name* index here and later (when all offsets are known) adjust
             *  instruction indexes accordingly - by obtaining function index and substituting it with summed up
             *  sizes of previous functions.
             *
             *  This makes it possible to declare functions and call them even if they are
             *  not yet defined (just like in C), and
             *  to simplify the code - there is one function that inserts the instruction and
             *  second that adjusts the operand.
             *  It is the same mechanism as with `jump` and `branch` instructions where their jumps are calculated
             *  when the size of bytecode is known.
             */
            // if second operand is empty, fill it with zero
            // which means that return value will be discarded
            if (reg == "") { reg = "0"; }

            program.call(fn_name, getint_op(resolveregister(reg, names)));
        } else if (str::startswith(line, "branch")) {
            /*  If branch is given three operands, it means its full, three-operands form is being used.
             *  Otherwise, it is short, two-operands form instruction and assembler should fill third operand accordingly.
             *
             *  In case of short-form `branch` instruction:
             *
             *      * first operand is index of the register to check,
             *      * second operand is the address to which to jump if register is true,
             *      * third operand is assumed to be the *next instruction*, i.e. instruction after the branch instruction,
             *
             *  In full (with three operands) form of `branch` instruction:
             *
             *      * third operands is the address to which to jump if register is false,
             */
            string condition, if_true, if_false;
            tie(condition, if_true, if_false) = get3operands(operands, false);

            int addrt, addrf;
            addrt = resolvejump(if_true, marks);
            addrf = (if_false.size() ? resolvejump(if_false, marks) : instruction+1);

            program.branch(getint_op(resolveregister(condition, names)), addrt, addrf);
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
        } else if (str::startswith(line, "end")) {
            program.end();
        } else if (str::startswith(line, "pass")) {
            program.pass();
        } else if (str::startswith(line, "halt")) {
            program.halt();
        } else {
            throw ("unimplemented instruction: " + instr);
        }
        ++instruction;
    }

    return program;
}


void assemble(Program& program, const vector<string>& lines, const vector<string>& function_names) {
    /** Assemble instructions in lines into a program.
     *  This function first garthers required information about markers, named registers and functions.
     *  Then, it passes all gathered data into compilation function.
     *
     *  :params:
     *
     *  program         - Program object which will be used for assembling
     *  lines           - lines with instructions
     *  function_names  - list of function names
     */
    map<string, int> marks = getmarks(lines);
    map<string, int> names = getnames(lines);
    compile(program, lines, marks, names, function_names);
}


int main(int argc, char* argv[]) {
    // setup command line arguments vector
    vector<string> args;
    string option;
    for (int i = 1; i < argc; ++i) {
        option = string(argv[i]);
        if (option == "--help") {
            SHOW_HELP = true;
            continue;
        } else if (option == "--version") {
            SHOW_VERSION = true;
            continue;
        } else if (option == "--verbose") {
            VERBOSE = true;
            continue;
        } else if (option == "--debug") {
            DEBUG = true;
            continue;
        } else if (option == "--lib") {
            AS_LIB = true;
            continue;
        } else if (option == "--Wall") {
            WARNING_ALL = true;
            continue;
        } else if (option == "--Eall") {
            ERROR_ALL = true;
            continue;
        } else if (option == "--Wmissing-end") {
            WARNING_MISSING_END = true;
            continue;
        } else if (option == "--Wempty-function") {
            WARNING_EMPTY_FUNCTION_BODY = true;
            continue;
        } else if (option == "--Wopless-frame") {
            WARNING_OPERANDLESS_FRAME = true;
            continue;
        } else if (option == "--Emissing-end") {
            ERROR_MISSING_END = true;
            continue;
        } else if (option == "--Eempty-function") {
            ERROR_EMPTY_FUNCTION_BODY = true;
            continue;
        } else if (option == "--Eopless-frame") {
            ERROR_OPERANDLESS_FRAME = true;
            continue;
        }
        args.push_back(argv[i]);
    }

    int ret_code = 0;

    if (SHOW_HELP or SHOW_VERSION) {
        cout << "wudoo VM assembler, version " << VERSION << endl;
        if (SHOW_HELP) {
            cout << "\nUSAGE:\n";
            cout << "    " << argv[0] << " [option...] <infile> [<outfile>]\n" << endl;
            cout << "OPTIONS:\n";
            cout << "    " << "--version            - show version\n"
                 << "    " << "--help               - display this message\n"
                 << "    " << "--verbose            - show verbose output\n"
                 << "    " << "--debug              - show debugging output\n"
                 << "    " << "--Wall               - warn about everything\n"
                 << "    " << "--Wmissin-end        - warn about missing 'end' instruction at the end of functions\n"
                 << "    " << "--Wempty-function    - warn about empty functions\n"
                 << "    " << "--Wopless-frame      - warn about frames without operands\n"
                 << "    " << "--Eall               - treat all warnings as errors\n"
                 << "    " << "--Emissin-end        - treat missing 'end' instruction at the end of function as error\n"
                 << "    " << "--Eempty-function    - treat empty function as error\n"
                 << "    " << "--Eopless-frame      - treat frames without operands as errors\n"
                 << "    " << "--lib-static         - assemble as a static library\n"
                 << "    " << "--lib-dynamic        - assemble as a dynamic library\n"
                 ;
        }
        return 0;
    }

    if (args.size() == 0) {
        cout << "fatal: no input file" << endl;
        return 1;
    }


    ////////////////////////////////
    // FIND FILENAME AND COMPILENAME
    string filename, compilename = "";
    if (args.size() == 2) {
        filename = args[0];
        compilename = args[1];
    } else if (args.size() == 1) {
        filename = args[0];
        if (AS_LIB) {
            compilename = (filename + ".wlib");
        } else {
            compilename = "a.out";
        }
    }

    if (VERBOSE or DEBUG) {
        cout << "message: assembling \"" << filename << "\" to \"" << compilename << "\"" << endl;
    }

    if (!filename.size()) {
        cout << "fatal: no file to assemble" << endl;
        return 1;
    }


    ////////////////
    // READ LINES IN
    ifstream in(filename, ios::in | ios::binary);
    if (!in) {
        cout << "fatal: file could not be opened: " << filename << endl;
        return 1;
    }

    vector<string> lines;
    vector<string> ilines;  // instruction lines
    string line;

    while (getline(in, line)) { lines.push_back(line); }
    ilines = getilines(lines);


    //////////////////////////////
    // SETUP INITIAL BYTECODE SIZE
    uint16_t bytes = 0;


    ////////////////////////
    // GATHER FUNCTION NAMES
    vector<string> function_names;
    try {
        function_names = getFunctionNames(lines);
    } catch (const string& e) {
        cout << "fatal: " << e << endl;
        return 1;
    }


    /////////////////////////
    // GET MAIN FUNCTION NAME
    string main_function = "";
    for (string line : ilines) {
        if (str::startswith(line, ".main:")) {
            cout << "setting main function to: ";
            main_function = str::lstrip(str::sub(line, 6));
            cout << main_function << endl;
            break;
        }
    }
    if (main_function == "" and not AS_LIB) {
        main_function = "main";
    }
    if (((VERBOSE and main_function != "main" and main_function != "") or DEBUG) and not AS_LIB) {
        cout << "debug (notice): main function set to: '" << main_function << "'" << endl;
    }


    ///////////////////////////////
    // GATHER FUNCTIONS' CODE LINES
    map<string, pair<bool, vector<string> > > functions;
    try {
         functions = getFunctions(ilines);
    } catch (const string& e) {
        cout << "error: function gathering failed: " << e << endl;
        return 1;
    }


    /////////////////////////////////////////
    // CHECK IF MAIN FUNCTION RETURNS A VALUE
    // FIXME: this is just a crude check - it does not acctually checks if these instructions set 0 register
    // this must be better implemented or we will receive "function did not set return register" exceptions at runtime
    bool main_is_defined = (find(function_names.begin(), function_names.end(), main_function) != function_names.end());
    if (not AS_LIB and main_is_defined) {
        string main_second_but_last;
        try {
            main_second_but_last = *(functions.at(main_function).second.end()-2);
        } catch (const std::out_of_range& e) {
            cout << "[asm] fatal: could not find main function (during return value check)" << endl;
            exit(1);
        }
        if (!str::startswith(main_second_but_last, "copy") and
            !str::startswith(main_second_but_last, "move") and
            !str::startswith(main_second_but_last, "swap") and
            !str::startswith(main_second_but_last, "izero")
            ) {
            cout << "fatal: main function does not return a value" << endl;
            return 1;
        }
    }
    if (not main_is_defined and (DEBUG or VERBOSE) and not AS_LIB) {
        cout << "notice: main function (" << main_function << ") is not defined, deferring main function check to post-link phase" << endl;
    }


    //////////////////////////////////////////////////////////
    // MAP FUNCTIONS TO ADDRESSES AND SET STARTING INSTRUCTION
    uint16_t starting_instruction = 0;  // the bytecode offset to first executable instruction
    map<string, uint16_t> function_addresses;
    try {
        for (string name : function_names) {
            function_addresses[name] = starting_instruction;
            try {
                starting_instruction += Program::countBytes(functions.at(name).second);
            } catch (const std::out_of_range& e) {
                throw ("could not find function '" + name + "'");
            }
        }
        bytes = Program::countBytes(ilines);
    } catch (const string& e) {
        cout << "error: bytecode size calculation failed: " << e << endl;
        return 1;
    }


    //////////////////////////
    // GENERATE ENTRY FUNCTION
    if (not AS_LIB) {
        function_names.push_back(ENTRY_FUNCTION_NAME);
        function_addresses[ENTRY_FUNCTION_NAME] = starting_instruction;
        // entry function sets global stuff
        ilines.insert(ilines.begin(), "ress global");
        // append entry function instructions...
        ilines.push_back("frame 0");
        // this must not be hardcoded because we have '.main:' assembler instruction
        // we also save return value in 1 register since 0 means "drop return value"
        ilines.push_back("call " + main_function + " 1");
        // then, register 1 is moved to register 0 so it counts as a return code
        ilines.push_back("move 1 0");
        ilines.push_back("halt");
        functions[ENTRY_FUNCTION_NAME] = pair<bool, vector<string> >(false, ilines);
        // instructions were added so bytecode size must be inreased
        bytes += OP_SIZES.at("ress");
        bytes += OP_SIZES.at("frame");
        bytes += OP_SIZES.at("call");
        bytes += main_function.size();
        bytes += OP_SIZES.at("move");
        bytes += OP_SIZES.at("halt");
    }


    /////////////////////////////////////////////////////////
    // GATHER LINKS, GET THEIR SIZES AND ADJUST BYTECODE SIZE
    vector<string> links = getlinks(ilines);
    vector<tuple<string, uint16_t, char*> > linked_libs_bytecode;
    vector<string> linked_function_names;
    map<string, vector<unsigned> > linked_libs_jumptables;
    uint16_t current_link_offset = bytes;

    for (string lnk : links) {
        ifstream libin(lnk, ios::in | ios::binary);
        if (!libin) {
            cout << "fatal: failed to link static library: '" << lnk << "'" << endl;
            exit(1);
        }

        if (DEBUG or VERBOSE) {
            cout << "[loader] message: linking with: '" << lnk << "\'" << endl;
        }

        unsigned lib_total_jumps;
        libin.read((char*)&lib_total_jumps, sizeof(unsigned));
        if (DEBUG) {
            cout << "[loader] entries in jump table: " << lib_total_jumps << endl;
        }

        vector<unsigned> lib_jumps;
        unsigned lib_jmp;
        for (unsigned i = 0; i < lib_total_jumps; ++i) {
            libin.read((char*)&lib_jmp, sizeof(unsigned));
            lib_jumps.push_back(lib_jmp);
            if (DEBUG) {
                cout << "  jump at byte: " << lib_jmp << endl;
            }
        }

        linked_libs_jumptables[lnk] = lib_jumps;

        // FIXME: urgent!!!
        uint16_t lib_function_ids_section_size = 0;
        char lib_buffer[16];
        libin.read(lib_buffer, sizeof(uint16_t));
        lib_function_ids_section_size = *((uint16_t*)lib_buffer);

        if (DEBUG or VERBOSE) {
            cout << "[loader] message: function mapping section size: " << lib_function_ids_section_size << " bytes" << endl;
        }

        char *lib_buffer_function_ids = new char[lib_function_ids_section_size];
        libin.read(lib_buffer_function_ids, lib_function_ids_section_size);
        char *lib_function_ids_map = lib_buffer_function_ids;

        int i = 0;
        string lib_fn_name;
        uint16_t lib_fn_address;
        while (i < lib_function_ids_section_size) {
            lib_fn_name = string(lib_function_ids_map);
            i += lib_fn_name.size() + 1;  // one for null character
            lib_fn_address = *((uint16_t*)(lib_buffer_function_ids+i));
            i += sizeof(uint16_t);
            lib_function_ids_map = lib_buffer_function_ids+i;
            function_addresses[lib_fn_name] = lib_fn_address+current_link_offset;
            linked_function_names.push_back(lib_fn_name);

            if (DEBUG) {
                cout << "  \"" << lib_fn_name << "\": entry point at byte: " << current_link_offset << '+' << lib_fn_address << endl;
            }
        }
        delete[] lib_buffer_function_ids;


        uint16_t lib_size = 0;
        // FIXME: error check
        libin.read(lib_buffer, 16);
        lib_size = *(uint16_t*)lib_buffer;

        char* linked_code = new char[lib_size];
        // FIXME: error echeck
        libin.read(linked_code, lib_size);

        linked_libs_bytecode.push_back( tuple<string, uint16_t, char*>(lnk, lib_size, linked_code) );
        bytes += lib_size;
    }


    //////////////////////////////////////////////////////////////
    // EXTEND FUNCTION NAMES VECTOR WITH NAMES OF LINKED FUNCTIONS
    for (string name : linked_function_names) { function_names.push_back(name); }


    /////////////////////////////
    // REPORT TOTAL BYTECODE SIZE
    if ((VERBOSE or DEBUG) and linked_function_names.size() != 0) {
        cout << "message: total required bytes: " << bytes << " bytes" << endl;
    }
    if (DEBUG) {
        cout << "debug: required bytes: " << (bytes-(bytes-current_link_offset)) << " local" << endl;
        cout << "debug: required bytes: " << (bytes-current_link_offset) << " linked" << endl;
    }


    ///////////////////////////
    // REPORT FIRST INSTRUCTION
    if ((VERBOSE or DEBUG) and not AS_LIB) {
        cout << "message: first instruction pointer: " << starting_instruction << endl;
    }


    ////////////////////////////////////////
    // CREATE OFSTREAM TO WRITE BYTECODE OUT
    ofstream out(compilename, ios::out | ios::binary);


    ////////////////////
    // CREATE JUMP TABLE
    vector<unsigned> jump_table;


    /////////////////////////////////////////////////////////
    // GENERATE BYTECODE OF LOCAL FUNCTIONS
    //
    // BYTECODE IS GENERATED HERE BUT NOT YET WRITTEN TO FILE
    // THIS MUST BE GENERATED HERE TO OBTAIN FILL JUMP TABLE
    map<string, tuple<int, byte*> > functions_bytecode;
    int functions_section_size = 0;
    for (string name : function_names) {
        // do not generate bytecode for functions that were linked
        if (find(linked_function_names.begin(), linked_function_names.end(), name) != linked_function_names.end()) { continue; }

        if (VERBOSE or DEBUG) {
            cout << "[asm] message: generating bytecode for function \"" << name << '"';
        }
        uint16_t fun_bytes = 0;
        try {
            fun_bytes = Program::countBytes(name == ENTRY_FUNCTION_NAME ? filter(functions.at(name).second) : functions.at(name).second);
            if (VERBOSE or DEBUG) {
                cout << " (" << fun_bytes << " bytes at byte " << functions_section_size << ')' << endl;
            }
        } catch (const string& e) {
            cout << e << endl;
            exit(1);
        } catch (const std::out_of_range& e) {
            cout << e.what() << endl;
            exit(1);
        }

        Program func(fun_bytes);
        try {
            assemble(func.setdebug(DEBUG), functions.at(name).second, function_names);
            func.calculateBranches(functions_section_size);
        } catch (const string& e) {
            cout << (DEBUG ? "\n" : "") << e << endl;
            exit(1);
        } catch (const char*& e) {
            cout << (DEBUG ? "\n" : "") << e << endl;
            exit(1);
        } catch (const std::out_of_range& e) {
            cout << (DEBUG ? "\n" : "") << "[asm] fatal: could not assemble function '" << name << "' (" << e.what() << ')' << endl;
            exit(1);
        }

        // store generated bytecode fragment for future use (we must not yet write it to the file to conform to bytecode format)
        functions_bytecode[name] = tuple<int, byte*>(func.size(), func.bytecode());

        // extend jump table with jumps from current function
        for (unsigned jmp : func.jumps()) {
            if (DEBUG) {
                cout << "[asm] debug: pushed to jump table: " << jmp << '+' << functions_section_size << endl;
            }
            jump_table.push_back(jmp+functions_section_size);
        }

        functions_section_size += func.size();
    }


    //////////////////////////
    // IF ASSEMBLING A LIBRARY
    // WRITE OUT JUMP TABLE
    if (AS_LIB) {
        if (DEBUG) {
            cout << "debug: jump table has " << jump_table.size() << " entries" << endl;
        }
        unsigned total_jumps = jump_table.size();
        out.write((const char*)&total_jumps, sizeof(unsigned));

        for (unsigned jmp : jump_table) {
            out.write((const char*)&jmp, sizeof(unsigned));
        }
    } else {
        if (DEBUG) {
            cout << "debug: skipping jump table write (not a library)" << endl;
        }
    }


    ///////////////////////////////////////////////
    // CHECK IF THE FUNCTION SET AS MAIN IS DEFINED
    // AS ALL THE FUNCTIONS (LOCAL OR LINKED) ARE
    // NOW AVAILABLE
    if (find(function_names.begin(), function_names.end(), main_function) == function_names.end() and not AS_LIB) {
        cout << "error: main function is undefined: " << main_function << endl;
        return 1;
    }


    ///////////////////////////////
    // PREPARE FUNCTION IDS SECTION
    uint16_t function_ids_section_size = 0;
    for (string name : function_names) { function_ids_section_size += name.size(); }
    // we need to insert address (uint16_t) after every function
    function_ids_section_size += sizeof(uint16_t) * function_names.size();
    // for null characters after function names
    function_ids_section_size += function_names.size();


    /////////////////////////////////////////////
    // WRITE OUT FUNCTION IDS SECTION
    // THIS ALSO INCLUDES IDS OF LINKED FUNCTIONS
    out.write((const char*)&function_ids_section_size, sizeof(uint16_t));
    uint16_t functions_size_so_far = 0;
    for (string name : function_names) {
        if (find(linked_function_names.begin(), linked_function_names.end(), name) != linked_function_names.end()) { continue; }
        // function name...
        out.write((const char*)name.c_str(), name.size());
        // ...requires terminating null character
        out.put('\0');
        // mapped address must come after name
        out.write((const char*)&functions_size_so_far, sizeof(uint16_t));
        // functions size must be incremented by the actual size of function's bytecode size
        // to give correct offset for next function
        try {
            functions_size_so_far += Program::countBytes(functions.at(name).second);
        } catch (const std::out_of_range& e) {
            cout << "fatal: could not find function '" << name << "' during bytecode write" << endl;
            exit(1);
        }
    }
    for (string name : linked_function_names) {
        // function name...
        out.write((const char*)name.c_str(), name.size());
        // ...requires terminating null character
        out.put('\0');
        // mapped address must come after name
        uint16_t address = function_addresses[name];
        out.write((const char*)&address, sizeof(uint16_t));
    }


    //////////////////////
    // WRITE BYTECODE SIZE
    out.write((const char*)&bytes, 16);


    ////////////////////////////////////////////
    // WRITE BYTECODE OF LOCAL FUNCTIONS TO FILE
    for (string name : function_names) {
        int fun_size;
        byte* fun_bytecode;
        tie(fun_size, fun_bytecode) = functions_bytecode[name];
        out.write((const char*)fun_bytecode, fun_size);

        // delete the bytecode pointer to avoid memory leak in assembler
        delete[] fun_bytecode;
    }


    ////////////////////////////////////
    // WRITE STATICALLY LINKED LIBRARIES
    // FIXME: implement this after we are able to load static libs
    uint16_t bytes_offset = (bytes-(bytes-current_link_offset));
    for (tuple<string, uint16_t, char*> lnk : linked_libs_bytecode) {
        string lib_name;
        byte* linked_bytecode;
        uint16_t linked_size;
        tie(lib_name, linked_size, linked_bytecode) = lnk;

        if (VERBOSE or DEBUG) {
            cout << "[linker] message: linked module \"" << lib_name <<  "\" written at offset " << bytes_offset << endl;
        }

        vector<unsigned> linked_jumptable;
        try {
            linked_jumptable = linked_libs_jumptables[lib_name];
        } catch (const std::out_of_range& e) {
            cout << "[linker] fatal: could not find jumptable for '" << lib_name << "' (maybe not loaded?)" << endl;
            exit(1);
        }
        for (unsigned jmp : linked_jumptable) {
            if (DEBUG) {
                cout << "[linker] adjusting jump: " << jmp << " -> " << (jmp+bytes_offset) << endl;
            }
            linked_bytecode[jmp] = (jmp+bytes_offset);
        }

        out.write(linked_bytecode, linked_size);
    }

    out.close();

    return ret_code;
}
