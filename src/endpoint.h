#ifndef ENDPOINT_H
#define ENDPOINT_H

#include "packetHandler.h"

using namespace std;

class Packet;

class Endpoint: public PacketHandler {
    public:
        void rxPacket(Packet *p);
        void txPacket(Packet *p);
};

#endif /* ENDPOINT_H */
