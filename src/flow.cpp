#include <stdlib.h>
#include <typeinfo>
#include <typeindex>
#include <map>
#include <nlohmann/json.hpp>

#include "flow.h"
#include "manager.h"
#include "packet.h"
#include "endpoint.h"
#include "networkObject.h"
#include "config.h"
#include "sarsa.h"

#define FLOW_STR        "Flow"
#define TX_PACKET_EVENT "flow create packet event"

using namespace std;

enum FlowType {
    BasicFlowType,
#ifdef _TEST
    TestFlowType,
#endif /* _TEST */
};

Flow *createFlow(json &flowConfig) {
    static map<string, FlowType> flowTypeMap = {
        {"basic", BasicFlowType},
#ifdef _TEST
        {"test", TestFlowType},
#endif /* _TEST */
    };


    if (!hasMemberOfType(flowConfig, "type", jsonString)) {
        throw runtime_error("Flow:\nNo string with name 'type'\n" + flowConfig.dump(4));
    }

    string flowTypeString = flowConfig["type"];
    FlowType flowType;
    try {
        flowType = flowTypeMap.at(flowTypeString);
    } catch (out_of_range&) {
        throw runtime_error("Flow:\nInvalid flow type: " + flowTypeString);
    }

    Flow *flow;

    switch (flowType) {
        case BasicFlowType:
            flow = new BasicFlow(flowConfig);
            break;
#ifdef _TEST
        case TestFlowType:
            flow = new TestFlow(flowConfig);
            break;
#endif /* _TEST */
        default:
            throw runtime_error("Flow:\nInvalid flow type: " + flowTypeString);
    }

    return flow;
}

//flowConfig already validated
Flow::Flow(json &flowConfig):
    NetworkObject(flowConfig["id"]),
    //TODO: Second parameter is initial state, should it be something other than 0?
    agent(Sarsa(man.getEndpoint(flowConfig["source_id"])->getWeights(), 0)) {
    this->sourceId = flowConfig["source_id"];
    this->destId = flowConfig["dest"];

    this->packetsCreated = 0;
    this->packetsArrived = 0;
    this->packetsDropped = 0;
    this->packetsErrored = 0;
    this->curPId = 0;
}

Flow::~Flow() {
    for (auto it : packets) {
        delete it.second;
    }
}

void Flow::removePacket(Packet *p) {
    packets.erase(p->getId());
}

bool Flow::addPacket(Packet *p) {
    return packets.emplace(p->getId(), p).second;
}

void Flow::packetArrived(Packet *p) {
    removePacket(p);
    packetsArrived++;
    delete p;
}

void Flow::packetDropped(Packet *p) {
    removePacket(p);
    packetsDropped++;
    delete p;
}

void Flow::packetError(Packet *p) {
    removePacket(p);
    packetsErrored++;
    delete p;
}

int Flow::newPacketId() {
    return curPId++;
}

int Flow::getPacketsCreated() {
    return packetsCreated;
}

int Flow::getPacketsArrived() {
    return packetsArrived;
}

int Flow::getPacketsDropped() {
    return packetsDropped;
}

int Flow::getPacketsErrored() {
    return packetsErrored;
}

bool Flow::validateSource() {
    if (man.getEndpoint(sourceId) == NULL) {
        fprintf(stderr, "Flow: endpoint of flow %d is missing\n", id);
        return false;
    }

    return true;
}

bool Flow::validateDest() {
    Endpoint *endpoint = man.getEndpoint(destId);
    if (endpoint == NULL) {
        fprintf(stderr, "Flow: destination %d of flow %d is missing\n", destId, id);
        return false;
    }

    return true;
}

bool Flow::validate() {
    bool valid = true;

    if (!validateSource()) {
        valid = false;
    }

    if (!validateDest()) {
        valid = false;
    }

    return valid;
}

Packet *Flow::createPacket(int ttl, int headSize, int bodySize) {
    int pId = newPacketId();

    Packet *p = new Packet(pId, sourceId, destId, id, ttl, headSize, bodySize);

    if (!addPacket(p)) {
        delete p;
        return NULL;
    }
    packetsCreated++;

    return p;
}

BasicFlow::BasicFlow(json &flowConfig): Flow(validateBasicFlowConfig(flowConfig)) {
    this->time = 0.001;
    this->headSize = 20;
    this->bodySize = 256;
    this->ttl = 15;
}

json &BasicFlow::validateBasicFlowConfig(json &flowConfig) {
    string message = "";
    if (!hasMemberOfType(flowConfig, "id", jsonInt)) {
        message += "No integer with name 'id'.\n";
    }

    if (!hasMemberOfType(flowConfig, "source_id", jsonInt)) {
        message += "No integer with name 'source_id'.\n";
    }

    if (!hasMemberOfType(flowConfig, "dest", jsonInt)) {
        message += "No integer with name 'dest'.\n";
    }

    if (!message.empty()) {
        message = "Basic Flow:\n" + message + flowConfig.dump(4);
        throw runtime_error(message);
    }
    return flowConfig;
}

second_t BasicFlow::nextTxTime() {
    return man.time + time;
}

void BasicFlow::startFlow() {
    second_t nextTx = nextTxTime();
    EventI *e1 = new Event<BasicFlow>(nextTx, &BasicFlow::txPacketEvent, this);
    man.pushEvent(e1);

    EventI *e2 = new Event<BasicFlow>(man.time + miTime, &BasicFlow::stepAgent, this);
    man.pushEvent(e2);
}

void BasicFlow::stepAgent() {
    //TODO: get state and reward, use transmissionSpeed
    double state = 0;
    double reward = 0;
    switch (agent.step(state, reward)) {
        case 0:
            transmissionSpeed *= 2;
            break;
        case 1:
            transmissionSpeed /= 2;
            break;
        case 2:
            transmissionSpeed++;
            break;
        case 3:
            transmissionSpeed--;
            break;
    }
    EventI *e = new Event<BasicFlow>(man.time + miTime, &BasicFlow::stepAgent, this);
    man.pushEvent(e);
}

void BasicFlow::txPacketEvent() {
    Endpoint *endpoint = man.getEndpoint(sourceId);
    // TODO test for error

    Packet *p = createPacket(ttl, headSize, bodySize);

    man.logTxEvent(FLOW_STR, id, TX_PACKET_EVENT, sourceId, p);

    endpoint->txPacket(p);

    second_t nextTx = nextTxTime();
    EventI *e = new Event<BasicFlow>(nextTx, &BasicFlow::txPacketEvent, this);
    man.pushEvent(e);
}

/* Explicit congestion notification flow */

ECNFlow::ECNFlow(json &flowConfig): Flow(validateECNFlowConfig(flowConfig)) {}

json &ECNFlow::validateECNFlowConfig(json &flowConfig) {
    string message = "";
    if (!hasMemberOfType(flowConfig, "id", jsonInt)) {
        message += "No integer with name 'id'.\n";
    }

    if (!hasMemberOfType(flowConfig, "source_id", jsonInt)) {
        message += "No integer with name 'source_id'.\n";
    }

    if (!hasMemberOfType(flowConfig, "dest", jsonInt)) {
        message += "No integer with name 'dest'.\n";
    }

    if (!message.empty()) {
        message = "Basic Flow:\n" + message + flowConfig.dump(4);
        throw runtime_error(message);
    }
    return flowConfig;
}

ECNPacket *ECNFlow::createPacket(int ttl, int headSize, int bodySize) {
    int pId = newPacketId();

    ECNPacket *p = new ECNPacket(pId, sourceId, destId, id, ttl, headSize, bodySize);

    if (!addPacket(p)) {
        delete p;
        return NULL;
    }
    packetsCreated++;

    return p;
}

second_t ECNFlow::nextTxTime() {
    return man.time + ((headSize + bodySize) / rate);
}

void ECNFlow::startFlow() {
    // create txPacket event
    second_t nextTx = nextTxTime();
    EventI *e1 = new Event<ECNFlow>(nextTx, &ECNFlow::txPacketEvent, this);
    man.pushEvent(e1);

    EventI *e2 = new Event<ECNFlow>(man.time + miTime, &ECNFlow::stepAgent, this);
    man.pushEvent(e2);
}

void ECNFlow::stepAgent() {
    //TODO: get state and reward, use transmissionSpeed
    double state = 0;
    double reward = 0;
    switch (agent.step(state, reward)) {
        case 0:
            transmissionSpeed *= 2;
            break;
        case 1:
            transmissionSpeed /= 2;
            break;
        case 2:
            transmissionSpeed++;
            break;
        case 3:
            transmissionSpeed--;
            break;
    }
    EventI *e = new Event<ECNFlow>(man.time + miTime, &ECNFlow::stepAgent, this);
    man.pushEvent(e);
}

void ECNFlow::txPacketEvent() {
    Endpoint *endpoint = man.getEndpoint(sourceId);

    // create packet
    ECNPacket *p = createPacket(ttl, headSize, bodySize);

    endpoint->txPacket(p);

    second_t nextTx = nextTxTime();
    EventI *e = new Event<ECNFlow>(nextTx, &ECNFlow::txPacketEvent, this);
    man.pushEvent(e);
}

void ECNFlow::packetArrived(Packet *p) {
    // TODO update data on ECN data
    Flow::packetArrived(p);
}

/* tests */
#ifdef _TEST
TestFlow::TestFlow(json &flowConfig): Flow(validateTestFlowConfig(flowConfig)) {}

json &TestFlow::validateTestFlowConfig(json &flowConfig) {
    string message = "";
    if (!hasMemberOfType(flowConfig, "id", jsonInt)) {
        message += "No integer with name 'id'.\n";
    }

    if (!hasMemberOfType(flowConfig, "source_id", jsonInt)) {
        message += "No integer with name 'source_id'.\n";
    }

    if (!hasMemberOfType(flowConfig, "dest", jsonInt)) {
        message += "No integer with name 'dest'.\n";
    }

    if (!message.empty()) {
        message = "Test Flow:\n" + message + flowConfig.dump(4);
        throw runtime_error(message);
    }
    return flowConfig;
}

second_t TestFlow::nextTxTime() {
    static const second_t min = 0.001, max = 0.01;

    return man.time + min + (float)rand() / (RAND_MAX / (max - min));
}

void TestFlow::startFlow() {
    second_t nextTx = nextTxTime();
    EventI *e1 = new Event<TestFlow>(nextTx, &TestFlow::txPacketEvent, this);
    man.pushEvent(e1);

    EventI *e2 = new Event<TestFlow>(man.time + miTime, &TestFlow::stepAgent, this);
    man.pushEvent(e2);
}

void TestFlow::stepAgent() {
    //TODO: get state and reward, use transmissionSpeed
    double state = 0;
    double reward = 0;
    switch (agent.step(state, reward)) {
        case 0:
            transmissionSpeed *= 2;
            break;
        case 1:
            transmissionSpeed /= 2;
            break;
        case 2:
            transmissionSpeed++;
            break;
        case 3:
            transmissionSpeed--;
            break;
    }
}

void TestFlow::txPacketEvent() {
    static const int hMin = 10, hMax = 50;
    static const int bMin = 40, bMax = 120;
    static const int ttl = 15;

    int hSize = hMin + rand() % (hMax - hMin);
    int bSize = bMin + rand() % (bMax - bMin);

    Endpoint *endpoint = man.getEndpoint(sourceId);
    // todo test for error

    Packet *p = createPacket(ttl, hSize, bSize);

    man.logTxEvent(FLOW_STR, id, TX_PACKET_EVENT, sourceId, p);

    endpoint->txPacket(p);

    // set up next
    second_t nextTx = nextTxTime();
    EventI *e = new Event<TestFlow>(nextTx, &TestFlow::txPacketEvent, this);
    man.pushEvent(e);
}
#endif /* _TEST */
