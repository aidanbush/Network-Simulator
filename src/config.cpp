#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>

#include "config.h"
#include "manager.h"
#include "endpoint.h"
#include "switch.h"
#include "interface.h"
#include "link.h"

using namespace std;

using json = nlohmann::json;

void combineJson(json& first, json& second) {
    for (json::iterator it = second.begin(); it != second.end(); ++it) {
      first[it.key()] = it.value();
    }
}

json readConfig(string filename) {
    ifstream configStream(filename);
    json config;

    configStream >> config;

    /*
    ifstream localConfigStream("../local.json");
    if (localConfigStream.good()) {
        json localConfig;
        localConfigStream >> localConfig;
        combineJson(config, localConfig);
    }
    */

    return config;
}

static bool checkConfigObjType(json &obj, jsonType type) {
    switch (type) {
        case jsonInt:
            return obj.is_number_integer();
        case jsonDouble:
            return obj.is_number_float() || obj.is_number_integer();
        case jsonArray:
            return obj.is_array();
    }

    // error
    return false;
}

bool hasMemberOfType(json &parent, string key, jsonType type) {
    if (parent.find(key) == parent.end()) {
        return false;
    }

    return checkConfigObjType(parent[key], type);
}

static bool checkConfig(json &config, vector<pair<string, jsonType>> &elems,
        string parent) {
    bool valid = true;

    for (pair<string, jsonType> e : elems) {
        if (config.find(e.first) == config.end()) {
            fprintf(stderr, "Error in %s, %s not found\n", parent.c_str(), e.first.c_str());
            valid = false;
        } else if (!checkConfigObjType(config[e.first],e.second)) {
            fprintf(stderr, "Error in %s with %s type\n", parent.c_str(), e.first.c_str());
            valid = false;
        }
    }

    return valid;
}

static bool checkConfigArray(json &config, string key, jsonType type,
        string parent) {
    if (!hasMemberOfType(config, key, jsonArray)) {
        fprintf(stderr, "Error in %s with key %s\n", parent.c_str(), key.c_str());
        return false;
    }

    bool valid = true;

    for (auto it : config[key].items()) {
        if (!checkConfigObjType(it.value(), type)) {
            fprintf(stderr, "Error in %s type in array %s\n", parent.c_str(), key.c_str());
            valid = false;
        }
    }

    return valid;
}

// static bool checkEndpointConfig(json &config) {
//     vector<pair<string, jsonType>> elems = {
//         {"id", jsonInt},
//         {"internal_speed", jsonInt},
//     };
//
//     return checkConfig(config, elems, "endpoint");
// }
//
// static bool checkSwitchConfig(json &config) {
//     vector<pair<string, jsonType>> elems = {
//         {"id", jsonInt},
//         {"internal_speed", jsonInt},
//     };
//
//     return checkConfig(config, elems, "switch");
// }

// static bool checkInterfaceConfig(json &config) {
//     vector<pair<string, jsonType>> elems = {
//         {"id", jsonInt},
//         {"handler_id", jsonInt},
//         {"link_buf_size", jsonInt},
//         {"handler_buf_size", jsonInt},
//     };
//
//     return checkConfig(config, elems, "interface");
// }

static bool checkLinkConfig(json &config) {
    vector<pair<string, jsonType>> elems = {
        {"id", jsonInt},
        {"speed", jsonInt},
        {"time", jsonDouble},
    };
    bool valid = true;

    if (!checkConfig(config, elems, "link")) {
        valid = false;
    }

    if (!checkConfigArray(config, "ifaces", jsonInt, "link")) {
        valid = false;
    }

    return valid;
}

// static bool addEndpoint(json &config) {
//     if (!checkEndpointConfig(config)) {
//         return false;
//     }
//
//     int id = config["id"];
//     int speed = config["internal_speed"];
//
//     Endpoint *e = new Endpoint(id, speed);
//
//     if (!man.addEndpoint(e)) {
//         fprintf(stderr, "Endpoint: error in adding id %d already exists\n", id);
//         delete e;
//         return false;
//     }
//
//     return true;
// }

// static bool addSwitch(json &config) {
//     if (!checkSwitchConfig(config)) {
//         return false;
//     }
//
//     int id = config["id"];
//     int speed = config["internal_speed"];
//
//     Switch *s = new Switch(id, speed);
//
//     if (!man.addSwitch(s)) {
//         fprintf(stderr, "Switch: error in adding id %d already exists\n", id);
//         delete s;
//         return false;
//     }
//
//     return true;
// }

// static bool addInterface(json &config) {
//     if (!checkInterfaceConfig(config)) {
//         return false;
//     }
//     PacketHandler *handler;
//
//     int id = config["id"];
//     int handlerID = config["handler_id"];
//     int linkBufSize = config["link_buf_size"];
//     int handlerBufSize = config["handler_buf_size"];
//
//     handler = man.getHandler(handlerID);
//     if (handler == NULL) {
//         fprintf(stderr, "Interface: handler with id %d does not exists\n", handlerID);
//         return false;
//     }
//
//     Interface *i = new Interface(id, handlerID, linkBufSize, handlerBufSize);
//
//     if (!man.addInterface(i)) {
//         delete i;
//         return false;
//     }
//
//     // inform handler
//     if (!handler->addInterfaceConfig(id)) {
//         fprintf(stderr, "Interface: error in linking handler %d with interface %d\n",
//                 handlerID, id);
//         delete i;
//         return false;
//     }
//
//     return true;
// }

static bool addLink(json &config) {
    if (!checkLinkConfig(config)) {
        return false;
    }

    int id = config["id"];
    int speed = config["speed"];
    second_t txTime = config["time"];

    Link *l = new Link(id, speed, txTime);

    if (!man.addLink(l)) {
        delete l;
        return false;
    }

    for (auto it : config["ifaces"].items()) {
        int ifaceID = it.value();
        if (!l->addDest(ifaceID)) {
            fprintf(stderr, "Link: interface with id %d unable to add\n", ifaceID);
            // delete and undo all adds?
            return false;
        }
    }

    // l->addToInterfaces();

    return true;
}

static bool parseConfig(json& config) {
    // json interator.value give json element

    bool success = true;

    for (json::iterator it = config["endpoints"].begin(); it != config["endpoints"].end(); ++it) {
        try {
            Endpoint *endpoint = new Endpoint(it.value());
        } catch (const runtime_error &e) {
            cerr << e.what() << endl;
            success = false;
            continue;
        }
        if (!man.addEndpoint(endpoint)) {
            cerr << "Endpoint:\nMultiple packet handlers exist with id '" << endpoint->getId() << "'." << endl;
            success = false;
        }
    }

    for (json::iterator it = config["switches"].begin(); it != config["switches"].end(); ++it) {
        try {
            Switch *netSwitch = new Switch(it.value());
        } catch (const runtime_error &e) {
            cerr << e.what() << endl;
            success = false;
            continue;
        }
        if (!man.addSwitch(netSwitch)) {
            cerr << "Switch:\nMultiple packet handlers exist with id '" << netSwitch->getId() << "'." << endl;
            success = false;
        }
    }

    for (json::iterator it = config["interfaces"].begin(); it != config["interfaces"].end(); ++it) {
        try {
            Interface *interface = new Interface(it.value());
        } catch (const runtime_error &e) {
            cerr << e.what() << endl;
            success = false;
            continue;
        }
        if (!addInterface(it.value())) {
            cerr << "Interface:\nMultiple interfaces exist with id '" << interface->getId() << "'." << endl;
            success = false;
        }
    }

    for (json::iterator it = config["links"].begin(); it != config["links"].end(); ++it) {
        if (!addLink(it.value())) {
            success = false;
        }
    }

    // TODO remove
    if (success && !man.linkHandlers()) {
        success = false;
    }

    return success;
}

bool loadConfig(string filename) {
    json config = readConfig(filename);

    if (!parseConfig(config)) {
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

int testConfig() {
    json config = readConfig("test.json");

    assert(parseConfig(config));

    assert(man.validateNetwork());

    man.deleteNetwork();

    return 1;
}

#endif /* _TEST */
