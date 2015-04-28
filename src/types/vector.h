#ifndef VIUA_TYPE_VECTOR_H
#define VIUA_TYPE_VECTOR_H

#pragma once

#include <string>
#include <vector>
#include "object.h"


class Vector : public Object {
    /** Vector type.
     */
    std::vector<Object*> internal_object;

    public:
        std::string type() const {
            return "Vector";
        }
        std::string str() const;
        bool boolean() const {
            return internal_object.size() != 0;
        }

        Object* copy() const {
            Vector* vec = new Vector();
            for (unsigned i = 0; i < internal_object.size(); ++i) {
                vec->push(internal_object[i]->copy());
            }
            return vec;
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
        ~Vector() {
            while (internal_object.size()) {
                delete internal_object.back();
                internal_object.pop_back();
            }
        }
};


#endif
