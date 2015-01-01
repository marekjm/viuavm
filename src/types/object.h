#ifndef TATANKA_TYPES_OBJECT_H
#define TATANKA_TYPES_OBJECT_H

#pragma once

#include <string>
#include <sstream>


class Object {
    public:
        virtual std::string type() const {
            return "Object";
        }
        virtual std::string str() const {
            std::ostringstream s;
            s << "<'" << type() << "' object at " << this << ">";
            return s.str();
        }
        virtual bool boolean() const {
            return false;
        }
};


#endif
