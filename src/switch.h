#ifndef SWITCH_H
#define SWITCH_H

#include "packetHandler.h"

using namespace std;

class Switch: public PacketHandler {
    public:
        Switch(int id, int internalSpeed);

        void rxPacket(Packet *p);

        void initSwitch();

        Interface *getIface(int destID);

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
