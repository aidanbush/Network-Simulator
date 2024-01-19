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
        // dest Id to  optimal cost and interface id's, and then remaining interface id's
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
        pair<int, int> coords; // x, y
        enum directions {
            RIGHT = 1,
            LEFT = 2,
            UP = 3,
            DOWN = 6,
        };

        // lookup table containing the optimal and deflect interfaces
        // index: right +1, left +2, up +3, down +6
        vector<pair<vector<int>, vector<int>>> manhattanRouting = vector<pair<vector<int>, vector<int>>>(9);

        double deflectThresh;

        default_random_engine generator;

        pair<int, int> getCoords(int id, int networkSize);
        int manhattanDistance(pair<int, int> coord1, pair<int, int> coord2);

        pair<vector<int>, vector<int>> generateRoutingLists(pair<int, int> destCoords);
        void createManhattanRoutingTable();
        pair<vector<int>, vector<int>> availableRouteSets(Packet *p);

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

        void rewardAction(int pId, double reward);

    protected:
        enum StateType {
            flowIdState,
            destIdState,
            hop1ShortState,
            hop1_2ShortState,
            hop2ShortState,
            sectionState3x3,
            deflectProbState,
            dropProbState
        };

        enum actionResult {
            actionDrop,
            actionIntentionalDrop,
            actionArrive,
            actionForward
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

        map<int, set<int>> hop1ShortStateMap; // destination to index of closest switches
        map<int, set<int>> hop1_2ShortStateMap; // destination to index of closest switches
        map<int, set<int>> hop2ShortStateMap; // destination to index of closest switches

        bool inNeighbourSector(Packet *p);
        bool inHomeSector(Packet *p);

        void setFlowIdState(vector<double> &state, Packet *p);
        void setDestIdState(vector<double> &state, Packet *p);
        void setHop1ShortState(vector<double> &state, Packet *p);
        void setHop1_2ShortState(vector<double> &state, Packet *p);
        void setHop2ShortState(vector<double> &state, Packet *p);
        void setSectionState3x3(vector<double> &state, Packet *p);
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

        void sendActionUpdate(int prevSwitch, Packet *p, actionResult result, double actionValue);
        void recieveActionUpdate(int pId, actionResult result, double nextValue, int nextMinHops);

        void recordAction(Packet *p, vector<double> context, int action);
        tuple<vector<double>, int, int> peekAction(int pId);
        tuple<vector<double>, int, int> retrieveAction(int pId);
        vector<int> availableInterfaces(Packet *p);

        map<int, stack<tuple<vector<double>, int, int>>> actionStore; // packet id -> (context, action, dest_id)
        vector<int> actionInterfaces;
        LinUCB *agent;
        string agentAlg;

        bool dropAction;
        // agent variables
        double regularizer;
        double delta;
        double discountFactor;
        // state variables
        int numFlows;
        set<StateType> stateTypes;

    private:
        json &validateManhattanBanditDeflectionSwitchConfig(json &switchConfig);
};

class PQDRSwitch: public RandomForwardSwitch {
    public:
        PQDRSwitch(json &switchConfig);

        virtual bool initSwitch();
        virtual void startSwitch();

    protected:
        int routePacket(Packet *p, int sourceInterfaceId);

        double learningRate; // alpha
        double recoveryLearningRate; // beta
        double discountFactor; // gamma
        double timeWindow; // used to reset discarded and transmitted packets counters

        // tables index dest, link
        vector<vector<double>> tableQ;
        vector<vector<double>> tableB;
        vector<vector<double>> tableR;
        vector<vector<second_t>> tableU;
        vector<double> tableP;

        bool blockInterface(int interfaceId, int flowId, int burstId, int packetId);
        void freeBlockedInterface(int flowId, int burstId);
        int getReservedInterface(int flowId, int burstId);
        void updateIfFree(int interfaceId);

        // the maps are used in conjunction
        // maps the interface id to the reserved flow id and burst id and last packet id to travel through
        map<int, tuple<int, int, int>> blockedInterfaces;
        // a map of flow id and burst id to blocked interfaces
        map<pair<int, int>, int> burstInterfaces;
        // a map of flow id and burst ids that are to be dropped to the time they were last encountered
        map<pair<int, int>, second_t> dropBursts;

        vector<int> discardedPackets;
        vector<int> transmittedPackets;

        default_random_engine generator;

    private:
        json &validatePQDRSwitchConfig(json &switchConfig);
};

#ifdef _TEST

int testSwitch();

#endif /* _TEST */

Switch *createSwitch(json &switchNetConfig);

#endif /* SWITCH_H */
