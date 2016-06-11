#ifndef SUPPORT_POINTER_H
#define SUPPORT_POINTER_H

namespace pointer {
    template<class T, class S> void inc(S*& p) {
        T* ptr = reinterpret_cast<T*>(p);
        ptr++;
        p = reinterpret_cast<S*>(ptr);
    }
}

#endif
