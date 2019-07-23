#ifdef _TEST
#include <string>
#include <stdexcept>

#include "throwAssert.h"

using namespace std;

void throwAssert(bool cond, string message /* = "" */) {
    if (!cond) {
        throw runtime_error(message);
    }
}

#endif /* _TEST */
