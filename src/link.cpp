#include <nlohmann/json.hpp>
#include <set>
#include <map>
#include <queue>

#include "link.h"
#include "packet.h"
#include "interface.h"
#include "manager.h"

using namespace std;
using json = nlohmann::json;

void LinkQueue::txPacketIfaceEvent() {
    // move packet from top of queue onto Iface
    LinkPacket p = pQueue.top();
    pQueue.pop();

    Interface *dest = man.getInterface(destID);
    // TODO error check
    dest->rxLink(p.packet);

    // if queue not empty create new event
    if (!pQueue.empty()) {
        second_t nextTx = pQueue.top().arriveTime;
        EventI *e = new Event<LinkQueue>(nextTx, &LinkQueue::txPacketIfaceEvent, this);
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

Link::Link(int id, int speed, second_t txTime): NetworkObject(id) {
    this->speed = speed;
    this->txTime = txTime;
}

void Link::addToInterfaces() {
    //TODO: maybe this vector should be stored in the object, it seems weird to create the map from a vector
    // and then throw it away, only to recreate it again here, however this is the only place it is needed
    // so it may be unecessary to store it in memory
    vector<int> neighbours;
    for (auto it: dests) {
        neighbours.push_back(it.first);
    }
    for (int i: neighbours) {
        Interface* interface = man.getInterface(i);
        interface->setLink(id, neighbours);
    }
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

bool Link::addDest(int ifaceID) {
    LinkQueue lq = LinkQueue();
    lq.destID = ifaceID;

    Interface *iface = man.getInterface(ifaceID);
    if (iface == NULL ||
            !dests.emplace(ifaceID, lq).second) {
        return false;
    }

    iface->setLink(id);
    return true;
}

bool Link::hasInterface(int ifaceID) {
    return dests.find(ifaceID) != dests.end();
}

// return 1 if interface was connected to link
int Link::removeDest(int interfaceId) {
    return dests.erase(interfaceId);
}

set<int> Link::getNeighbours() {
    set<int> neighbours;
    Interface *iface;

    for (auto& [id, lQueue] : dests) {
        iface = man.getInterface(id);
        neighbours.insert(iface->getHandlerID());
    }

    return neighbours;
}

bool Link::validateLinkQueues() {
    bool valid = true;

    for (auto& [ifaceID, linkQueue] : dests) {
        if (ifaceID != linkQueue.destID) {
            fprintf(stderr, "Link: link dest id %d and linkQueue dest id %d differ\n",
                    ifaceID, linkQueue.destID);
            valid = false;
        }

        Interface *iface = man.getInterface(ifaceID);
        if (iface == NULL) {
            fprintf(stderr, "Link: interface %d of link %d is missing\n",
                    ifaceID, id);
            valid = false;
        } else {
            if (!(iface->getLinkID() == id)) {
                fprintf(stderr, "Link: interface %d does not know of link %d",
                    ifaceID, id);
                valid = false;
            }
        }
    }

    return valid;
}

bool Link::validateVariables() {
    bool valid = true;

    if (speed < 1) {
        fprintf(stderr, "Link: %d has invalid speed %d\n", id, speed);
        valid = false;
    }

    if (txTime < 0) {
        fprintf(stderr, "Link: %d has invalid tx time %f\n", id, txTime);
        valid = false;
    }

    return valid;
}

bool Link::validate() {
    bool valid = true;

    if (!validateLinkQueues()) {
        valid = false;
    }

    if (!validateVariables()) {
        valid = false;
    }

    return valid;
}

#ifdef _TEST
#include <assert.h>

int testLink() {
    const int i1ID = 1,
          i1HID = 1,
          i1LBSize = 1,
          i1HBsize = 1;
    const int l1ID = 1,
          l1Speed = 80000;
    const second_t l1TxTime = 0.0005;

    Interface *i1 = new Interface(i1ID, i1HID, i1LBSize, i1HBsize);
    assert(man.addInterface(i1));

    Link *l1 = new Link(l1ID, l1Speed, l1TxTime);
    assert(man.addLink(l1));

    assert(l1->getID() == l1ID);
    assert(l1->getSpeed() == l1Speed);

    assert(l1->addDest(i1ID));
    assert(!l1->addDest(i1ID));

    assert(l1->removeDest(i1ID));
    assert(!l1->removeDest(i1ID));

    man.deleteNetwork();

    return 1;
}
#endif /* _TEST */
