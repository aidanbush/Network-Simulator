#ifndef ENDPOINT_H
#define ENDPOINT_H

#include "networkObject.h"
#include "packetHandler.h"

using namespace std;

#include "packetHandler.h"

class Endpoint: public PacketHandler {
    public:
        void handlePacket(Packet *p);
        void txPacket(Packet *p);
};

#endif // ENDPOINT_H
