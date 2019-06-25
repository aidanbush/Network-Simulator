#include <stdio.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <vector>

#include "../manager.h"
#include "../packet.h"
#include "../link.h"
#include "../interface.h"
#include "../endpoint.h"
#include "../switch.h"

using json = nlohmann::json;

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

    // TODO: Add config tests

    if (errors == 0) {
        printf("All tests Passed!\n");
        return 0;
    }

    printf("%d tests failed\n", errors);
    return 1;
}
