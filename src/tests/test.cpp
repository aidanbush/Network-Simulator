#include <stdio.h>

#include "../manager.h"
#include "../switch.h"

Manager man;

int main() {
    Switch s = Switch(1);
    s.testOne();

    while (!man.pq.empty()) {
        EventI* e = man.pq.top();
        e->call();
        man.pq.pop();
        delete e;
    }

    printf("Tests Passed!\n");
    return 0;
}
