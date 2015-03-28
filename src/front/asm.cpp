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
#include "../assembler/assembler.h"
using namespace std;


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;

// are we assembling a library?
bool AS_LIB = false;

bool VERBOSE = false;
bool DEBUG = false;
bool SCREAM = false;

bool WARNING_ALL = false;
bool ERROR_ALL = false;


// WARNINGS
bool WARNING_MISSING_END = false;
bool WARNING_EMPTY_FUNCTION_BODY = false;
bool WARNING_OPERANDLESS_FRAME = false;
bool WARNING_GLOBALS_IN_LIB = false;


// ERRORS
bool ERROR_MISSING_END = false;
bool ERROR_EMPTY_FUNCTION_BODY = false;
bool ERROR_OPERANDLESS_FRAME = false;
bool ERROR_GLOBALS_IN_LIB = false;


// ASSEMBLY CONSTANTS
const string ENTRY_FUNCTION_NAME = "__entry";


tuple<int, bool> resolvejump(string jmp, const map<string, int>& marks) {
    /*  This function is used to resolve jumps in `jump` and `branch` instructions.
     */
    int addr = 0;
    bool absolute = false;
    if (str::isnum(jmp)) {
        addr = stoi(jmp);
    } else if (jmp[0] == '.' and str::isnum(str::sub(jmp, 1))) {
        addr = stoi(str::sub(jmp, 1));
        absolute = true;
    } else if (jmp[0] == '.') {
        // FIXME
        cout << "FIXME: global marker jumps (jumps to functions) are not implemented yet" << endl;
        exit(1);
    } else {
        try {
            addr = marks.at(jmp);
        } catch (const std::out_of_range& e) {
            throw ("jump to unrecognised marker: " + jmp);
        }
    }
    return tuple<int, bool>(addr, absolute);
}

string resolveregister(string reg, const map<string, int>& names) {
    /*  This function is used to register numbers when a register is accessed, e.g.
     *  in `istore` instruction or in `branch` in condition operand.
     *
     *  This function MUST return string as teh result is further passed to assembler::operands::getint() function which *expects* string.
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
    tie(rega, regb, regr) = assembler::operands::get3(operands);
    rega = resolveregister(rega, names);
    regb = resolveregister(regb, names);
    regr = resolveregister(regr, names);

    // feed chunks into Bytecode Programming API
    try {
        (program.*THREE_INTOP_ASM_FUNCTIONS.at(instr))(assembler::operands::getint(rega), assembler::operands::getint(regb), assembler::operands::getint(regr));
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

Program& compile(Program& program, const vector<string>& lines, map<string, int>& marks, map<string, int>& names) {
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

        if (DEBUG and SCREAM) {
            cout << "[asm] compiling line: `" << line << "`" << endl;
        }

        if (line == "nop") {
            program.nop();
        } else if (str::startswith(line, "izero")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.izero(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "istore")) {
            string regno_chnk, number_chnk;
            tie(regno_chnk, number_chnk) = assembler::operands::get2(operands);
            program.istore(assembler::operands::getint(resolveregister(regno_chnk, names)), assembler::operands::getint(resolveregister(number_chnk, names)));
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
            program.iinc(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "idec")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.idec(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "fstore")) {
            string regno_chnk, float_chnk;
            tie(regno_chnk, float_chnk) = assembler::operands::get2(operands);
            program.fstore(assembler::operands::getint(resolveregister(regno_chnk, names)), stod(float_chnk));
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
            tie(regno_chnk, byte_chnk) = assembler::operands::get2(operands);
            program.bstore(assembler::operands::getint(resolveregister(regno_chnk, names)), assembler::operands::getbyte(resolveregister(byte_chnk, names)));
        } else if (str::startswith(line, "itof")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            if (b_chnk.size() == 0) { b_chnk = a_chnk; }
            program.itof(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "ftoi")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            if (b_chnk.size() == 0) { b_chnk = a_chnk; }
            program.ftoi(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "strstore")) {
            string reg_chnk, str_chnk;
            reg_chnk = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, reg_chnk.size()));
            str_chnk = str::extract(operands);
            program.strstore(assembler::operands::getint(resolveregister(reg_chnk, names)), str_chnk);
        } else if (str::startswith(line, "vec")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.vec(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "vinsert")) {
            string vec, src, pos;
            tie(vec, src, pos) = assembler::operands::get3(operands, false);
            if (pos == "") { pos = "0"; }
            program.vinsert(assembler::operands::getint(resolveregister(vec, names)), assembler::operands::getint(resolveregister(src, names)), assembler::operands::getint(resolveregister(pos, names)));
        } else if (str::startswith(line, "vpush")) {
            string regno_chnk, number_chnk;
            tie(regno_chnk, number_chnk) = assembler::operands::get2(operands);
            program.vpush(assembler::operands::getint(resolveregister(regno_chnk, names)), assembler::operands::getint(resolveregister(number_chnk, names)));
        } else if (str::startswith(line, "vpop")) {
            string vec, dst, pos;
            tie(vec, dst, pos) = assembler::operands::get3(operands, false);
            if (dst == "") { dst = "0"; }
            if (pos == "") { pos = "-1"; }
            program.vpop(assembler::operands::getint(resolveregister(vec, names)), assembler::operands::getint(resolveregister(dst, names)), assembler::operands::getint(resolveregister(pos, names)));
        } else if (str::startswith(line, "vat")) {
            string vec, dst, pos;
            tie(vec, dst, pos) = assembler::operands::get3(operands, false);
            if (pos == "") { pos = "-1"; }
            program.vat(assembler::operands::getint(resolveregister(vec, names)), assembler::operands::getint(resolveregister(dst, names)), assembler::operands::getint(resolveregister(pos, names)));
        } else if (str::startswith(line, "vlen")) {
            string regno_chnk, number_chnk;
            tie(regno_chnk, number_chnk) = assembler::operands::get2(operands);
            program.vlen(assembler::operands::getint(resolveregister(regno_chnk, names)), assembler::operands::getint(resolveregister(number_chnk, names)));
        } else if (str::startswith(line, "not")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.lognot(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "and")) {
            assemble_three_intop_instruction(program, names, "and", operands);
        } else if (str::startswith(line, "or")) {
            assemble_three_intop_instruction(program, names, "or", operands);
        } else if (str::startswith(line, "move")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.move(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "copy")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.copy(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "ref")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.ref(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "swap")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.swap(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "free")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.free(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "empty")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.empty(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "isnull")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.isnull(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "ress")) {
            program.ress(operands);
        } else if (str::startswith(line, "tmpri")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.tmpri(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "tmpro")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.tmpro(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "print")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.print(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "echo")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.echo(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "closure")) {
            string fn_name, reg;
            tie(fn_name, reg) = assembler::operands::get2(operands);
            program.closure(fn_name, assembler::operands::getint(resolveregister(reg, names)));
        } else if (str::startswith(line, "clframe")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.clframe(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "clcall")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.clcall(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "frame")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            if (a_chnk.size() == 0) { a_chnk = "0"; }
            if (b_chnk.size() == 0) { b_chnk = "16"; }  // default number of local registers
            program.frame(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "param")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.param(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "paref")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.paref(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "arg")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.arg(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "call")) {
            /** Full form of call instruction has two operands: function name and return value register index.
             *  If call is given only one operand - it means it is the instruction index and returned value is discarded.
             *  To explicitly state that return value should be discarder, 0 can be supplied as second operand.
             */
            /** Why is the function supplied as a *string* and not direct instruction pointer?
             *  That would be faster - c'mon couldn't assembler just calculate offsets and insert them?
             *
             *  Nope.
             *
             *  Yes, it *would* be faster if calls were just precalculated jumps.
             *  However, by them being strings we get plenty of flexibility, good-quality stack traces, and
             *  a place to put plenty of debugging info.
             *  All that at a cost of just one map lookup; the overhead is minimal and gains are big.
             *  What's not to love?
             *
             *  Of course, you, my dear reader, are free to take this code (it's GPL after all!) and
             *  modify it to suit your particular needs - in that case that would be calculating call jumps
             *  at compile time and exchanging CALL instructions with JUMP instructions.
             *
             *  Good luck with debugging your code, then.
             */
            string fn_name, reg;
            tie(fn_name, reg) = assembler::operands::get2(operands);

            // if second operand is empty, fill it with zero
            // which means that return value will be discarded
            if (reg == "") { reg = "0"; }

            program.call(fn_name, assembler::operands::getint(resolveregister(reg, names)));
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
            tie(condition, if_true, if_false) = assembler::operands::get3(operands, false);

            int addrt_target, addrf_target;
            bool addrt_is_absolute, addrf_is_absolute;
            tie(addrt_target, addrt_is_absolute) = resolvejump(if_true, marks);
            if (if_false != "") {
                tie(addrf_target, addrf_is_absolute) = resolvejump(if_false, marks);
            } else {
                addrf_is_absolute = false;
                addrf_target = instruction+1;
            }

            program.branch(assembler::operands::getint(resolveregister(condition, names)), addrt_target, addrt_is_absolute, addrf_target, addrf_is_absolute);
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
            int jump_target;
            bool is_absolute;
            tie(jump_target, is_absolute) = resolvejump(operands, marks);
            program.jump(jump_target, is_absolute);
        } else if (str::startswith(line, "end")) {
            program.end();
        } else if (str::startswith(line, "halt")) {
            program.halt();
        } else {
            throw ("unimplemented instruction: " + instr);
        }
        ++instruction;
    }

    return program;
}


void assemble(Program& program, const vector<string>& lines) {
    /** Assemble instructions in lines into a program.
     *  This function first garthers required information about markers, named registers and functions.
     *  Then, it passes all gathered data into compilation function.
     *
     *  :params:
     *
     *  program         - Program object which will be used for assembling
     *  lines           - lines with instructions
     */
    map<string, int> marks = assembler::ce::getmarks(lines);
    map<string, int> names = assembler::ce::getnames(lines);
    compile(program, lines, marks, names);
}


int main(int argc, char* argv[]) {
    // setup command line arguments vector
    vector<string> args;
    string option;

    string filename(""), compilename("");

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
        } else if (option == "--scream") {
            SCREAM = true;
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
        } else if (option == "--Wglobals-in-lib") {
            WARNING_GLOBALS_IN_LIB = true;
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
        } else if (option == "--Eglobals-in-lib") {
            ERROR_GLOBALS_IN_LIB = true;
            continue;
        } else if (option == "--out" or option == "-o") {
            if (i < argc-1) {
                compilename = string(argv[++i]);
            } else {
                cout << "error: option '" << argv[i] << "' requires an argument: filename" << endl;
                exit(1);
            }
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
                 << "    " << "--scream             - show so much debugging output it becomes noisy\n"
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
    filename = args[0];
    if (compilename == "") {
        if (AS_LIB) {
            compilename = (filename + ".wlib");
        } else {
            compilename = "a.out";
        }
    }

    if (VERBOSE or DEBUG) {
        cout << "message: assembling \"" << filename << "\" to \"" << compilename << "\"" << endl;
    }


    //////////////////////////////////////////
    // GATHER LINKS OBTAINED FROM COMMAND LINE
    vector<string> commandline_given_links;
    for (unsigned i = 1; i < args.size(); ++i) {
        commandline_given_links.push_back(args[i]);
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
    ilines = assembler::ce::getilines(lines);


    //////////////////////////////
    // SETUP INITIAL BYTECODE SIZE
    uint16_t bytes = 0;


    ////////////////////////
    // GATHER FUNCTION NAMES
    vector<string> function_names;
    try {
        function_names = assembler::ce::getFunctionNames(lines);
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
         functions = assembler::ce::getFunctions(ilines);
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
        if (DEBUG) {
            cout << "generating __entry function" << endl;
        }
        function_names.push_back(ENTRY_FUNCTION_NAME);
        function_addresses[ENTRY_FUNCTION_NAME] = starting_instruction;
        // entry function sets global stuff
        ilines.insert(ilines.begin(), "ress global");
        // append entry function instructions...
        ilines.push_back("frame 1");
        ilines.push_back("paref 0 1");
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
        bytes += OP_SIZES.at("paref");
        bytes += OP_SIZES.at("call");
        bytes += main_function.size()+1;
        bytes += OP_SIZES.at("move");
        bytes += OP_SIZES.at("halt");
    }


    /////////////////////////////////////////////////////////
    // GATHER LINKS, GET THEIR SIZES AND ADJUST BYTECODE SIZE
    vector<string> links = assembler::ce::getlinks(ilines);
    vector<tuple<string, uint16_t, char*> > linked_libs_bytecode;
    vector<string> linked_function_names;
    map<string, vector<unsigned> > linked_libs_jumptables;
    uint16_t current_link_offset = bytes;

    for (string lnk : commandline_given_links) {
        if (find(links.begin(), links.end(), lnk) == links.end()) {
            links.push_back(lnk);
        }
    }

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
                cout << "  jump at byte: " << lib_jmp << endl;// ", target: " << lib_jmp_target << endl;
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


    ////////////////////////
    // VERIFY FUNCTION CALLS
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not str::startswith(line, "call")) {
            continue;
        }

        string function = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
        bool is_undefined = (find(function_names.begin(), function_names.end(), function) == function_names.end());

        if (is_undefined) {
            cout << "fatal: call to undefined function '" << function << "' at line " << i << " in " << filename << endl;
            exit(1);
        }
    }

    ///////////////////////////
    // VERIFY CLOSURE CREATIONS
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not str::startswith(line, "closure")) {
            continue;
        }

        line = str::lstrip(str::sub(line, str::chunk(line).size()));
        string function = str::chunk(line);
        bool is_undefined = (find(function_names.begin(), function_names.end(), function) == function_names.end());

        if (is_undefined) {
            cout << "fatal: closure from undefined function '" << function << "' at line " << i << " in " << filename << endl;
            exit(1);
        }

        // second chunk of closure instruction, must be an integer
        line = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
        if (line.size() == 0) {
            cout << "fatal: second operand missing from closure instruction at line " << i << " in " << filename << endl;
            exit(1);
        } else if (not str::isnum(line)) {
            cout << "fatal: second operand is not an integer in closure instruction at line " << i << " in " << filename << endl;
            exit(1);
        }
    }

    ///////////////////////////
    // VERIFY RESS INSTRUCTIONS
    vector<string> legal_register_sets = {
        "global",   // global register set
        "local",    // local register set for function
        "static",   // static register set
        "temp",     // temporary register set
    };
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not str::startswith(line, "ress")) {
            continue;
        }

        string registerset_name = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));

        if (find(legal_register_sets.begin(), legal_register_sets.end(), registerset_name) == legal_register_sets.end()) {
            cout << "fatal: illegal register set name in ress instruction: '" << registerset_name << "' at line " << i << " in " << filename;
            exit(1);
        }
        if (registerset_name == "global" and AS_LIB) {
            if (ERROR_GLOBALS_IN_LIB or ERROR_ALL) {
                cout << "fatal: global registers used in library function at line " << i << " in " << filename;
                exit(1);
            } else if (WARNING_GLOBALS_IN_LIB or WARNING_ALL) {
                cout << "warning: global registers used in library function at line " << i << " in " << filename;
            }
        }
    }

    ////////////////////////////
    // VERIFY FRAME INSTRUCTIONS
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not str::startswith(line, "frame")) {
            continue;
        }

        line = str::lstrip(str::sub(line, str::chunk(line).size()));

        if (line.size() == 0) {
            if (ERROR_OPERANDLESS_FRAME or ERROR_ALL) {
                cout << "fatal: frame instruction without operands at line " << i << " in " << filename;
                exit(1);
            } else if (WARNING_OPERANDLESS_FRAME or WARNING_ALL) {
                cout << "warning: frame instruction without operands at line " << i << " in " << filename;
            }
        }
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

    vector<tuple<int, int> > jump_positions;

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
            cout << "fatal: error during function size count (pre-assembling): " << e << endl;
            exit(1);
        } catch (const std::out_of_range& e) {
            cout << e.what() << endl;
            exit(1);
        }

        Program func(fun_bytes);
        try {
            assemble(func.setdebug(DEBUG), functions.at(name).second);
        } catch (const string& e) {
            cout << (DEBUG ? "\n" : "") << "fatal: error during assembling: " << e << endl;
            exit(1);
        } catch (const char*& e) {
            cout << (DEBUG ? "\n" : "") << "fatal: error during assembling: " << e << endl;
            exit(1);
        } catch (const std::out_of_range& e) {
            cout << (DEBUG ? "\n" : "") << "[asm] fatal: could not assemble function '" << name << "' (" << e.what() << ')' << endl;
            exit(1);
        }

        byte* btcode = func.bytecode();

        // store generated bytecode fragment for future use (we must not yet write it to the file to conform to bytecode format)
        functions_bytecode[name] = tuple<int, byte*>(func.size(), btcode);

        vector<unsigned> jumps = func.jumps();
        // extend jump table with jumps from current function
        for (unsigned i = 0; i < jumps.size(); ++i) {
            unsigned jmp = jumps[i];
            if (DEBUG) {
                cout << "[asm] debug: pushed to jump table: " << jmp << '+' << functions_section_size << endl;
            }
            jump_table.push_back(jmp+functions_section_size);
            jump_positions.push_back(tuple<int, int>(jmp+functions_section_size, functions_section_size));
        }

        vector<unsigned> jumps_absolute = func.jumpsAbsolute();
        for (unsigned i = 0; i < jumps_absolute.size(); ++i) {
            if (DEBUG) {
                cout << "[asm] debug: pushed to jump table: " << jumps_absolute[i] << "+0 (absolute jump)" << endl;
            }
            jump_positions.push_back(tuple<int, int>(jumps_absolute[i]+functions_section_size, 0));
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

        unsigned jmp;
        for (unsigned i = 0; i < total_jumps; ++i) {
            jmp = jump_table[i];
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
        if (DEBUG) {
            cout << "[asm:write] writing function '" << name << "' to call address table";
        }
        if (find(linked_function_names.begin(), linked_function_names.end(), name) != linked_function_names.end()) {
            if (DEBUG) {
                cout << ": delayed" << endl;
            }
            continue;
        }
        if (DEBUG) {
            cout << endl;
        }

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
            cout << "fatal: could not find function '" << name << "' during call address table write" << endl;
            exit(1);
        }
    }
    // FIXME: iteration over linked functions to put them to the call address table
    //        should be done in the loop above (for local functions)
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

    byte* program_bytecode = new byte[bytes];
    int program_bytecode_used = 0;

    ///////////////////////////////////////////////////////
    // WRITE BYTECODE OF LOCAL FUNCTIONS TO BYTECODE BUFFER
    for (string name : function_names) {
        // linked functions are to be inserted later
        if (find(linked_function_names.begin(), linked_function_names.end(), name) != linked_function_names.end()) { continue; }

        if (DEBUG) {
            cout << "[asm] pushing bytecode of local function '" << name << "' to final byte array" << endl;
        }
        int fun_size = 0;
        byte* fun_bytecode = 0;
        tie(fun_size, fun_bytecode) = functions_bytecode[name];

        for (int i = 0; i < fun_size; ++i) {
            program_bytecode[program_bytecode_used+i] = fun_bytecode[i];
        }
        program_bytecode_used += fun_size;
    }

    // free memory allocated for bytecode of local functions
    for (pair<string, tuple<int, byte*>> fun : functions_bytecode) {
        delete[] get<1>(fun.second);
    }

    Program calculator(bytes);
    if (DEBUG) {
        cout << "[asm:post] calculating branches..." << endl;
    }
    calculator.fill(program_bytecode).calculateJumps(jump_positions);


    ////////////////////////////////////
    // WRITE STATICALLY LINKED LIBRARIES
    // FIXME: implement this after we are able to load static libs
    uint16_t bytes_offset = current_link_offset;
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

        unsigned jmp, jmp_target;
        for (unsigned i = 0; i < linked_jumptable.size(); ++i) {
            jmp = linked_jumptable[i];
            jmp_target = *((unsigned*)(linked_bytecode+jmp));
            if (DEBUG) {
                cout << "[linker] adjusting jump: at position " << jmp << ", " << jmp_target << '+' << bytes_offset << " -> " << (jmp_target+bytes_offset) << endl;
            }
            *((int*)(linked_bytecode+jmp)) += bytes_offset;
        }

        //out.write(linked_bytecode, linked_size);
        for (int i = 0; i < linked_size; ++i) {
            program_bytecode[program_bytecode_used+i] = linked_bytecode[i];
        }
        program_bytecode_used += linked_size;
    }

    out.write((const char*)program_bytecode, bytes);
    out.close();

    return ret_code;
}
