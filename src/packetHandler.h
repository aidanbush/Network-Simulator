#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H

#include <vector>

#include "interface.h"
#include "networkObject.h"
#include "packet.h"

using namespace std;

class Interface;

class PacketHandler: public NetworkObject {
    public:
        vector<Interface *> getInterfaces();
        void addInterface(Interface *iface);
        void removeInterface(int ifaceID);

        vector<PacketHandler *> getNeighbours();
        int getInternalSpeed() {return internalSpeed; }

        virtual void rxPacket(Packet *p) = 0;

    protected:
        vector<Interface *> ifaces;

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
