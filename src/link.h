#ifndef LINK_H
#define LINK_H

#include <queue>
#include <unordered_map>

#include "networkObject.h"
#include "packet.h"
#include "interface.h"

class Interface;

using namespace std;

class LinkQueue {
    public:
        Interface *dest;

        void txPacketIfaceEvent();// event to move packet to dest
        void txPacket(Packet *p, int txTime);

    private:
        struct LinkPacket {
            Packet *packet;
            int arriveTime;
        };

        struct LinkPacketCmp {
            bool operator()(const LinkPacket lhs, const LinkPacket rhs) const {
                return lhs.arriveTime > rhs.arriveTime;
            }
        };

        priority_queue<LinkPacket, vector<LinkPacket>, LinkPacketCmp> pQueue; // queue
};

class Link: public NetworkObject {
    public:
        void txPacket(Packet *p, int sourceID);

    private:
        unordered_map<int, LinkQueue> dests; // map sourceID to LinkQueue
        int speed; // bits/second
        int txTime; // microseconds
};

#endif // LINK_H
