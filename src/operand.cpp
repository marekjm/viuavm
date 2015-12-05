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

unique_ptr<viua::operand::Operand> viua::operand::extract(byte*& ip) {
    /** Extract operand from given address.
     *
     *  After the operand is extracted, the address pointer is increased by the size
     *  of the operand.
     *  Address is modifier in-place.
     */
    OperandType ot = *reinterpret_cast<OperandType*>(ip);
    ++ip;

    unique_ptr<viua::operand::Operand> operand;
    switch (ot) {
        case OT_REGISTER_INDEX:
            operand.reset(new RegisterIndex(static_cast<unsigned>(*reinterpret_cast<int*>(ip))));
            ip += sizeof(int);
            break;
        case OT_REGISTER_REFERENCE:
            operand.reset(new RegisterReference(static_cast<unsigned>(*reinterpret_cast<int*>(ip))));
            ip += sizeof(int);
            break;
        case OT_ATOM:
            operand.reset(new Atom(string(ip)));
            ip += (static_cast<Atom*>(operand.get())->get().size() + 1);
            break;
        default:
            throw OperandTypeException();
    }

    return operand;
}
