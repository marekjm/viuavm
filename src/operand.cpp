#include <viua/bytecode/operand_types.h>
#include <viua/assert.h>
#include <viua/operand.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/thread.h>
#include <viua/exceptions.h>
using namespace std;


Type* viua::operand::Atom::resolve(Thread* cpu) {
    throw new UnresolvedAtomException(atom);
    // just to satisfy the compiler, after the exception is not thrown unconditionally
    // return real object
    return nullptr;
}


Type* viua::operand::RegisterIndex::resolve(Thread* t) {
    return t->obtain(index);
}
unsigned viua::operand::RegisterIndex::get(Thread* cpu) const {
    return index;
}


Type* viua::operand::RegisterReference::resolve(Thread* t) {
    return t->obtain(static_cast<Integer*>(t->obtain(index))->as_integer());
}
unsigned viua::operand::RegisterReference::get(Thread* cpu) const {
    Type* o = cpu->obtain(index);
    viua::assertions::assert_typeof(o, "Integer");
    return static_cast<Integer*>(o)->value();
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

unsigned viua::operand::getRegisterIndexOrException(viua::operand::Operand* o, Thread* t) {
    unsigned index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(o)) {
        index = ri->get(t);
    } else {
        throw new Exception("invalid operand type");
    }
    return index;
}
