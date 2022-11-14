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

        virtual bool initSwitch();

        int getInterfaceId(int destId);

        bool validate();

#ifdef _TEST
        static int testRoutingTableSearch();
#endif /* _TEST */

    protected:
        struct routingSearchElem {
            double cost;
            int curId;
            int firstId;
            friend bool operator<(const routingSearchElem lhs, const routingSearchElem rhs) {
                return lhs.cost > rhs.cost;
            }
        };

        map<int, int> routingTable; // dest Id to interface Id
        vector<pair<int, int>> switchNeighbourIfaces; // all the interfaces that connect to a switch (interface id, switch id)

        virtual int routePacket(Packet *p);

        static double txCost(Switch *source, int destId);
        static double txCost(Interface *interface);

        static void initializeNeighbours(priority_queue<routingSearchElem> &fringe,
                Switch *netSwitch);
        static void addNeighbours(priority_queue<routingSearchElem> &fringe,
                set<int> &explored, routingSearchElem curElem);

        bool setupRoutingTable();
        void printRoutingTable();

        void setNeighbours();

    private:
        static json &validateSwitchConfig(json &switchConfig);
};

class RandomDeflectionSwitch: public Switch {
    public:
        RandomDeflectionSwitch(json &switchConfig);

        virtual bool initSwitch();

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

class MDCSwitch: public RandomDeflectionSwitch {
    public:
        MDCSwitch(json &switchConfig);

        void updateDeflectionCost(int flowId, int interfaceId, double cost);
    protected:
        // flowId -> interfaceId -> (cost, deflectCount)
        map<int, map<int, pair<double, int>>> flowInterfaceCost;
        int alphaLimiter = 1000;

        void initializeInterfaceCost(Packet *p);
        int getLowestCostDeflect(Packet *p, vector<int> deflectInterfaces);

        int routePacket(Packet *p);

    private:
        json &validateMDCSwitchConfig(json &switchConfig);
};

class ManhattanBanditDeflectionSwitch: public RandomDeflectionSwitch {
    public:
        ManhattanBanditDeflectionSwitch(json &switchConfig);

        bool initSwitch();

        void rewardAction(int pId, double reward);

    protected:
        int routePacket(Packet *p);

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

    private:
        json &validateManhattanBanditDeflectionSwitchConfig(json &switchConfig);
};

#ifdef _TEST

int testSwitch();

#endif /* _TEST */

Switch *createSwitch(json &switchNetConfig);

#endif /* SWITCH_H */
