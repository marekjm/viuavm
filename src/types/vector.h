#ifndef WUDOO_TYPE_VECTOR_H
#define WUDOO_TYPE_VECTOR_H

#pragma once

#include <string>
#include <vector>
#include <sstream>
#include "object.h"


class Vector : public Object {
    /** Vector type.
     */
    std::vector<Object*> internal_object;

    public:
        std::string type() const {
            return "Vector";
        }
        std::string str() const {
            std::ostringstream oss;
            oss << "[";
            for (unsigned i = 0; i < internal_object.size(); ++i) {
                oss << internal_object[i]->str() << (i < internal_object.size()-1 ? ", " : "");
            }
            oss << "]";
            return oss.str();
        }
        bool boolean() const {
            return internal_object.size() != 0;
        }

        Object* copy() const {
            return new Vector();
        }

        std::vector<Object*>& value() { return internal_object; }

        Object* insert(int, Object*);
        Object* push(Object*);
        Object* pop(int);
        Object* at(int);
        int len();

        Vector() {}
        Vector(const std::vector<Object*>& v) {
            for (unsigned i = 0; i < v.size(); ++i) {
                internal_object.push_back(v[i]->copy());
            }
        }
};


#endif
