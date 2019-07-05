#ifndef FLOW_H
#define FLOW_H

#include <nlohmann/json.hpp>

#include "networkObject.h"
#include "manager.h"

using namespace std;
using json = nlohmann::json;

class Packet;
class Endpoint;

class Flow: public NetworkObject {
    public:
        Flow(json &config);

        virtual void startFlow() = 0;
        virtual void txPacketEvent() = 0;

        void packetArrived(Packet *p);
        void packetDropped(Packet *p);
        void packetError(Packet *p);

        bool validate();

    protected:
        vector<Packet *> packets;
        int sourceID;
        int destID;

        int newPacketID();

        virtual second_t nextTxTime() = 0;
        // stats
};

class BasicFlow: public Flow {
    public:
        BasicFlow(json &config);

        void startFlow();
        void txPacketEvent();

    private:
        second_t time;

        int headSize;
        int bodySize;

        int ttl;

        second_t nextTxTime();
};

#ifdef _TEST
class TestFlow: public Flow {
    public:
        using Flow::Flow;

        void startFlow();
        void txPacketEvent();

    private:
        second_t nextTxTime();
};
#endif /* _TEST */

Flow *createFlow(json &config);

#endif /* FLOW_H */
