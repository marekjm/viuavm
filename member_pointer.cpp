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
};


typedef void (Base::*HELLO)();


int main() {
    Base* bptr = new Derived();

    HELLO bhello = &Base::hello;

    (bptr->*bhello)();

    return 0;
}
