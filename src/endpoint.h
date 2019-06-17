#ifndef ENDPOINT_H
#define ENDPOINT_H

#include "packetHandler.h"

using namespace std;

class Packet;

class Endpoint: public PacketHandler {
    public:
        Endpoint(int id, int internalSpeed);

        void rxPacket(Packet *p);
        void txPacket(Packet *p);

#ifdef _TEST
        static int endpointToEndpoint();
#endif /* _TEST */
};

#ifdef _TEST
int testEndpoint();
#endif /* _TEST */

#endif /* ENDPOINT_H */
