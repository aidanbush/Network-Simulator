#include "link.h"
#include "manager.h"

using namespace std;

Link::Link(int id, int speed, second_t txTime) {
    this->id = id;
    this->speed = speed;
    this->txTime = txTime;
}

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

int Link::addDest(Interface *iface) {
    if (dests.find(iface->getID()) != dests.end()) {
        return 0;
    }

    dests[iface->getID()] = LinkQueue();
    dests[iface->getID()].dest = iface;

    return 1;
}

// return 1 if interface was connected to link
int Link::removeDest(int ifaceID) {
    return dests.erase(ifaceID);
}

#ifdef _TEST
#include <assert.h>

#define L_ID    1
#define SPEED   8000000
#define TX_TIME ((second_t)0.0005)

int testLink() {
    Link *l1 = new Link(L_ID, SPEED, TX_TIME);

    assert(l1->getID() == L_ID);
    assert(l1->getSpeed() == SPEED);

    // test addDest
    Interface *I1 = new Interface();

    assert(l1->addDest(I1));
    assert(!l1->addDest(I1));

    assert(l1->removeDest(I1->getID()));
    assert(!l1->removeDest(I1->getID()));

    delete I1;

    // test txPacket

    delete l1;

    return 1;
}
#endif /* _TEST */
