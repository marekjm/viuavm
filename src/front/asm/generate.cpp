#include <cstdint>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <viua/bytecode/maps.h>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/loader.h>
#include <viua/program.h>
#include <viua/cg/tokenizer.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/front/asm.h>
using namespace std;


extern bool VERBOSE;
extern bool DEBUG;
extern bool SCREAM;


// ASSEMBLY CONSTANTS
const string ENTRY_FUNCTION_NAME = "__entry";


template<class T> void bwrite(ofstream& out, const T& object) {
    out.write(reinterpret_cast<const char*>(&object), sizeof(T));
}


tuple<int, enum JUMPTYPE> resolvejump(string jmp, const map<string, int>& marks, int instruction_index = -1) {
    /*  This function is used to resolve jumps in `jump` and `branch` instructions.
     */
    int addr = 0;
    enum JUMPTYPE jump_type = JMP_RELATIVE;
    if (str::isnum(jmp, false)) {
        addr = stoi(jmp);
    } else if (jmp[0] == '.' and str::isnum(str::sub(jmp, 1LU))) {
        addr = stoi(str::sub(jmp, 1));
        jump_type = JMP_ABSOLUTE;
    } else if (jmp.substr(0, 2) == "0x") {
        stringstream ss;
        ss << hex << jmp;
        ss >> addr;
        jump_type = JMP_TO_BYTE;
    } else if (jmp[0] == '-') {
        if (instruction_index < 0) { throw ("invalid use of relative jump: " + jmp); }
        addr = (instruction_index + stoi(jmp));
        if (addr < 0) { throw "use of relative jump results in a jump to negative index"; }
    } else if (jmp[0] == '+') {
        if (instruction_index < 0) { throw ("invalid use of relative jump: " + jmp); }
        addr = (instruction_index + stoi(jmp.substr(1)));
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
    return tuple<int, enum JUMPTYPE>(addr, jump_type);
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
    { "iadd", &Program::opiadd },
    { "isub", &Program::opisub },
    { "imul", &Program::opimul },
    { "idiv", &Program::opidiv },
    { "ilt",  &Program::opilt },
    { "ilte", &Program::opilte },
    { "igt",  &Program::opigt },
    { "igte", &Program::opigte },
    { "ieq",  &Program::opieq },

    { "fadd", &Program::opfadd },
    { "fsub", &Program::opfsub },
    { "fmul", &Program::opfmul },
    { "fdiv", &Program::opfdiv },
    { "flt",  &Program::opflt },
    { "flte", &Program::opflte },
    { "fgt",  &Program::opfgt },
    { "fgte", &Program::opfgte },
    { "feq",  &Program::opfeq },

    { "and",  &Program::opand },
    { "or",   &Program::opor },

    { "enclose", &Program::openclose },
    { "enclosecopy", &Program::openclosecopy },
    { "enclosemove", &Program::openclosemove },

    { "insert", &Program::opinsert },
    { "remove", &Program::opremove },
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
    /** Return lines for current function.
     *
     *  Filters out all non-local (i.e. outside the scope of current function) and non-opcode lines.
     */
    vector<string> filtered;

    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = lines[i];
        if (str::startswith(line, ".mark:") or str::startswith(line, ".name:") or str::startswith(line, ".main:") or str::startswith(line, ".link:") or str::startswith(line, ".signature:") or str::startswith(line, ".bsignature:") or str::startswith(line, ".type:")) {
            /*  Lines beginning with `.mark:` are just markers placed in code and
             *  are do not produce any bytecode.
             *  Lines beginning with `.name:` are asm directives that assign human-rememberable names to
             *  registers.
             *  Lines beginning with `.signature:` and `.bsignature:` covey information about functions and
             *  blocks that will be available at runtime but may not be available during compilation.
             *  Lines beginning with `.type:` inform the assembler that definition of this type will be
             *  supplied later (either by `.class:` block, or by dynamically defining the type during runtime).
             *
             *  Assembler directives are discarded by the assembler during the bytecode-generation phase
             *  so they can be skipped in this step as fast as possible
             *  to avoid complicating code that appears later and
             *  deals with assembling CPU instructions.
             */
            continue;
        }

        if (str::startswith(line, ".function:") or str::startswith(line, ".block:")) {
            // skip function and block definition blocks
            while (lines[++i] != ".end");
            continue;
        }
        if (str::startswith(line, ".class:")) {
            // skip class definition lines
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

        instr = str::chunk(line);
        operands = str::lstrip(str::sub(line, instr.size()));

        vector<string> tokens = tokenize(operands);

        if (DEBUG and SCREAM) {
            cout << "[asm] compiling line: `" << line << "`" << endl;
        }

        if (line == "nop") {
            program.opnop();
        } else if (str::startswith(line, "izero")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.opizero(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "istore")) {
            string regno_chnk, number_chnk;
            tie(regno_chnk, number_chnk) = assembler::operands::get2(operands);
            program.opistore(assembler::operands::getint(resolveregister(regno_chnk, names)), assembler::operands::getint(resolveregister(number_chnk, names)));
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
            program.opiinc(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "idec")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.opidec(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "fstore")) {
            string regno_chnk, float_chnk;
            tie(regno_chnk, float_chnk) = assembler::operands::get2(operands);
            program.opfstore(assembler::operands::getint(resolveregister(regno_chnk, names)), static_cast<float>(stod(float_chnk)));
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
            program.opbstore(assembler::operands::getint(resolveregister(regno_chnk, names)), assembler::operands::getbyte(resolveregister(byte_chnk, names)));
        } else if (str::startswith(line, "itof")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            if (b_chnk.size() == 0) { b_chnk = a_chnk; }
            program.opitof(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "ftoi")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            if (b_chnk.size() == 0) { b_chnk = a_chnk; }
            program.opftoi(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "stoi")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            if (b_chnk.size() == 0) { b_chnk = a_chnk; }
            program.opstoi(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "stof")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            if (b_chnk.size() == 0) { b_chnk = a_chnk; }
            program.opstof(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "strstore")) {
            string reg_chnk, str_chnk;
            reg_chnk = str::chunk(operands);
            operands = str::lstrip(str::sub(operands, reg_chnk.size()));
            str_chnk = str::extract(operands);
            program.opstrstore(assembler::operands::getint(resolveregister(reg_chnk, names)), str_chnk);
        } else if (str::startswith(line, "vec")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.opvec(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "vinsert")) {
            string vec, src, pos;
            tie(vec, src, pos) = assembler::operands::get3(operands, false);
            if (pos == "") { pos = "0"; }
            program.opvinsert(assembler::operands::getint(resolveregister(vec, names)), assembler::operands::getint(resolveregister(src, names)), assembler::operands::getint(resolveregister(pos, names)));
        } else if (str::startswith(line, "vpush")) {
            string regno_chnk, number_chnk;
            tie(regno_chnk, number_chnk) = assembler::operands::get2(operands);
            program.opvpush(assembler::operands::getint(resolveregister(regno_chnk, names)), assembler::operands::getint(resolveregister(number_chnk, names)));
        } else if (str::startswith(line, "vpop")) {
            string vec, dst, pos;
            tie(vec, dst, pos) = assembler::operands::get3(operands, false);
            if (dst == "") { dst = "0"; }
            if (pos == "") { pos = "-1"; }
            program.opvpop(assembler::operands::getint(resolveregister(vec, names)), assembler::operands::getint(resolveregister(dst, names)), assembler::operands::getint(resolveregister(pos, names)));
        } else if (str::startswith(line, "vat")) {
            string vec, dst, pos;
            tie(vec, dst, pos) = assembler::operands::get3(operands, false);
            if (pos == "") { pos = "-1"; }
            program.opvat(assembler::operands::getint(resolveregister(vec, names)), assembler::operands::getint(resolveregister(dst, names)), assembler::operands::getint(resolveregister(pos, names)));
        } else if (str::startswith(line, "vlen")) {
            string regno_chnk, number_chnk;
            tie(regno_chnk, number_chnk) = assembler::operands::get2(operands);
            program.opvlen(assembler::operands::getint(resolveregister(regno_chnk, names)), assembler::operands::getint(resolveregister(number_chnk, names)));
        } else if (str::startswith(line, "not")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.opnot(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "and")) {
            assemble_three_intop_instruction(program, names, "and", operands);
        } else if (str::startswith(line, "or")) {
            assemble_three_intop_instruction(program, names, "or", operands);
        } else if (str::startswith(line, "move")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.opmove(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "copy")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.opcopy(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "ptr")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.opptr(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "swap")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.opswap(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "delete")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.opdelete(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "empty")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.opempty(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "isnull")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.opisnull(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "ress")) {
            program.opress(operands);
        } else if (str::startswith(line, "tmpri")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.optmpri(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "tmpro")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.optmpro(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "print")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.opprint(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "echo")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.opecho(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswithchunk(line, "enclose")) {
            assemble_three_intop_instruction(program, names, "enclose", operands);
        } else if (str::startswithchunk(line, "enclosecopy")) {
            assemble_three_intop_instruction(program, names, "enclosecopy", operands);
        } else if (str::startswithchunk(line, "enclosemove")) {
            assemble_three_intop_instruction(program, names, "enclosemove", operands);
        } else if (str::startswith(line, "closure")) {
            string fn_name, reg;
            tie(reg, fn_name) = assembler::operands::get2(operands);
            program.opclosure(assembler::operands::getint(resolveregister(reg, names)), fn_name);
        } else if (str::startswith(line, "function")) {
            string fn_name, reg;
            tie(reg, fn_name) = assembler::operands::get2(operands);
            program.opfunction(assembler::operands::getint(resolveregister(reg, names)), fn_name);
        } else if (str::startswith(line, "fcall")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.opfcall(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "frame")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            if (a_chnk.size() == 0) { a_chnk = "0"; }
            if (b_chnk.size() == 0) { b_chnk = "16"; }  // default number of local registers
            program.opframe(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "param")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.opparam(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "pamv")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.oppamv(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswithchunk(line, "arg")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.oparg(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "argc")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.opargc(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "call")) {
            /** Full form of call instruction has two operands: function name and return value register index.
             *  If call is given only one operand - it means it is the instruction index and returned value is discarded.
             *  To explicitly state that return value should be discarderd 0 can be supplied as second operand.
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
            tie(reg, fn_name) = assembler::operands::get2(operands);

            // if second operand is empty, fill it with zero
            // which means that return value will be discarded
            if (fn_name == "") {
                fn_name = reg;
                reg = "0";
            }

            program.opcall(assembler::operands::getint(resolveregister(reg, names)), fn_name);
        } else if (str::startswith(line, "tailcall")) {
            string fn_name = str::chunk(operands);
            program.optailcall(fn_name);
        } else if (str::startswith(line, "process")) {
            string fn_name, reg;
            tie(reg, fn_name) = assembler::operands::get2(operands);
            program.opprocess(assembler::operands::getint(resolveregister(reg, names)), fn_name);
        } else if (str::startswith(line, "join")) {
            string a_chnk, b_chnk;
            tie(a_chnk, b_chnk) = assembler::operands::get2(operands);
            program.opjoin(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)));
        } else if (str::startswith(line, "receive")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.opreceive(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "watchdog")) {
            string fn_name = str::chunk(operands);
            program.opwatchdog(fn_name);
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
            enum JUMPTYPE addrt_jump_type, addrf_jump_type;
            tie(addrt_target, addrt_jump_type) = resolvejump(if_true, marks, i);
            if (if_false != "") {
                tie(addrf_target, addrf_jump_type) = resolvejump(if_false, marks, i);
            } else {
                addrf_jump_type = JMP_RELATIVE;
                addrf_target = instruction+1;
            }

            if (DEBUG) {
                if (addrt_jump_type == JMP_TO_BYTE) {
                    cout << line << " => truth jump to byte";
                } else if (addrt_jump_type == JMP_ABSOLUTE) {
                    cout << line << " => truth absolute jump";
                } else {
                    cout << line << " => truth relative jump";
                }
                cout << ": " << addrt_target << endl;

                if (addrf_jump_type == JMP_TO_BYTE) {
                    cout << line << " => false jump to byte";
                } else if (addrf_jump_type == JMP_ABSOLUTE) {
                    cout << line << " => false absolute jump";
                } else {
                    cout << line << " => false relative jump";
                }
                cout << ": " << addrf_target << endl;
            }

            program.opbranch(assembler::operands::getint(resolveregister(condition, names)), addrt_target, addrt_jump_type, addrf_target, addrf_jump_type);
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
            enum JUMPTYPE jump_type;
            tie(jump_target, jump_type) = resolvejump(operands, marks, i);

            if (DEBUG) {
                if (jump_type == JMP_TO_BYTE) {
                    cout << line << " => false jump to byte";
                } else if (jump_type == JMP_ABSOLUTE) {
                    cout << line << " => false absolute jump";
                } else {
                    cout << line << " => false relative jump";
                }
                cout << ": " << jump_target << endl;
            }

            program.opjump(jump_target, jump_type);
        } else if (str::startswith(line, "try")) {
            program.optry();
        } else if (str::startswith(line, "catch")) {
            string type_chnk, catcher_chnk;
            type_chnk = str::extract(operands);
            operands = str::lstrip(str::sub(operands, type_chnk.size()));
            catcher_chnk = str::chunk(operands);
            program.opcatch(type_chnk, catcher_chnk);
        } else if (str::startswith(line, "pull")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.oppull(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "enter")) {
            string block_name = str::chunk(operands);
            program.openter(block_name);
        } else if (str::startswith(line, "throw")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.opthrow(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "leave")) {
            program.opleave();
        } else if (str::startswith(line, "import")) {
            string str_chnk;
            str_chnk = str::extract(operands);
            program.opimport(str_chnk);
        } else if (str::startswith(line, "link")) {
            string str_chnk;
            str_chnk = str::chunk(operands);
            program.oplink(str_chnk);
        } else if (str::startswith(line, "class")) {
            string class_name, reg;
            tie(reg, class_name) = assembler::operands::get2(operands);
            program.opclass(assembler::operands::getint(resolveregister(reg, names)), class_name);
        } else if (str::startswith(line, "derive")) {
            string base_class_name, reg;
            tie(reg, base_class_name) = assembler::operands::get2(operands);
            program.opderive(assembler::operands::getint(resolveregister(reg, names)), base_class_name);
        } else if (str::startswith(line, "attach")) {
            string function_name, method_name, reg;
            tie(reg, function_name, method_name) = assembler::operands::get3(operands);
            program.opattach(assembler::operands::getint(resolveregister(reg, names)), function_name, method_name);
        } else if (str::startswith(line, "register")) {
            string regno_chnk;
            regno_chnk = str::chunk(operands);
            program.opregister(assembler::operands::getint(resolveregister(regno_chnk, names)));
        } else if (str::startswith(line, "new")) {
            string class_name, reg;
            tie(reg, class_name) = assembler::operands::get2(operands);
            program.opnew(assembler::operands::getint(resolveregister(reg, names)), class_name);
        } else if (str::startswith(line, "msg")) {
            string reg, mtd;
            tie(reg, mtd) = assembler::operands::get2(operands);
            program.opmsg(assembler::operands::getint(resolveregister(reg, names)), mtd);
        } else if (str::startswithchunk(line, "insert")) {
            assemble_three_intop_instruction(program, names, "insert", operands);
        } else if (str::startswithchunk(line, "remove")) {
            assemble_three_intop_instruction(program, names, "remove", operands);
        } else if (str::startswith(line, "return")) {
            program.opreturn();
        } else if (str::startswith(line, "halt")) {
            program.ophalt();
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


map<string, uint64_t> mapInvocableAddresses(uint64_t& starting_instruction, const vector<string>& names, const map<string, vector<string> >& sources) {
    map<string, uint64_t> addresses;
    for (string name : names) {
        addresses[name] = starting_instruction;
        try {
            starting_instruction += Program::countBytes(sources.at(name));
        } catch (const std::out_of_range& e) {
            throw ("could not find block '" + name + "'");
        }
    }
    return addresses;
}

vector<string> expandSource(const vector<string>& lines, map<long unsigned, long unsigned>& expanded_lines_to_source_lines) {
    vector<string> stripped_lines;

    for (unsigned i = 0; i < lines.size(); ++i) {
        stripped_lines.push_back(str::lstrip(lines[i]));
    }

    vector<string> asm_lines;
    for (unsigned i = 0; i < stripped_lines.size(); ++i) {
        if (stripped_lines[i] == "") {
            expanded_lines_to_source_lines[asm_lines.size()] = i;
            asm_lines.push_back(lines[i]);
        } else if (str::startswith(stripped_lines[i], ".signature")) {
            expanded_lines_to_source_lines[asm_lines.size()] = i;
            asm_lines.push_back(lines[i]);
        } else if (str::startswith(stripped_lines[i], ".bsignature")) {
            expanded_lines_to_source_lines[asm_lines.size()] = i;
            asm_lines.push_back(lines[i]);
        } else if (str::startswith(stripped_lines[i], ".function")) {
            expanded_lines_to_source_lines[asm_lines.size()] = i;
            asm_lines.push_back(lines[i]);
        } else if (str::startswith(stripped_lines[i], ".end")) {
            expanded_lines_to_source_lines[asm_lines.size()] = i;
            asm_lines.push_back(lines[i]);
        } else if (stripped_lines[i][0] == ';') {
            expanded_lines_to_source_lines[asm_lines.size()] = i;
            asm_lines.push_back(lines[i]);
        } else if (not str::contains(stripped_lines[i], '(')) {
            expanded_lines_to_source_lines[asm_lines.size()] = i;
            asm_lines.push_back(lines[i]);
        } else {
            vector<vector<string>> decoded_lines = decode_line(stripped_lines[i]);
            auto indent = (lines[i].size() - stripped_lines[i].size());
            for (decltype(decoded_lines)::size_type j = 0; j < decoded_lines.size(); ++j) {
                expanded_lines_to_source_lines[asm_lines.size()] = i;
                asm_lines.push_back(str::strmul<char>(' ', indent) + str::join<char>(decoded_lines[j], ' '));
            }
        }
    }

    return asm_lines;
}

uint64_t writeCodeBlocksSection(ofstream& out, const invocables_t& blocks, const vector<string>& linked_block_names, uint64_t block_bodies_size_so_far = 0) {
    uint64_t block_ids_section_size = 0;
    for (string name : blocks.names) { block_ids_section_size += name.size(); }
    // we need to insert address after every block
    block_ids_section_size += sizeof(uint64_t) * blocks.names.size();
    // for null characters after block names
    block_ids_section_size += blocks.names.size();

    /////////////////////////////////////////////
    // WRITE OUT BLOCK IDS SECTION
    // THIS ALSO INCLUDES IDS OF LINKED blocks.bodies
    bwrite(out, block_ids_section_size);
    for (string name : blocks.names) {
        if (DEBUG) {
            cout << "[asm:write] writing block '" << name << "' to block address table";
        }
        if (find(linked_block_names.begin(), linked_block_names.end(), name) != linked_block_names.end()) {
            if (DEBUG) {
                cout << ": delayed" << endl;
            }
            continue;
        }
        if (DEBUG) {
            cout << endl;
        }

        // block name...
        out.write(name.c_str(), name.size());
        // ...requires terminating null character
        out.put('\0');
        // mapped address must come after name
        // FIXME: use uncasted uint64_t
        bwrite(out, block_bodies_size_so_far);
        // blocks.bodies size must be incremented by the actual size of block's bytecode size
        // to give correct offset for next block
        try {
            block_bodies_size_so_far += Program::countBytes(blocks.bodies.at(name));
        } catch (const std::out_of_range& e) {
            cout << "fatal: could not find block '" << name << "' during address table write" << endl;
            exit(1);
        }
    }

    return block_bodies_size_so_far;
}

int generate(const vector<string>& expanded_lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines, vector<string>& ilines, invocables_t& functions, invocables_t& blocks, const string& filename, string& compilename, const vector<string>& commandline_given_links, const compilationflags_t& flags) {
    //////////////////////////////
    // SETUP INITIAL BYTECODE SIZE
    uint64_t bytes = 0;


    /////////////////////////
    // GET MAIN FUNCTION NAME
    string main_function = "";
    for (string line : ilines) {
        if (str::startswith(line, ".main:")) {
            if (DEBUG) {
                cout << "setting main function to: ";
            }
            main_function = str::lstrip(str::sub(line, 6));
            cout << main_function << endl;
            break;
        }
    }
    if (main_function == "" and not flags.as_lib) {
        main_function = "main";
    }
    if (((VERBOSE and main_function != "main" and main_function != "") or DEBUG) and not flags.as_lib) {
        cout << "debug (notice): main function set to: '" << main_function << "'" << endl;
    }


    /////////////////////////////////////////
    // CHECK IF MAIN FUNCTION RETURNS A VALUE
    // FIXME: this is just a crude check - it does not acctually checks if these instructions set 0 register
    // this must be better implemented or we will receive "function did not set return register" exceptions at runtime
    bool main_is_defined = (find(functions.names.begin(), functions.names.end(), main_function) != functions.names.end());
    if (not flags.as_lib and main_is_defined) {
        string main_second_but_last;
        try {
            main_second_but_last = *(functions.bodies.at(main_function).end()-2);
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
    if (not main_is_defined and (DEBUG or VERBOSE) and not flags.as_lib) {
        cout << "notice: main function (" << main_function << ") is not defined in " << filename << ", deferring main function check to post-link phase" << endl;
    }


    /////////////////////////////////
    // MAP FUNCTIONS TO ADDRESSES AND
    // MAP blocks.bodies TO ADDRESSES AND
    // SET STARTING INSTRUCTION
    uint64_t starting_instruction = 0;  // the bytecode offset to first executable instruction
    map<string, uint64_t> function_addresses;
    map<string, uint64_t> block_addresses;
    try {
        block_addresses = mapInvocableAddresses(starting_instruction, blocks.names, blocks.bodies);
        function_addresses = mapInvocableAddresses(starting_instruction, functions.names, functions.bodies);
        bytes = Program::countBytes(ilines);
    } catch (const string& e) {
        cout << "error: bytecode size calculation failed: " << e << endl;
        return 1;
    }


    //////////////////////////
    // GENERATE ENTRY FUNCTION
    if (not flags.as_lib) {
        if (DEBUG) {
            cout << "generating __entry function" << endl;
        }
        functions.names.push_back(ENTRY_FUNCTION_NAME);
        function_addresses[ENTRY_FUNCTION_NAME] = starting_instruction;
        // entry function sets global stuff (FIXME: not really)
        ilines.insert(ilines.begin(), "ress local");
        // append entry function instructions...
        ilines.push_back("frame 1");
        ilines.push_back("param 0 1");
        // this must not be hardcoded because we have '.main:' assembler instruction
        // we also save return value in 1 register since 0 means "drop return value"
        ilines.push_back("call 1 " + main_function);
        // then, register 1 is moved to register 0 so it counts as a return code
        ilines.push_back("move 0 1");
        ilines.push_back("halt");
        functions.bodies[ENTRY_FUNCTION_NAME] = ilines;
        // instructions were added so bytecode size must be inreased
        bytes += OP_SIZES.at("ress");
        bytes += OP_SIZES.at("frame");
        bytes += OP_SIZES.at("param");
        bytes += OP_SIZES.at("call");
        bytes += main_function.size()+1;
        bytes += OP_SIZES.at("move");
        bytes += OP_SIZES.at("halt");
    }


    /////////////////////////////////////////////////////////
    // GATHER LINKS, GET THEIR SIZES AND ADJUST BYTECODE SIZE
    vector<string> links = assembler::ce::getlinks(ilines);
    vector<tuple<string, uint64_t, byte*> > linked_libs_bytecode;
    vector<string> linked_function_names;
    vector<string> linked_block_names;
    map<string, vector<uint64_t> > linked_libs_jumptables;
    uint64_t current_link_offset = bytes;

    // map of symbol names to name of the module the symbol came from
    map<string, string> symbol_sources;

    for (string lnk : commandline_given_links) {
        if (find(links.begin(), links.end(), lnk) == links.end()) {
            links.push_back(lnk);
        } else {
            throw ("requested to link module '" + lnk + "' more than once");
        }
    }

    for (string lnk : links) {
        if (DEBUG or VERBOSE) {
            cout << "[loader] message: linking with: '" << lnk << "\'" << endl;
        }

        Loader loader(lnk);
        loader.load();

        vector<string> fn_names = loader.getFunctions();
        for (string fn : fn_names) {
            if (function_addresses.count(fn)) {
                throw ("duplicate symbol '" + fn + "' found when linking '" + lnk + "' (previously found in '" + symbol_sources.at(fn) + "')");
            }
        }

        vector<uint64_t> lib_jumps = loader.getJumps();
        if (DEBUG) {
            cout << "[loader] entries in jump table: " << lib_jumps.size() << endl;
            for (unsigned i = 0; i < lib_jumps.size(); ++i) {
                cout << "  jump at byte: " << lib_jumps[i] << endl;
            }
        }

        linked_libs_jumptables[lnk] = lib_jumps;

        map<string, uint64_t> fn_addresses = loader.getFunctionAddresses();
        for (string fn : fn_names) {
            function_addresses[fn] = fn_addresses.at(fn) + current_link_offset;
            symbol_sources[fn] = lnk;
            linked_function_names.push_back(fn);
            if (DEBUG) {
                cout << "  \"" << fn << "\": entry point at byte: " << current_link_offset << '+' << fn_addresses.at(fn) << endl;
            }
        }

        linked_libs_bytecode.push_back( tuple<string, uint64_t, byte*>(lnk, loader.getBytecodeSize(), loader.getBytecode()) );
        bytes += loader.getBytecodeSize();
    }


    //////////////////////////////////////////////////////////////
    // EXTEND FUNCTION NAMES VECTOR WITH NAMES OF LINKED FUNCTIONS
    for (string name : linked_function_names) { functions.names.push_back(name); }


    /////////////////////////////////////////////////////////////////////////
    // AFTER HAVING OBTAINED LINKED NAMES, IT IS POSSIBLE TO VERIFY CALLS AND
    // CALLABLE (FUNCTIONS, CLOSURES, ETC.) CREATIONS
    string report;
    if ((report = assembler::verify::functionCallsAreDefined(filename, expanded_lines, expanded_lines_to_source_lines, functions.names, functions.signatures)).size()) {
        cout << report << endl;
        exit(1);
    }
    if ((report = assembler::verify::callableCreations(expanded_lines, expanded_lines_to_source_lines, functions.names, functions.signatures)).size()) {
        cout << report << endl;
        exit(1);
    }


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
    if ((VERBOSE or DEBUG) and not flags.as_lib) {
        cout << "message: first instruction pointer: " << starting_instruction << endl;
    }


    ////////////////////////////////////////
    // CREATE OFSTREAM TO WRITE BYTECODE OUT
    ofstream out(compilename, ios::out | ios::binary);


    ////////////////////
    // CREATE JUMP TABLE
    vector<uint64_t> jump_table;


    /////////////////////////////////////////////////////////
    // GENERATE BYTECODE OF LOCAL FUNCTIONS AND blocks.bodies
    //
    // BYTECODE IS GENERATED HERE BUT NOT YET WRITTEN TO FILE
    // THIS MUST BE GENERATED HERE TO OBTAIN FILL JUMP TABLE
    map<string, tuple<uint64_t, byte*> > functions_bytecode;
    map<string, tuple<uint64_t, byte*> > block_bodies_bytecode;
    uint64_t functions_section_size = 0;
    uint64_t block_bodies_section_size = 0;

    vector<tuple<uint64_t, uint64_t> > jump_positions;

    for (string name : blocks.names) {
        // do not generate bytecode for blocks.bodies that were linked
        if (find(linked_block_names.begin(), linked_block_names.end(), name) != linked_block_names.end()) { continue; }

        if (VERBOSE or DEBUG) {
            cout << "[asm] message: generating bytecode for block \"" << name << '"';
        }
        uint64_t fun_bytes = 0;
        try {
            fun_bytes = Program::countBytes(blocks.bodies.at(name));
            if (VERBOSE or DEBUG) {
                cout << " (" << fun_bytes << " bytes at byte " << block_bodies_section_size << ')' << endl;
            }
        } catch (const string& e) {
            cout << "fatal: error during block size count (pre-assembling): " << e << endl;
            exit(1);
        } catch (const std::out_of_range& e) {
            cout << e.what() << endl;
            exit(1);
        }

        Program func(fun_bytes);
        func.setdebug(DEBUG).setscream(SCREAM);
        try {
            assemble(func, blocks.bodies.at(name));
        } catch (const string& e) {
            cout << (DEBUG ? "\n" : "") << "fatal: error during assembling: " << e << endl;
            exit(1);
        } catch (const char*& e) {
            cout << (DEBUG ? "\n" : "") << "fatal: error during assembling: " << e << endl;
            exit(1);
        } catch (const std::out_of_range& e) {
            cout << (DEBUG ? "\n" : "") << "[asm] fatal: could not assemble block '" << name << "' (" << e.what() << ')' << endl;
            exit(1);
        }

        vector<uint64_t> jumps = func.jumps();
        vector<uint64_t> jumps_absolute = func.jumpsAbsolute();

        vector<tuple<uint64_t, uint64_t> > local_jumps;
        for (unsigned i = 0; i < jumps.size(); ++i) {
            uint64_t jmp = jumps[i];
            local_jumps.push_back(tuple<int, int>(jmp, block_bodies_section_size));
        }
        func.calculateJumps(local_jumps);

        byte* btcode = func.bytecode();

        // store generated bytecode fragment for future use (we must not yet write it to the file to conform to bytecode format)
        block_bodies_bytecode[name] = tuple<uint64_t, byte*>(func.size(), btcode);

        // extend jump table with jumps from current block
        for (unsigned i = 0; i < jumps.size(); ++i) {
            uint64_t jmp = jumps[i];
            if (DEBUG) {
                cout << "[asm] debug: pushed relative jump to jump table: " << jmp << '+' << block_bodies_section_size << endl;
            }
            jump_table.push_back(jmp+block_bodies_section_size);
        }

        for (unsigned i = 0; i < jumps_absolute.size(); ++i) {
            if (DEBUG) {
                cout << "[asm] debug: pushed absolute jump to jump table: " << jumps_absolute[i] << "+0" << endl;
            }
            jump_positions.push_back(tuple<int, int>(jumps_absolute[i]+block_bodies_section_size, 0));
        }

        block_bodies_section_size += func.size();
    }

    // functions section size, must be offset by the size of block section
    functions_section_size = block_bodies_section_size;

    for (string name : functions.names) {
        // do not generate bytecode for functions that were linked
        if (find(linked_function_names.begin(), linked_function_names.end(), name) != linked_function_names.end()) { continue; }

        if (VERBOSE or DEBUG) {
            cout << "[asm] message: generating bytecode for function \"" << name << '"';
        }
        uint64_t fun_bytes = 0;
        try {
            fun_bytes = Program::countBytes(name == ENTRY_FUNCTION_NAME ? filter(functions.bodies.at(name)) : functions.bodies.at(name));
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
        func.setdebug(DEBUG).setscream(SCREAM);
        try {
            assemble(func, functions.bodies.at(name));
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

        vector<uint64_t> jumps = func.jumps();
        vector<uint64_t> jumps_absolute = func.jumpsAbsolute();

        vector<tuple<uint64_t, uint64_t> > local_jumps;
        for (unsigned i = 0; i < jumps.size(); ++i) {
            uint64_t jmp = jumps[i];
            local_jumps.push_back(tuple<int, int>(jmp, functions_section_size));
        }
        func.calculateJumps(local_jumps);

        byte* btcode = func.bytecode();

        // store generated bytecode fragment for future use (we must not yet write it to the file to conform to bytecode format)
        functions_bytecode[name] = tuple<uint64_t, byte*>{func.size(), btcode};

        // extend jump table with jumps from current function
        for (unsigned i = 0; i < jumps.size(); ++i) {
            uint64_t jmp = jumps[i];
            if (DEBUG) {
                cout << "[asm] debug: pushed relative jump to jump table: " << jmp << '+' << functions_section_size << endl;
            }
            jump_table.push_back(jmp+functions_section_size);
        }

        for (unsigned i = 0; i < jumps_absolute.size(); ++i) {
            if (DEBUG) {
                cout << "[asm] debug: pushed absolute jump to jump table: " << jumps_absolute[i] << "+0" << endl;
            }
            jump_positions.push_back(tuple<int, int>(jumps_absolute[i]+functions_section_size, 0));
        }

        functions_section_size += func.size();
    }


    //////////////////////////
    // IF ASSEMBLING A LIBRARY
    // WRITE OUT JUMP TABLE
    if (flags.as_lib) {
        if (DEBUG) {
            cout << "debug: jump table has " << jump_table.size() << " entries" << endl;
        }
        uint64_t total_jumps = jump_table.size();
        bwrite(out, total_jumps);

        uint64_t jmp;
        for (unsigned i = 0; i < total_jumps; ++i) {
            jmp = jump_table[i];
            bwrite(out, jmp);
        }
    }


    ///////////////////////////////////////////////
    // CHECK IF THE FUNCTION SET AS MAIN IS DEFINED
    // AS ALL THE FUNCTIONS (LOCAL OR LINKED) ARE
    // NOW AVAILABLE
    if (find(functions.names.begin(), functions.names.end(), main_function) == functions.names.end() and not flags.as_lib) {
        cout << "[asm:pre] fatal: main function is undefined: " << main_function << endl;
        return 1;
    }


    /////////////////////////////////////////////////////////////
    // WRITE BLOCK AND FUNCTION ENTRY POINT ADDRESSES TO BYTECODE
    uint64_t functions_size_so_far = writeCodeBlocksSection(out, blocks, linked_block_names);
    functions_size_so_far = writeCodeBlocksSection(out, functions, linked_function_names, functions_size_so_far);
    for (string name : linked_function_names) {
        // function name...
        out.write(name.c_str(), name.size());
        // ...requires terminating null character
        out.put('\0');
        // mapped address must come after name
        uint64_t address = function_addresses[name];
        bwrite(out, address);
    }


    //////////////////////
    // WRITE BYTECODE SIZE
    bwrite(out, bytes);

    byte* program_bytecode = new byte[bytes];
    uint64_t program_bytecode_used = 0;

    ////////////////////////////////////////////////////
    // WRITE BYTECODE OF LOCAL BLOCKS TO BYTECODE BUFFER
    for (string name : blocks.names) {
        // linked blocks.bodies are to be inserted later
        if (find(linked_block_names.begin(), linked_block_names.end(), name) != linked_block_names.end()) { continue; }

        if (DEBUG) {
            cout << "[asm] pushing bytecode of local block '" << name << "' to final byte array" << endl;
        }
        uint64_t fun_size = 0;
        byte* fun_bytecode = nullptr;
        tie(fun_size, fun_bytecode) = block_bodies_bytecode[name];

        for (uint64_t i = 0; i < fun_size; ++i) {
            program_bytecode[program_bytecode_used+i] = fun_bytecode[i];
        }
        program_bytecode_used += fun_size;
    }


    ///////////////////////////////////////////////////////
    // WRITE BYTECODE OF LOCAL FUNCTIONS TO BYTECODE BUFFER
    for (string name : functions.names) {
        // linked functions are to be inserted later
        if (find(linked_function_names.begin(), linked_function_names.end(), name) != linked_function_names.end()) { continue; }

        if (DEBUG) {
            cout << "[asm] pushing bytecode of local function '" << name << "' to final byte array" << endl;
        }
        int fun_size = 0;
        byte* fun_bytecode = nullptr;
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
    calculator.setdebug(DEBUG).setscream(SCREAM);
    if (DEBUG) {
        cout << "[asm:post] calculating absolute jumps..." << endl;
    }
    calculator.fill(program_bytecode).calculateJumps(jump_positions);


    ////////////////////////////////////
    // WRITE STATICALLY LINKED LIBRARIES
    uint64_t bytes_offset = current_link_offset;
    for (tuple<string, uint64_t, byte*> lnk : linked_libs_bytecode) {
        string lib_name;
        byte* linked_bytecode;
        uint64_t linked_size;
        tie(lib_name, linked_size, linked_bytecode) = lnk;

        if (VERBOSE or DEBUG) {
            cout << "[linker] message: linked module \"" << lib_name <<  "\" written at offset " << bytes_offset << endl;
        }

        vector<uint64_t> linked_jumptable;
        try {
            linked_jumptable = linked_libs_jumptables[lib_name];
        } catch (const std::out_of_range& e) {
            cout << "[linker] fatal: could not find jumptable for '" << lib_name << "' (maybe not loaded?)" << endl;
            exit(1);
        }

        uint64_t jmp, jmp_target;
        for (unsigned i = 0; i < linked_jumptable.size(); ++i) {
            jmp = linked_jumptable[i];
            // we know what we're doing here
            jmp_target = *reinterpret_cast<uint64_t*>(linked_bytecode+jmp);
            if (DEBUG) {
                cout << "[linker] adjusting jump: at position " << jmp << ", " << jmp_target << '+' << bytes_offset << " -> " << (jmp_target+bytes_offset) << endl;
            }
            *reinterpret_cast<uint64_t*>(linked_bytecode+jmp) += bytes_offset;
        }

        for (decltype(linked_size) i = 0; i < linked_size; ++i) {
            program_bytecode[program_bytecode_used+i] = linked_bytecode[i];
        }
        program_bytecode_used += linked_size;
    }

    out.write(reinterpret_cast<const char*>(program_bytecode), bytes);
    out.close();

    return 0;
}
