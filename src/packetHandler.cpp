#include "packetHandler.h"

using namespace std;

vector<Interface> PacketHandler::getInterfaces() {
    return interfaces;
}

void PacketHandler::addInterface(Interface interface) {
    interfaces.push_back(interface);
}

void PacketHandler::removeInterface(int interfaceId) {
    //TODO
}

vector<PacketHandler> PacketHandler::getNeighbours() {
    //TODO
    vector<PacketHandler> temp;
    return temp;
}
