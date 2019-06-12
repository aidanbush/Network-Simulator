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
        vector<Interface> getInterfaces();
        void addInterface(Interface interface);
        void removeInterface(int interfaceId);

        vector<PacketHandler> getNeighbours();
        int getInternalSpeed() {return internalSpeed; }

        virtual void handlePacket(Packet *p) = 0;

    protected:
        vector<Interface> interfaces;

        int id;
        int internalSpeed;
};

#ifdef _TEST
class TestHandler: public PacketHandler {
    public:
        TestHandler(int internalSpeed) {this->internalSpeed = internalSpeed; }

        void handlePacket(Packet *) {; }
};
#endif /* _TEST */

#endif /* PACKET_HANDLER_H */
