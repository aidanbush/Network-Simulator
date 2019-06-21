#ifndef SWITCH_H
#define SWITCH_H
#include <nlohmann/json.hpp>

#include "packetHandler.h"

using namespace std;

using json = nlohmann::json;

class Switch: public PacketHandler {
    public:
        Switch(json switchConfig);

        void rxPacket(Packet *p);

        void initSwitch();

        int getInterfaceId(int destID);

    protected:
        struct routingSearchElem {
            double cost;
            PacketHandler *handler;
            int firstID;
            bool operator()(const routingSearchElem lhs, const routingSearchElem rhs) {
                return lhs.cost > rhs.cost;
            }
        };

        map<int, int> routingTable; // dest ID to interface ID

        int routePacket(Packet *p);

        static double txCost(Switch *source, int destID);
        static double txCost(Interface *iface);
        static void initializeNeighbours(map<int, routingSearchElem> &fringe,
                Switch *netSwitch);
        static void addNeighbours(map<int, routingSearchElem> &fringe,
                routingSearchElem curElem);

        void setupRoutingTable();
};

#endif // SWITCH_H
