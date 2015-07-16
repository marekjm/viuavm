#ifndef VIUA_TYPES_PROTOTYPE_H
#define VIUA_TYPES_PROTOTYPE_H

#pragma once

#include <viua/types/type.h>


class Prototype: public Type {
    /** A prototype of a type.
     *
     *  This type is used internally inside the VM.
     */
    public:
        virtual std::string type() const {
            return "Prototype";
        }
        virtual std::string str() const {
            std::ostringstream s;
            s << "<'" << type() << "' object at " << this << ">";
            return s.str();
        }
        virtual std::string repr() const {
            return str();
        }
        virtual bool boolean() const {
            return true;
        }

        virtual std::vector<std::string> bases() const {
            return std::vector<std::string>{"Type"};
        }
        virtual std::vector<std::string> inheritancechain() const {
            return std::vector<std::string>{"Type"};
        }

        virtual Type* copy() const {
            return new Prototype();
        }

        // We need to construct and destroy our basic object.
        Prototype() {}
        virtual ~Prototype() {}
};


#endif
