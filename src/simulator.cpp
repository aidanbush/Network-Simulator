#include <stdio.h>
#include <unistd.h>
#include <libgen.h>
#include <queue>
#include <map>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

#include "manager.h"
#include "config.h"

#define DEFAULT_CONFIG  "config.json"

using namespace std;

using json = nlohmann::json;

#ifndef _TEST
Manager man;

void printUsage(char *pName) {
    printf("Usage %s [OPTIONS] [config]\n"
            "Network simulator\n\n"
            "If config file is not specified it defaults to \"" DEFAULT_CONFIG "\"\n\n"
            "Options\n"
            "  -s step through event by event\n"
            "  -h this usage message\n", basename(pName));
}

int main(int argc, char **argv) {
    int c;
    bool step = false;
    char *configFile = DEFAULT_CONFIG;

    while ((c = getopt(argc, argv, "hs")) != -1) {
        switch (c) {
            case 's':
                step = true;
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }

    if (optind < argc - 1) {
        printf("Too many arguments\n");
        printUsage(argv[0]);
        return 1;
    }

    if (optind == argc -1) {
        configFile = argv[optind];
    }


    if (!loadConfig(configFile)) {
        return 1;
    }

    while (man.numEvents() > 0) {
        EventI* e = man.popEvent();
        e->call();
        delete e;

        if (step) {
            getchar();
        }
    }

    return 0;
}

#endif /* _TEST */
