#ifndef VIUA_ASSERT_H
#define VIUA_ASSERT_H

#pragma once

#include <string>
#include <viua/types/type.h>
#include <viua/types/exception.h>


template<class T> void assert_implements(Type* object, const std::string& s) {
    /** Use this assertion when casting to interface type (C++ abstract class).
     *
     *  Example: casting Vector to Iterator.
     */
    if (dynamic_cast<T*>(object) == nullptr) {
        throw new Exception(object->type() + " does not implement " + s);
    }
}

void assert_typeof(Type* object, const std::string& expected) {
    /** Use this assertion when strict type checking is required.
     *
     *  Example: checking if an object is an Integer.
     */
    if (object->type() != expected) {
        throw new Exception("expected <" + expected + ">, got <" + object->type() + ">");
    }
}


#endif
