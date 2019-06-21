#include "packetHandler.h"

using namespace std;

vector<Interface*> PacketHandler::getInterfaces() {
    return interfaces;
}

void PacketHandler::addInterface(Interface* interface) {
    interfaces.push_back(interface);
}

void PacketHandler::removeInterface(int interfaceId) {
    for (auto it = interfaces.begin(); it != interfaces.end(); ++it) {
        if ((**it).getId() == interfaceId) {
            interfaces.erase(it);
            break;
        }
    }
}

vector<PacketHandler> PacketHandler::getNeighbours() {
    //TODO
    vector<PacketHandler> temp;
    return temp;
}
