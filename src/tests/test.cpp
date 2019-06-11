#include <stdio.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <vector>

#include "../manager.h"
#include "../switch.h"

using json = nlohmann::json;

// Manager man;

struct money {
    string currency;
    float value;
};
void from_json(const json& j, money& m) {
    j.at("currency").get_to(m.currency);
    j.at("value").get_to(m.value);
}

int main() {
    // Switch s = Switch(1);
    // s.testOne();
    //
    // while (!man.pq.empty()) {
    //     EventI* e = man.pq.top();
    //     e->call();
    //     man.pq.pop();
    //     delete e;
    // }
    
    ifstream i("test.json");
    json j;
    i >> j;
    float pi = j["pi"];
    vector<int> list = j["list"];
    cout << pi << endl;
    for (auto it : list) {
        cout << it << " ";
    }
    cout << endl;
    //money m = j["object"];
    json o = j["object"];
    auto m = o.get<money>();
    cout << m.currency << endl << m.value << endl;

    printf("Tests Passed!\n");
    return 0;
}
