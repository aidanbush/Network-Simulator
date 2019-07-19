#ifdef _TEST
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <vector>

#include "../manager.h"
#include "../packet.h"
#include "../link.h"
#include "../interface.h"
#include "../endpoint.h"
#include "../switch.h"
#include "../config.h"

Manager man;

int main() {
    int errors = 0;

    if (!testPacket()) {
        printf("Packet tests failed\n");
        errors++;
    } else {
        printf("Packet tests passed\n");
    }

    if (!testLink()) {
        printf("Link tests failed\n");
        errors++;
    } else {
        printf("Link tests passed\n");
    }

    if (!testInterface()) {
        printf("Interface tests failed\n");
        errors++;
    } else {
        printf("Interface tests passed\n");
    }

    if (!testEndpoint()) {
        printf("Endpoint tests failed\n");
        errors++;
    } else {
        printf("Endpoint tests passed\n");
    }

    if (!testSwitch()) {
        printf("Switch tests failed\n");
        errors++;
    } else {
        printf("Switch tests passed\n");
    }

    try {
        if (!testConfig()) {
            printf("Config tests failed\n");
            errors++;
        } else {
            printf("Config tests passed\n");
        }
    } catch(...) {
        printf("Config tests failed\n");
        errors++;
    }

    if (errors == 0) {
        printf("All tests Passed!\n");
        return 0;
    }

    printf("%d test%s failed\n", errors, errors == 1 ? "" : "s");
    return 1;
}
#endif //_TEST
