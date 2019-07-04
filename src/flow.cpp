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

#define FLOW_STR        "Flow"
#define TX_PACKET_EVENT "flow create packet event"

using namespace std;

enum flowType {
    BasicFlowType,
#ifdef _TEST
    TestFlowType,
#endif /* _TEST */
};

Flow *createFlow(json &config) {
    static map<string, size_t> flowTypeMap = {
        {"basic", BasicFlowType},
#ifdef _TEST
        {"test", TestFlowType},
#endif /* _TEST */
    };

    Flow *flow;

    if (config.find("type") == config.end()) {
        fprintf(stderr, "Error in flow, 'type' not found\n");
        return NULL;
    }

    if (!config["type"].is_string()) {
        fprintf(stderr, "Error in flow with 'type' not string type\n");
        return NULL;
    }

    string type = config["type"];

    auto it = flowTypeMap.find(type);
    if (it == flowTypeMap.end()) {
        fprintf(stderr, "Error flow type %s does not exist\n", type.c_str());
        return NULL;
    }

    switch (it->second) {
        case BasicFlowType:
            try {
                flow = new BasicFlow(config);
            } catch (...) {
                return NULL;
            }
            break;
#ifdef _TEST
        case TestFlowType:
            try {
                flow = new TestFlow(config);
            } catch (...) {
                return NULL;
            }
            break;
#endif /* _TEST */
        default:
            fprintf(stderr, "Internal error, cant find type %s\n", type.c_str());
            return NULL;
    }

    return flow;
}

Flow::Flow(json &config): NetworkObject(config["id"]) {
    if (config.find("source_id") == config.end()) {
        fprintf(stderr, "Error in flow 'source_id' not found\n");
        // error
    }

    if (!config["source_id"].is_number_integer()) {
        fprintf(stderr, "Error in flow 'source_id' not type integer\n");
        // error
    }
    this->sourceID = config["source_id"];

    if (config.find("dests") == config.end()) {
        fprintf(stderr, "Error in flow 'dests' not found\n");
        // error
    }

    if (!config["dests"].is_array()) {
        fprintf(stderr, "Error in flow 'dests' not type array\n");
    }

    for (auto it : config["dests"].items()) {
        if (!it.value().is_number_integer()) {
            fprintf(stderr, "Error in flow 'dests' element not type integer\n");
            // error
        }
        this->destID = it.value();
    }
}

void Flow::packetArrived(Packet *p) {
    delete p;
}

void Flow::packetDropped(Packet *p) {
    delete p;
}

void Flow::packetError(Packet *p) {
    delete p;
}

int Flow::newPacketID() {
    static int pID = 0;
    return pID++;
}

bool Flow::validate() {
    // TODO implement
    return true;
}

BasicFlow::BasicFlow(json &config): Flow(config) {
    this->time = 0.001;
    this->minHeadSize = 20;
    this->maxHeadSize = 32;
    this->minBodySize = 80;
    this->maxBodySize = 512;
    this->ttl = 15;
}

second_t BasicFlow::nextTxTime() {
    return man.time + time;
}

void BasicFlow::StartFlow() {
    second_t nextTx = nextTxTime();
    EventI *e = new Event<BasicFlow>(nextTx, &BasicFlow::txPacketEvent, this);
    man.pushEvent(e);
}

void BasicFlow::txPacketEvent() {
    int headSize = minHeadSize + rand() % (maxHeadSize - minHeadSize);
    int bodySize = minBodySize + rand() % (maxBodySize - minBodySize);
    int pId = newPacketID();

    Packet *p = new Packet(pId, sourceID, destID, this, ttl, headSize, bodySize);

    man.logTxEvent(FLOW_STR, id, TX_PACKET_EVENT, endpoint->getID(), p);

    endpoint->txPacket(p);

    second_t nextTx = nextTxTime();
    EventI *e = new Event<BasicFlow>(nextTx, &BasicFlow::txPacketEvent, this);
    man.pushEvent(e);
}

#ifdef _TEST
second_t TestFlow::nextTxTime() {
    static const second_t min = 0.001, max = 0.01;

    return man.time + min + (float)rand() / (RAND_MAX / (max - min));
}

void TestFlow::StartFlow() {
    second_t nextTx = nextTxTime();
    EventI *e = new Event<TestFlow>(nextTx, &TestFlow::txPacketEvent, this);
    man.pushEvent(e);
}

void TestFlow::txPacketEvent() {
    static const int hMin = 10, hMax = 50;
    static const int bMin = 40, bMax = 120;
    static const int ttl = 15;

    int hSize = hMin + rand() % (hMax - hMin);
    int bSize = bMin + rand() % (bMax - bMin);
    int pID = newPacketID();

    Packet *p = new Packet(pID, sourceID, destID, this, ttl, hSize, bSize);

    man.logTxEvent(FLOW_STR, id, TX_PACKET_EVENT, endpoint->getID(), p);

    endpoint->txPacket(p);

    // set up next
    second_t nextTx = nextTxTime();
    EventI *e = new Event<TestFlow>(nextTx, &TestFlow::txPacketEvent, this);
    man.pushEvent(e);
}
#endif /* _TEST */
