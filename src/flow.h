#ifndef FLOW_H
#define FLOW_H

#include "networkObject.h"
#include "manager.h"

using namespace std;

class Packet;
class Endpoint;

class Flow: public NetworkObject {
    public:
        Flow(int id, Endpoint *endpoint, int destID);

        virtual void StartFlow() = 0;
        virtual void txPacketEvent() = 0;

        void packetArrived(Packet *p);
        void packetDropped(Packet *p);
        void packetError(Packet *p);

        bool validate();

    protected:
        vector<Packet *> packets;
        Endpoint *endpoint;
        int sourceID;
        int destID;

        int newPacketID();

        // stats
};

#ifdef _TEST
class TestFlow: public Flow {
    public:
        using Flow::Flow;

        void StartFlow();
        void txPacketEvent();
        second_t nextTxTime();
};
#endif /* _TEST */

#endif /* FLOW_H */
