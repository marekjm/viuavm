#ifndef SUPPORT_POINTER_H
#define SUPPORT_POINTER_H

namespace pointer {
    template<class T, class S> void inc(S*& p) {
        T* ptr = (T*)p;
        ptr++;
        p = (S*)ptr;
    }
}

#endif
