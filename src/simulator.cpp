#include <stdio.h>
#include <unistd.h>
#include <libgen.h>
#include <signal.h>
#include <errno.h>

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

volatile sig_atomic_t exitSim;

void sigintHandler(__attribute__((unused)) int par) {
    exitSim = 1;
}

bool createSigintHandler() {
    struct sigaction sa = {};
    sa.sa_handler = &sigintHandler;

    int err = sigaction(SIGINT, &sa, NULL);
    if (err == -1) {
        perror("sigaction");
    }

    return err != -1;
}

void printUsage(char *pName) {
    printf("Usage %s [OPTIONS] [config]\n"
            "Network simulator\n\n"
            "If config file is not specified it defaults to \"" DEFAULT_CONFIG "\"\n\n"
            "Options\n"
            "  -l [filename] sets the log file, if not set uses stdout\n"
            "  -s step through event by event\n"
            "  -q [level] supresses output at the level given\n"
            "    level: 1 only event logging\n"
            "    level: 2+ all output\n"
            "  -r [seed] sets the random seed for the run\n"
            "  -f [filename] sets the csv output filename\n"
            "  -h this usage message\n", basename(pName));
}

int main(int argc, char **argv) {
    int c;
    bool step = false;
    char const *configFile = DEFAULT_CONFIG;

    srand((int)time(0));

    if (!createSigintHandler()) {
        return 1;
    }

    while ((c = getopt(argc, argv, "q:hsl:r:f:o")) != -1) {
        switch (c) {
            case 'q':
                man.setSuppressOutput(atoi(optarg));
                break;
            case 's':
                step = true;
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            case 'l':
                if (!man.setLogFile(optarg)) {
                    fprintf(stderr, "multiple log files specified\n");
                    printUsage(argv[0]);
                    return 1;
                }
                break;
            case 'r':
                srand(atoi(optarg));
                break;
            case 'f':
                if (!man.setCSVFilename(optarg)) {
                    fprintf(stderr, "multiple csv files specified\n");
                    printUsage(argv[0]);
                    return 1;
                }
                break;
            default:
                fprintf(stderr, "Unkown option\n");
                printUsage(argv[0]);
                return 1;
        }
    }

    if (optind == argc - 1) {
        configFile = argv[optind];
    } else if (optind == argc - 2) {
        configFile = argv[optind];
        loadParams(argv[optind + 1]);
    } else {
        fprintf(stderr, "Incorrect number of arguments\n");
        printUsage(argv[0]);
        return 1;
    }

    if (!loadConfig(configFile)) {
        return 1;
    }

    if (!man.startSimulator()) {
        man.deleteNetwork();
        return 1;
    }

    while (man.numEvents() > 0 && !exitSim) {
        EventI *e = man.popEvent();
        e->call();
        delete e;

        if (step) {
            getchar();
        }
    }

    man.deleteNetwork();

    return 0;
}

#endif /* _TEST */
