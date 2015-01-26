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


// MISC FLAGS
bool DEBUG = false;
bool WARNING_ALL = false;
bool ERROR_ALL = false;


// WARNINGS
bool WARNING_MISSING_END = true;
bool WARNING_EMPTY_FUNCTION_BODY = true;
bool WARNING_OPERANDLESS_FRAME = false;


// ERRORS
bool ERROR_MISSING_END = true;
bool ERROR_EMPTY_FUNCTION_BODY = true;
bool ERROR_OPERANDLESS_FRAME = false;


// LOGIC
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
            // names can be safely skipped as they are not instructions
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

vector<string> getFunctionNames(const vector<string>& lines) {
    vector<string> names;

    string line, holdline;
    for (unsigned i = 0; i < lines.size(); ++i) {
        holdline = line = lines[i];
        if (!str::startswith(line, ".def:")) { continue; }

        for (int j = i+1; lines[j] != ".end"; ++j, ++i) {}

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
        if (DEBUG) { cout << " + defining function: " << name << endl; }
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
                throw ("error: function '" + name + "' is empty");
            } else if (WARNING_EMPTY_FUNCTION_BODY or WARNING_ALL) {
                cout << ("warning: function '" + name + "' is empty\n");
            }
        }
        if (flines.back() != "end") {
            if (ERROR_MISSING_END or ERROR_ALL) {
                throw ("error: missing 'end' at the end of function '" + name + "'");
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
            // Jinkies! This name was not declared.
            throw ("undeclared name: " + reg);
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
 *  This mapping (and the assemble_three_intop_instruction() function) *seriously* reduce the amount of code repetition
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
    { "ilt", &Program::ilt },
    { "ilte", &Program::ilte },
    { "igt", &Program::igt },
    { "igte", &Program::igte },
    { "ieq", &Program::ieq },

    { "and", &Program::logand },
    { "or", &Program::logor },
};

void assemble_three_intop_instruction(Program& program, map<string, int>& names, const string& instr, const string& operands) {
    string rega, regb, regr;
    tie(rega, regb, regr) = get3operands(operands);
    rega = resolveregister(rega, names);
    regb = resolveregister(regb, names);
    regr = resolveregister(regr, names);

    // feed chunks into Bytecode Programming API
    (program.*THREE_INTOP_ASM_FUNCTIONS.at(instr))(getint_op(rega), getint_op(regb), getint_op(regr));
}


Program& compile(Program& program, const vector<string>& lines, map<string, int>& marks, map<string, int>& names, const vector<string>& function_names) {
    /** Compile instructions into bytecode using bytecode generation API.
     *
     */
    if (DEBUG) { cout << "assembling:" << '\n'; }

    string line;
    int instruction = 0;  // instruction counter
    for (unsigned i = 0; i < lines.size(); ++i) {
        /*  This is main assembly loop.
         *  It iterates over lines with instructions and
         *  uses bytecode generation API to fill the program with instructions and
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
            if (DEBUG) { cout << " -  skip: +" << instruction+1 << ": " << line << '\n'; }
            continue;
        }

        if (str::startswith(line, ".def:")) {
            // just skip function lines
            while (lines[++i] != ".end");
            continue;
        }

        string instr;
        string operands;
        istringstream iss(line);

        instr = str::chunk(line);
        operands = str::lstrip(str::sub(line, instr.size()));

        if (DEBUG) { cout << " *  assemble: +" << instruction+1 << ": " << instr; }

        if (str::startswith(line, "istore")) {
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
        } else if (str::startswith(line, "bstore")) {
            string regno_chnk, byte_chnk;
            tie(regno_chnk, byte_chnk) = get2operands(operands);
            program.bstore(getint_op(resolveregister(regno_chnk, names)), getbyte_op(resolveregister(byte_chnk, names)));
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
        } else if (str::startswith(line, "ret")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.ret(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "print")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.print(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "echo")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.echo(getint_op(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "frame")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            if (ERROR_OPERANDLESS_FRAME or ERROR_ALL) {
                throw "frame instruction without operand";
            } else if (WARNING_OPERANDLESS_FRAME or WARNING_ALL) {
                cout << "warning: frame instruction without operand: inserting zero" << endl;
            }
            if (regno_chnk.size() == 0) { regno_chnk = "0"; }
            program.frame(getint_op(resolveregister(regno_chnk, names)));
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
            string fun_name, reg;
            tie(fun_name, reg) = get2operands(operands);

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
            int instruction_index = -1;
            for (unsigned i = 0; i < function_names.size(); ++i) {
                if (fun_name == function_names[i]) {
                    instruction_index = (int)i;
                    break;
                }
            }

            // if second operand is empty, fill it with zero
            // which means that return value will be discarded
            if (reg == "") { reg = "0"; }

            if (DEBUG) { cout << ' ' << fun_name << ' ' << reg << endl; }
            program.call(instruction_index, getint_op(resolveregister(reg, names)));
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

        if (DEBUG) { cout << '\n'; }

        ++instruction;
    }
    if (DEBUG) { cout << endl; }

    return program;
}


void assemble(Program& program, const vector<string>& lines, const vector<string>& function_names) {
    /** Assemble instructions in lines into a program.
     *  This function first garthers required information about markers, named registers and functions.
     *  Then, it passes all gathered data into compilation function.
     *
     *  :params:
     *
     *  program     - Program object which will be used for assembling
     *  lines       - lines with instructions
     */
    if (DEBUG) { cout << "gathering markers:" << '\n'; }
    map<string, int> marks = getmarks(lines);
    if (DEBUG) { cout << endl; }

    if (DEBUG) { cout << "gathering names:" << '\n'; }
    map<string, int> names = getnames(lines);
    if (DEBUG) { cout << endl; }

    compile(program, lines, marks, names, function_names);
}


int main(int argc, char* argv[]) {
    // setup command line arguments vector
    vector<string> args;
    for (int i = 0; i < argc; ++i) { args.push_back(argv[i]); }

    int ret_code = 0;

    if (argc > 1 and args[1] == "--help") {
        cout << "wudoo VM assembler, version " << VERSION << endl;
        cout << args[0] << " <infile> [<outfile>]" << endl;
        return 0;
    }

    if (argc < 2) {
        cout << "fatal: no input file" << endl;
        return 1;
    }

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

    vector<string> function_names = getFunctionNames(lines);
    map<string, pair<bool, vector<string> > > functions;
    try {
         functions = getFunctions(ilines);
    } catch (const string& e) {
        cout << "fatal: error during function gathering: " << e << endl;
        return 1;
    }

    try {
        if (DEBUG) { cout << "functions:\n"; }
        for (string name : function_names) {
            if (DEBUG) { cout << " *  " <<  name << ": " << Program::countBytes(functions.at(name).second) << " bytes" << endl; }
            starting_instruction += Program::countBytes(functions.at(name).second);
        }

        bytes = Program::countBytes(ilines);
    } catch (const string& e) {
        cout << "fatal: error furing bytecode size calculations: " << e << endl;
        return 1;
    }

    if (DEBUG) { cout << "total required bytes: " <<  bytes << endl; }
    if (DEBUG) { cout << "executable offset: " << starting_instruction << endl; }

    Program program(bytes);
    try {
        assemble(program.setdebug(DEBUG), ilines, function_names);
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

    if (DEBUG) { cout << "calculating jumps:\n"; }
    try {
        /*  Here, starting_instruction is used as offset.
         *  This is because all branches and jumps should be adjusted to the amount of bytes
         *  that precede them.
         */
        program.calculateBranches(starting_instruction);
        program.calculateCalls(function_names, functions);
    } catch (const char*& e) {
        cout << "fatal: branch calculation failed: " << e << endl;
        return 1;
    }
    if (DEBUG) { cout << "OK" << endl; }

    byte* bytecode = program.bytecode();
    int functions_section_size = 0;

    ofstream out(compilename, ios::out | ios::binary);
    out.write((const char*)&bytes, 16);
    out.write((const char*)&starting_instruction, 16);
    for (string name : function_names) {
        if (DEBUG) { cout << "generating bytecode for function (at bytecode " << functions_section_size << "): " << name << endl; }
        Program func(Program::countBytes(functions.at(name).second));
        try {
            assemble(func, functions.at(name).second, function_names);
            func.calculateBranches();
            func.calculateCalls(function_names, functions);
        } catch (const string& e) {
            cout << e << endl;
            exit(1);
        }
        byte* btcd = func.bytecode();
        out.write((const char*)btcd, func.size());
        functions_section_size += func.size();
        delete[] btcd;
    }
    out.write((const char*)bytecode, bytes);
    out.close();

    delete[] bytecode;

    return ret_code;
}
