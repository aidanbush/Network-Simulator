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

#define SWITCH_STR          "Switch"
#define SWITCH_RX_EVENT_STR "switch rx"

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
    // TODO if interface id == -1 then drop the packet
    if (interfaceId == NULL_ID) {
        p->drop();
        man.logEvent(SWITCH_STR, id, SWITCH_RX_EVENT_STR, "Packet dropped");
        return;
    }

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
    set<pair<int, int>> interfaceSet;

    // for all interfaces
    for (auto const& [handlerId, interfaceId] : interfaces) {
        // if handler is an switch then add to set
        if (man.getSwitch(handlerId) != NULL) {
            interfaceSet.insert({interfaceId, handlerId});
        }
    }

    switchNeighbourIfaces.resize(interfaceSet.size());
    // convert to vector
    copy(interfaceSet.begin(), interfaceSet.end(), switchNeighbourIfaces.begin());
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

    // TODO set coordinates based on id and network size
    networkSize = switchConfig["network_size"];
}

json &RandomDeflectionSwitch::validateRandomDeflectionSwitchConfig(json &switchConfig) {
    string message = "";

    if (!hasMemberOfType(switchConfig, "deflect_thresh", jsonDouble)) {
        // TODO error
        message += "No double with name 'deflect_thresh'.\n";
    }

    if (!hasMemberOfType(switchConfig, "network_size", jsonInt)) {
        // TODO error
        message += "No int with name 'network_size'.\n";
    }

    if (!message.empty()) {
        message = "RandomDeflectionSwitch:\n" + message + switchConfig.dump(4);
        throw runtime_error(message);
    }

    return switchConfig;
}

pair<int, int> RandomDeflectionSwitch::getCoords(int netId, int networkSize) {
    int n = networkSize + 1;
    return {(netId / n) % n, netId % n}; // x, y
}

bool RandomDeflectionSwitch::initSwitch() {
    bool ret = Switch::initSwitch();

    this->coords = getCoords(id, networkSize);
    generator.seed(man.random());

    return ret;
}

int RandomDeflectionSwitch::manhattanDistance(pair<int, int> coord1, pair<int, int> coord2) {
    // |x - x| + |y - y|
    return abs(coord1.first - coord2.first) + abs(coord1.second - coord2.second);
}

int RandomDeflectionSwitch::routePacket(Packet *p) {
    vector<int> optimalInterfaces;
    vector<int> remainingInterfaces;

    pair<int, int> destinationCoords = getCoords(p->getDest(), networkSize);

    // if destination is the current node then pass to endpoint
    // TODO fix this hack
    if (destinationCoords == coords) {
        return Switch::routePacket(p);
    }

    int minDist = manhattanDistance(destinationCoords, coords);

    for (auto it : switchNeighbourIfaces) {
        // if room in buffer
        Interface *interface = man.getInterface(it.first);
        // this is wrong does not let anything in
        // TODO check if there is room for the packet in the buffer
        if (interface->getOutBufferCurrentSize() + p->fullSize() <= interface->getOutBufferTotalSize()) {
            pair<int, int> neighbourCoord = getCoords(it.second/*neighbouring switch*/, networkSize);
            int distance = manhattanDistance(destinationCoords, neighbourCoord);

            if (distance > minDist) {
                remainingInterfaces.push_back(it.first);
            } else if (distance == minDist) {
                optimalInterfaces.push_back(it.first);
            } else { // new min distance
                copy(optimalInterfaces.begin(), optimalInterfaces.end(), back_inserter(remainingInterfaces));
                optimalInterfaces.clear();
                optimalInterfaces.push_back(it.first);

                minDist = distance;
            }
        }
    }

    if (!optimalInterfaces.empty()) {
        return optimalInterfaces[generator() % optimalInterfaces.size()];
    }

    if (!remainingInterfaces.empty()) {
        return remainingInterfaces[generator() % remainingInterfaces.size()];
    }

    return NULL_ID;
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
