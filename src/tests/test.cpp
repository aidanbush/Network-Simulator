#include <stdio.h>

#include "../simulator.h"
#include "../switch.h"

int main() {
    Switch s = Switch(1);
    s.testOne();

    while (!pq.empty()) {
        EventI* e = pq.top();
        e->call();
        pq.pop();
        delete e;
    }

    printf("Tests Passed!\n");
    return 0;
}
