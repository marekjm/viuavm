#ifndef VIUA_OPERAND_H
#define VIUA_OPERAND_H

#pragma once


#include <string>
#include <tuple>
#include <viua/bytecode/bytetypedef.h>


// forward declaration since we need a pointer to CPU
class CPU;
// and a pointer to Type
class Type;


namespace viua {
    namespace operand {
        class Operand {
            public:
                virtual Type* resolve(CPU*) = 0;
        };

        class Atom: public Operand {
                std::string atom;
            public:
                inline std::string get() const { return atom; }

                Type* resolve(CPU*) override;

                Atom(const std::string& a): atom(a) {}
        };

        class RegisterIndex: public Operand {
                unsigned index;
            public:
                inline unsigned get() const { return index; }
                Type* resolve(CPU*) override;
                RegisterIndex(unsigned i): index(i) {}
        };

        class RegisterReference: public Operand {
                unsigned index;
            public:
                inline unsigned get() const { return index; }
                Type* resolve(CPU*) override;
                RegisterReference(unsigned i): index(i) {}
        };

        std::tuple<viua::operand::Operand*, byte*> extract(byte* ip);
    }
}


#endif
