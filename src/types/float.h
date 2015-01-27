#ifndef TATANKA_TYPES_FLOAT_H
#define TATANKA_TYPES_FLOAT_H

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
