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
    //TODO: this line doesn't work anymore, need to get the interface from the global map which isn't implemented yet
    //dest->rxLink(p.packet);
    
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
        EventI *e = new Event<LinkQueue>(lp.arriveTime, &LinkQueue::txPacketIfaceEvent, this);
        man.pushEvent(e);
    }
}

Link::Link(json linkConfig) : NetworkObject(linkConfig["id"]) {
    this->speed = linkConfig["speed"];
    this->txTime = linkConfig["txTime"];
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
    return dests.emplace(interfaceId, lq);
}

// return 1 if interface was connected to link
int Link::removeDest(int interfaceId) {
    return dests.erase(interfaceId);
}

#ifdef _TEST
#include <assert.h>

#define L_ID    1
#define SPEED   8000000
#define TX_TIME ((second_t)0.0005)

#define I_ID        1
#define I_LB_SIZE   48000
#define I_HB_SIZE   32000

int testLink() {
    Link *l1 = new Link(L_ID, SPEED, TX_TIME);

    assert(l1->getID() == L_ID);
    assert(l1->getSpeed() == SPEED);

    // test addDest
    Interface *i1 = new Interface(I_ID, l1, I_LB_SIZE, I_HB_SIZE);

    assert(l1->addDest(i1));
    assert(!l1->addDest(i1));

    assert(l1->removeDest(i1->getID()));
    assert(!l1->removeDest(i1->getID()));

    delete i1;

    // test txPacket

    delete l1;

    return 1;
}
#endif /* _TEST */
