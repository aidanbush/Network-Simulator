#ifndef LINK_H
#define LINK_H
#include <nlohmann/json.hpp>

#include <queue>
#include <map>
#include <set>

#include "networkObject.h"
#include "manager.h"

class Interface;
class Packet;

using namespace std;
using json = nlohmann::json;

class LinkQueue {
    public:
        LinkQueue(int destId, int linkId);

        int destId;
        int linkId;

        void txPacketIfaceEvent(); // event to move packet to dest
        void txPacket(Packet *p, second_t txTime);

    private:
        struct LinkPacket {
            Packet *packet;
            second_t arriveTime;
        };

        struct LinkPacketCmp {
            bool operator()(const LinkPacket lhs, const LinkPacket rhs) const {
                return lhs.arriveTime > rhs.arriveTime;
            }
        };

        priority_queue<LinkPacket, vector<LinkPacket>, LinkPacketCmp> pQueue;
};

class Link: public NetworkObject {
    public:
        Link(json &LinkConfig);

        bool addToInterfaces();

        void txPacket(Packet *p, int sourceId);

        bool hasInterface(int ifaceId);
        bool removeDest(int interfaceId);

        int getSpeed();
        second_t getTxTime();

        set<int> getNeighbours();

        bool validate();

    private:
        static int validateLinkConfig(json &linkConfig);

        bool validateLinkQueues();
        bool validateVariables();

        map<int, LinkQueue> dests; // map interfaceId to LinkQueue
        int speed; // bits/second
        second_t txTime;
};

#ifdef _TEST
int testLink();
#endif /* _TEST */

#endif /* LINK_H */
