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

Switch::Switch(json switchConfig): PacketHandler(switchConfig) {}

void Switch::rxPacket(Packet *p) {
    int interfaceId = routePacket(p);
    //TODO: get interface from global map
    //interfaces[interfaceId]->rxHandler(p);
}

int Switch::routePacket(Packet *p) {
    auto destID = routingTable.find(p->getDest());
    if (destID == routingTable.end()) {
        // TODO handle error
    }

    return destID->second;
}

int Switch::getInterfaceId(int destID) {
    auto elem = interfaces.find(destID);
    if (elem == interfaces.end()) {
        return -1;
    }
    return elem->second;
}

// time / speed
double Switch::txCost(Switch *source, int destID) {
    //TODO: get interface from global map
    //Interface *interface = source->getInterfaceId(destID);

    return 0.0;//Switch::txCost(interface);
}

double Switch::txCost(Interface *iface) {
    return iface->getLinkTxTime() / iface->getLinkSpeed();
}

void Switch::initializeNeighbours(map<int, routingSearchElem> &fringe,
        Switch *netSwitch) {
    //TODO: need replacement for getIfaceNeighbours, which is incompatible with multiple destination links
            //as it returned a map of interfaces to destination packet handlers
    // map<Interface *, PacketHandler *> neighbours = netSwitch->getIfaceNeighbours();
    // map<Interface *, PacketHandler *>::iterator it;
    // double cost;
    // routingSearchElem newElem;
    //
    // for (auto const& [iface, handler] : neighbours) {
    //     cost = Switch::txCost(iface);
    //
    //     auto dupElem = fringe.find(handler->getID());
    //     // if cost > current cost continue
    //     if (dupElem != fringe.end() && cost > dupElem->second.cost) {
    //         continue;
    //     }
    //
    //     newElem = {
    //         .cost = cost,
    //         .handler = handler,
    //         .firstID = iface->getID(),
    //     };
    //     // add to
    //     fringe.insert({handler->getID(), newElem});
    // }
}

// only add neighbours ir switch
void Switch::addNeighbours(map<int, routingSearchElem> &fringe,
        routingSearchElem curElem) {
    Switch *netSwitch = dynamic_cast<Switch *>(curElem.handler);
    if (netSwitch == NULL) {
        return;
    }

    vector<int> neighbours = netSwitch->getNeighbours();
    double cost;
    routingSearchElem newElem;

    for (int n: neighbours) {
        //TODO: get interface from global map
        // cost = Switch::txCost(netSwitch, n->getID()) + curElem.cost;
//
//         auto dupElem = fringe.find(n->getID());
//         // if cost > current cost continue
//         if (dupElem != fringe.end() && cost > dupElem->second.cost) {
//             continue;
//         }
//         // add to
//         newElem = {
//             .cost = cost,
//             .handler = n,
//             .firstID = curElem.firstID,
//         };
    }
}

void Switch::setupRoutingTable() {
    // TODO currently does not behave properly
    map<int, routingSearchElem> fringe; // cost sorted priority queue mapping destID to (cost, obj, ifaceID)
    set<int> explored; // explored packetHandlers
    routingSearchElem curElem;

    explored.insert(id);

    // add neigbours
    Switch::initializeNeighbours(fringe, this);

    // while fringe not empty
    while (fringe.begin() != fringe.end()) {
        auto elem = fringe.begin();
        explored.insert(elem->first);
        fringe.erase(elem->first);

        if (dynamic_cast<Endpoint *>(elem->second.handler) != NULL) {
            routingTable.insert(pair<int, int>(elem->first, elem->second.firstID));
        } else {
            Switch::addNeighbours(fringe, elem->second);
        }
    }
}

void Switch::initSwitch() {
    // set up routing table
    setupRoutingTable();
}
