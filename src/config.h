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
    jsonBool,
};

bool checkConfigObjType(json &obj, JsonType type);

bool loadConfig(string netFilename, string testFilename);

void loadParams(string filename);

bool hasMember(json &parent, string key);

bool hasMemberOfType(json &parent, string key, JsonType type);

bool checkArrayType(json &config, JsonType type);

#ifdef _TEST
int testConfig();
#endif /* _TEST */

#endif /* CONFIG_H */
