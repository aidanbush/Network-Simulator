#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <nlohmann/json.hpp>

#include "packetHandler.h"

using namespace std;

using json = nlohmann::json;

class Packet;

class Endpoint: public PacketHandler {
    public:
        Endpoint(int id, int speed);
        void rxPacket(Packet *p);
        int txPacket(Packet *p);
        bool validate();

#ifdef _TEST
        static int endpointToEndpoint();
#endif /* _TEST */
};

#ifdef _TEST
int testEndpoint();
#endif /* _TEST */

#endif /* ENDPOINT_H */
