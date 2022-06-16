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
            GoodputReward,
            ThroughputDemandReward,
            LogThroughput,
            LogGoodput,
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

class MDCFlow: public BasicFlow {
    friend Flow *createFlow(json &flowNetConfig, json &flowTestConfig);
    public:

        double getAverageHops() {return averageHopCount; }

    protected:
        MDCFlow(json &flowConfig);
        double averageHopCount;
        int totalPacketsArrived;

        int alphaLimiter = 1000;

        Packet *getNextPacket(bool fromSource);

        void packetArrived(Packet *p);
};

class DataQueue {
    public:
        DataQueue(int flowId);

        void push_back(Packet *p, second_t timeoutTime);
        bool pop(Generator::PacketData &pData, int &dataId);
        int removeData(int dataId); // returns the maximum acked Byte

        struct transitPacket {
            Generator::PacketData pData;
            int dataId;
            second_t timeout;
        };

    private:
        struct queuePacket {
            second_t timeoutTime;
            int uniqueId;
        };

        struct queuePacketComparator {
            bool operator ()(const queuePacket lhs, const queuePacket rhs) const {
                return lhs.timeoutTime > rhs.timeoutTime;
            }
        };

        struct lookupElem {
            int uniqueId;
        };

        struct dataElem {
            Generator::PacketData pData;
            int dataId;
            bool deleted;
        };

        // queue for packets not timed out
        priority_queue<queuePacket, vector<queuePacket>, queuePacketComparator> untimedoutQueue;
        // queue for packets timed out
        priority_queue<queuePacket, vector<queuePacket>, queuePacketComparator> timedoutQueue;

        // uId is the key, contains the actual packets that the queues and lookup table refer to
        unordered_map<int, dataElem> dataTable;
        map<int, lookupElem> lookupTable; // dataId is the key

        int curUId;

        int flowId;

        int newUId();

        void timeoutEvent();
};

class DataFlow: public Flow {
    public:
        DataFlow(json &flowConfig);

        ~DataFlow();

        virtual void notifyPacketTimeout() = 0;

    protected:
        int curDataPId;

        int newDataId(Generator::PacketData pData);

        // used for goodput, is calculated using acks, only when a packet is acked is it counted towards
        int maxAckedByte; // should be working
        int ackedBytesArrived;
        double goodput;
        double oldGoodput;

        DataQueue *queue;

        second_t retransmitTimeout;
        bool timeoutEventScheduled;

        // track what has been recieved for TCP style acks
        struct recievedPacket {
            int dataId;
            int size; // body size
            friend bool operator >(const recievedPacket& lhs, const recievedPacket& rhs) {
                return lhs.dataId > rhs.dataId;
            }
            friend bool operator <(const recievedPacket& lhs, const recievedPacket& rhs) {
                return rhs > lhs;
            }
            friend bool operator ==(const recievedPacket& lhs, const recievedPacket& rhs) {
                return lhs.dataId == rhs.dataId;
            }
        };


        priority_queue<recievedPacket, vector<recievedPacket>, greater<recievedPacket>> recievedPackets;

        void addRecievedPacket(Packet *p);
        int getAckDataId();

        bool nextRetransmitionPacket(DataQueue::transitPacket &p);
        void addRetransmitPacket(Packet *p);

        void packetTimeoutEvent();

        Packet *createNextPacket(bool fromSource);
        Packet *getNextPacket(bool fromSource);

        void packetArrived(Packet *p);
        void sourcePacketArrived(Packet *p);
        void sinkPacketArrived(Packet *p);
};

class ECNFlow: public DataFlow {
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

        void notifyPacketTimeout();

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

/*
class CUBICFlow: public DataFlow {
    friend Flow *createFlow(json &flowNetConfig, json &flowTestConfig);
    public:
        CUBICFlow(json &flowConfig);

    private:
        int cwnd;

        int tcpFriendliness;
        int fastConvergence;
        double beta;
        double C;

        int ssthresh; // slow start threshold
};
*/

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
