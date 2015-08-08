#include <iostream>
#include <string>
using namespace std;


class Base {
    public:
        virtual void hello() {
            cout << "Hello World! (from Base)" << endl;
        }
};

class Middle: public Base {
    public:
        virtual void hello() {
            cout << "hello world! (from Middle)" << endl;
        }
};

class Derived: public Middle {
    public:
        virtual void hello() {
            cout << "hello world! (from Derived)" << endl;
        }

        virtual void hi() {
            cout << "Hi! (from Derived)" << endl;
        }
};


typedef void (Base::*MEMBER_FUNCTION)();


int main() {
    Base* bptr = new Derived();

    MEMBER_FUNCTION bhello = static_cast<MEMBER_FUNCTION>(&Derived::hi);

    (bptr->*bhello)();

    return 0;
}
