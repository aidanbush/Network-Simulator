#ifndef LINK_H
#define LINK_H
#include <nlohmann/json.hpp>

#include <queue>
#include <map>

#include "networkObject.h"
#include "manager.h"

class Interface;
class Packet;

using namespace std;
using json = nlohmann::json;

class LinkQueue {
    public:
        int destId;

        void txPacketInterfaceEvent();// event to move packet to dest
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
        Link(json linkConfig);
        
        void txPacket(Packet *p, int sourceId);
        int addDest(int interfaceId);
        int removeDest(int interfaceId);
        
        int getSpeed();
        second_t getTxTime();
        
    private:
        map<int, LinkQueue> dests; // map interfaceID to LinkQueue
        int speed; // bits/second
        second_t txTime;
};

#ifdef _TEST
int testLink();
#endif /* _TEST */

#endif /* LINK_H */
