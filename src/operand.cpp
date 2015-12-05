#include <string>
#include <tuple>
#include <viua/bytecode/operand_types.h>
#include <viua/operand.h>
#include <viua/types/type.h>
#include <viua/cpu/cpu.h>
#include <viua/exceptions.h>
using namespace std;


Type* viua::operand::Atom::resolve(CPU* cpu) {
    throw new UnresolvedAtomException(atom);
    // just to satisfy the compiler, after the exception is not thrown unconditionally
    // return real object
    return nullptr;
}

Type* viua::operand::RegisterIndex::resolve(CPU* cpu) {
    throw new OutOfRangeException("resolving registers via Operand is not implemented");
    return nullptr;
}

Type* viua::operand::RegisterReference::resolve(CPU* cpu) {
    throw new OutOfRangeException("resolving registers via Operand is not implemented");
    return nullptr;
}

tuple<viua::operand::Operand*, byte*> viua::operand::extract(byte* ip) {
    OperandType ot = *reinterpret_cast<OperandType*>(ip);
    ++ip;

    viua::operand::Operand *operand = nullptr;
    switch (ot) {
        case OT_REGISTER_INDEX:
            operand = new RegisterIndex(static_cast<unsigned>(*reinterpret_cast<int*>(ip)));
            ip += sizeof(int);
            break;
        case OT_REGISTER_REFERENCE:
            operand = new RegisterReference(static_cast<unsigned>(*reinterpret_cast<int*>(ip)));
            ip += sizeof(int);
            break;
        case OT_ATOM:
            operand = new Atom(string(ip));
            ip += (static_cast<Atom*>(operand)->get().size() + 1);
            break;
        default:
            throw OperandTypeException();
    }

    return tuple<viua::operand::Operand*, byte*>(operand, ip);
}
