#include <nlohmann/json.hpp>
#include <set>
#include <map>
#include <queue>

#include "link.h"
#include "packet.h"
#include "interface.h"
#include "manager.h"
#include "config.h"

#define LINK_STR            "Link"
#define TX_PKT_EVENT_STR    "link tx packet"

using namespace std;
using json = nlohmann::json;

LinkQueue::LinkQueue(int destId, int linkId) {
    this->destId = destId;
    this->linkId = linkId;
}

void LinkQueue::txPacketIfaceEvent() {
    // move packet from top of queue onto Iface
    LinkPacket p = pQueue.top();
    pQueue.pop();

    man.logTxEvent(LINK_STR, linkId, TX_PKT_EVENT_STR, destId, p.packet);

    Interface *dest = man.getInterface(destId);
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

Link::Link(json &linkConfig): NetworkObject(validateLinkConfig(linkConfig)) {
    this->speed = linkConfig["speed"];
    this->txTime = linkConfig["time"];

    for (auto it: linkConfig["interfaces"].items()) {
        LinkQueue lq = LinkQueue(it.value(), id);
        dests.emplace(it.value(), lq);
    }
}

int Link::validateLinkConfig(json &linkConfig) {
    string message = "";
    if (!hasMemberOfType(linkConfig, "id", jsonInt)) {
        message += "No integer with name 'id'.\n";
    }

    if (!hasMemberOfType(linkConfig, "speed", jsonInt)) {
        message += "No integer with name 'speed'.\n";
    }

    if (!hasMemberOfType(linkConfig, "time", jsonDouble)) {
        message += "No integer with name 'time'.\n";
    }

    if (!hasMemberOfType(linkConfig, "interfaces", jsonArray)) {
        message += "No array with name 'interfaces'.\n";
    } else if (!checkArrayType(linkConfig["interfaces"], jsonInt)) {
        message += "Array 'interfaces' has non integer entry.\n";
    }

    if (!message.empty()) {
        message = "Link:\n" + message + linkConfig.dump(4);
        throw runtime_error(message);
    }
    return linkConfig["id"];
}

bool Link::addToInterfaces() {
    //TODO: maybe this vector should be stored in the object, it seems weird to create the map from a vector
    // and then throw it away, only to recreate it again here, however this is the only place it is needed
    // so it may be unecessary to store it in memory
    vector<int> neighbours;
    for (auto it: dests) {
        neighbours.push_back(it.first);
    }
    for (int i: neighbours) {
        Interface *interface = man.getInterface(i);
        if (interface == NULL) {
            return false;
        }

        if (!interface->setLink(id, neighbours)) {
            return false;
        }
    }
    return true;
}

int Link::getSpeed() {
    return speed;
}

second_t Link::getTxTime() {
    return txTime;
}

void Link::txPacket(Packet *p, int sourceId) {
    bool first = true;

    // enqueue packet into queue for destinations
    for (auto& [id, lQueue] : dests) {
        if (id != sourceId) {
            // clone packet if not first
            if (!first) {
                p = p->clone();
            }
            lQueue.txPacket(p, txTime);
            first = false;
        }
    }
}

bool Link::hasInterface(int interfaceId) {
    return dests.find(interfaceId) != dests.end();
}

bool Link::removeDest(int interfaceId) {
    return dests.erase(interfaceId);
}

set<int> Link::getNeighbours() {
    set<int> neighbours;
    Interface *interface;

    for (auto& [id, lQueue] : dests) {
        interface = man.getInterface(id);
        neighbours.insert(interface->getHandlerId());
    }

    return neighbours;
}

bool Link::validateLinkQueues() {
    bool valid = true;

    for (auto& [interfaceId, linkQueue] : dests) {
        if (interfaceId != linkQueue.destId) {
            fprintf(stderr, "Link: link dest id %d and linkQueue dest id %d differ\n",
                    interfaceId, linkQueue.destId);
            valid = false;
        }

        Interface *interface = man.getInterface(interfaceId);
        if (interface == NULL) {
            fprintf(stderr, "Link: interface %d of link %d is missing\n",
                    interfaceId, id);
            valid = false;
        } else {
            if (!(interface->getLinkId() == id)) {
                fprintf(stderr, "Link: interface %d does not know of link %d",
                    interfaceId, id);
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
#include "tests/throwAssert.h"

int testLink() {
    const int i1Id = 1,
          i1HId = 1,
          i1LBSize = 1,
          i1HBsize = 1;
    const int l1Id = 1,
          l1Speed = 80000;
    const second_t l1TxTime = 0.0005;

    json interfaceJson = {
        {"id", i1Id},
        {"handler_id", i1HId},
        {"out_buf_size", i1LBSize},
        {"in_buf_size", i1HBsize}
    };
    Interface *i1 = new Interface(interfaceJson);
    throwAssert(man.addInterface(i1));

    json linkJson = {
        {"id", l1Id},
        {"speed", l1Speed},
        {"time", l1TxTime},
        {"interfaces", {i1Id}}
    };
    Link *l1 = new Link(linkJson);
    l1->addToInterfaces();
    throwAssert(man.addLink(l1));

    throwAssert(l1->getId() == l1Id);
    throwAssert(l1->getSpeed() == l1Speed);

    throwAssert(l1->removeDest(i1Id));
    throwAssert(!l1->removeDest(i1Id));

    man.deleteNetwork();

    return 1;
}
#endif /* _TEST */
