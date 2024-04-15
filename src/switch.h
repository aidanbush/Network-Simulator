#ifndef SWITCH_H
#define SWITCH_H

#include <nlohmann/json.hpp>
#include <map>
#include <queue>
#include <set>
#include <random>
#include <stack>
#include <tuple>

#include "packetHandler.h"

class LinUCB;
class NDDAgent;
class MBDPacket;

typedef double second_t;

using namespace std;

using json = nlohmann::json;

class Switch: public PacketHandler {
    public:
        Switch(json &switchConfig);

        virtual void rxPacket(Packet *p, int sourceInterfaceId);

        void txPacket(Packet *p);

        virtual bool initSwitch();
        virtual void startSwitch();

        int getInterfaceId(int destId);

        double costToDest(int destId);

        bool validate();

#ifdef _TEST
        static int testRoutingTableSearch();
#endif /* _TEST */

    protected:
        // dest Id to {optimal cost, optimal interface id's, remaining interface id's}
        map<int, tuple<double, set<int>, set<int>>> routingTable;
        vector<pair<int, int>> switchNeighbourIfaces; // all the interfaces that connect to a switch (interface id, switch id)

        int getNeighbourSwitch(int interfaceId);

        virtual void dropPacket(Packet *p);
        virtual void timeoutPacket(Packet *p);
        virtual void deflectPacket(Packet *p);
        virtual void forwardPacket(Packet *p);
        virtual void updateForwardOrDeflect(Packet *p, int forwardInterfaceId);
        virtual void arrivePacket(Packet *p);

        int droppedPackets;
        int timedOutPackets;
        int deflectedPackets; // sent along a non-shortest path
        int forwardedPackets; // sent along a shortest path
        int encounteredPackets; // packets that arrive and are not consumed
        int actionablePackets; // packets that get to being able to take an aciton on
        double averageAvailableInterfaces;

        void recordData();
        void resetData();

        virtual int routePacket(Packet *p, int sourceInterfaceId);

        static double txCost(int sourceId, int destId);
        static double txCost(Switch *source, int destId);
        static double txCost(Interface *interface);

        static vector<int> getNeighbours(int switchId);
        static int findSmallest(set<int> unvisited, map<int, double> dist);

        bool setupRoutingTable();
        void printRoutingTable();

        void setNeighbours();

        void updateAverageAvailableInterfaces(int availableInterfaces, int maxInterfaces);

    private:
        static json &validateSwitchConfig(json &switchConfig);
};

class RandomForwardSwitch: public Switch {
    public:
        RandomForwardSwitch(json &switchConfig);

        virtual bool initSwitch();
        virtual void startSwitch();

    protected:
        int routePacket(Packet *p, int sourceInterfaceId);

        default_random_engine generator;

    private:
        json &validateRandomForwardSwitchConfig(json &switchConfig);
};

class RandomDeflectionSwitch: public Switch {
    public:
        RandomDeflectionSwitch(json &switchConfig);

        virtual bool initSwitch();
        virtual void startSwitch();

    protected:
        int networkSize;

        double deflectThresh;

        default_random_engine generator;

        int routePacket(Packet *p, int sourceInterfaceId);

        void setRerouteLists();

    private:
        json &validateRandomDeflectionSwitchConfig(json &switchConfig);
};

class ManhattanBanditDeflectionSwitch: public RandomDeflectionSwitch {
    public:
        ManhattanBanditDeflectionSwitch(json &switchConfig);

        virtual void rxPacket(Packet *p, int sourceInterfaceId);

        bool initSwitch();
        void startSwitch();

        double getDeflectProb() {return deflectProb; }
        double getDropProb() {return dropProb; }
        int getSection() {return this->section; }

        void rewardAction(int pId, double reward);

    protected:
        enum StateType {
            flowIdState,
            destIdState,
            hop1ShortState,
            hop1_2ShortState,
            hop2ShortState,
            sectionState,
            deflectProbState,
            dropProbState
        };

        enum actionResult {
            actionDrop,
            actionIntentionalDrop,
            actionArrive,
            actionForward
        };

        enum ActionLimit {
            actionLimitNone,
            actionLimitOnlyDeflect,
            actionLimitForwardFirst
        };

        map<int, set<int>> createShortestLookupTable(vector<int> switches);

        // state variables
        int hop1ShortStateDims;
        int hop1_2ShortStateDims;
        int hop2ShortStateDims;
        // deflect prob state
        double deflectProb;
        double deflectProbTau;
        int deflectProbStateDims;
        double dropProb;
        double dropProbTau;
        int dropProbStateDims;
        double prevPacketArriveTime;

        map<int, int> hop1ShortStateMap; // destination to index of closest switches
        map<int, int> hop1_2ShortStateMap; // destination to index of closest switches
        map<int, int> hop2ShortStateMap; // destination to index of closest switches

        tuple<map<int, int>, int> shortestHopsStateMap(int lowHops, int highHops);
        void setFlowIdState(vector<double> &state, Packet *p);
        void setDestIdState(vector<double> &state, Packet *p);
        void setHop1ShortState(vector<double> &state, Packet *p);
        void setHop1_2ShortState(vector<double> &state, Packet *p);
        void setHop2ShortState(vector<double> &state, Packet *p);
        void setSectionState(vector<double> &state, Packet *p);
        void setDeflectProbState(vector<double> &state, Packet *p);
        void setDropProbState(vector<double> &state, Packet *p);

        void forwardPacket(Packet *p);
        void deflectPacket(Packet *p);
        void dropPacket(Packet *p);
        void arrivePacket(Packet *p);

        void updateDeflectAndDropProbabilities(bool deflect, bool drop);
        void updateDeflectionProbability(bool deflect);
        void updateDropProbability(bool deflect);

        int routePacket(Packet *p, int sourceInterfaceId);

        void setupStates();
        int getNumDims();
        vector<double> getState(Packet *p);

        int takeAgentAction(int sourceInterfaceId, Packet *p, vector<int> availableActions);
        void sendActionUpdate(int prevSwitch, Packet *p, actionResult result, double actionValue);
        void recieveActionUpdate(int pId, actionResult result, double nextValue, int nextMinHops);

        void recordAction(MBDPacket *p, vector<double> context, int action);
        tuple<vector<double>, int, int> peekAction(int pId);
        tuple<vector<double>, int, int> retrieveAction(int pId);
        vector<int> availableInterfaces(Packet *p);
        vector<int> availableForwardInterfaces(Packet *p);
        vector<int> availableDeflectInterfaces(Packet *p);

        map<int, stack<tuple<vector<double>, int, int>>> actionStore; // packet id -> (context, action, dest_id)
        vector<int> actionInterfaces; // vector of neighbouring interfaces - the index cooresponds to the action
        map<int, int> interfaceToAction; // interface id to action
        LinUCB *agent;
        string agentAlg;

        bool dropAction;
        int section;
        int numSections;
        // agent variables
        double regularizer;
        double delta;
        double discountFactor;
        // state variables
        int numFlows;
        set<StateType> stateTypes;

        ActionLimit actionLimit;

    private:
        json &validateManhattanBanditDeflectionSwitchConfig(json &switchConfig);
};

class NDDSwitch: public RandomForwardSwitch {
    public:
        NDDSwitch(json &switchConfig);

        bool initSwitch();
        void startSwitch();

        void DNTimerEvent();

    protected:
        struct NDDFeedbackMessage {
            int deflectionId;
            int DHC;
        };

        struct deflectionData {
            vector<int> state;
            int action;
            vector<int> actionSet;
            second_t DfT;
        };
        struct deflectionTimerElem {
            int deflectionId;
            second_t timeout;
        };
        struct deflectionTimerComparator {
            bool operator()(const deflectionTimerElem lhs, const deflectionTimerElem rhs) {
                if (lhs.timeout == rhs.timeout) {
                    return lhs.deflectionId > rhs.deflectionId;
                }
                return lhs.timeout > rhs.timeout;
            }
        };

        int routePacket(Packet *p, int sourceInterfaceId);
        void dropPacketFeedback(Packet *p);
        vector<int> getState(Packet *p);
        void feedbackArrived(NDDFeedbackMessage feedback);
        double calculateReward(double TTT, int DHC);

        void recordAction(int deflectionId, deflectionData data, second_t timeout);

        // constants
        int DHCMax;
        second_t DNMaxTime;

        NDDAgent *agent;
        vector<int> actionInterfaces; // index (action) to interface id
        map<int, int> interfaceToAction; // interface id to action

        // variables
        bool onlyForward; // used for validating the only forwarding behaviour
        bool multipleUpdates;
        // for waiting on feedback
        second_t DNTimer;
        int deflectionIdCounter;

        map<int, deflectionData> deflectionLookup;
        priority_queue<deflectionTimerElem, vector<deflectionTimerElem>, deflectionTimerComparator> deflectionTimeoutQueue;

        // TODO refactor to be dest to state not interfaces
        map<int, int> destToState; // maps all the destination id's to their corresponding state id based on the output interfaces they use

        int numDestStates;

    private:
        json &validateNDDSwitchConfig(json &switchConfig);
};

#ifdef _TEST

int testSwitch();

#endif /* _TEST */

Switch *createSwitch(json &switchNetConfig);

#endif /* SWITCH_H */
