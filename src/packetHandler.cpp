#include "packetHandler.h"
#include "interface.h"
#include "packet.h"

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
    // TODO
    vector<PacketHandler *> temp;
    return temp;
}
