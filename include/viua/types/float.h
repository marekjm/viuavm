#ifndef VIUA_TYPES_FLOAT_H
#define VIUA_TYPES_FLOAT_H

#pragma once

#include <string>
#include <ios>
#include <sstream>
#include "type.h"


class Float : public Type {
    /** Basic integer type.
     *  It is suitable for mathematical operations.
     */
    float data;

    public:
        std::string type() const {
            return "Float";
        }
        std::string str() const {
            std::ostringstream s;
            // std::fixed because 1.0 will yield '1' and not '1.0' when stringified
            s << std::fixed << data;
            return s.str();
        }
        bool boolean() const { return data != 0; }

        float& value() { return data; }

        Type* copy() const {
            return new Float(data);
        }

        Float(float n = 0): data(n) {}
};


#endif
