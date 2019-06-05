#include <queue>

#include "switch.h"
#include "manager.h"

using namespace std;

#ifndef _TEST
Manager man;

int main() {
    // load configuration

    while (!man.pq.empty()) {
        EventI* e = man.pq.top();
        e->call();
        man.pq.pop();
        delete e;
    }

    return 0;
}

#endif // _TEST
