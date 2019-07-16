#include <map>
#include <set>
#include <nlohmann/json.hpp>

#include "switch.h"
#include "manager.h"
#include "interface.h"
#include "packet.h"
#include "packetHandler.h"
#include "endpoint.h"
#include "config.h"

using namespace std;

using json = nlohmann::json;

Switch::Switch(json &switchConfig): PacketHandler(validateSwitchConfig(switchConfig)) {}

json &Switch::validateSwitchConfig(json &switchConfig) {
    string message = "";
    if (!hasMemberOfType(switchConfig, "id", jsonInt)) {
        message += "No integer with name 'id'.\n";
    }

    if (!hasMemberOfType(switchConfig, "internal_speed", jsonInt)) {
        message += "No integer with name 'internal_speed'.\n";
    }

    if (!message.empty()) {
        message = "Switch:\n" + message + switchConfig.dump(4);
        throw runtime_error(message);
    }
    return switchConfig;
}

void Switch::rxPacket(Packet *p) {
    int ifaceId = routePacket(p);

    Interface *iface = man.getInterface(ifaceId);

    iface->rxHandler(p);
}

int Switch::routePacket(Packet *p) {
    auto destId = routingTable.find(p->getDest());
    if (destId == routingTable.end()) {
        // TODO handle error
    }

    return destId->second;
}

int Switch::getInterfaceId(int destId) {
    auto elem = interfaces.find(destId);
    if (elem == interfaces.end()) {
        return -1;
    }

    return elem->second;
}

// time / speed
double Switch::txCost(Switch *source, int destId) {
    int ifaceId = source->getInterfaceId(destId);
    Interface *iface = man.getInterface(ifaceId);

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

    for (auto const& [handlerId, ifaceId] : neighbours) {
        iface = man.getInterface(ifaceId);
        cost = Switch::txCost(iface);

        newElem = {
            .cost = cost,
            .curId = handlerId,
            .firstId = ifaceId,
        };

        fringe.push(newElem);
    }
}

// only add neighbours ir switch
void Switch::addNeighbours(priority_queue<routingSearchElem> &fringe,
        set<int> &explored, routingSearchElem curElem) {
    Switch *netSwitch = man.getSwitch(curElem.curId);
    if (netSwitch == NULL) {
        return;
    }

    vector<int> neighbours = netSwitch->getNeighbours();
    double cost;
    routingSearchElem newElem;

    for (int neighbourId : neighbours) {
        if (explored.find(neighbourId) != explored.end()) {
            continue;
        }

        cost = Switch::txCost(netSwitch, neighbourId) + curElem.cost;

        // add to
        newElem = {
            .cost = cost,
            .curId = neighbourId,
            .firstId = curElem.firstId,
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
        if (!explored.insert(curElem.curId).second) {
            continue;
        }

        if (man.getEndpoint(curElem.curId) != NULL) {
            routingTable.insert(pair<int, int>(curElem.curId, curElem.firstId));
        } else if (man.getSwitch(curElem.curId) != NULL) {
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
    static const int s1Id = 1,
                 s1InternalSpeed = 100;
    json switchJson = {
        {"id", s1Id},
        {"internal_speed", s1InternalSpeed}
    };
    Switch *s1 = new Switch(switchJson);

    assert(s1->getId() == s1Id);
    assert(s1->getInternalSpeed() == s1InternalSpeed);

    delete s1;

    return Switch::testRoutingTableSearch();
}

#endif /* _TEST */
