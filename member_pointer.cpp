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
            cout << "Hello World! (from Middle)" << endl;
        }

        virtual void greet() {
            cout << "Greetings! (from Middle)" << endl;
        }
};

class Derived: public Middle {
    public:
        virtual void hi() {
            cout << "Hi! (from Derived)" << endl;
        }
};


typedef void (Base::*MEMBER_FUNCTION)();


int main() {
    /** Simple case - call a member function of derived class through a base class pointer.
     *  Thanks to the dynamic dispatch we got it covered for us.
     */
    Base* base_derived_ptr = new Derived();
    MEMBER_FUNCTION hi = static_cast<MEMBER_FUNCTION>(&Derived::hi);
    (base_derived_ptr->*hi)();

    /** Slightly more advanced case - call a member function down the inheritance hierarchy.
     *  Again, thanks to the dynamic dispatch we got it covered for us.
     */
    MEMBER_FUNCTION greet = static_cast<MEMBER_FUNCTION>(&Middle::greet);
    MEMBER_FUNCTION hello = static_cast<MEMBER_FUNCTION>(&Middle::hello);
    (base_derived_ptr->*greet)();
    (base_derived_ptr->*hello)();

    /** ACHTUNG! WARNING! UWAGA!
     *
     *  This is the case we should fear - trying to call a member function
     *  defined high in the inheritance hierarchy on a class somewhere lower.
     *  Segmentation fault is the way the system reponds to such shenanigans.
     */
    Base* base_middle_ptr = new Middle();
    //(base_middle_ptr->*hi)();

    return 0;
}
