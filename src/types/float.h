#ifndef VIUA_TYPES_FLOAT_H
#define VIUA_TYPES_FLOAT_H

#pragma once

#include <string>
#include <sstream>
#include "object.h"


class Float : public Object {
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
            s << data;
            if ((int)data == data) {    // if the two numbers are equal after the mantissa is discarded, then it means that it is .0
                s << ".0";  // make sure that we always print the mantissa (even if it is .0)
            }
            return s.str();
        }
        bool boolean() const { return data != 0; }

        float& value() { return data; }

        Object* copy() const {
            return new Float(data);
        }

        Float(float n = 0): data(n) {}
};


#endif
