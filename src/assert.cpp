#include <viua/assert.h>
using namespace std;


void viua::assertions::assert_typeof(Type* object, const string& expected) {
    /** Use this assertion when strict type checking is required.
     *
     *  Example: checking if an object is an Integer.
     */
    if (object->type() != expected) {
        throw new TypeException(expected, object->type());
    }
}
