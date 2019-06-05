#include <queue>
#include <iostream>

#include "switch.h"
#include "simulator.h"

using namespace std;

priority_queue<EventI*, vector<EventI*>, comparator> pq;

int main() {
    Switch s = Switch(1);
    s.testOne();
    while (!pq.empty()) {
        EventI* e = pq.top();
        e->call();
        pq.pop();
    }
    return 0;
}
