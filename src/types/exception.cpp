#include <string>
#include "exception.h"
using namespace std;


string Exception::what() {
    /** Stay compatible with standatd exceptions and
     *  provide what() method.
     */
    return cause;
}
