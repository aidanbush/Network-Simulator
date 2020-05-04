#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <nlohmann/json.hpp>
#include <vector>

#include "packetHandler.h"
#include "agent.h"

using namespace std;

using json = nlohmann::json;

class Packet;

class Endpoint: public PacketHandler {
    public:
        Endpoint(json &endpointConfig);
        void rxPacket(Packet *p);
        int txPacket(Packet *p);
        bool validate();
        vector<double> *getWeights();
        AgentType getAgentType();
#ifdef _TEST
        static int endpointToEndpoint();
#endif /* _TEST */

    private:
        vector<double> weights;
        static json &validateEndpointConfig(json &endpointConfig);
        AgentType agentType;
};

#ifdef _TEST
int testEndpoint();
#endif /* _TEST */

#endif /* ENDPOINT_H */
