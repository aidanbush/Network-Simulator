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

        void handlePacket(Packet *p);

    protected:
        vector<Interface> interfaces;

        int id;
};

#endif // PACKET_HANDLER_H
