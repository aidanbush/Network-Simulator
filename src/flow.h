#ifndef FLOW_H
#define FLOW_H

#include <nlohmann/json.hpp>
#include <map>
#include <set>

#include "networkObject.h"
#include "manager.h"
#include "generator.h"

using namespace std;
using json = nlohmann::json;

class Packet;
class Endpoint;
class Switch;

class Flow: public NetworkObject {
    public:
        ~Flow();

        virtual void initializeFlow();
        virtual void startFlow() = 0;
        virtual void stopFlow() = 0;
        virtual void txPacketEvent() = 0;

        virtual void packetArrived(Packet *p);
        void packetDropped(Packet *p);
        void packetError(Packet *p);

        int getPacketsCreated();
        int getPacketsArrived();
        int getPacketsDropped();
        int getPacketsErrored();

        double getMaxRate();

        bool validate();

        virtual double getAveragePacketSizeBytes() = 0;

        virtual void packetGenerationNotification() = 0;

        int getBurstLastPacketId(int burstId); // returns the packet id of the last packet in the burst if known otherwise NULL_ID

    protected:

        bool running;

        Packet *sendingPacket;

        Flow(json &flowConfig);
        bool validateSource();
        bool validateDest();

        int ackHeadSize;
        int ackBodySize;

        virtual void sourcePacketArrived(Packet *p);
        virtual void sinkPacketArrived(Packet *p);

        void removePacket(Packet *p);
        bool addPacket(Packet *p);

        virtual Packet *getNextPacket(bool fromSource);

        int newPacketId(bool fromSource);

        int curSourcePId;
        int newestArrivedId;
        int curSinkPId;

        map<int, Packet *> sourcePackets;
        map<int, Packet *> sinkPackets;
        int sourceId;
        int destId;

        // burst tracking
        map<int, set<int>> bursts; // burstId -> set of packetIds ordered increasing value, since packets id are increasing the ordering will be by time created

        int ttl;
        int minHops;

        second_t nextTxTime(Packet *p);

        // stats
        int packetsCreated;
        int packetsArrived;
        int acksArrived;
        int packetsTimedOut;
        int packetsDropped;
        int packetsSent;
        int packetsErrored;
        int bytesSent;
        int bytesArrived;
        double throughput; // bytes/s
        double sentRate;
        double averageHops;
        int outOfOrderCount;
        second_t averageRTT;
        second_t minRTT;

        int totalPacketsSent;
        int totalPacketsDropped;

        double rate = 0; // bits/s
        double oldRate = 0;
        double maxRate;

        Generator *generator;

        second_t startTime;
        second_t endTime;

    private:
        void validateFlowConfig(json &flowConfig);
};

class BasicFlow: public Flow {
    friend Flow *createFlow(json &flowNetConfig, json &flowTestConfig);
    public:

        void initializeFlow();
        void startFlow();
        void stopFlow();
        virtual void txPacketEvent();

        double getAveragePacketSizeBytes();

        void packetGenerationNotification();

    protected:
        BasicFlow(json &flowConfig);

        static json &validateBasicFlowConfig(json &flowConfig);

        void recordData();
        void resetData();

        Packet *createAckPacket(Packet *p);

        void txAck(Packet *p);
        void packetArrived(Packet *p);
        void sourcePacketArrived(Packet *p);
};

class MBDFlow: public BasicFlow {
    friend Flow *createFlow(json &flowNetConfig, json &flowTestConfig);

    protected:
        MBDFlow(json &flowConfig);
        Packet *getNextPacket(bool fromSource);
};

#ifdef _TEST
class TestFlow: public Flow {
    friend Flow *createFlow(json &flowNetConfig, json &flowTestConfig);
    public:
        TestFlow(json &flowConfig);

        void startFlow();
        void stepAgent();
        void txPacketEvent();

    private:
        static json &validateTestFlowConfig(json &flowConfig);

        second_t nextTxTime(Packet *p);
};
#endif /* _TEST */

Flow *createFlow(json &flowNetConfig, json &flowTestConfig);

#endif /* FLOW_H */
