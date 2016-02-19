#include <sstream>
#include <viua/types/reference.h>
using namespace std;


Type* Reference::pointsTo() const {
    return *pointer;
}

void Reference::rebind(Type* ptr) {
    delete (*pointer);
    (*pointer) = ptr;
}


string Reference::type() const {
    return (*pointer)->type();
}

string Reference::str() const {
    return (*pointer)->str();
}
string Reference::repr() const {
    return (*pointer)->repr();
}
bool Reference::boolean() const {
    return (*pointer)->boolean();
}

vector<string> Reference::bases() const {
    return vector<string>({});
}
vector<string> Reference::inheritancechain() const {
    return vector<string>({});
}

Reference* Reference::copy() const {
    ++(*counter);
    return new Reference(pointer, counter);
}

Reference::Reference(Type *ptr): pointer(new Type*(ptr)), counter(new unsigned(1)) {
}
Reference::~Reference() {
    /** Copies of the reference may be freely spawned and destroyed, but
     *  the internal object *MUST* be preserved until its refcount reaches zero.
     */
    if ((--(*counter)) == 0) {
        cout << hex;
        delete *pointer;
        delete pointer;
        delete counter;
    }
}
