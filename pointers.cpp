#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;


template<class T> class Pointer {
    T *ptr;
    bool valid;

    void attach() {
        ptr->pointers.push_back(this);
        valid = true;
    }
    void detach() {
        if (valid) {
            ptr->pointers.erase(find(ptr->pointers.begin(), ptr->pointers.end(), this));
        }
        valid = false;
    }

    public:
        void invalidate(T* p) {
            if (p == ptr) {
                valid = false;
            }
        }
        bool expired() {
            return !valid;
        }
        void reset(T* p) {
            detach();
            ptr = p;
            attach();
        }
        T* to() {
            return ptr;
        }

        Pointer(T *p): ptr(p), valid(true) {
            attach();
        }
        ~Pointer() {
            detach();
        }
};


class Object {
    vector<Pointer<Object>*> pointers;
    friend Pointer<Object>;

    public:
        Object() {}
        ~Object() {
            cout << "~Object()" << endl;
            for (auto i : pointers) {
                i->invalidate(this);
            }
        }
};


int main(int argc, char** argv) {
    Object *o = new Object();
    Object *b = new Object();
    Pointer<Object> ptr(o);

    cout << "expired? " << ptr.expired() << " (to " << ptr.to() << ')' << endl;
    delete o;
    cout << "expired? " << ptr.expired() << " (to " << ptr.to() << ')' << endl;

    ptr.reset(b);

    cout << "expired? " << ptr.expired() << " (to " << ptr.to() << ')' << endl;
    delete b;
    cout << "expired? " << ptr.expired() << " (to " << ptr.to() << ')' << endl;

    return 0;
}
