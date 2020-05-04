#ifndef CONFIG_H
#define CONFIG_H
#include <nlohmann/json.hpp>

using namespace std;

using json = nlohmann::json;

enum JsonType {
    jsonInt,
    jsonDouble,
    jsonString,
    jsonArray,
};

bool loadConfig(string filename);

void loadParams(string filename);

bool hasMemberOfType(json &parent, string key, JsonType type);

bool checkArrayType(json &config, JsonType type);

#ifdef _TEST
int testConfig();
#endif /* _TEST */

#endif /* CONFIG_H */
