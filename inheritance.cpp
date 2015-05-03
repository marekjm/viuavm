#include <iostream>
#include <string>
#include <vector>
using namespace std;


class Base {
    protected:
        const string type_string = "Base";

    public:
        virtual const string type() { return type_string; }
        virtual const vector<string> typechain() { return vector<string>{type_string}; }
        Base() {}
};

class Object: public Base {
    protected:
        const string type_string = "Object";

    public:
        virtual const string type() { return type_string; }
        virtual const vector<string> typechain();
        Object() {};
};

class Derived: public Object {
    protected:
        const string type_string = "Derived";

    public:
        virtual const string type() { return type_string; }
        virtual const vector<string> typechain();
        Derived() {};
};

class Descendant: public Derived {
    protected:
        const string type_string = "Descendant";

    public:
        virtual const string type() { return type_string; }
        virtual const vector<string> typechain();
        Descendant() {};
};

class Separate {
    protected:
        const string type_string = "Separate";

    public:
        virtual const string type() { return type_string; }
        const vector<string> typechain() {
            return vector<string>{type_string};
        }
        Separate() {}
};

class Child: public Descendant, Separate {
    protected:
        const string type_string = "Child";

    public:
        virtual const string type() { return type_string; }
        virtual const vector<string> typechain();
        Child() {};
};


int main() {
    Base base;
    cout << base.type() << endl;
    const vector<string> typechain_base = base.typechain();
    for (unsigned i = 0; i < typechain_base.size(); ++i) {
        cout << "  " << typechain_base[i] << endl;
    }
    cout << endl;


    Object object;
    cout << object.type() << endl;
    const vector<string> typechain_object = object.typechain();
    for (unsigned i = 0; i < typechain_object.size(); ++i) {
        cout << "  " << typechain_object[i] << endl;
    }
    cout << endl;


    Derived derived;
    cout << derived.type() << endl;
    const vector<string> typechain_derived = derived.typechain();
    for (unsigned i = 0; i < typechain_derived.size(); ++i) {
        cout << "  " << typechain_derived[i] << endl;
    }
    cout << endl;


    Descendant descendant;
    cout << descendant.type() << endl;
    const vector<string> typechain_descendant = descendant.typechain();
    for (unsigned i = 0; i < typechain_descendant.size(); ++i) {
        cout << "  " << typechain_descendant[i] << endl;
    }
    cout << endl;


    Child child;
    cout << child.type() << endl;
    const vector<string> typechain_child = child.typechain();
    for (unsigned i = 0; i < typechain_child.size(); ++i) {
        cout << "  " << typechain_child[i] << endl;
    }
    cout << endl;

    return 0;
}


const vector<string> Object::typechain() {
    vector<string> types = {type_string};
    vector<string> super_types = Base::typechain();
    for (unsigned i = 0; i < super_types.size(); ++i) {
        types.push_back(super_types[i]);
    }
    return types;
}

const vector<string> Derived::typechain() {
    vector<string> types = {type_string};
    vector<string> super_types = Object::typechain();
    for (unsigned i = 0; i < super_types.size(); ++i) {
        types.push_back(super_types[i]);
    }
    return types;
}

const vector<string> Descendant::typechain() {
    vector<string> types = {type_string};
    vector<string> super_types = Object::typechain();
    for (unsigned i = 0; i < super_types.size(); ++i) {
        types.push_back(super_types[i]);
    }
    return types;
}

const vector<string> Child::typechain() {
    vector<string> types = {type_string};
    vector<string> super_types = (Descendant::typechain() + Separate::typechain());
    for (unsigned i = 0; i < super_types.size(); ++i) {
        types.push_back(super_types[i]);
    }
    super_types = Separate::typechain();
    for (unsigned i = 0; i < super_types.size(); ++i) {
        types.push_back(super_types[i]);
    }

    return types;
}
