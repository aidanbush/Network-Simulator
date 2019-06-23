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

// Manager man;

// struct money {
//     string currency;
//     float value;
// };
// void from_json(const json& j, money& m) {
//     j.at("currency").get_to(m.currency);
//     j.at("value").get_to(m.value);
// }
// void combineJson(json& first, json& second) {
//     for (json::iterator it = second.begin(); it != second.end(); ++it) {
//       first[it.key()] = it.value();
//     }
// }

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

    // TODO: Add config tests
    // ifstream i("test.json");
    // json j;
    // i >> j;
    // for (json::iterator it = j.begin(); it != j.end(); ++it) {
    //   std::cout << it.key() << " : " << it.value() << "\n";
    // }
    // ifstream ii("local.json");
    //
    // if (ii.good()) {
    //     json jj;
    //     ii >> jj;
    //     combineJson(j, jj);
    // }
    //
    // for (json::iterator it = j.begin(); it != j.end(); ++it) {
    //     std::cout << it.key() << " : " << it.value() << "\n";
    // }
    // float pi = j["pi"];
    // vector<int> list = j["list"];
    // cout << pi << endl;
    // for (auto it : list) {
    //     cout << it << " ";
    // }
    // cout << endl;
    // //money m = j["object"];
    // json o = j["object"];
    // auto m = o.get<money>();
    // cout << m.currency << endl << m.value << endl;

    if (!testSwitch()) {
        printf("Switch tests failed\n");
        errors++;
    } else {
        printf("Switch tests passed\n");
    }

    if (errors == 0) {
        printf("All tests Passed!\n");
        return 0;
    }

    printf("%d tests failed\n", errors);
    return 1;
}
