#ifndef VIUA_OPERAND_H
#define VIUA_OPERAND_H

#pragma once


#include <memory>
#include <string>
#include <tuple>
#include <viua/bytecode/bytetypedef.h>


// forward declaration since we need a pointer to CPU
class Thread;
// and a pointer to Type
class Type;


namespace viua {
    namespace operand {
        class Operand {
            public:
                virtual Type* resolve(Thread*) = 0;
                virtual ~Operand() {}
        };

        class Atom: public Operand {
                std::string atom;
            public:
                inline std::string get() const { return atom; }

                Type* resolve(Thread*) override;

                Atom(const std::string& a): atom(a) {}
        };

        class RegisterIndex: public Operand {
                unsigned index;
            protected:
                RegisterIndex(): index(0) {}
            public:
                virtual unsigned get(Thread*) const;
                Type* resolve(Thread*) override;

                RegisterIndex(unsigned i): index(i) {}
        };

        class RegisterReference: public RegisterIndex {
                unsigned index;
            public:
                unsigned get(Thread*) const override;
                Type* resolve(Thread*) override;

                RegisterReference(unsigned i): index(i) {}
        };

        class Primitive: public Operand {
            public:
                Type* resolve(Thread*) = 0;
        };

        class Int: public Primitive {
                int integer;
            public:
                Type* resolve(Thread*) override;

                Int(int i): integer(i) {}
        };

        std::unique_ptr<viua::operand::Operand> extract(byte*& ip);
        unsigned getRegisterIndexOrException(Operand*, Thread*);
    }
}


#endif
