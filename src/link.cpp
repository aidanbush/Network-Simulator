#include <nlohmann/json.hpp>

#include "link.h"
#include "packet.h"
#include "interface.h"
#include "manager.h"

using namespace std;
using json = nlohmann::json;

void LinkQueue::txPacketInterfaceEvent() {
    // move packet from top of queue onto Iface
    LinkPacket p = pQueue.top();
    pQueue.pop();

    PacketHandler *dest = man.getHandler();
    // TODO error check
    dest->rxLink(p.packet);

    // if queue not empty create new event
    if (!pQueue.empty()) {
        second_t nextTx = pQueue.top().arriveTime;
        EventI *e = new Event<LinkQueue>(nextTx, &LinkQueue::txPacketInterfaceEvent, this);
        man.pushEvent(e);
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
        EventI *e = new Event<LinkQueue>(lp.arriveTime, &LinkQueue::txPacketInterfaceEvent, this);
        man.pushEvent(e);
    }
}

Link::Link(int id, int speed, second_t txTime): NetworkObject(id) {
    this->speed = speed;
    this->txTime = txTime;
}

int Link::getSpeed() {
    return speed;
}

second_t Link::getTxTime() {
    return txTime;
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

bool Link::addDest(int interfaceId) {
    LinkQueue lq = LinkQueue();
    lq.destId = interfaceId;
    return dests.emplace(interfaceId, lq).second;
}

// return 1 if interface was connected to link
int Link::removeDest(int interfaceId) {
    return dests.erase(interfaceId);
}

#ifdef _TEST
#include <assert.h>

int testLink() {
    const int i1ID = 1;
    const int l1ID = 1,
          l1Speed = 80000;
    const second_t l1TxTime = 0.0005;

    Link *l1 = new Link(l1ID, l1Speed, l1TxTime);

    assert(l1->getID() == l1ID);
    assert(l1->getSpeed() == l1Speed);

    assert(l1->addDest(i1ID));
    assert(!l1->addDest(i1ID));

    assert(l1->removeDest(i1->getID()));
    assert(!l1->removeDest(i1->getID()));

    // test txPacket

    delete l1;

    return 1;
}
#endif /* _TEST */
