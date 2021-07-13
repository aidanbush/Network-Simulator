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

        virtual void initializeFlow();
        virtual void startFlow() = 0;
        virtual void stopFlow() = 0;
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

        virtual void packetGenerationNotification() = 0;

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

        Packet *sendingPacket;

        double initialAverageReward();

        Flow(json &flowConfig);
        bool validateSource();
        bool validateDest();

        int ackHeadSize;
        int ackBodySize;

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

        second_t nextTxTime(Packet *p);

        // stats
        int packetsCreated;
        int packetsArrived;
        int acksArrived;
        int packetsDropped;
        int packetsSent;
        int packetsErrored;
        int bytesSent;
        int bytesArrived;
        double throughput; // bytes/s
        double sentRate;
        double oldThroughput;
        second_t averageRTT;
        second_t minRTT;

        int totalPacketsSent;
        int totalPacketsDropped;

        Agent *agent;
        second_t miTime = 0.01; // 10 ms
        double rate = 0; // bits/s
        double oldRate = 0;
        double oldECN = 0;
        double maxRate;

        // TODO move out of Flow into ECNFlow
        double totalReward = 0;

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
        void stepAgent();
        void txPacketEvent();

        double getAveragePacketSizeBytes();

        void packetGenerationNotification();

    private:
        BasicFlow(json &flowConfig);

        static json &validateBasicFlowConfig(json &flowConfig);

        void recordData();
        void resetData();

        Packet *createAckPacket(Packet *p);

        void txAck(Packet *p);
        void packetArrived(Packet *p);
        void sourcePacketArrived(Packet *p);
};

class ECNFlow: public Flow {
    friend Flow *createFlow(json &flowNetConfig, json &flowTestConfig);
    public:

        void initializeFlow();
        void startFlow();
        void stopFlow();
        void takeAction(pair<double, double> action);
        void stepAgent();
        void txPacketEvent();

        double getAveragePacketSizeBytes();

        void packetGenerationNotification();

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

        ECNPacket *sendingPacket;

        int packetsUntagged;
        double averageECN;
        int totalPacketsUntagged;

        int numSteps;
        int maxSteps;

        vector<double> getState();
        double getReward();
        void resetState();

        void updateStats();
        void updateStatsPostStep(pair<double, double> action);
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
