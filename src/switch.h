#ifndef SWITCH_H
#define SWITCH_H

#include <nlohmann/json.hpp>
#include <map>
#include <queue>
#include <set>
#include <random>
#include <stack>

#include "packetHandler.h"

class LinUCB;

using namespace std;

using json = nlohmann::json;

class Switch: public PacketHandler {
    public:
        Switch(json &switchConfig);

        void rxPacket(Packet *p);

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
        /*
        struct routingSearchElem {
            double cost;
            int curId;
            int firstId;
            friend bool operator<(const routingSearchElem lhs, const routingSearchElem rhs) {
                return lhs.cost > rhs.cost;
            }
        };
        */

        map<int, pair<double, set<int>>> routingTable; // dest Id to cost and interface Id's
        vector<pair<int, int>> switchNeighbourIfaces; // all the interfaces that connect to a switch (interface id, switch id)

        virtual int routePacket(Packet *p);

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
        int routePacket(Packet *p);

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

        int routePacket(Packet *p);

        void setRerouteLists();

    private:
        json &validateRandomDeflectionSwitchConfig(json &switchConfig);
};

class ManhattanBanditDeflectionSwitch: public RandomDeflectionSwitch {
    public:
        ManhattanBanditDeflectionSwitch(json &switchConfig);

        bool initSwitch();
        void startSwitch();

        void rewardAction(int pId, double reward);

    protected:
        enum StateType {
            flowIdState,
            destIdState,
            hop1ShortState,
            hop1_2ShortState,
            hop2ShortState
        };

        map<int, set<int>> createShortestLookupTable(vector<int> switches);

        // state variables
        int hop1ShortStateDims;
        int hop1_2ShortStateDims;
        int hop2ShortStateDims;

        map<int, set<int>> hop1ShortStateMap; // destination to index of closest switches
        map<int, set<int>> hop1_2ShortStateMap; // destination to index of closest switches
        map<int, set<int>> hop2ShortStateMap; // destination to index of closest switches

        int routePacket(Packet *p);

        void setupStates();
        int getNumDims();
        vector<double> getState(Packet *p);

        void recordAction(Packet *p, vector<double> context, int action);
        pair<vector<double>, int> retrieveAction(int pId);
        vector<int> availableInterfaces(Packet *p);

        map<int, stack<pair<vector<double>, int>>> actionStore; // packet id -> (context, action)
        vector<int> actionInterfaces;
        LinUCB *agent;

        bool dropAction;
        // agent variables
        double regularizer;
        double delta;
        // state variables
        int numFlows;
        set<StateType> stateTypes;

    private:
        json &validateManhattanBanditDeflectionSwitchConfig(json &switchConfig);
};

#ifdef _TEST

int testSwitch();

#endif /* _TEST */

Switch *createSwitch(json &switchNetConfig);

#endif /* SWITCH_H */
