#include "link.h"
#include "manager.h"

using namespace std;

void LinkQueue::txPacketIfaceEvent() {
    // move packet from top of queue onto Iface
    LinkPacket p = pQueue.top();
    pQueue.pop();
    dest->rxLink(p.packet);

    // if queue not empty create new event
    if (!pQueue.empty()) {
        second_t nextTx = pQueue.top().arriveTime;
        EventI *e = new Event<LinkQueue>(nextTx, &LinkQueue::txPacketIfaceEvent, this);
        man.pq.push(e);
    }
}

void LinkQueue::txPacket(Packet *p, second_t txTime) {
    LinkPacket lp = {
        .packet = p,
        .arriveTime = man.time + txTime,
    };

    pQueue.push(lp);

    // if now one element add tx event
    if (pQueue.size() == 1) {
        EventI *e = new Event<LinkQueue>(lp.arriveTime, &LinkQueue::txPacketIfaceEvent, this);
        man.pq.push(e);
    }
}

int Link::getSpeed() {
    return speed;
}

void Link::txPacket(Packet *p, int sourceID) {
    bool first = true;

    // enqueue packet into queue for destinations
    for (auto& [id, lQueue] : dests) {
        if (id != sourceID) {
            // clone packet if not first
            if (!first) {
                p = p->clone();
            }
            lQueue.txPacket(p, txTime);
            first = false;
        }
    }
}
