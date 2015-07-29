#ifndef VIUA_TYPES_OBJECT_H
#define VIUA_TYPES_OBJECT_H

#pragma once

#include <map>
#include <viua/types/type.h>


class Object: public Type {
    /** A generic object class.
     *
     *  This type is used internally inside the VM.
     */

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

        virtual Type* copy() const;

        Object(const std::string& tn): type_name(tn) {}
        virtual ~Object() {}
};


#endif
