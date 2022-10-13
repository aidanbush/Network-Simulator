#include <map>
#include <set>
#include <nlohmann/json.hpp>
#include <torch/torch.h>

#include "switch.h"
#include "manager.h"
#include "interface.h"
#include "packet.h"
#include "packetHandler.h"
#include "endpoint.h"
#include "config.h"

#include "linUCB.h"

#define SWITCH_STR          "Switch"
#define SWITCH_RX_EVENT_STR "switch rx"

using namespace std;

using json = nlohmann::json;

enum SwitchType {
    BasicSwitchType,
    RandomDeflectSwitchType,
    MDCSwitchType,
    ManhattanBanditDeflectionSwitchType,
};

Switch *createSwitch(json &switchNetConfig) {
    static map<string, SwitchType> switchTypeMap = {
        {"basic", BasicSwitchType},
        {"rand_deflect", RandomDeflectSwitchType},
        {"mdc", MDCSwitchType},
        {"mbd", ManhattanBanditDeflectionSwitchType}
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
        case MDCSwitchType:
            netSwitch = new MDCSwitch(switchNetConfig);
            break;
        case ManhattanBanditDeflectionSwitchType:
            netSwitch = new ManhattanBanditDeflectionSwitch(switchNetConfig);
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
    // if timeout drop
    if (p->outOfTime()) {
        p->drop();
        man.logEvent(SWITCH_STR, id, SWITCH_RX_EVENT_STR, "Packet timeout drop");
        return;
    }

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
    p->decTTL();

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

pair<vector<int>, vector<int>> RandomDeflectionSwitch::generateRoutingLists(pair<int, int> destCoords) {
    pair<vector<int>, vector<int>> routes; // optimal, deflect

    if (destCoords == coords) {
        return routes;
    }

    int minDist = manhattanDistance(destCoords, coords);

    for (auto it : switchNeighbourIfaces) {
        pair<int, int> neighbourCoord = getCoords(it.second/*neighbouring switch*/, networkSize);
        int distance = manhattanDistance(destCoords, neighbourCoord);

        if (distance > minDist) {
            routes.second.push_back(it.first);
        } else if (distance == minDist) {
            routes.first.push_back(it.first);
        } else { // new min distance
            copy(routes.first.begin(), routes.first.end(), back_inserter(routes.second));
            routes.first.clear();
            routes.first.push_back(it.first);

            minDist = distance;
        }
    }

    return routes;
}

void RandomDeflectionSwitch::createManhattanRoutingTable() {
    pair<int, int> destCoords;
    // for none left and right
    for (int i = 0; i < 3; i++) {
        // for none up and down
        for (int j = 0; j < 3; j++) {
            destCoords.first = coords.first + ((i+1) %3) -1;
            destCoords.second = coords.second + ((j+1) %3) -1;

            manhattanRouting[i+j*3] = generateRoutingLists(destCoords);
        }
    }
}

bool RandomDeflectionSwitch::initSwitch() {
    bool ret = Switch::initSwitch();

    this->coords = getCoords(id, networkSize);
    createManhattanRoutingTable();

    generator.seed(man.random());

    return ret;
}

int RandomDeflectionSwitch::manhattanDistance(pair<int, int> coord1, pair<int, int> coord2) {
    // |x - x| + |y - y|
    return abs(coord1.first - coord2.first) + abs(coord1.second - coord2.second);
}

// blocked interfaces are not removed
pair<vector<int>, vector<int>> RandomDeflectionSwitch::availableRouteSets(Packet *p) {
    pair<int, int> dest = getCoords(p->getDest(), networkSize);

    int i = 0;

    if (dest.first > coords.first) { // if dest.x > x : right
        i += RIGHT;
    } else if (dest.first < coords.first) { // if dest.x < x : left
        i += LEFT;
    }
    if (dest.second > coords.second) { // if dest.y > y : up
        i += UP;
    } else if (dest.second < coords.second) { // if dest.y < y : down
        i += DOWN;
    }

    // use lookup tables for all 8 + 1 possible directions (+1 is at dest)
    return manhattanRouting[i];
}

int RandomDeflectionSwitch::routePacket(Packet *p) {
    pair<vector<int>, vector<int>> routes = availableRouteSets(p);

    // if both sets empty
    if (routes.first.empty() && routes.second.empty()) {
        // TODO this is a hack change it
        return Switch::routePacket(p);
    }

    vector<int> optimalInterfaces;
    vector<int> deflectInterfaces;

    for (auto it : routes.first) {
        // if room add to optimal
        Interface *interface = man.getInterface(it);
        if (interface->getOutBufferCurrentSize() + p->fullSize() <= interface->getOutBufferTotalSize()) {
            optimalInterfaces.push_back(it);
        }
    }

    // if not empty randomly send to one
    if (!optimalInterfaces.empty()) {
        return optimalInterfaces[generator() % optimalInterfaces.size()];
    }

    // deflect interfaces
    for (auto it : routes.second) {
        // if room add to deflect
        Interface *interface = man.getInterface(it);
        if (interface->getOutBufferCurrentSize() + p->fullSize() <= interface->getOutBufferTotalSize()) {
            deflectInterfaces.push_back(it);
        }
    }

    // if not empty randomly send to one
    if (!deflectInterfaces.empty()) {
        return deflectInterfaces[generator() % deflectInterfaces.size()];
    }

    // drop packet
    return NULL_ID;
}

/* MinimalDeflectionCostSwitch */

MDCSwitch::MDCSwitch(json &switchConfig):
    RandomDeflectionSwitch(validateMDCSwitchConfig(switchConfig)) {
}

json &MDCSwitch::validateMDCSwitchConfig(json &switchConfig) {
    return switchConfig;
}

void MDCSwitch::initializeInterfaceCost(Packet *p) {
    vector<int> allDeflectInterfaces = availableRouteSets(p).second;
    int flowId = p->getFlow();

    // for all interfaces add into flow map elements
    for (auto it : allDeflectInterfaces) {
        flowInterfaceCost[flowId][it] = {2.0, 0};
    }
}

int MDCSwitch::getLowestCostDeflect(Packet *p, vector<int> deflectInterfaces) {
    int flowId = p->getFlow();

    // if flow entry doesn't exists create
    if (flowInterfaceCost[flowId].empty()) {
        initializeInterfaceCost(p);
    }

    map<int, pair<double, int>> interfaceCosts = flowInterfaceCost[flowId];

    // go through list of deflect interfaces removing elements that are not max
    double minCost = interfaceCosts[deflectInterfaces[0]].first;
    vector<int> interfaces;
    for (auto it : deflectInterfaces) {
        double cost = interfaceCosts[it].first;
        // if < min cost replace
        if (cost < minCost) {
            minCost = cost;
            interfaces.clear();
            interfaces.push_back(it);
        } else if (cost == minCost) { // if = min cost add to
            interfaces.push_back(it);
        }
    }
    // randomly select from remaining interfaces
    return interfaces[generator() % interfaces.size()];
}

int MDCSwitch::routePacket(Packet *p) {
    pair<vector<int>, vector<int>> routes = availableRouteSets(p);

    // at destination send to endpoint
    if (routes.first.empty() && routes.second.empty()) {
        // TODO this is a hack change it
        return Switch::routePacket(p);
    }

    vector<int> optimalInterfaces;
    vector<int> deflectInterfaces;

    for (auto it : routes.first) {
        // if room add to optimal
        Interface *interface = man.getInterface(it);
        if (interface->getOutBufferCurrentSize() + p->fullSize() <= interface->getOutBufferTotalSize()) {
            optimalInterfaces.push_back(it);
        }
    }

    // if not empty randomly send to one
    if (!optimalInterfaces.empty()) {
        return optimalInterfaces[generator() % optimalInterfaces.size()];
    }

    // get list of interfaces it can deflect on
    for (auto it : routes.second) {
        // if room add to deflect
        Interface *interface = man.getInterface(it);
        if (interface->getOutBufferCurrentSize() + p->fullSize() <= interface->getOutBufferTotalSize()) {
            deflectInterfaces.push_back(it);
        }
    }

    // if empty drop
    if (deflectInterfaces.empty()) {
        return NULL_ID;
    }

    // else find flow or create entry
    // find lowest valued interface
    int deflectIface = getLowestCostDeflect(p, deflectInterfaces);

    // mark packet
    MDCPacket *MDCP = dynamic_cast<MDCPacket *>(p);
    MDCP->recordDeflection(id, deflectIface);

    return deflectIface;
}

void MDCSwitch::updateDeflectionCost(int flowId, int interfaceId, double cost) {
    if (flowInterfaceCost[flowId][interfaceId].second < alphaLimiter) {
        flowInterfaceCost[flowId][interfaceId].second++;
    }

    int n = flowInterfaceCost[flowId][interfaceId].second;

    flowInterfaceCost[flowId][interfaceId].first += (cost - flowInterfaceCost[flowId][interfaceId].first) / n;
}

/* ManhattanBanditDeflectionSwitch */

ManhattanBanditDeflectionSwitch::ManhattanBanditDeflectionSwitch(json &switchConfig):
    RandomDeflectionSwitch(switchConfig) {
    this->regularizer = switchConfig["regularizer"];
    this->delta = switchConfig["delta"];
    this->numFlows = switchConfig["num_flows"];
}

json &ManhattanBanditDeflectionSwitch::validateManhattanBanditDeflectionSwitchConfig(json &switchConfig) {
    string message = "";
    if (!hasMemberOfType(switchConfig, "regularizer", jsonDouble)) {
        message += "No double with name 'regularizer'.\n";
    }

    if (!hasMemberOfType(switchConfig, "delta", jsonDouble)) {
        message += "No double with name 'delta'.\n";
    }

    if (!hasMemberOfType(switchConfig, "num_flows", jsonInt)) {
        message += "No integer with name 'num_flows'.\n";
    }

    if (!message.empty()) {
        message = "ManhattanBanditDeflectionSwitch:\n" + message + switchConfig.dump(4);
        throw runtime_error(message);
    }

    return switchConfig;
}

bool ManhattanBanditDeflectionSwitch::initSwitch() {
    bool ret = RandomDeflectionSwitch::initSwitch();

    // TODO setup action interface list
    /*vector<int> actionInterfaces;*/ // add to class
    for (auto it: interfaces) {
        // if not endpoint TODO remove endpoints this is a hack
        if (man.getEndpoint(it.first) == NULL) {
            actionInterfaces.push_back(it.second);
        }
    }

    int observeDims = this->numFlows;/* number of flow */
    // neighbour Ifaces - destination iface
    int numActions = switchNeighbourIfaces.size();/* number of interfaces +1 if drop*/;
    int seed = man.random();/* get from manager random */
    // initialize agent
    agent = new LinUCB(observeDims, numActions, this->regularizer, this->delta, seed);

    return ret;
}

int ManhattanBanditDeflectionSwitch::routePacket(Packet *p) {
    // at destination send to endpoint
    if (getCoords(p->getDest(), networkSize) == coords) { // check if at dest TODO make check correct
        // TODO this is a hack change it
        return Switch::routePacket(p);
    }

    // get state
    vector<double> state;
    int flowId = p->getFlow();
    // flow ids go from 1-numFlows
    for (int i = 1; i <= this->numFlows; i++) {
        if (i == flowId) {
            state.push_back(1);
        } else {
            state.push_back(0);
        }
    }

    // select action using agent
    int action = agent->selectAction(state);

    // record action in switch and packet
    recordAction(p, state, action);

    if (action > this->numFlows) {
        return NULL_ID;
    }

    // convert action into interface
    return actionInterfaces[action];
}

void ManhattanBanditDeflectionSwitch::recordAction(Packet *p, vector<double> context, int action) {
    // TODO if exists add to queue else create queue
    actionStore.emplace(p->getId(), pair<vector<double>, int>{context, action});

    MBDPacket *MBDP = dynamic_cast<MBDPacket *>(p);
    MBDP->recordAction(id);
}

pair<vector<double>, int> ManhattanBanditDeflectionSwitch::retrieveAction(int pId) {
    auto it = actionStore.find(pId);
    if (it == actionStore.end()) {
        return {{}, -1};
    }

    // make copy
    vector<double> state = it->second.first;
    int action = it->second.second;
    // remove
    actionStore.erase(pId);
    // return
    return {state, action};
}

void ManhattanBanditDeflectionSwitch::rewardAction(int pId, double reward) {
    // get context action pair
    pair<vector<double>, int> stateAction = retrieveAction(pId);
    // if there is no action
    if (stateAction.second == -1) {
        return;
    }

    // apply update
    agent->updateAgent(stateAction.first, stateAction.second, reward);
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
