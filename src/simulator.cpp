#include <stdio.h>
#include <queue>
#include <map>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

#include "switch.h"
#include "endpoint.h"
#include "interface.h"
#include "link.h"
#include "manager.h"

using namespace std;

using json = nlohmann::json;

#ifndef _TEST
Manager man;

void combineJson(json& first, json& second) {
    for (json::iterator it = second.begin(); it != second.end(); ++it) {
      first[it.key()] = it.value();
    }
}

json readConfig() {
    ifstream configStream("../config.json");
    json config;
    configStream >> config;
    ifstream localConfigStream("local.json");
    if (localConfigStream.good()) {
        json localConfig;
        localConfigStream >> localConfig;
        combineJson(config, localConfig);
    }
    return config;
}

void parseConfig(json& config, map<int, PacketHandler*>& packetHandlers,
                map<int, Interface*>& interfaces, map<int, Link*>& links) {
    for (json::iterator it = config["endpoints"].begin(); it != config["endpoints"].end(); ++it) {
        //it.value() gives json object of endpoint
        packetHandlers.emplace(it.value()["id"], new Endpoint(it.value()));
    }
    
    for (json::iterator it = config["switches"].begin(); it != config["switches"].end(); ++it) {
        //it.value() gives json object of switch
        packetHandlers.emplace(it.value()["id"], new Switch(it.value()));
    }
    
    for (json::iterator it = config["interfaces"].begin(); it != config["interfaces"].end(); ++it) {
        //it.value() gives json object of interface
        Interface* interface = new Interface(it.value());
        interfaces.emplace(interface->getId(), interface);
    }
    
    for (json::iterator it = config["links"].begin(); it != config["links"].end(); ++it) {
        //it.value() gives json object of link
        Link* linck = new Link(it.value());
        links.emplace(linck->getId(), linck);
        linck->addToInterfaces();
    }
}

int main() {
    // load configuration
    json config = readConfig();
    map<int, PacketHandler*> packetHandlers;
    map<int, Interface*> interfaces;
    map<int, Link*> links;
    parseConfig(config, packetHandlers, interfaces, links);
    while (man.numEvents() > 0) {
        EventI* e = man.popEvent();
        e->call();
        delete e;
    }

    return 0;
}

#endif // _TEST
