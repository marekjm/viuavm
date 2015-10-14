#ifndef VIUA_TYPE_VECTOR_H
#define VIUA_TYPE_VECTOR_H

#pragma once

#include <string>
#include <vector>
#include "type.h"


class Vector : public Type {
    /** Vector type.
     */
    std::vector<Type*> internal_object;

    public:
        std::string type() const {
            return "Vector";
        }
        std::string str() const;
        bool boolean() const {
            return internal_object.size() != 0;
        }

        Type* copy() const {
            Vector* vec = new Vector();
            for (unsigned i = 0; i < internal_object.size(); ++i) {
                vec->push(internal_object[i]->copy());
            }
            return vec;
        }

        std::vector<Type*>& value() { return internal_object; }

        Type* insert(long int, Type*);
        Type* push(Type*);
        Type* pop(long int);
        Type* at(long int);
        int len();

        Vector() {}
        Vector(const std::vector<Type*>& v) {
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
