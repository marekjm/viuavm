#ifndef VIUA_TYPES_PROTOTYPE_H
#define VIUA_TYPES_PROTOTYPE_H

#pragma once

#include <map>
#include <viua/types/type.h>


class Prototype: public Type {
    /** A prototype of a type.
     *
     *  This type is used internally inside the VM.
     */

    std::string type_name;
    std::vector<std::string> ancestors;
    std::map<std::string, std::string> methods;
    std::vector<std::string> attributes;

    public:
        virtual std::string type() const;
        virtual bool boolean() const;

        std::string getTypeName() const;

        // attach a function as a method to the prototype
        Prototype* attach(const std::string&, const std::string&);

        // add an attribute to the prototype
        Prototype* add(const std::string&);

        // push a type to the inheritance chain of the prototype
        Prototype* derive(const std::string&);


        virtual std::vector<std::string> bases() const {
            return std::vector<std::string>{"Type"};
        }
        virtual std::vector<std::string> inheritancechain() const {
            return std::vector<std::string>{"Type"};
        }

        virtual Type* copy() const;

        Prototype(const std::string& tn): type_name(tn) {}
        virtual ~Prototype() {}
};


#endif
