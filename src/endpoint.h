#ifndef ENDPOINT_H
#define ENDPOINT_H

#include "networkObject.h"
#include "packetHandler.h"

using namespace std;

#include "packetHandler.h"

class Endpoint: PacketHandler {
    public:
        void handlePacket(Packet *p);
};

#endif // ENDPOINT_H
