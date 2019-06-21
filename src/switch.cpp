#include <map>
#include <set>

#include "switch.h"
#include "manager.h"
#include "interface.h"
#include "packet.h"
#include "packetHandler.h"
#include "endpoint.h"

using namespace std;

Switch::Switch(int id, int speed) {
    this->id = id;
    this->internalSpeed = speed;
}

void Switch::rxPacket(Packet *p) {
    int ifaceID = routePacket(p);

    ifaces[ifaceID]->rxHandler(p);
}

int Switch::routePacket(Packet *p) {
    auto destID = routingTable.find(p->getDest());
    if (destID == routingTable.end()) {
        // TODO handle error
    }

    return destID->second;
}

Interface *Switch::getIface(int destID) {
    auto elem = ifaces.find(destID);
    if (elem == ifaces.end()) {
        return NULL;
    }
    return elem->second;
}

// time / speed
double Switch::txCost(Switch *source, int destID) {
    Interface *iface = source->getIface(destID);

    return Switch::txCost(iface);
}

double Switch::txCost(Interface *iface) {
    return iface->getLinkTxTime() / iface->getLinkSpeed();
}

void Switch::initializeNeighbours(priority_queue<routingSearchElem> &fringe,
        Switch *netSwitch) {
    map<Interface *, PacketHandler *> neighbours = netSwitch->getIfaceNeighbours();
    double cost;
    routingSearchElem newElem;

    for (auto const& [iface, handler] : neighbours) {
        cost = Switch::txCost(iface);

        newElem = {
            .cost = cost,
            .curID = handler->getID(),
            .firstID = iface->getID(),
        };

        fringe.push(newElem);
    }
}

// only add neighbours ir switch
void Switch::addNeighbours(priority_queue<routingSearchElem> &fringe,
        set<int> &explored, routingSearchElem curElem) {
    Switch *netSwitch = man.getSwitch(curElem.curID);// = dynamic_cast<Switch *>(curElem.handler);
    if (netSwitch == NULL) {
        return;
    }

    double cost;
    int id;
    routingSearchElem newElem;

    vector<PacketHandler *> neighbours = netSwitch->getNeighbours();

    for (PacketHandler *n : neighbours) {
        id = n->getID();
        if (explored.find(id) != explored.end()) {
            continue;
        }

        cost = Switch::txCost(netSwitch, n->getID()) + curElem.cost;

        // add to
        newElem = {
            .cost = cost,
            .curID = n->getID(),
            .firstID = curElem.firstID,
        };

        fringe.push(newElem);
    }
}

void Switch::setupRoutingTable() {
    priority_queue<routingSearchElem> fringe;
    set<int> explored; // explored packetHandlers
    routingSearchElem curElem;

    explored.insert(id);

    // add neigbours
    Switch::initializeNeighbours(fringe, this);

    // while fringe not empty
    while (!fringe.empty()) {
        routingSearchElem elem = fringe.top();
        fringe.pop();

        // continue if not new element
        if (!explored.insert(elem.curID).second) {
            continue;
        }

        //if (dynamic_cast<Endpoint *>(elem.second.handler) != NULL) {
        if (man.getEndpoint(elem.curID) != NULL) {
            routingTable.insert(pair<int, int>(elem.curID, elem.firstID));
        } else if (man.getEndpoint(elem.curID) != NULL) {
            Switch::addNeighbours(fringe, explored, elem);
        } else {
            // TODO: handle error
        }
    }
}

void Switch::initSwitch() {
    // set up routing table
    setupRoutingTable();
}

#ifdef _TEST

#include <assert.h>

int testSwitch() {
    static const int s1ID = 1,
                 s1InternalSpeed = 100;
    Switch *s1 = new Switch(s1ID, s1InternalSpeed);

    assert(s1->getID() == s1ID);
    assert(s1->getInternalSpeed() == s1InternalSpeed);

    delete s1;

    return 1;
}

#endif /* _TEST */
