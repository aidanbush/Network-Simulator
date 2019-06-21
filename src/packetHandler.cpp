#include <map>
#include <vector>

#include "packetHandler.h"
#include "interface.h"
#include "packet.h"
#include "manager.h"

using namespace std;

int PacketHandler::getInternalSpeed() {
    return internalSpeed;
}

void PacketHandler::removeInterface(int interfaceId) {
    for (auto it = interfaces.rbegin(); it != interfaces.rend(); ++it) {
        if (it->second == interfaceId) {
            interfaces.erase(it);
        }
    }
}

bool PacketHandler::addInterface(int destID, int interfaceId) {
    return interfaces.emplace(destId, interfaceId)
}

vector<int> PacketHandler::getInterfaces() {
    vector<int> interfaces;
    for (auto const& it : interfaces) {
        interfaces.push_back(it.second);
    }
    return interfaces;
}

vector<int> PacketHandler::getNeighbours() {
    vector<int> neighbours;
    for (auto const& it : interfaces) {
        neighbours.push_back(it.first);
    }
    return neighbours;
}
