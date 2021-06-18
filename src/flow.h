#ifndef FLOW_H
#define FLOW_H

#include <nlohmann/json.hpp>
#include <map>

#include "networkObject.h"
#include "manager.h"
#include "agent.h"
#include "generator.h"

using namespace std;
using json = nlohmann::json;

class Packet;
class ECNPacket;
class Endpoint;

class Flow: public NetworkObject {
    public:
        ~Flow();

        virtual void startFlow() = 0;
        virtual void stepAgent() = 0;
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

        double getTotalReward();

        virtual double getAveragePacketSizeBytes() = 0;

    protected:
        // TODO move out of Flow into ECNFlow
        enum RewardType {
            BasicReward,
            RateReward,
            LogReward,
            AdvancedReward,
            AdvancedPenaltyReward,
            NegativeReward,
            OffsetReward,
            ECNReward,
            ExpertReward,
            ThroughputReward,
            ThroughputDemandReward,
            LogThroughput,
            REMY1,
            REMY2,
        };
        RewardType rewardType;

        bool running;

        double initialAverageReward();

        Flow(json &flowConfig);
        bool validateSource();
        bool validateDest();

        virtual void sourcePacketArrived(Packet *p);
        virtual void sinkPacketArrived(Packet *p);

        void removePacket(Packet *p);
        bool addPacket(Packet *p);

        Packet *getNextPacket(bool fromSource);

        int newPacketId(bool fromSource);

        int curSourcePId;
        int curSinkPId;

        map<int, Packet *> sourcePackets;
        map<int, Packet *> sinkPackets;
        int sourceId;
        int destId;

        int ttl;

        virtual second_t nextTxTime(Packet *p) = 0;

        // stats
        int packetsCreated;
        int packetsArrived;
        int acksArrived;
        int packetsDropped;
        int packetsErrored;
        int bytesSent;
        int bytesArrived;
        double throughput; // bytes/s
        double sentRate;
        double oldThroughput;
        second_t averageRTT;
        second_t minRTT;

        Agent *agent;
        second_t miTime = 0.01; // 10 ms
        double rate = 0; // bits/s
        double oldRate = 0;
        double oldECN = 0;
        double maxRate;

        // TODO move out of Flow into ECNFlow
        double totalReward = 0;

        Generator *generator;

    private:
        void validateFlowConfig(json &flowConfig);
};

class BasicFlow: public Flow {
    friend Flow *createFlow(json &flowConfig);
    public:

        void startFlow();
        void stepAgent();
        void txPacketEvent();

        double getAveragePacketSizeBytes();

    private:
        BasicFlow(json &flowConfig);

        static json &validateBasicFlowConfig(json &flowConfig);

        second_t time;

        int headSize;
        int bodySize;

        second_t nextTxTime(Packet *p);
};

class ECNFlow: public Flow {
    friend Flow *createFlow(json &flowConfig);
    public:

        void startFlow();
        void stepAgent();
        void txPacketEvent();

        double getAveragePacketSizeBytes();

    private:
        ECNFlow(json &flowConfig);

        ECNPacket *getNextPacket(bool fromSource);
        ECNPacket *createAckPacket(ECNPacket *toAck);

        void txAck(ECNPacket *toAckPacket);

        void packetArrived(Packet *p);
        void packetDropped(Packet *p);
        void packetError(Packet *p);

        void sourcePacketArrived(Packet *p);
        void sinkPacketArrived(Packet *p);

        static json &validateECNFlowConfig(json &flowConfig);

        int ackHeadSize;
        int ackBodySize;

        int packetsUntagged;
        int packetsSent;
        double averageECN;
        int totalPacketsUntagged;
        int totalPacketsSent;
        int totalPacketsDropped;

        int numSteps;
        int maxSteps;

        vector<double> getState();
        double getReward();
        void resetState();

        void updateStats();
        void updateStatsPostStep(pair<double, double> action);

        second_t nextTxTime(Packet *p);
};

#ifdef _TEST
class TestFlow: public Flow {
    friend Flow *createFlow(json &flowConfig);
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

Flow *createFlow(json &flowConfig);

#endif /* FLOW_H */
