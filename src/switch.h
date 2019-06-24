#ifndef SWITCH_H
#define SWITCH_H
#include <nlohmann/json.hpp>

#include <map>
#include <queue>
#include <set>

#include "packetHandler.h"

using namespace std;

using json = nlohmann::json;

class Switch: public PacketHandler {
    public:
        Switch(int id, int speed);

        void rxPacket(Packet *p);

        void initSwitch();

        int getInterfaceId(int destID);

    protected:
        struct routingSearchElem {
            double cost;
            int curID;
            int firstID;
            friend bool operator<(const routingSearchElem lhs, const routingSearchElem rhs) {
                return lhs.cost > rhs.cost;
            }
        };

        map<int, int> routingTable; // dest ID to interface ID

        int routePacket(Packet *p);

        static double txCost(Switch *source, int destID);
        static double txCost(Interface *iface);

        static void initializeNeighbours(priority_queue<routingSearchElem> &fringe,
                Switch *netSwitch);
        static void addNeighbours(priority_queue<routingSearchElem> &fringe,
                set<int> &explored, routingSearchElem curElem);

        void setupRoutingTable();
};

#ifdef _TEST

int testSwitch();

#endif /* _TEST */

#endif /* SWITCH_H */
