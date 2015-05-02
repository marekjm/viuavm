#include <string>
#include "exception.h"
using namespace std;


string Exception::what() const {
    /** Stay compatible with standatd exceptions and
     *  provide what() method.
     */
    return cause;
}
