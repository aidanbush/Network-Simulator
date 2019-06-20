#include <map>
#include <vector>

#include "packetHandler.h"
#include "interface.h"
#include "packet.h"
#include "manager.h"

using namespace std;

int PacketHandler::addInterface(int destID, Interface *iface) {
    if (ifaces.find(destID) != ifaces.end()) {
        return 0;
    }

    ifaces[destID] = iface;
    return 1;
}

void PacketHandler::removeInterface(int ifaceID) {
    // TODO
}

vector<PacketHandler *> PacketHandler::getNeighbours() {
    vector<PacketHandler *> neighbours;
    PacketHandler *handler;

    for (auto const& [id, iface] : ifaces) {
        handler = man.getHandler(id);
        neighbours.push_back(handler);
    }

    return neighbours;
}

map<Interface *, PacketHandler *> PacketHandler::getIfaceNeighbours() {
    map<Interface *, PacketHandler *> neighbours;

    // for each interface
    for (auto const& [id, iface] : ifaces) {
        neighbours.insert({iface, man.getHandler(id)});
    }

    return neighbours;
}
