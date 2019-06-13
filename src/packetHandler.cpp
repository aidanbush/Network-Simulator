#include "packetHandler.h"

using namespace std;

vector<Interface *> PacketHandler::getInterfaces() {
    return ifaces;
}

void PacketHandler::addInterface(Interface *iface) {
    ifaces.push_back(iface);
}

void PacketHandler::removeInterface(int ifaceID) {
    // TODO
}

vector<PacketHandler *> PacketHandler::getNeighbours() {
    // TODO
    vector<PacketHandler *> temp;
    return temp;
}
