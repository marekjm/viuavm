#ifndef TATANKA_TYPES_OBJECT_H
#define TATANKA_TYPES_OBJECT_H

#pragma once

#include <string>
#include <sstream>


class Object {
    /** Base class for all derived types.
     *  Wudoo uses an Object-based Hierarchy to allow easier storage in registers and
     *  to take advantage of C++ polymorphism.
     *
     *  Instead of void* Wudoo holds Object* so when registers are delete'ed proper destructor
     *  is always called.
     */
    public:
        /** Interface of an Object.
         *
         *  Derived objects are expected to override this methods, but incase they do not
         *  here are provided safe defaults.
         */
        virtual std::string type() const {
            /*  Basic type is just `Object`.
             */
            return "Object";
        }
        virtual std::string str() const {
            /*  By default, Wudoo provides string output a la Python.
             *  This means - type of the object and its location in memory.
             */
            std::ostringstream s;
            s << "<'" << type() << "' object at " << this << ">";
            return s.str();
        }
        virtual bool boolean() const {
            /*  Boolean defaults to false.
             *  This is because in if, loops etc. we will NOT execute code depending on unknown state.
             *  If a derived object overrides this method it is free to return true as it sees fit, but
             *  the default is to NOT carry any actions.
             */
            return false;
        }

        // We need to construct and desory our basic object.
        Object() {}
        virtual ~Object() {}
};


#endif
