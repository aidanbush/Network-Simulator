#include <map>
#include <set>
#include <limits>
#include <nlohmann/json.hpp>
#include <torch/torch.h>
#include <utility>

#include "switch.h"
#include "manager.h"
#include "interface.h"
#include "packet.h"
#include "packetHandler.h"
#include "endpoint.h"
#include "config.h"
#include "observer.h"
#include "flow.h"

#include "linUCB.h"
#include "NDDAgents.h"

#define SWITCH_STR          "Switch"
#define SWITCH_RX_EVENT_STR "switch rx"

#define REWARD_DROP             0
#define REWARD_INTENTIONAL_DROP 0.1
#define REWARD_ARRIVAL          1

using namespace std;

using json = nlohmann::json;

enum SwitchType {
    BasicSwitchType,
    RandomDeflectSwitchType,
    RandomForwardSwitchType,
    ManhattanBanditDeflectionSwitchType,
    NDDSwitchType,
};

Switch *createSwitch(json &switchNetConfig) {
    static map<string, SwitchType> switchTypeMap = {
        {"basic", BasicSwitchType},
        {"rand_deflect", RandomDeflectSwitchType},
        {"rand_forward", RandomForwardSwitchType},
        {"mbd", ManhattanBanditDeflectionSwitchType},
        {"NDD", NDDSwitchType},
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
        case RandomForwardSwitchType:
            netSwitch = new RandomForwardSwitch(switchNetConfig);
            break;
        case ManhattanBanditDeflectionSwitchType:
            netSwitch = new ManhattanBanditDeflectionSwitch(switchNetConfig);
            break;
        case NDDSwitchType:
            netSwitch = new NDDSwitch(switchNetConfig);
            break;
        default:
            throw runtime_error("Switch:\nInvalid switch type: " + switchTypeString);
    }

    return netSwitch;
}

Switch::Switch(json &switchConfig): PacketHandler(validateSwitchConfig(switchConfig)) {
    this->sampleIndex = 0;
}

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

double Switch::calculateStateEntropy(map<int, double> actionProbs) {
    double entropy = 0.0;
    for (auto const& [_, prob] : actionProbs) {
        entropy -= prob * log2(prob);
    }
    return entropy;
}

double Switch::calculateEntropy(map<vector<double>, map<int, int>> counts) {
    if (counts.size() == 0) {
        return NAN;
    }
    map<vector<double>, map<int, double>> actionProbs;
    map<vector<double>, int> stateCounts;
    int totalCount = 0;

    // precompute \pi(a|x) and C(x) and C(X) for d(x)
    for (auto const& [state, values] : counts) {
        // calculate C(x) and C(X)
        stateCounts[state] = 0;
        for (auto const& [_, actionCounts] : values) {
            stateCounts[state] += actionCounts;
        }
        totalCount += stateCounts[state];

        // calculate \pi(a|x)
        for (auto const& [action, actionCount] : values) {
            if (actionCount <= 0) {
                throw runtime_error("actionCount is 0 for some action, should be above 0");
            }
            actionProbs[state][action] = double(actionCount) / stateCounts[state];
        }
    }

    // calculate the weighted entropy
    double entropy = 0;
    for (auto const& [state, actionProbs] : actionProbs) {
        double stateProb = double(stateCounts[state]) / totalCount;
        double stateEntropy = this->calculateStateEntropy(actionProbs);

        entropy += stateProb * stateEntropy;
    }

    return entropy;
}

void Switch::resetData() {
    //for all neighbours
    for (auto it : switchNeighbourIfaces) {
        Interface *iface = man.getInterface(it.first);
        iface->resetLinkUsage();
    }

    this->droppedPackets = 0;
    this->timedOutPackets = 0;
    this->deflectedPackets = 0;
    this->forwardedPackets = 0;
    this->encounteredPackets = 0;
    this->actionablePackets = 0;
    this->availableForwardIfaceCount = 0;
    this->availableIfaceCount = 0;
}

void Switch::recordData() {
    double averageOutgoingLinkUsage = 0;

    for (auto it : switchNeighbourIfaces) {
        Interface *iface = man.getInterface(it.first);
        double usage = iface->getLinkUsage();
        averageOutgoingLinkUsage += usage;
        observer.logLinkData(id, it.second, "LinkUsage", usage);
    }

    averageOutgoingLinkUsage /= switchNeighbourIfaces.size();

    double availableIfaceRatio = 1;
    double availableForwardIfaceRatio = 1;
    if (actionablePackets != 0) {
        availableIfaceRatio = double(this->availableIfaceCount) / actionablePackets;
        availableForwardIfaceRatio = double(this->availableForwardIfaceCount) / actionablePackets;
    }

    observer.logSwitchData(id, "droppedPackets", droppedPackets);
    observer.logSwitchData(id, "timedOutPackets", timedOutPackets);
    observer.logSwitchData(id, "deflectedPackets", deflectedPackets);
    observer.logSwitchData(id, "forwardedPackets", forwardedPackets);
    observer.logSwitchData(id, "encounteredPackets", encounteredPackets);
    observer.logSwitchData(id, "actionablePackets", actionablePackets);
    observer.logSwitchData(id, "averageAvailableInterfacesRatio", availableIfaceRatio);
    observer.logSwitchData(id, "averageForwardInterfacesRatio", availableForwardIfaceRatio);
    observer.logSwitchData(id, "averageOutgoingLinkUsage", averageOutgoingLinkUsage);

    this->resetData();

    this->sampleIndex++;

    second_t nextRecord = man.time + man.miTime;
    EventI *e = new Event<Switch>(nextRecord, &Switch::recordData, this);
    man.pushEvent(e);
}

void Switch::dropPacket(Packet *p) {
    man.logEvent(SWITCH_STR, id, SWITCH_RX_EVENT_STR, "Packet route drop packet: " + to_string(p->getId()));
    droppedPackets++;
    p->drop();
}

void Switch::timeoutPacket(Packet *p) {
    man.logEvent(SWITCH_STR, id, SWITCH_RX_EVENT_STR, "Packet timeout drop packet: " + to_string(p->getId()));
    timedOutPackets++;
    dropPacket(p);
}

void Switch::deflectPacket(Packet *p) {
    deflectedPackets++;
}

void Switch::forwardPacket(Packet *p) {
    forwardedPackets++;
}

void Switch::arrivePacket(Packet *p) {
    man.logEvent(SWITCH_STR, id, SWITCH_RX_EVENT_STR, "Packet: " + to_string(p->getId()) + " arrived");
    p->arrive();
}

void Switch::updateForwardOrDeflect(Packet *p, int forwardInterfaceId) {
    set<int> shortestInterfaces = get<1>(routingTable[p->getDest()]);
    if (shortestInterfaces.find(forwardInterfaceId) != shortestInterfaces.end()) {
        forwardPacket(p);
    } else {
        deflectPacket(p);
    }
}

void Switch::rxPacket(Packet *p, int sourceInterfaceId) {
    encounteredPackets++;

    // if packet arrived then consume
    if (p->getDest() == id) {
        arrivePacket(p);
        return;
    }

    // if timeout drop
    if (p->outOfTime()) {
        timeoutPacket(p);
        return;
    }

    actionablePackets++;

    // record number of avaiable interfaces - all and forwarding
    for (int ifaceId : get<1>(routingTable[p->getDest()])) {
        Interface *iface = man.getInterface(ifaceId);
        if (iface->getOutBufferCurrentSize() + p->fullSize() <= iface->getOutBufferTotalSize()) {
            this->availableForwardIfaceCount++;
            this->availableIfaceCount++;
        }
    }

    for (int ifaceId : get<2>(routingTable[p->getDest()])) {
        Interface *iface = man.getInterface(ifaceId);
        if (iface->getOutBufferCurrentSize() + p->fullSize() <= iface->getOutBufferTotalSize()) {
            this->availableIfaceCount++;
        }
    }

    int interfaceId = routePacket(p, sourceInterfaceId);

    // if no interface selected, drop the packet
    if (interfaceId == NULL_ID) {
        dropPacket(p);
        return;
    }

    // check if valid interface for switch
    if (!this->hasInterface(interfaceId)) {
        throw runtime_error("Switch: rxPacket: trying to route to non existing interface");
    }

    // if in routing map then forward otherwise deflect
    updateForwardOrDeflect(p, interfaceId);

    Interface *interface = man.getInterface(interfaceId);

    // handle packets
    p->decTTL();

    man.logEvent(SWITCH_STR, id, SWITCH_RX_EVENT_STR, "Forwarding Packet: " + to_string(p->getId())
            + " to Interface: " + to_string(interfaceId));

    interface->rxHandler(p);
}

// send brand new packet, use rxPacket
void Switch::txPacket(Packet *p) {
    rxPacket(p, NULL_ID);
}

int Switch::routePacket(Packet *p, int sourceInterfaceId) {
    auto destId = routingTable.find(p->getDest());
    if (destId == routingTable.end()) {
        throw runtime_error("Switch: routePacket: not able to route to destination");
    }

    return *get<1>(destId->second).begin();
    //return *destId->second.second.begin();
}

int Switch::getInterfaceId(int destId) {
    auto elem = interfaces.find(destId);
    if (elem == interfaces.end()) {
        return -1;
    }

    return elem->second;
}

// time / speed
double Switch::txCost(int sourceId, int destId) {
    Switch *source = man.getSwitch(sourceId);
    return txCost(source, destId);
}

double Switch::txCost(Switch *source, int destId) {
    int interfaceId = source->getInterfaceId(destId);
    Interface *interface = man.getInterface(interfaceId);

    return Switch::txCost(interface);
}

double Switch::txCost(Interface *interface) {
    return 1.0;
    //return interface->getLinkTxTime() / double(interface->getLinkSpeed());
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

vector<int> Switch::getNeighbours(int handlerId) {
    PacketHandler *handler = man.getHandler(handlerId);
    return handler->getNeighbours();
}

int Switch::findSmallest(set<int> unvisited, map<int, double> dist) {
    int minId = *unvisited.begin();
    double minCost = dist[minId];

    for (int it: unvisited) {
        double newCost = dist[it];
        if (newCost < minCost) {
            minCost = newCost;
            minId = it;
        }
    }

    return minId;
}

// dijkstra
bool Switch::setupRoutingTable() {
    map<int, double> dist; // map destinations to lowest cost
    map<int, set<int>> prev; // map destinations to set of optimal interfaces
    set<int> unvisited;

    vector<int> handlers = man.getHandlers();

    for (int handler: handlers) {
        dist[handler] = numeric_limits<double>::infinity();
        prev[handler] = set<int>({});
        unvisited.emplace(handler);
    }
    dist[id] = 0;

    // setup neighbours and initial actions
    unvisited.erase(id);
    for (pair<int, int> neighbour: interfaces) {
        double altCost = dist[id] + txCost(id, neighbour.first);

        dist[neighbour.first] = altCost;
        prev[neighbour.first] = set<int>{neighbour.second};
    }

    while (!unvisited.empty()) {
        int handler = findSmallest(unvisited, dist);
        unvisited.erase(handler);

        vector<int> neighbours = getNeighbours(handler);

        for (int neighbourHandler: neighbours) {
            double altCost = dist[handler] + txCost(handler, neighbourHandler);

            if (altCost < dist[neighbourHandler]) {
                dist[neighbourHandler] = altCost;
                prev[neighbourHandler] = prev[handler];
            } else if (altCost == dist[neighbourHandler]) {
                prev[neighbourHandler].insert(prev[handler].begin(), prev[handler].end());
            }
        }
    }

    // create routing table
    for (int handlerId: handlers) {
        if (handlerId == id) {
            continue;
        }

        if (dist[handlerId] == numeric_limits<double>::infinity()) {
            return false;
        }

        vector<int> allInterfacesVec = this->getInterfaces();
        set<int> allInterfacesSet = set<int>(allInterfacesVec.begin(), allInterfacesVec.end());
        // todo this should be the current switches interfaces not the destination!!!
        set<int> nonOptimalInterfaces;/*all of the handlers interfaces - optimal interfaces*/
        set<int> optimalInterfaces = prev[handlerId];
        set_difference(allInterfacesSet.begin(), allInterfacesSet.end(),
                optimalInterfaces.begin(), optimalInterfaces.end(),
                inserter(nonOptimalInterfaces, nonOptimalInterfaces.begin()));

        routingTable[handlerId] = {dist[handlerId], optimalInterfaces, nonOptimalInterfaces};

        ostringstream optimalStream;
        copy(optimalInterfaces.begin(), optimalInterfaces.end(), ostream_iterator<int>(optimalStream, ","));
        string optimalString = optimalStream.str();

        ostringstream nonOptimalStream;
        copy(nonOptimalInterfaces.begin(), nonOptimalInterfaces.end(), ostream_iterator<int>(nonOptimalStream, ","));
        string nonOptimalString = nonOptimalStream.str();

        ostringstream allStream;
        copy(allInterfacesSet.begin(), allInterfacesSet.end(), ostream_iterator<int>(allStream, ","));
        string allString = allStream.str();


        man.logEvent(SWITCH_STR, id, "Switch: setupRoutingTable",
                "dest: " + to_string(handlerId) + " num optimal: " + to_string(optimalInterfaces.size()) +
                " num non-optimal: " + to_string(nonOptimalInterfaces.size()) +
                " optimal: " + optimalString + " non-optimal: " + nonOptimalString +
                " all interfaces " + allString);
    }

    return true;
}

void Switch::printRoutingTable() {
    printf("switch: %d routingTable:\n", id);
    for (auto it : routingTable) {
        printf("\tdest: %d cost: %e interfaces:", it.first, get<0>(it.second));
        for (int interface: get<1>(it.second)) {
            printf(" %d", interface);
        }
        printf("\n");
    }
}

double Switch::costToDest(int destId) {
    if (destId == this->id) {
        return 0.0;
    }
    return get<0>(routingTable[destId]);
}

bool Switch::initSwitch() {
    second_t nextRecord = man.time + man.miTime;
    EventI *e = new Event<Switch>(nextRecord, &Switch::recordData, this);
    man.pushEvent(e);

    setNeighbours();
    bool ret = setupRoutingTable();
    return ret;
}

void Switch::startSwitch() {
    this->resetData();
}

bool Switch::validate() {
    bool valid = true;

    if (!validateHandler()) {
        valid = false;
    }

    return valid;
}

/* RandomForwardSwitch */

RandomForwardSwitch::RandomForwardSwitch(json &switchConfig):
    Switch(validateRandomForwardSwitchConfig(switchConfig)){
}

json &RandomForwardSwitch::validateRandomForwardSwitchConfig(json &switchConfig) {
    return switchConfig;
}

bool RandomForwardSwitch::initSwitch() {
    bool ret = Switch::initSwitch();

    generator.seed(man.random());

    return ret;
}

void RandomForwardSwitch::startSwitch() {
    Switch::startSwitch();
}

int RandomForwardSwitch::routePacket(Packet *p, int sourceInterfaceId) {
    // go through forwarding ports and select aviable ones
    vector<int> forwardingIfaces;

    set<int> routingIfaces = get<1>(routingTable[p->getDest()]);
    for (int iface : routingIfaces) {
        // if free
        Interface *interface = man.getInterface(iface);
        if (interface->getOutBufferCurrentSize() + p->fullSize() <= interface->getOutBufferTotalSize()) {
            forwardingIfaces.push_back(iface);
        }
    }

    // if there are any
    if (!forwardingIfaces.empty()) {
        // randomly select one
        return forwardingIfaces[generator() % forwardingIfaces.size()];
    }

    // drop
    return NULL_ID;
}

/* RandomDeflectionSwitch */

RandomDeflectionSwitch::RandomDeflectionSwitch(json &switchConfig):
    Switch(validateRandomDeflectionSwitchConfig(switchConfig)) {
    // set threshold
    this->deflectThresh = switchConfig["deflect_thresh"];

    networkSize = switchConfig["network_size"];
}

json &RandomDeflectionSwitch::validateRandomDeflectionSwitchConfig(json &switchConfig) {
    string message = "";

    if (!hasMemberOfType(switchConfig, "deflect_thresh", jsonDouble)) {
        message += "No double with name 'deflect_thresh'.\n";
    }

    if (!hasMemberOfType(switchConfig, "network_size", jsonInt)) {
        message += "No int with name 'network_size'.\n";
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

    return ret;
}

void RandomDeflectionSwitch::startSwitch() {
    Switch::startSwitch();
}

int RandomDeflectionSwitch::routePacket(Packet *p, int sourceInterfaceId) {
    vector<int> optimalInterfaces;
    vector<int> deflectInterfaces;

    for (int iface: get<1>(this->routingTable.find(p->getDest())->second)) {
        // if room add to optimal
        Interface *interface = man.getInterface(iface);
        if (interface->getOutBufferCurrentSize() + p->fullSize() <= interface->getOutBufferTotalSize()) {
            optimalInterfaces.push_back(iface);
        }
    }

    // if not empty randomly send to one
    if (!optimalInterfaces.empty()) {
        return optimalInterfaces[generator() % optimalInterfaces.size()];
    }

    RandDeflectPacket *RDP = dynamic_cast<RandDeflectPacket *>(p);
    if (RDP == NULL) {
        throw runtime_error("RandomForwardSwitch:\nMBD switch passed a non MBD packet\n");
    }

    if (RDP->deflectionsRemaining() <= 0) {
        man.logEvent(SWITCH_STR, id, "RandomDeflectionSwitch: routePacket",
                "out of deflections dropping packet: " + to_string(RDP->getId()));
        p->setTimedOut(); // form of timeout
        return NULL_ID;
    }

    // deflect interfaces
    for (int iface: get<2>(this->routingTable.find(p->getDest())->second)) {
        // if room add to deflect
        Interface *interface = man.getInterface(iface);
        if (interface->getOutBufferCurrentSize() + p->fullSize() <= interface->getOutBufferTotalSize()) {
            deflectInterfaces.push_back(iface);
        }
    }

    // if not empty randomly send to one
    if (!deflectInterfaces.empty()) {
        RDP->recordDeflection();
        return deflectInterfaces[generator() % deflectInterfaces.size()];
    }

    // drop packet
    return NULL_ID;
}

/* ManhattanBanditDeflectionSwitch */

ManhattanBanditDeflectionSwitch::ManhattanBanditDeflectionSwitch(json &switchConfig):
    RandomDeflectionSwitch(validateManhattanBanditDeflectionSwitchConfig(switchConfig)) {
    static map<string, StateType> stateTypeMap = {
        {"flow_id", flowIdState},
        {"dest_id", destIdState},
        {"1_hop_shortest", hop1ShortState},
        {"1-2_hop_shortest", hop1_2ShortState},
        {"2_hop_shortest", hop2ShortState},
        {"section", sectionState},
        {"deflect_probability", deflectProbState},
        {"drop_probability", dropProbState},
    };
    static map<string, ActionLimit> actionLimitMap = {
        {"none", actionLimitNone},
        {"only_deflect", actionLimitOnlyDeflect},
        {"forward_first", actionLimitForwardFirst},
    };

    this->regularizer = switchConfig["regularizer"];
    this->delta = switchConfig["delta"];
    this->discountFactor = switchConfig["discount_factor"];
    this->numFlows = switchConfig["num_flows"];
    this->dropAction = switchConfig["drop_action"];
    this->agentAlg = switchConfig["agent_alg"];
    this->actionLimit = actionLimitMap[switchConfig["action_limit"]];
    this->section = switchConfig["section"];
    this->numSections = switchConfig["num_sections"];

    this->deflectProbTau = 1; // in seconds TODO use config file to import
    this->deflectProb = 0;
    this->dropProbTau = 1; // in seconds TODO use config file to import
    this->dropProb = 0;
    this->prevPacketArriveTime = man.time;

    // load in state
    for (json::iterator it = switchConfig["states"].begin(); it != switchConfig["states"].end(); ++it) {
        // map to type
        if (stateTypeMap.find(it.value()) ==  stateTypeMap.end()) {
            string message = SWITCH_STR + string(" state type ") + string(it.value()) + " is not supported\n";
            throw runtime_error(message);
        }
        StateType type = stateTypeMap[it.value()];
        stateTypes.emplace(type);// TODO sort to improve performance
    }

    if (stateTypes.empty()) {
        throw runtime_error("no states provided\n");
    }

    for (int i : switchConfig["entropy_intervals"]) {
        this->entropyIntervals.push_back(i);
    }
    this->rewardSum = 0.0;
    this->actionsRewarded = 0;
    this->learningActionsTaken = 0;
    for (int i : this->entropyIntervals) {
        this->entropyActionsTaken[i] = 0;
    }
}

void ManhattanBanditDeflectionSwitch::forwardPacket(Packet *p) {
    updateDeflectAndDropProbabilities(false, false);
    Switch::forwardPacket(p);
}

void ManhattanBanditDeflectionSwitch::deflectPacket(Packet *p) {
    updateDeflectAndDropProbabilities(true, false);
    Switch::deflectPacket(p);
}

void ManhattanBanditDeflectionSwitch::dropPacket(Packet *p) {
    //TODO ignore timed out packets
    updateDeflectAndDropProbabilities(true, true);
    Switch::dropPacket(p);
}

void ManhattanBanditDeflectionSwitch::arrivePacket(Packet *p) {
    updateDeflectAndDropProbabilities(false, false);
    Switch::arrivePacket(p);
}

void ManhattanBanditDeflectionSwitch::updateDeflectAndDropProbabilities(bool deflect, bool drop) {
    updateDeflectionProbability(deflect);
    updateDropProbability(drop);
    // update prev time
    this->prevPacketArriveTime = man.time;
}

void ManhattanBanditDeflectionSwitch::updateDeflectionProbability(bool deflect) {
    double deflectVal = deflect;
    // update avg
    double alpha = 1 - exp(-(man.time - this->prevPacketArriveTime) / this->deflectProbTau);
    this->deflectProb += alpha * (deflectVal - deflectProb);
}

void ManhattanBanditDeflectionSwitch::updateDropProbability(bool drop) {
    double dropVal = drop;
    // update avg
    double alpha = 1 - exp(-(man.time - this->prevPacketArriveTime) / this->dropProbTau);
    this->dropProb += alpha * (dropVal - dropProb);
}

json &ManhattanBanditDeflectionSwitch::validateManhattanBanditDeflectionSwitchConfig(json &switchConfig) {
    string message = "";
    if (!hasMemberOfType(switchConfig, "drop_action", jsonBool)) {
        message += "No bool with name 'drop_action'.\n";
    }

    if (!hasMemberOfType(switchConfig, "regularizer", jsonDouble)) {
        message += "No double with name 'regularizer'.\n";
    }

    if (!hasMemberOfType(switchConfig, "discount_factor", jsonDouble)) {
        message += "No double with name 'discount_factor'.\n";
    }

    if (!hasMemberOfType(switchConfig, "delta", jsonDouble)) {
        message += "No double with name 'delta'.\n";
    }

    if (!hasMemberOfType(switchConfig, "num_flows", jsonInt)) {
        message += "No integer with name 'num_flows'.\n";
    }

    if (!hasMemberOfType(switchConfig, "states", jsonArray)) {
        message += "No array with name 'states'";
    } else if(!checkArrayType(switchConfig["states"], jsonString)) {
        message += "Array states does not have all elements of type string";
    }

    if (!hasMemberOfType(switchConfig, "agent_alg", jsonString)) {
        message += "No string with name 'agent_alg'.\n";
    }

    if (!hasMemberOfType(switchConfig, "action_limit", jsonString)) {
        message += "No string with name 'agent_alg'.\n";
    }

    if (!hasMemberOfType(switchConfig, "section", jsonInt)) {
        message += "No int with name 'section'.\n";
    }

    if (!hasMemberOfType(switchConfig, "num_sections", jsonInt)) {
        message += "No int with name 'num_sections'.\n";
    }

    if (!hasMemberOfType(switchConfig, "entropy_intervals", jsonArray)) {
        message += "No array with name 'entropy_intervals'.\n";
    } else if(!checkArrayType(switchConfig["entropy_intervals"], jsonInt)) {
        message += "Array entropy_intervals does not have all elements of type string";
    }

    if (!message.empty()) {
        message = "ManhattanBanditDeflectionSwitch:\n" + message + switchConfig.dump(4);
        throw runtime_error(message);
    }

    return switchConfig;
}

bool ManhattanBanditDeflectionSwitch::initSwitch() {
    return RandomDeflectionSwitch::initSwitch();
}

void ManhattanBanditDeflectionSwitch::startSwitch() {
    RandomDeflectionSwitch::startSwitch();

    // setup action interface list
    for (auto it: interfaces) {
        this->actionInterfaces.push_back(it.second);
        man.logEvent(SWITCH_STR, id, "MBDSwitch: startSwitch actionInterfaces", "interface id: " +
                to_string(it.second));
    }

    for (size_t i = 0; i < this->actionInterfaces.size(); i++) {
        this->interfaceToAction.emplace(this->actionInterfaces[i], i);
    }

    // add drop action
    if (this->dropAction) {
        actionInterfaces.push_back(NULL_ID);
    }

    setupStates();

    int observeDims = getNumDims();
    man.logEvent(SWITCH_STR, id, "MBDSwitch: startSwitch:",
            "id: " + to_string(this->id) + " observeDims: " + to_string(observeDims));
    // neighbour Ifaces - destination iface
    int numActions;
    if (this->dropAction) {
        numActions = switchNeighbourIfaces.size() + 1;
    } else {
        numActions = switchNeighbourIfaces.size();
    }
    int seed = man.random();/* get from manager random */
    // initialize agent

    agent = new LinUCB(observeDims, numActions, this->regularizer, this->delta,
            this->discountFactor, this->agentAlg, seed);
}

void ManhattanBanditDeflectionSwitch::resetData() {
    for (int i : this->entropyIntervals) {
        if (sampleIndex % i == i - 1) {
            this->allActionsEntropyCounts[i].clear();
            this->deflectionEntropyCounts[i].clear();

            this->entropyActionsTaken[i] = 0;
        }
    }

    this->learningActionsTaken = 0;

    this->rewardSum = 0.0;
    this->actionsRewarded = 0;

    Switch::resetData();
}

void ManhattanBanditDeflectionSwitch::recordData() {
    // if entropyInterval'th sample - sampleIndex starts at 0
    double allEntropy = NAN;
    double deflectionEntropy = NAN;
    double entropyActions = NAN;
    double averageReward = NAN;
    if (actionsRewarded > 0) {
        averageReward = this->rewardSum / this->actionsRewarded;
    }

    for (int i : this->entropyIntervals) {
        if (this->sampleIndex % i == i - 1) {
            allEntropy = this->calculateEntropy(allActionsEntropyCounts[i]);
            deflectionEntropy = this->calculateEntropy(deflectionEntropyCounts[i]);
            entropyActions = double(this->entropyActionsTaken[i]);
        } else {
            allEntropy = NAN;
            deflectionEntropy = NAN;
            entropyActions = NAN;
        }

        observer.logSwitchData(id, "allActionsEntropy_" + to_string(i), allEntropy);
        observer.logSwitchData(id, "deflectionEntropy_" + to_string(i), deflectionEntropy);
        observer.logSwitchData(id, "numAvailableActions_" + to_string(i), switchNeighbourIfaces.size());
        observer.logSwitchData(id, "entropyActionsTaken_" + to_string(i), entropyActions);
    }


    observer.logSwitchData(id, "learningActionsTaken", this->learningActionsTaken);

    observer.logSwitchData(id, "averageReward", averageReward);
    observer.logSwitchData(id, "actionsRewarded", this->actionsRewarded);

    Switch::recordData();
}

map<int, set<int>> ManhattanBanditDeflectionSwitch::createShortestLookupTable(vector<int> switchIds) {
    map<int, set<int>> shortStateMap;
    // for all destinations
    vector<int> destinations = man.getHandlers();

    for (int dest: destinations) {
        if (dest == id) {
            continue;
        }

        set<int> closestSwitches = set<int>{0};
        int switchId = switchIds[0];
        Switch *s = man.getSwitch(switchId);

        double minCost = s->costToDest(dest);

        // for all closest switches
        for (size_t i = 1; i < switchIds.size(); i++) {
            Switch *s = man.getSwitch(switchIds[i]);
            double cost = s->costToDest(dest);

            if (cost < minCost) {
                closestSwitches = set<int>{i};
                minCost = cost;
            } else if (cost == minCost) {
                closestSwitches.insert(i);
            }
        }

        shortStateMap[dest] = closestSwitches;
    }

    return shortStateMap;
}

tuple<map<int, int>, int> ManhattanBanditDeflectionSwitch::shortestHopsStateMap(int lowHops, int highHops) {
    // get list of switches between low and high hops away
    set<int> switches;
    for (auto it : this->routingTable) {
        if (get<0>(it.second) >= lowHops && get<0>(it.second) <= highHops) {
            switches.insert(it.first);
        }
    }

    // map of destination to set of closets switches
    map<int, set<int>> closestSwitches;
    // loop through destinations
    for (auto it : this->routingTable) {
        int dest = it.first;

        // for each destination record set of closest switches
        // use neighbour switches routing table to get hops to dest
        set<int> closest;
        closest.insert(*switches.begin());
        Switch *netSwitch = man.getSwitch(*switches.begin());
        double closestHops = netSwitch->costToDest(dest);

        for (int switchId : switches) {
            netSwitch = man.getSwitch(switchId);
            double hops = netSwitch->costToDest(dest);

            if (hops < closestHops) {
                closest.clear();
                closest.insert(switchId);
                closestHops = hops;
            } else if (hops == closestHops) {
                closest.insert(switchId);
            }
        }

        closestSwitches.emplace(dest, closest);
    }

    set<set<int>> switchCombinations;
    // create set of set of switches to create ordering and index of these unique closest sets
    for (auto it: closestSwitches) {
        switchCombinations.insert(it.second);
    }

    map<set<int>, int> switchSetToState;// map switches to state index
    int stateIndex = 0;
    for (set<int> it : switchCombinations) {
        switchSetToState.emplace(it, stateIndex);
        stateIndex++;
    }

    // create map of destination index in the previous set
    map<int, int> destToState;
    for (auto it : this->routingTable) {
        int dest = it.first;
        destToState.emplace(dest, switchSetToState[closestSwitches[dest]]);
    }

    return {destToState, stateIndex};
}

void ManhattanBanditDeflectionSwitch::setupStates() {
    for (auto it: stateTypes) {
        switch (it) {
            case hop1ShortState:
                tie(this->hop1ShortStateMap, this->hop1ShortStateDims) = this->shortestHopsStateMap(1,1);
                break;
            case hop1_2ShortState:
                tie(this->hop1_2ShortStateMap, this->hop1_2ShortStateDims) = this->shortestHopsStateMap(1,2);
                break;
            case hop2ShortState:
                tie(this->hop2ShortStateMap, this->hop2ShortStateDims) = this->shortestHopsStateMap(2,2);
                break;
            case deflectProbState:
                // TODO create a map for all neighbours, initialize to 0
                deflectProbStateDims = switchNeighbourIfaces.size();
                break;
            case dropProbState:
                // TODO create a map for all neighbours, initialize to 0
                dropProbStateDims = switchNeighbourIfaces.size();
                break;
        }
    }
}

int ManhattanBanditDeflectionSwitch::getNumDims() {
    int numDims = 0;
    for (auto it: stateTypes) {
        switch (it) {
            case flowIdState:
                numDims += numFlows;
                break;
            case destIdState:
                numDims += networkSize * networkSize;
                break;
            case hop1ShortState:
                numDims += hop1ShortStateDims;
                break;
            case hop1_2ShortState:
                numDims += hop1_2ShortStateDims;
                break;
            case hop2ShortState:
                numDims += hop2ShortStateDims;
                break;
            case sectionState:
                numDims += this->numSections;
                break;
            case deflectProbState:
                numDims += deflectProbStateDims;
                break;
            case dropProbState:
                numDims += dropProbStateDims;
                break;
            default:
                throw runtime_error("ManhattanBanditDeflectionSwitch: getNumDims: unsupported state");
                break;
        }
    }

    return numDims;
}

vector<int> ManhattanBanditDeflectionSwitch::availableForwardInterfaces(Packet *p) {
    vector<int> available;

    for (int ifaceId : get<1>(this->routingTable[p->getDest()])) {
        Interface *interface = man.getInterface(ifaceId);
        if (interface->getOutBufferCurrentSize() + p->fullSize() <= interface->getOutBufferTotalSize()) {
            available.push_back(this->interfaceToAction[ifaceId]);
        }
    }

    return available;
}

vector<int> ManhattanBanditDeflectionSwitch::availableDeflectInterfaces(Packet *p) {
    vector<int> available;

    for (int ifaceId : get<2>(this->routingTable[p->getDest()])) {
        Interface *interface = man.getInterface(ifaceId);
        if (interface->getOutBufferCurrentSize() + p->fullSize() <= interface->getOutBufferTotalSize()) {
            available.push_back(this->interfaceToAction[ifaceId]);
        }
    }

    return available;
}

vector<int> ManhattanBanditDeflectionSwitch::availableInterfaces(Packet *p) {
    vector<int> available;

    for (size_t i = 0; i < this->actionInterfaces.size(); i++) {
        int ifaceId = this->actionInterfaces[i];
        // account for drop action
        if (ifaceId == NULL_ID) {
            available.push_back(i);
            continue;
        }

        Interface *interface = man.getInterface(ifaceId);
        // check if there is room to take the action
        if (interface->getOutBufferCurrentSize() + p->fullSize() <= interface->getOutBufferTotalSize()) {
            available.push_back(i);
        }
    }

    return available;
}

void ManhattanBanditDeflectionSwitch::setFlowIdState(vector<double> &state, Packet *p) {
    // flow state
    int flowId = p->getFlow();
    // flow ids go from 1-numFlows
    for (int i = 1; i <= this->numFlows; i++) {
        if (i == flowId) {
            state.push_back(1);
        } else {
            state.push_back(0);
        }
    }
}

void ManhattanBanditDeflectionSwitch::setDestIdState(vector<double> &state, Packet *p) {
    // destination state
    int dest = p->getDest();
    for (int i = 1; i <= this->networkSize * this->networkSize; i++) {
        if (i == dest) {
            state.push_back(1);
        } else {
            state.push_back(0);
        }
    }
}

void ManhattanBanditDeflectionSwitch::setHop1ShortState(vector<double> &state, Packet *p) {
    int stateOffset = state.size();

    for (int i = 0; i < hop1ShortStateDims; i++) {
        state.push_back(0);
    }

    /*if (!inHomeSector(p)) {
        return;
    }*/
    /*if (!inNeighbourSector(p)) {
        return;
    }*/

    int oneHotIndex = hop1ShortStateMap[p->getDest()];
    state[stateOffset + oneHotIndex] = 1;
}

void ManhattanBanditDeflectionSwitch::setHop1_2ShortState(vector<double> &state, Packet *p) {
    int stateOffset = state.size();

    for (int i = 0; i < hop1_2ShortStateDims; i++) {
        state.push_back(0);
    }

    int oneHotIndex = hop1_2ShortStateMap[p->getDest()];
    state[stateOffset + oneHotIndex] = 1;
}

void ManhattanBanditDeflectionSwitch::setHop2ShortState(vector<double> &state, Packet *p) {
    int stateOffset = state.size();

    for (int i = 0; i < hop2ShortStateDims; i++) {
        state.push_back(0);
    }

    int oneHotIndex = hop2ShortStateMap[p->getDest()];
    state[stateOffset + oneHotIndex] = 1;
}

void ManhattanBanditDeflectionSwitch::setSectionState(vector<double> &state, Packet *p) {
    int stateOffset = state.size();

    for (int i = 0; i < this->numSections; i++) {
        state.push_back(0);
    }

    ManhattanBanditDeflectionSwitch *destSwitch =
        dynamic_cast<ManhattanBanditDeflectionSwitch *>(man.getSwitch(p->getDest()));

    // dynamic cast to MBDSwitch
    int destSection = destSwitch->getSection();

    /*
    // if home section
    if (inHomeSector(destSection == this->section)) {
        return;
    }*/

    state[stateOffset + destSection] = 1;
}

void ManhattanBanditDeflectionSwitch::setDeflectProbState(vector<double> &state, Packet *p) {
    // go through all neighbours as set state to be the mean
    for (pair<int, int> neighbour : switchNeighbourIfaces) {
        ManhattanBanditDeflectionSwitch *neighbourSwitch =
            dynamic_cast<ManhattanBanditDeflectionSwitch *>(man.getSwitch(neighbour.second));
        double neighbourDeflectProb = neighbourSwitch->getDeflectProb();
        state.push_back(neighbourDeflectProb);
    }
}

void ManhattanBanditDeflectionSwitch::setDropProbState(vector<double> &state, Packet *p) {
    // go through all neighbours as set state to be the mean
    for (pair<int, int> neighbour : switchNeighbourIfaces) {
        ManhattanBanditDeflectionSwitch *neighbourSwitch =
            dynamic_cast<ManhattanBanditDeflectionSwitch *>(man.getSwitch(neighbour.second));
        double neighbourDropProb = neighbourSwitch->getDropProb();
        state.push_back(neighbourDropProb);
    }
}

vector<double> ManhattanBanditDeflectionSwitch::getState(Packet *p) {
    vector<double> state;

    for (auto it: stateTypes) {
        switch (it) {
            case flowIdState:
                setFlowIdState(state, p);
                break;

            case destIdState:
                setDestIdState(state, p);
                break;

            case hop1ShortState:
                setHop1ShortState(state, p);
                break;

            case hop1_2ShortState:
                setHop1_2ShortState(state, p);
                break;

            case hop2ShortState:
                setHop2ShortState(state, p);
                break;

            case sectionState:
                setSectionState(state, p);
                break;

            case deflectProbState:
                setDeflectProbState(state, p);
                break;

            case dropProbState:
                setDropProbState(state, p);
                break;
            default:
                throw runtime_error("ManhattanBanditDeflectionSwitch: getState: unsupported state");
                break;
        }
    }

    return state;
}

void ManhattanBanditDeflectionSwitch::sendActionUpdate(int prevSwitch, Packet *p, actionResult result, double actionValue) {
    // inputs : source id, reward, packetId
    // send an update to the neighbouring switch
    ManhattanBanditDeflectionSwitch *neighbourSwitch = dynamic_cast<ManhattanBanditDeflectionSwitch *>(man.getSwitch(prevSwitch));
    if (neighbourSwitch == NULL) {
        throw runtime_error("sendActionUpdate: Previous switch: " + to_string(prevSwitch) + " does not exist or is not mbd switch");
    }

    int minHops = int(this->costToDest(p->getDest()));
    neighbourSwitch->recieveActionUpdate(p->getId(), result, actionValue, minHops);
}

void ManhattanBanditDeflectionSwitch::recieveActionUpdate(int pId, actionResult result, double actionValue, int nextMinHops) {
    switch(result) {
        case actionIntentionalDrop:
            this->rewardAction(pId, REWARD_INTENTIONAL_DROP); // TODO create define
            break;
        case actionDrop:
            this->rewardAction(pId, REWARD_DROP); // TODO create define
            break;
        case actionArrive:
            this->rewardAction(pId, REWARD_ARRIVAL); // TODO create define
            break;
        case actionForward:
            {
                tuple<vector<double>, int, int> stateAction = this->peekAction(pId);
                // from here to dest TODO might be good to check routing table
                double minHops = this->costToDest(get<2>(stateAction));
                double reward = (minHops * actionValue) / (nextMinHops + actionValue);
                //update pId with r;
                this->rewardAction(pId, reward);
            }
            break;
    }
}

void ManhattanBanditDeflectionSwitch::rxPacket(Packet *p, int sourceInterfaceId) {
    // TODO if the packet is at it's destination update action
#ifdef ONE_HOP_REWARD
    if (p->getDest() == id && sourceInterfaceId != NULL_ID) { // address when a packet was just created
        int prevSwitch = interfaceToNeighbour[sourceInterfaceId];
        sendActionUpdate(prevSwitch, p, actionArrive, 0);
    }
#endif /* ONE_HOP_REWARD */
    Switch::rxPacket(p, sourceInterfaceId);
}

int ManhattanBanditDeflectionSwitch::routePacket(Packet *p, int sourceInterfaceId) {
    MBDPacket *MBDP = dynamic_cast<MBDPacket *>(p);
    if (MBDP == NULL) {
        throw runtime_error("Switch:\nMBD switch passed a non MBD packet\n");
    }

    vector<int> nonBlockedActions = availableInterfaces(p);

    // force a drop if all ports are blocked
    if ((!dropAction && nonBlockedActions.empty()) || (dropAction && nonBlockedActions.size() == 1)) {
#ifdef ONE_HOP_REWARD
        if (sourceInterfaceId != NULL_ID) { // address when a packet was just created
            int prevSwitch = interfaceToNeighbour[sourceInterfaceId];
            sendActionUpdate(prevSwitch, p, actionDrop, 0);
        }
#endif /* ONE_HOP_REWARD */
        return NULL_ID;
    }

    // if not enough hops left to get to destination drop
    if (this->costToDest(p->getDest()) > p->getTTL()) {
        man.logEvent(SWITCH_STR, id, "MBDSwitch: routePacket",
                "not enough hops to get to destination; packet: " + to_string(p->getId()));
#ifdef ONE_HOP_REWARD
        if (sourceInterfaceId != NULL_ID) { // address when a packet was just created
            int prevSwitch = interfaceToNeighbour[sourceInterfaceId];
            sendActionUpdate(prevSwitch, p, actionDrop, 0);
        }
#endif /* ONE_HOP_REWARD */
        p->setTimedOut();
        return NULL_ID;
    }

    int actionInterface = NULL_ID;

    // if only deflect or forward first take forward actions is avaiable
    if (this->actionLimit == this->actionLimitOnlyDeflect ||
        this->actionLimit == this->actionLimitForwardFirst ) {
        // if forward set not empty set
        vector<int> forwardAvailableActions = this->availableForwardInterfaces(p);

        if (!forwardAvailableActions.empty()) {
            if (this->actionLimit == this->actionLimitOnlyDeflect) {
                // if actionLimitOnlyDeflect randomy select forward switch
                vector<int> availableActions = {forwardAvailableActions[generator() % forwardAvailableActions.size()]};
                // force agent to take that action
                actionInterface = this->takeAgentAction(sourceInterfaceId, p, availableActions);
            } else if (this->actionLimit == this->actionLimitForwardFirst) {
                // if actionForward agent takes an action from the forward set
                actionInterface = this->takeAgentAction(sourceInterfaceId, p, forwardAvailableActions);
            } else {
                throw runtime_error("MBDSwitch:\nroutePacket: attempting to use non valid actionLimit\n");
            }
        } else { // deflect
            actionInterface = this->takeAgentAction(sourceInterfaceId, p, nonBlockedActions);
        }
    } else {
        actionInterface = this->takeAgentAction(sourceInterfaceId, p, nonBlockedActions);
    }

    return actionInterface;
}

int ManhattanBanditDeflectionSwitch::takeAgentAction(int sourceInterfaceId, Packet *p, vector<int> availableActions) {
    MBDPacket *MBDP = dynamic_cast<MBDPacket *>(p);
    if (MBDP == NULL) {
        throw runtime_error("Switch:\nMBD switch passed a non MBD packet\n");
    }

    vector<double> state = getState(p);
    pair<int, double> action = agent->selectAction(state, availableActions);
    int actionInterface = actionInterfaces[action.first];

    // if deflection record or drop if too many deflections
    if (get<2>(this->routingTable.find(p->getDest())->second).contains(actionInterface)) {
        if (MBDP->deflectionsRemaining() <= 0) {
            man.logEvent(SWITCH_STR, id, "MBDSwitch: routePacket",
                    "packet ran out of deflections; packet: " + to_string(p->getId()));
#ifdef ONE_HOP_REWARD
            if (sourceInterfaceId != NULL_ID) { // address when a packet was just created
                int prevSwitch = interfaceToNeighbour[sourceInterfaceId];
                sendActionUpdate(prevSwitch, p, actionDrop, 0);
            }
#endif /* ONE_HOP_REWARD */
            p->setTimedOut(); // form of timeout
            return NULL_ID;
        }

        MBDP->recordDeflection();
    }

    recordAction(MBDP, state, action.first);

    // update previous switch with the value from agent->selectAction
#ifdef ONE_HOP_REWARD
    if (sourceInterfaceId != NULL_ID) { // packet arrived from a neighbour we need to update them
        int prevSwitch = interfaceToNeighbour[sourceInterfaceId];

        if (actionInterface != NULL_ID) {
            sendActionUpdate(prevSwitch, p, actionForward, action.second);
        } else {
            sendActionUpdate(prevSwitch, p, actionDrop, 0);
            recieveActionUpdate(p->getId(), actionIntentionalDrop, 0, 0);
        }
    } else if (actionInterface == NULL_ID) { // packet was just created and dropped
        recieveActionUpdate(p->getId(), actionIntentionalDrop, 0, 0);
    }
#endif /* ONE_HOP_REWARD */

    // record actions in entropy maps
    bool isShortest = get<1>(this->routingTable.find(p->getDest())->second).contains(actionInterface);
    for (int i : this->entropyIntervals) {
        this->allActionsEntropyCounts[i][state][action.first]++;

        if (isShortest) {
            this->deflectionEntropyCounts[i][state][0]++;
        } else {
            this->deflectionEntropyCounts[i][state][1]++;
        }

        this->entropyActionsTaken[i]++;
    }
    this->learningActionsTaken++;

    return actionInterface;
}

void ManhattanBanditDeflectionSwitch::recordAction(MBDPacket *MBDP, vector<double> context, int action) {
    // TODO if exists add to queue else create queue
#ifdef ONE_HOP_REWARD
    this->actionStore[MBDP->getId()].push(tuple<vector<double>, int, int>{context, action, MBDP->getDest()});
#else /* ONE_HOP_REWARD */
    MBDP->recordAction(id);
#endif /* ONE_HOP_REWARD */
}

tuple<vector<double>, int, int> ManhattanBanditDeflectionSwitch::peekAction(int pId) {
    // if nothing return NULL values
    if (this->actionStore[pId].empty()) {
        return {{}, -1, NULL_ID};
    }

    // make copy
    vector<double> state = get<0>(this->actionStore[pId].top());
    int action = get<1>(this->actionStore[pId].top());
    int destId = get<2>(this->actionStore[pId].top());

    // return
    return {state, action, destId};
}

tuple<vector<double>, int, int> ManhattanBanditDeflectionSwitch::retrieveAction(int pId) {
    tuple<vector<double>, int, int> actionTuple = peekAction(pId);

    // remove
    this->actionStore[pId].pop();
    // return
    return actionTuple;
}

void ManhattanBanditDeflectionSwitch::rewardAction(int pId, double reward) {
    // get context action pair
    tuple<vector<double>, int, int> stateAction = this->retrieveAction(pId);
    // if there is no action
    if (get<1>(stateAction) == -1) {
        return;
    }

    this->rewardSum += reward;
    this->actionsRewarded++;

    // apply update
    agent->updateAgent(get<0>(stateAction), get<1>(stateAction), reward);
}

/* NDDSwitch */

NDDSwitch::NDDSwitch(json &switchConfig): RandomForwardSwitch(validateNDDSwitchConfig(switchConfig)) {
    this->DHCMax = switchConfig["DHC_max"];
    this->DNMaxTime = switchConfig["DN_max_time"];

    this->deflectionIdCounter = 0;

    this->onlyForward = switchConfig["only_forward"];
    this->multipleUpdates = switchConfig["multiple_updates"];

    // create agent
    agent = createNDDAgent(switchConfig["NDDAgent"]);

    this->rewardSum = 0.0;
    this->actionsRewarded = 0;
    this->learningActionsTaken = 0;
}

json &NDDSwitch::validateNDDSwitchConfig(json &switchConfig) {
    string message = "";

    if (!hasMemberOfType(switchConfig, "DHC_max", jsonInt)) {
        message += "No int with name 'DHC_max'.\n";
    }

    if (!hasMemberOfType(switchConfig, "DN_max_time", jsonDouble)) {
        message += "No double with name 'DN_max_time'.\n";
    }

    if (!hasMemberOfType(switchConfig, "only_forward", jsonBool)) {
        message += "No bool with name 'only_forward'.\n";
    }

    if (!hasMemberOfType(switchConfig, "multiple_updates", jsonBool)) {
        message += "No bool with name 'multiple_updates'.\n";
    }

    // NDD agent
    if (!hasMember(switchConfig, "NDDAgent")) {
        message += "No int with name 'NDDAgent'.\n";
    }

    if (!message.empty()) {
        message = "NDDSwitch:\n" + message + switchConfig.dump(4);
        throw runtime_error(message);
    }

    return switchConfig;
}

bool NDDSwitch::initSwitch() {
    Switch::initSwitch();

    // setup action interfaces lookup table
    for (auto it: interfaces) {
        this->actionInterfaces.push_back(it.second);
    }

    for (size_t i = 0; i < this->actionInterfaces.size(); i++) {
        this->interfaceToAction.emplace(this->actionInterfaces[i], i);
    }

    // map outgoing optimal interfaces to a int corresponding to the state based on that the switches they use
    map<set<int>, int> interfaceToStateInt;
    int currentStateInt = 0;

    for (auto &[dest, routingTuple] : routingTable) {
        set<int> optimalIfaces = get<1>(routingTuple);

        if (interfaceToStateInt.find(optimalIfaces) == interfaceToStateInt.end()) {
            interfaceToStateInt.emplace(optimalIfaces, currentStateInt);
            currentStateInt++;
        }
        int stateInt = interfaceToStateInt[optimalIfaces];
        // add to dest to state
        this->destToState.emplace(dest, stateInt);
    }

    this->numDestStates = currentStateInt;

    int numActions = this->actionInterfaces.size();
    vector<int> stateRanges;
    // add interface binary states
    for (size_t i = 0; i < this->switchNeighbourIfaces.size(); i++) {
        stateRanges.push_back(2);
    }
    // add optimal interface state
    stateRanges.push_back(currentStateInt);

    agent->init(stateRanges, numActions);

    return true;
}

void NDDSwitch::startSwitch() {
    RandomForwardSwitch::startSwitch();
}

void NDDSwitch::resetData() {
    this->learningActionsTaken = 0;
    this->rewardSum = 0.0;
    this->actionsRewarded = 0;
}

void NDDSwitch::recordData() {
    observer.logSwitchData(id, "learningActionsTaken", this->learningActionsTaken);
    observer.logSwitchData(id, "averageReward", this->rewardSum / this->actionsRewarded);
    observer.logSwitchData(id, "actionsRewarded", this->actionsRewarded);

    Switch::recordData();
}

vector<int> NDDSwitch::getState(Packet *p) {
    vector<int> state = {};
    // set avaiable interfaces, 1 == available
    for (pair<int,int> ifaceSwitch: switchNeighbourIfaces) {
        int iface = ifaceSwitch.first;
        Interface *interface = man.getInterface(iface);
        if (interface->getOutBufferCurrentSize() + p->fullSize() <= interface->getOutBufferTotalSize()) {
            state.push_back(1);
        } else {
            state.push_back(0);
        }
    }

    state.push_back(this->destToState[p->getDest()]);

    return state;
}

void NDDSwitch::recordAction(int deflectionId, deflectionData data, second_t timeout) {
    this->deflectionLookup.emplace(deflectionId, data);
    this->deflectionTimeoutQueue.emplace((deflectionTimerElem){
            .deflectionId = deflectionId,
            .timeout = timeout
            });

    this->learningActionsTaken++;

    if (deflectionTimeoutQueue.size() == 1) {
        EventI *e = new Event<NDDSwitch>(timeout, &NDDSwitch::DNTimerEvent, this);
        man.pushEvent(e);
    }
}

int NDDSwitch::routePacket(Packet *p, int sourceInterfaceId) {
    vector<int> routingIfaces;
    NDDPacket *NDDp = dynamic_cast<NDDPacket *>(p);
    if (NDDp == NULL) {
        throw runtime_error("NDDSwitch:\npacket is not an NDDPacket");
    }

    man.logEvent(SWITCH_STR, id, "NDDSwitch: routePacket", "packet: " + to_string(NDDp->getId()));

    for (int iface: get<1>(this->routingTable[p->getDest()])) {
        Interface *interface = man.getInterface(iface);
        if (interface->getOutBufferCurrentSize() + p->fullSize() <= interface->getOutBufferTotalSize()) {
            routingIfaces.push_back(iface);
        }
    }

    // if any optimal interfaces avaiable take them
    if (!routingIfaces.empty()) {
        man.logEvent(SWITCH_STR, id, "NDDSwitch: routePacket", "optimal path - packet: " +
                to_string(NDDp->getId()));
        //randomly select amongst the optimal interfaces
        return routingIfaces[generator() % routingIfaces.size()];
    }

    // if only forwarding drop packet as there is congestion
    if (this->onlyForward) {
        man.logEvent(SWITCH_STR, id, "NDDSwitch: routePacket", "only forwarding - dropping packet: " +
                to_string(NDDp->getId()));
        return NULL_ID;
    }

    // otherwise go through non optimal interfaces
    // determine the set of actions that are possible
    vector<int> availableActions, allActions;
    for (int iface: get<2>(this->routingTable[p->getDest()])) {
        Interface *interface = man.getInterface(iface);
        if (interface->getOutBufferCurrentSize() + p->fullSize() <= interface->getOutBufferTotalSize()) {
            availableActions.push_back(this->interfaceToAction[iface]);
        }
        allActions.push_back(this->interfaceToAction[iface]);
    }

    // if no available deflection interfaces drop packet
    if (availableActions.empty()) {
        man.logEvent(SWITCH_STR, id, "NDDSwitch: routePacket",
                "dropping Packet: " + to_string(NDDp->getId()));
        return NULL_ID;
    }

    vector<int> state = this->getState(p);
    //if packet.deflection_id == NULL_ID and no current deflecting packet
    if (NDDp->getDeflectionId() == NULL_ID && (this->multipleUpdates || this->deflectionLookup.size() == 0)) {
        int action = agent->selectAction(state, availableActions, true); // true allows exploration
        vector<int> actionSet = allActions; // TODO this should be avaiable deflection actions

        this->deflectionIdCounter++;
        if (!NDDp->deflect(this->deflectionIdCounter, this->id, 1)) {
            throw runtime_error("NDDSwitch:\nroutePacket: packet already deflected with no deflection id\n");
        }

        deflectionData data = {
            .state = state,
            .action = action,
            .actionSet = actionSet,
            .DfT = man.time,
        };
        second_t timeout = man.time + this->DNMaxTime;
        this->recordAction(deflectionIdCounter, data, timeout);

        //store action in best actions?
        return this->actionInterfaces[action];
    }

    // undeflected packet and currently deflecting - this deflection is unrecorded
    if (NDDp->getDeflectionId() == NULL_ID && (!this->multipleUpdates && this->deflectionLookup.size() != 0)){
        man.logEvent(SWITCH_STR, id, "NDDSwitch: routePacket",
                "deflecting undeflected packet while tracking another: " +
                to_string(NDDp->getId()));
        int action = agent->selectAction(state, availableActions, false);
        return this->actionInterfaces[action];
    }

    // the packet was deflected and must be dropped
    if (NDDp->getDHC() >= this->DHCMax) {
        man.logEvent(SWITCH_STR, id, "NDDSwitch: routePacket", "drop DHC packet: " +
                to_string(NDDp->getId()));
        this->dropPacketFeedback(p);
        p->setTimedOut(); // too many deflections therefor timed out
        return NULL_ID;
    }

    // take deflection action
    man.logEvent(SWITCH_STR, id, "NDDSwitch: routePacket", "deflect previously deflected packet: " +
            to_string(NDDp->getId()) + " deflection id:" + to_string(NDDp->getDeflectionId()));
    int action = agent->selectAction(state, availableActions, false);
    // outgoing_interface = best previous action
    NDDp->incrementDHC();
    return this->actionInterfaces[action];
}

void NDDSwitch::dropPacketFeedback(Packet *p) {
    NDDPacket *NDDp = dynamic_cast<NDDPacket *>(p);
    if (NDDp == NULL) {
        throw runtime_error("Switch:\nNDD switch passed a non NDD packet\n");
    }

    if (NDDp->getDeflectionId() == NULL_ID) {
        throw runtime_error("NDDSwitch:\n dropPacketFeedback: deflectionId == NULL_ID packet id " +
                to_string(NDDp->getId()) + "\n");
    }

    int deflectingSwitch = NDDp->getDeflectingSwitchId();
    NDDFeedbackMessage feedback = {
        .deflectionId = NDDp->getDeflectionId(),
        .DHC = NDDp->getDHC(),
    };

    Switch *s = man.getSwitch(deflectingSwitch);
    NDDSwitch *NDDs = dynamic_cast<NDDSwitch *>(s);
    NDDs->feedbackArrived(feedback);
}

void NDDSwitch::DNTimerEvent() {
    // check top of queue
    deflectionTimerElem topElem = this->deflectionTimeoutQueue.top();
    this->deflectionTimeoutQueue.pop();

    if (man.time != topElem.timeout) {
        throw runtime_error("NDDSwitch:\n DNTimerEvent man.time " + to_string(man.time) +
                " != top element timeout " + to_string(topElem.timeout) + "\n");
    }

    // if in lookup table perform update
    if (this->deflectionLookup.find(topElem.deflectionId) != this->deflectionLookup.end()) {
        deflectionData data = this->deflectionLookup[topElem.deflectionId];
        this->deflectionLookup.erase(topElem.deflectionId);

        double reward = calculateReward(0.0, 0.0);

        this->rewardSum += reward;
        this->actionsRewarded++;

        agent->update(data.state, data.action, reward, data.actionSet);

        man.logEvent(SWITCH_STR, id, "DNTimerEvent", "deflection notificaiton timer elapsed without feedback: " +
                to_string(topElem.deflectionId) + " reward: " + to_string(reward));
    }

    // loop through pq until items - while not empty and the top element is not in the lookup table
    topElem = this->deflectionTimeoutQueue.top();
    while (!this->deflectionTimeoutQueue.empty() &&
            this->deflectionLookup.find(topElem.deflectionId) == this->deflectionLookup.end()) {
        // remove
        this->deflectionTimeoutQueue.pop();
        topElem = this->deflectionTimeoutQueue.top();
    }

    // create next event
    if (this->deflectionTimeoutQueue.size() != 0) {
        topElem = this->deflectionTimeoutQueue.top();
        EventI *e = new Event<NDDSwitch>(topElem.timeout, &NDDSwitch::DNTimerEvent, this);
        man.pushEvent(e);
    }
}

void NDDSwitch::feedbackArrived(NDDFeedbackMessage feedback) {
    // if id not in table skip
    int deflectionId = feedback.deflectionId;
    if (this->deflectionLookup.find(deflectionId) == this->deflectionLookup.end()) {
        man.logEvent(SWITCH_STR, id, "dropPacketFeedback",
                "recieved feedback message timed out for deflection: " + to_string(deflectionId));
        return;
    }

    deflectionData data = this->deflectionLookup[deflectionId];
    this->deflectionLookup.erase(deflectionId);

    double TTT = man.time - data.DfT;
    double reward = calculateReward(TTT, feedback.DHC);

    this->rewardSum += reward;
    this->actionsRewarded++;

    agent->update(data.state, data.action, reward, data.actionSet);

    man.logEvent(SWITCH_STR, id, "dropPacketFeedback", "recieved feedback message for deflection: " +
            to_string(deflectionId) + " reward: " + to_string(reward));
}

double NDDSwitch::calculateReward(double TTT, int DHC) {
    return (TTT / this->DNMaxTime + double(DHC) / this->DHCMax) / 2;
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
