#include <queue>
#include <iostream>

#include "switch.h"
#include "simulator.h"

using namespace std;

priority_queue<EventI*, vector<EventI*>, comparator> pq;

#ifndef _TEST

int main() {
    // load configuration

    while (!pq.empty()) {
        EventI* e = pq.top();
        e->call();
        pq.pop();
        delete e;
    }

    return 0;
}

#endif // _TEST
