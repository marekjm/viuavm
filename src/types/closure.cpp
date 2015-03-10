#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include "object.h"
#include "closure.h"
using namespace std;


string Closure::str() const {
    ostringstream oss;
    oss << "Closure: " << function_name;
    return oss.str();
}
