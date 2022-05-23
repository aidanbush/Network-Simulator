#ifndef SWITCH_H
#define SWITCH_H

#include <nlohmann/json.hpp>
#include <map>
#include <queue>
#include <set>
#include <random>

#include "packetHandler.h"

using namespace std;

using json = nlohmann::json;

class Switch: public PacketHandler {
    public:
        Switch(json &switchConfig);

        void rxPacket(Packet *p);

        bool initSwitch();

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
        vector<int> switchNeighbourIfaces; // all the interfaces that connect to a switch

        int routePacket(Packet *p);

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

        bool initSwitch();

    protected:
        double deflectThresh;

        default_random_engine generator;

        map<int, vector<int>> rerouteLists;

        int routePacket(Packet *p);

        void setRerouteLists();

    private:
        json &validateRandomDeflectionSwitchConfig(json &switchConfig);
};

#ifdef _TEST

int testSwitch();

#endif /* _TEST */

Switch *createSwitch(json &switchNetConfig);

#endif /* SWITCH_H */
