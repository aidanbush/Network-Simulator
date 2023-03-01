#include <map>
#include <set>
#include <vector>
#include <nlohmann/json.hpp>
#include <iostream>

#include "packetHandler.h"
#include "interface.h"
#include "packet.h"
#include "manager.h"
#include "config.h"

using namespace std;

using json = nlohmann::json;

//Should only be called by Endpoint or switch constructors which already validate the json
PacketHandler::PacketHandler(json &handlerConfig): NetworkObject(handlerConfig["id"]) {
    this->internalSpeed = handlerConfig["internal_speed"];
}

int PacketHandler::getInternalSpeed() {
    return internalSpeed;
}

double PacketHandler::getMaxOutputRate() {
    double maxRate = 0;

    for (auto it = interfaces.rbegin(); it != interfaces.rend(); ++it) {
        Interface *interface = man.getInterface(it->second);
        maxRate = max(maxRate, (double)interface->getLinkSpeed());
    }

    return maxRate;
}

void PacketHandler::removeInterface(int interfaceId) {
    for (auto it = interfaces.rbegin(); it != interfaces.rend(); ++it) {
        if (it->second == interfaceId) {
            // TODO ensure interface is deleted and any other reference dealt with
            interfaces.erase(it.base());
        }
    }
}

bool PacketHandler::addInterface(int destId, int interfaceId) {
    return interfaces.emplace(destId, interfaceId).second;
}

bool PacketHandler::addInterfaceConfig(int interfaceId) {
    int destId, lowest;

    if (interfaces.empty()) {
        destId = -1;
    } else {
        lowest = interfaces.begin()->first;

        if (lowest >= 0) {
            destId = -1;
        } else {
            destId = lowest - 1;
        }
    }

    return interfaces.emplace(destId, interfaceId).second;
}

bool PacketHandler::hasInterface(int interfaceId) {
    for (auto& it : interfaces) {
        if (it.second == interfaceId) {
            return true;
        }
    }

    return false;
}

vector<int> PacketHandler::getInterfaces() {
    vector<int> interfaceVec;

    for (auto const& it : interfaces) {
        interfaceVec.push_back(it.second);
    }

    return interfaceVec;
}

vector<int> PacketHandler::getNeighbours() {
    // TODO handle duplicates
    vector<int> neighbours;

    for (auto const& it : interfaces) {
        neighbours.push_back(it.first);
    }

    return neighbours;
}

bool PacketHandler::connectNeighbours() {
    // for each interface until past negative neighbour ids
    set<int> interfaceIds;

    auto it = interfaces.begin();
    while (it != interfaces.end()) {
        if (it->first < 0) {
            interfaceIds.insert(it->second);
            it = interfaces.erase(it);
        } else {
            it++;
        }
    }

    for (int interfaceId : interfaceIds) {
        // add neighbours
        Interface *interface = man.getInterface(interfaceId);
        set<int> neighbours = interface->getNeighbours();

        for (int destId : neighbours) {
            interfaces.emplace(destId, interfaceId);
        }
    }

    return true;
}

bool PacketHandler::validateInterfaces() {
    bool valid = true;

    set<int> checkedIfaces;

    for (auto& it : interfaces) {
        PacketHandler *neighbour = man.getHandler(it.first);
        Interface *interface = man.getInterface(it.second);

        // check neighbour
        if (neighbour == NULL) {
            fprintf(stderr, "Handler: neighbour %d of handler %d is missing\n",
                    it.first, id);
            valid = false;
        } else {
            // TODO other checks for neighbour?
        }

        // check interface
        if (checkedIfaces.insert(it.second).second == false) {
            continue;
        }

        if (interface == NULL) {
            fprintf(stderr, "Handler: interface %d of handler %d is missing\n",
                    it.second, id);
            valid = false;
        } else {
            if (!(interface->getHandlerId() == id)) {
                fprintf(stderr, "Handler: interface %d, does not know of handler %d\n",
                        it.second, id);
                valid = false;
            }
        }
    }

    return valid;
}

bool PacketHandler::validateVariables() {
    bool valid = true;

    if (internalSpeed < 0) {
        fprintf(stderr, "Handler: %d has invalid internal speed %d\n", id,
                internalSpeed);
        valid = false;
    }

    return valid;
}

bool PacketHandler::validateHandler() {
    bool valid = validateInterfaces();

    if (!validateVariables()) {
        valid = false;
    }

    return valid;
}
