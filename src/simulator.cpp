#include <queue>

#include "switch.h"
#include "manager.h"

using namespace std;

#ifndef _TEST
Manager man;

int main() {
    // load configuration

    while (man.numEvents() > 0) {
        EventI* e = man.popEvent();
        e->call();
        delete e;
    }

    return 0;
}

#endif // _TEST
