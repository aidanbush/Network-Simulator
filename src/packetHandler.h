#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H

#include <vector>
#include <map>

#include "networkObject.h"

using namespace std;

class Interface;
class Packet;

class PacketHandler: public NetworkObject {
    public:
        int addInterface(int destID, Interface *iface);
        void removeInterface(int ifaceID);

        vector<PacketHandler *> getNeighbours();
        int getInternalSpeed() {return internalSpeed; }

        virtual void rxPacket(Packet *p) = 0;

    protected:
        map<int, Interface *> ifaces; // map neighbour id to iface

        int id;
        int internalSpeed;
};

#ifdef _TEST
class TestHandler: public PacketHandler {
    public:
        TestHandler(int internalSpeed) {this->internalSpeed = internalSpeed; }

        void rxPacket(Packet *) {; }
};
#endif /* _TEST */

#endif /* PACKET_HANDLER_H */
