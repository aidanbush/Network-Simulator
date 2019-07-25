#ifdef _TEST
#include <string>
#include <stdexcept>

#include "throwAssert.h"

#define MAX_STACK_TRACE_SIZE 10

using namespace std;

void throwAssertInner(bool cond, string file, int line, string condString) {
    if (!cond) {
        throw runtime_error("Assertion Error:\n" + file + ": " + to_string(line) + ": " + condString + "\n");
    }
}

#endif /* _TEST */
