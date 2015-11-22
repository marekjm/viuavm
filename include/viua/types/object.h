#ifndef VIUA_TYPES_OBJECT_H
#define VIUA_TYPES_OBJECT_H

#pragma once

#include <map>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/types/type.h>


class Object: public Type {
    /** A generic object class.
     *
     *  This type is used internally inside the VM.
     */
    protected:
        std::string type_name;
        std::map<std::string, Type*> attributes;

    public:
        virtual std::string type() const;
        virtual bool boolean() const;

        virtual std::vector<std::string> bases() const {
            return std::vector<std::string>{"Type"};
        }
        virtual std::vector<std::string> inheritancechain() const {
            return std::vector<std::string>{"Type"};
        }

        virtual void set(Frame*, RegisterSet*, RegisterSet*);
        virtual void get(Frame*, RegisterSet*, RegisterSet*);
        inline Type* at(const std::string& s) { return attributes.at(s); }

        virtual Type* copy() const;

        Object(const std::string& tn);
        virtual ~Object();
};


#endif
