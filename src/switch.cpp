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

void Switch::setupRoutingTable() {
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
}

void Switch::initSwitch() {
    // set up routing table
    setupRoutingTable();
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

int testSwitch() {
    static const int s1ID = 1,
                 s1InternalSpeed = 100;
    string switchJson = "{\"id\":" + to_string(s1ID) + ",\"internal_speed\":" + to_string(s1InternalSpeed) + "}";
    Switch *s1 = new Switch(json::parse(switchJson));

    assert(s1->getID() == s1ID);
    assert(s1->getInternalSpeed() == s1InternalSpeed);

    delete s1;

    return 1;
}

#endif /* _TEST */
