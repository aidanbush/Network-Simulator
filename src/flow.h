#ifndef FLOW_H
#define FLOW_H

#include <nlohmann/json.hpp>
#include <map>

#include "networkObject.h"
#include "manager.h"
#include "agent.h"

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

        void seed(int seed);

        double getTotalReward();

    protected:
        // TODO move out of Flow into ECNFlow
        enum RewardType {
            BasicReward,
            RateReward,
            LogReward,
            NegativeReward,
            OffsetReward,
            ECNReward,
            ExpertReward,
        };
        RewardType rewardType;

        Flow(json &flowConfig);
        bool validateSource();
        bool validateDest();

        void removePacket(Packet *p);
        bool addPacket(Packet *p);

        Packet *createPacket(int ttl, int headSize, int bodySize);

        int newPacketId();

        int curPId;

        map<int, Packet *> packets;
        int sourceId;
        int destId;

        virtual second_t nextTxTime() = 0;

        // stats
        int packetsCreated;
        int packetsArrived;
        int packetsDropped;
        int packetsErrored;

        Agent *agent;
        second_t miTime = 0.01; // 10 ms
        double rate = 0;
        double oldRate = 0;
        double oldECN = 0;
        double maxRate;

        // TODO move out of Flow into ECNFlow
        double totalReward = 0;
        vector<double> rateList;
        vector<double> rewardList;
        vector<double> actionMultList;
        vector<double> actionAddList;
        vector<double> multMeanList;
        vector<double> multStdevList;
        vector<double> addMeanList;
        vector<double> addStdevList;

        second_t maxTime;
};

class BasicFlow: public Flow {
    friend Flow *createFlow(json &flowConfig);
    public:

        void startFlow();
        void stepAgent();
        void txPacketEvent();

    private:
        BasicFlow(json &flowConfig);

        static json &validateBasicFlowConfig(json &flowConfig);

        second_t time;

        int headSize;
        int bodySize;

        int ttl;

        second_t nextTxTime();
};

class ECNFlow: public Flow {
    friend Flow *createFlow(json &flowConfig);
    public:

        void startFlow();
        void stepAgent();
        void txPacketEvent();

    private:
        ECNFlow(json &flowConfig);

        ECNPacket *createPacket(int ttl, int headSize, int bodySize);

        void packetArrived(Packet *p);
        void packetDropped(Packet *p);
        void packetError(Packet *p);

        static json &validateECNFlowConfig(json &flowConfig);

        int headSize;
        int bodySize;
        int ttl;

        int packetsUntagged;
        int packetsSent;
        double averageECN;
        int totalPacketsUntagged;
        int totalPacketsSent;
        vector<double> averageECNList;

        double getState();
        double getReward();
        void resetState();

        void updateStats();
        void updateStatsPostStep(pair<double, double> action);

        void printCSV(string filename, vector<double> vec);

        ECNPacket *createPacket();

        second_t nextTxTime();
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

        second_t nextTxTime();
};
#endif /* _TEST */

Flow *createFlow(json &flowConfig);

#endif /* FLOW_H */
