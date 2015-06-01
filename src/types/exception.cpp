#include <string>
#include "exception.h"
using namespace std;


string Exception::what() const {
    /** Stay compatible with standatd exceptions and
     *  provide what() method.
     */
    return cause;
}

string Exception::etype() const {
    /** Returns exception type.
     *
     *  Basic type is 'Exception' and is returned by the type() method.
     *  This method returns detailed type of the exception.
     */
    return detailed_type;
}
