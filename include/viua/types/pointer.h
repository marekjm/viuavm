#ifndef VIUA_TYPES_POINTER_H
#define VIUA_TYPES_POINTER_H

#pragma once

#include <vector>
#include <viua/types/type.h>
#include <viua/cpu/frame.h>


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

        virtual void expired(Frame*, RegisterSet*, RegisterSet*);

        std::string str() const override;

        std::string type() const override;
        bool boolean() const override;

        std::vector<std::string> bases() const override {
            return std::vector<std::string>{"Type"};
        }
        std::vector<std::string> inheritancechain() const override {
            return std::vector<std::string>{"Type"};
        }

        Type* copy() const override;

        Pointer();
        Pointer(Type* t);
        virtual ~Pointer();
};


#endif
