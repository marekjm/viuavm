#ifndef VIUA_TYPES_POINTER_H
#define VIUA_TYPES_POINTER_H

#pragma once

#include <vector>
#include <algorithm>
#include <viua/types/type.h>


class Pointer: public Type {
        Type* points_to;
        bool valid;

        void attach();
        void detach();
    public:
        void invalidate(Type* t);
        bool expired();
        void reset(Type* t);
        Type* to();

        std::string type() const override;
        bool boolean() const override;

        std::vector<std::string> bases() const override {
            return std::vector<std::string>{"Type"};
        }
        std::vector<std::string> inheritancechain() const override {
            return std::vector<std::string>{"Type"};
        }

        Type* copy() const override;

        Pointer(Type* t);
        virtual ~Pointer();
};


#endif
