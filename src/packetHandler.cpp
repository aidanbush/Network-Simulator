#include <map>
#include <set>
#include <vector>
#include <nlohmann/json.hpp>

#include "packetHandler.h"
#include "interface.h"
#include "packet.h"
#include "manager.h"

using namespace std;

using json = nlohmann::json;

PacketHandler::PacketHandler(int id, int speed): NetworkObject(id) {
    this->internalSpeed = speed;
}

int PacketHandler::getInternalSpeed() {
    return internalSpeed;
}

void PacketHandler::removeInterface(int interfaceId) {
    for (auto it = interfaces.rbegin(); it != interfaces.rend(); ++it) {
        if (it->second == interfaceId) {
            // TODO ensure interface is deleted and any other reference dealt with
            interfaces.erase(it.base());
        }
    }
}

bool PacketHandler::addInterface(int destID, int interfaceID) {
    return interfaces.emplace(destID, interfaceID).second;
}

bool PacketHandler::addInterfaceConfig(int ifaceID) {
    int destID, lowest;

    if (interfaces.empty()) {
        destID = -1;
    } else {
        lowest = interfaces.begin()->first;

        if (lowest >= 0) {
            destID = -1;
        } else {
            destID = lowest - 1;
        }
    }

    return interfaces.emplace(destID, ifaceID).second;
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
    set<int> ifaceIDs;

    auto it = interfaces.begin();
    while (it != interfaces.end()) {
        if (it->first < 0) {
            ifaceIDs.insert(it->second);
            it = interfaces.erase(it);
        } else {
            it++;
        }
    }

    for (int ifaceID : ifaceIDs) {
        // add neighbours
        Interface *iface = man.getInterface(ifaceID);
        set<int> neighbours = iface->getNeighbours();

        for (int destID : neighbours) {
            interfaces.emplace(destID, ifaceID);
        }
    }

    return true;
}
