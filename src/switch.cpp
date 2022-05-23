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

enum SwitchType {
    BasicSwitchType,
    RandomDeflectSwitchType,
};

Switch *createSwitch(json &switchNetConfig) {
    static map<string, SwitchType> switchTypeMap = {
        {"basic", BasicSwitchType},
        {"rand_deflect", RandomDeflectSwitchType},
    };


    if (!hasMemberOfType(switchNetConfig, "type", jsonString)) {
        throw runtime_error("Switch:\nNo string with name 'type'\n" + switchNetConfig.dump(4));
    }

    string switchTypeString = switchNetConfig["type"];
    SwitchType switchType;
    try {
        switchType = switchTypeMap.at(switchTypeString);
    } catch (out_of_range&) {
        throw runtime_error("Switch:\nInvalid switch type: " + switchTypeString);
    }

    Switch *netSwitch;

    switch (switchType) {
        case BasicSwitchType:
            netSwitch = new Switch(switchNetConfig);
            break;
        case RandomDeflectSwitchType:
            netSwitch = new RandomDeflectionSwitch(switchNetConfig);
            break;
        default:
            throw runtime_error("Switch:\nInvalid switch type: " + switchTypeString);
    }

    return netSwitch;
}

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
    int interfaceId = routePacket(p);

    Interface *interface = man.getInterface(interfaceId);

    // handle packets
    //tagPacketOut(p, interface);

    interface->rxHandler(p);
}

int Switch::routePacket(Packet *p) {
    auto destId = routingTable.find(p->getDest());
    if (destId == routingTable.end()) {
        throw runtime_error("Switch: routePacket: not able to route to destination\n");
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
    int interfaceId = source->getInterfaceId(destId);
    Interface *interface = man.getInterface(interfaceId);

    return Switch::txCost(interface);
}

double Switch::txCost(Interface *interface) {
    return interface->getLinkTxTime() / interface->getLinkSpeed();
}

void Switch::initializeNeighbours(priority_queue<routingSearchElem> &fringe,
        Switch *netSwitch) {
    double cost;
    routingSearchElem newElem;
    Interface *interface;

    map<int, int> neighbours = netSwitch->interfaces;

    for (auto const& [handlerId, interfaceId] : neighbours) {
        interface = man.getInterface(interfaceId);
        cost = Switch::txCost(interface);

        newElem = {
            .cost = cost,
            .curId = handlerId,
            .firstId = interfaceId,
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

void Switch::setNeighbours() {
    set<int> interfaces;
    // for all interfaces
    for (auto const& [handlerId, interfaceId] : routingTable) {
        // if handler is an switch then add to set
        if (man.getSwitch(handlerId) != NULL) {
            interfaces.insert(interfaceId);
        }
    }

    // convert to vector
    copy(interfaces.begin(), interfaces.end(), switchNeighbourIfaces.begin());
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
    setNeighbours();
    return setupRoutingTable();
}

bool Switch::validate() {
    bool valid = true;

    if (!validateHandler()) {
        valid = false;
    }

    return valid;
}

/* RandomDeflectionSwitch */

RandomDeflectionSwitch::RandomDeflectionSwitch(json &switchConfig):
    Switch(validateRandomDeflectionSwitchConfig(switchConfig)) {
    // set threshold
    this->deflectThresh = switchConfig["deflect_thresh"];
}

json &RandomDeflectionSwitch::validateRandomDeflectionSwitchConfig(json &switchConfig) {
    string message = "";

    if (!hasMemberOfType(switchConfig, "deflect_thresh", jsonDouble)) {
        // TODO error
        message += "No double with name 'deflect_thresh'.\n";
    }

    if (!message.empty()) {
        message = "RandomDeflectionSwitch:\n" + message + switchConfig.dump(4);
        throw runtime_error(message);
    }

    return switchConfig;
}

bool RandomDeflectionSwitch::initSwitch() {
    bool ret = Switch::initSwitch();
    generator.seed(man.random());
    setRerouteLists();
    return ret;
}

void RandomDeflectionSwitch::setRerouteLists() {
    // create rerouteLists
    for (int i = 0; i < switchNeighbourIfaces.size(); i++) {
        // create vector missing element i
        vector<int> newNeighbours;
        for (int j = 0; j < switchNeighbourIfaces.size(); j++) { // copy all but element i
            if (j == i) {
                continue;
            }
            newNeighbours.emplace_back(switchNeighbourIfaces[j]);
        }

        rerouteLists.emplace(switchNeighbourIfaces[i], newNeighbours);
    }
}

int RandomDeflectionSwitch::routePacket(Packet *p) {
    int routeIfaceId = Switch::routePacket(p);
    int nextIfaceId = routeIfaceId;

    // check if >= threshold
    // get interface
    Interface *interface = man.getInterface(routeIfaceId);
    // check how full and compare
    if ((double(interface->getOutBufferCurrentSize()) / interface->getOutBufferTotalSize()) >= deflectThresh) {
        // if empty dont reroute
        if (rerouteLists[routeIfaceId].size() > 0) {
            // randomly deflect
            nextIfaceId = rerouteLists[routeIfaceId][generator() % rerouteLists[routeIfaceId].size()];
        }
    }

    return nextIfaceId;
}

#ifdef _TEST

#include "config.h"
#include "tests/throwAssert.h"

int Switch::testRoutingTableSearch() {
    // create network
    loadConfig("switchTest.json");

    man.startSimulator();

    // test routes
    Switch *s = man.getSwitch(1);
    throwAssert(s->routingTable.find(7)->second == 4);
    throwAssert(s->routingTable.find(8)->second == 1);
    throwAssert(s->routingTable.find(9)->second == 1);
    throwAssert(s->routingTable.find(10)->second == 2);
    throwAssert(s->routingTable.find(11)->second == 3);

    s = man.getSwitch(2);
    throwAssert(s->routingTable.find(7)->second == 5);
    throwAssert(s->routingTable.find(8)->second == 7);
    throwAssert(s->routingTable.find(9)->second == 8);
    throwAssert(s->routingTable.find(10)->second == 6);
    throwAssert(s->routingTable.find(11)->second == 5);

    s = man.getSwitch(3);
    throwAssert(s->routingTable.find(7)->second == 9);
    throwAssert(s->routingTable.find(8)->second == 10);
    throwAssert(s->routingTable.find(9)->second == 10);
    throwAssert(s->routingTable.find(10)->second == 11);
    throwAssert(s->routingTable.find(11)->second == 9);

    s = man.getSwitch(4);
    throwAssert(s->routingTable.find(7)->second == 12);
    throwAssert(s->routingTable.find(8)->second == 12);
    throwAssert(s->routingTable.find(9)->second == 12);
    throwAssert(s->routingTable.find(10)->second == 12);
    throwAssert(s->routingTable.find(11)->second == 13);

    s = man.getSwitch(5);
    throwAssert(s->routingTable.find(7)->second == 14);
    throwAssert(s->routingTable.find(8)->second == 15);
    throwAssert(s->routingTable.find(9)->second == 14);
    throwAssert(s->routingTable.find(10)->second == 14);
    throwAssert(s->routingTable.find(11)->second == 14);

    s = man.getSwitch(6);
    throwAssert(s->routingTable.find(7)->second == 16);
    throwAssert(s->routingTable.find(8)->second == 16);
    throwAssert(s->routingTable.find(9)->second == 16);
    throwAssert(s->routingTable.find(10)->second == 16);
    throwAssert(s->routingTable.find(11)->second == 17);

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

    throwAssert(s1->getId() == s1Id);
    throwAssert(s1->getInternalSpeed() == s1InternalSpeed);

    delete s1;

    return Switch::testRoutingTableSearch();
}

#endif /* _TEST */
