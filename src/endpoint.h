#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <nlohmann/json.hpp>
#include <vector>

#include "packetHandler.h"

using namespace std;

using json = nlohmann::json;

class Packet;

class Endpoint: public PacketHandler {
    public:
        Endpoint(json &endpointConfig);
        void rxPacket(Packet *p, int sourceInterfaceId);
        void txPacket(Packet *p);
        bool validate();

#ifdef _TEST
        static int endpointToEndpoint();
#endif /* _TEST */

    private:
        static json &validateEndpointConfig(json &endpointConfig);
};

#ifdef _TEST
int testEndpoint();
#endif /* _TEST */

#endif /* ENDPOINT_H */
