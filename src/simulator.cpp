#include <stdio.h>
#include <queue>
#include <map>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

#include "manager.h"
#include "config.h"

using namespace std;

using json = nlohmann::json;

#ifndef _TEST
Manager man;

int main() {
    if (!loadConfig("local.json")) {
        return 1;
    }

    while (man.numEvents() > 0) {
        EventI* e = man.popEvent();
        e->call();
        delete e;
    }

    return 0;
}

#endif /* _TEST */
