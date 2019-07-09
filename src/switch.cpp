#include <map>
#include <set>
#include <nlohmann/json.hpp>

#include "switch.h"
#include "manager.h"
#include "interface.h"
#include "packet.h"
#include "packetHandler.h"
#include "endpoint.h"

using namespace std;

using json = nlohmann::json;

Switch::Switch(int id, int speed): PacketHandler(id, speed) {}

void Switch::rxPacket(Packet *p) {
    int ifaceID = routePacket(p);

    Interface *iface = man.getInterface(ifaceID);

    iface->rxHandler(p);
}

int Switch::routePacket(Packet *p) {
    auto destID = routingTable.find(p->getDest());
    if (destID == routingTable.end()) {
        // TODO handle error
    }

    return destID->second;
}

int Switch::getInterfaceID(int destID) {
    auto elem = interfaces.find(destID);
    if (elem == interfaces.end()) {
        return -1;
    }

    return elem->second;
}

// time / speed
double Switch::txCost(Switch *source, int destID) {
    int ifaceID = source->getInterfaceID(destID);
    Interface *iface = man.getInterface(ifaceID);

    return Switch::txCost(iface);
}

double Switch::txCost(Interface *iface) {
    return iface->getLinkTxTime() / iface->getLinkSpeed();
}

void Switch::initializeNeighbours(priority_queue<routingSearchElem> &fringe,
        Switch *netSwitch) {
    double cost;
    routingSearchElem newElem;
    Interface *iface;

    map<int, int> neighbours = netSwitch->interfaces;

    for (auto const& [handlerID, ifaceID] : neighbours) {
        iface = man.getInterface(ifaceID);
        cost = Switch::txCost(iface);

        newElem = {
            .cost = cost,
            .curID = handlerID,
            .firstID = ifaceID,
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

    vector<int> neighbours = netSwitch->getNeighbours();
    double cost;
    routingSearchElem newElem;

    for (int neighbourID : neighbours) {
        if (explored.find(neighbourID) != explored.end()) {
            continue;
        }

        cost = Switch::txCost(netSwitch, neighbourID) + curElem.cost;

        // add to
        newElem = {
            .cost = cost,
            .curID = neighbourID,
            .firstID = curElem.firstID,
        };

        fringe.push(newElem);
    }
}

bool Switch::setupRoutingTable() {
    priority_queue<routingSearchElem> fringe;
    set<int> explored; // explored packetHandlers
    routingSearchElem curElem;

    explored.insert(id);

    // add neigbours
    Switch::initializeNeighbours(fringe, this);

    // while fringe not empty
    while (!fringe.empty()) {
        curElem = fringe.top();
        fringe.pop();

        // continue if not new element
        if (!explored.insert(curElem.curID).second) {
            continue;
        }

        //if (dynamic_cast<Endpoint *>(elem.second.handler) != NULL) {
        if (man.getEndpoint(curElem.curID) != NULL) {
            routingTable.insert(pair<int, int>(curElem.curID, curElem.firstID));
        } else if (man.getSwitch(curElem.curID) != NULL) {
            Switch::addNeighbours(fringe, explored, curElem);
        } else {
            // TODO: handle error
            fprintf(stderr, "Error in routing UCS unkown packetHandler type\n");
        }
    }

    // validate routing table
    vector<int> endpoints = man.getEndpoints();
    for (int eId : endpoints) {
        if (routingTable.find(eId) == routingTable.end()) {
            fprintf(stderr, "Error: Switch: %d routing table does not have endpoint %d\n", id, eId);
            return false;
        }
    }

    return true;
}

void Switch::printRoutingTable() {
    printf("switch: %d routingTable:\n", id);
    for (auto it : routingTable) {
        printf("\tdest: %d interface: %d\n", it.first, it.second);
    }
}

bool Switch::initSwitch() {
    return setupRoutingTable();
}

bool Switch::validate() {
    bool valid = true;

    if (!validateHandler()) {
        valid = false;
    }

    return valid;
}

#ifdef _TEST

#include <assert.h>
#include "config.h"

int Switch::testRoutingTableSearch() {
    // create network
    loadConfig("switchTest.json");

    man.startSimulator();

    // test routes
    Switch *s = man.getSwitch(1);
    assert(s->routingTable.find(7)->second == 4);
    assert(s->routingTable.find(8)->second == 1);
    assert(s->routingTable.find(9)->second == 1);
    assert(s->routingTable.find(10)->second == 2);
    assert(s->routingTable.find(11)->second == 3);

    s = man.getSwitch(2);
    assert(s->routingTable.find(7)->second == 5);
    assert(s->routingTable.find(8)->second == 7);
    assert(s->routingTable.find(9)->second == 8);
    assert(s->routingTable.find(10)->second == 6);
    assert(s->routingTable.find(11)->second == 5);

    s = man.getSwitch(3);
    assert(s->routingTable.find(7)->second == 9);
    assert(s->routingTable.find(8)->second == 10);
    assert(s->routingTable.find(9)->second == 10);
    assert(s->routingTable.find(10)->second == 11);
    assert(s->routingTable.find(11)->second == 9);

    s = man.getSwitch(4);
    assert(s->routingTable.find(7)->second == 12);
    assert(s->routingTable.find(8)->second == 12);
    assert(s->routingTable.find(9)->second == 12);
    assert(s->routingTable.find(10)->second == 12);
    assert(s->routingTable.find(11)->second == 13);

    s = man.getSwitch(5);
    assert(s->routingTable.find(7)->second == 14);
    assert(s->routingTable.find(8)->second == 15);
    assert(s->routingTable.find(9)->second == 14);
    assert(s->routingTable.find(10)->second == 14);
    assert(s->routingTable.find(11)->second == 14);

    s = man.getSwitch(6);
    assert(s->routingTable.find(7)->second == 16);
    assert(s->routingTable.find(8)->second == 16);
    assert(s->routingTable.find(9)->second == 16);
    assert(s->routingTable.find(10)->second == 16);
    assert(s->routingTable.find(11)->second == 17);

    man.deleteNetwork();

    return 1;
}

int testSwitch() {
    static const int s1ID = 1,
                 s1InternalSpeed = 100;
    Switch *s1 = new Switch(s1ID, s1InternalSpeed);

    assert(s1->getID() == s1ID);
    assert(s1->getInternalSpeed() == s1InternalSpeed);

    delete s1;

    return Switch::testRoutingTableSearch();
}

#endif /* _TEST */
