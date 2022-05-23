#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>

#include "config.h"
#include "manager.h"
#include "endpoint.h"
#include "switch.h"
#include "interface.h"
#include "link.h"
#include "flow.h"

using namespace std;

using json = nlohmann::json;

json readConfig(string filename) {
    ifstream configStream(filename);
    json config;
    configStream >> config;

    return config;
}

bool checkConfigObjType(json &obj, JsonType type) {
    switch (type) {
        case jsonInt:
            return obj.is_number_integer();
        case jsonDouble:
            return obj.is_number();
        case jsonString:
            return obj.is_string();
        case jsonArray:
            return obj.is_array();
    }

    // error
    return false;
}

bool hasMember(json &parent, string key) {
    if (parent.find(key) == parent.end()) {
        return false;
    }

    return true;
}

bool hasMemberOfType(json &parent, string key, JsonType type) {
    if (hasMember(parent, key)) {
        return checkConfigObjType(parent[key], type);
    }

    return false;
}

bool checkArrayType(json &config, JsonType type) {
    bool valid = true;

    for (auto it : config.items()) {
        if (!checkConfigObjType(it.value(), type)) {
            valid = false;
        }
    }

    return valid;
}

static bool parseConfig(json& netConfig, json &testConfig) {
    // json interator.value give json element
    bool success = true;

    // exit time config
    if (hasMemberOfType(testConfig, "exit_time", jsonDouble)) {
        man.setExitTime(testConfig["exit_time"]);
    }

    for (json::iterator it = netConfig["endpoints"].begin(); it != netConfig["endpoints"].end(); ++it) {
        try {
            Endpoint *endpoint = new Endpoint(it.value());
            if (!man.addEndpoint(endpoint)) {
                cerr << "Endpoint:\nMultiple packet handlers exist with id '" << endpoint->getId() << "'." << endl;
                delete endpoint;
                success = false;
            }
        } catch (const runtime_error &e) {
            cerr << e.what() << endl;
            success = false;
        }
    }

    for (json::iterator it = netConfig["switches"].begin(); it != netConfig["switches"].end(); ++it) {
        try {
            Switch *netSwitch = createSwitch(it.value());
            if (!man.addSwitch(netSwitch)) {
                cerr << "Switch:\nMultiple packet handlers exist with id '" << netSwitch->getId() << "'." << endl;
                delete netSwitch;
                success = false;
            }
        } catch (const runtime_error &e) {
            cerr << e.what() << endl;
            success = false;
        }
    }

    for (json::iterator it = netConfig["interfaces"].begin(); it != netConfig["interfaces"].end(); ++it) {
        try {
            Interface *interface = new Interface(it.value());
            if (!man.addInterface(interface)) {
                cerr << "Interface:\nMultiple interfaces exist with id '" << interface->getId() << "'." << endl;
                delete interface;
                success = false;
            }
        } catch (const runtime_error &e) {
            cerr << e.what() << endl;
            success = false;
        }
    }

    for (json::iterator it = netConfig["links"].begin(); it != netConfig["links"].end(); ++it) {
        try {
            Link *netLink = new Link(it.value());
            if (!man.addLink(netLink)) {
                cerr << "Link:\nMultiple links exist with id '" << netLink->getId() << "'." << endl;
                delete netLink;
                success = false;
            }

            if (success && !netLink->addToInterfaces()) {
                cerr << "Error connecting network." << endl;
                success = false;
            }
        } catch (const runtime_error &e) {
            cerr << e.what() << endl;
            success = false;
        }
    }

    json flowTestConfig;
    if (hasMember(testConfig, "flows")) {
        flowTestConfig = testConfig["flows"];
    }

    for (json::iterator it = netConfig["flows"].begin(); it != netConfig["flows"].end(); ++it) {
        try {
            Flow *flow = createFlow(it.value(), flowTestConfig);
            if (!man.addFlow(flow)) {
                delete flow;
                success = false;
            }
        } catch (const runtime_error &e) {
            cerr << e.what() << endl;
            success = false;
        }
    }

    return success;
}

bool loadConfig(string netFilename, string testFilename) {
    json netConfig = readConfig(netFilename);
    json testConfig = readConfig(testFilename);

    if (!parseConfig(netConfig, testConfig)) {
        man.deleteNetwork();
        return false;
    }

    if (!man.validateNetwork()) {
        man.deleteNetwork();
        return false;
    }

    return true;
}

#ifdef _TEST
#include "tests/throwAssert.h"

int testConfig() {
    json config = readConfig("test.json");

    throwAssert(parseConfig(config));

    throwAssert(man.validateNetwork());

    man.deleteNetwork();

    return 1;
}

#endif /* _TEST */
