#include <map>
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

bool PacketHandler::addInterface(int destId, int interfaceId) {
    return interfaces.emplace(destId, interfaceId).second;
}

vector<int> PacketHandler::getInterfaces() {
    vector<int> interfaceVec;

    for (auto const& it : interfaces) {
        interfaceVec.push_back(it.second);
    }

    return interfaceVec;
}

vector<int> PacketHandler::getNeighbours() {
    vector<int> neighbours;

    for (auto const& it : interfaces) {
        neighbours.push_back(it.first);
    }

    return neighbours;
}
