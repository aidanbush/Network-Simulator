#ifndef CONFIG_H
#define CONFIG_H
#include <nlohmann/json.hpp>

using namespace std;

using json = nlohmann::json;

enum jsonType {
    jsonInt,
    jsonDouble,
    jsonArray,
};

bool loadConfig(string filename);

bool hasMemberOfType(json &parent, string key, jsonType type);

#ifdef _TEST
int testConfig();
#endif /* _TEST */

#endif /* CONFIG_H */
