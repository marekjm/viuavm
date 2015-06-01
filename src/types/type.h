#ifndef VIUA_TYPES_TYPE_H
#define VIUA_TYPES_TYPE_H

#pragma once

#include <string>
#include <sstream>
#include <vector>


class Type {
    /** Base class for all derived types.
     *  Viua uses an object-based hierarchy to allow easier storage in registers and
     *  to take advantage of C++ polymorphism.
     *
     *  Instead of void* Viua holds Type* so when registers are delete'ed proper destructor
     *  is always called.
     */
    public:
        /** Basic interface of a Type.
         *
         *  Derived objects are expected to override this methods, but in case they do not
         *  Type provides safe defaults.
         */
        virtual std::string type() const {
            /*  Basic type is just `Type`.
             */
            return "Type";
        }
        virtual std::string str() const {
            /*  By default, Wudoo provides string output a la Python.
             *  This means - type of the object and its location in memory.
             */
            std::ostringstream s;
            s << "<'" << type() << "' object at " << this << ">";
            return s.str();
        }
        virtual std::string repr() const {
            /** This is akin to Python's repr.
             *  String returned by this method can be used to represent the value in source code.
             */
            return str();
        }
        virtual bool boolean() const {
            /*  Boolean defaults to false.
             *  This is because in if, loops etc. we will NOT execute code depending on unknown state.
             *  If a derived object overrides this method it is free to return true as it sees fit, but
             *  the default is to NOT carry any actions.
             */
            return false;
        }

        std::vector<std::string> bases() const {
            return std::vector<std::string>{"Type"};
        }
        std::vector<std::string> inheritancechain() const {
            return std::vector<std::string>{"Type"};
        }

        virtual Type* copy() const = 0;

        // We need to construct and destroy our basic object.
        Type() {}
        virtual ~Type() {}
};


#endif
