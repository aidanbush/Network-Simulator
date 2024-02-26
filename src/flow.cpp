#include <stdlib.h>
#include <typeinfo>
#include <typeindex>
#include <map>
#include <ctime>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>

#include "flow.h"
#include "manager.h"
#include "packet.h"
#include "endpoint.h"
#include "switch.h"
#include "networkObject.h"
#include "config.h"
#include "generator.h"
#include "observer.h"

#define FLOW_STR        "Flow"
#define TX_PACKET_EVENT "flow create packet event"

// All following defines can be overridden in flowDefines.h, which can be modified for local testing
#if __has_include("flowDefines.h")
#include "flowDefines.h"
#else
#define MIN_RATE 500.0
#define ACK_HEADER_SIZE 20
#define ACK_BODY_SIZE 10
#define STAT_FILE_DIRECTORY "results"
#define NO_PACKET_WAIT 0.01
#endif // __has_include

#define MAX_RTT 1

using namespace std;

enum FlowType {
    BasicFlowType,
    MBDFlowType,
    NDDFlowType,
#ifdef _TEST
    TestFlowType,
#endif /* _TEST */
};

void combineFlowConfigs(json &flowNetConfig, json &flowTestConfig) {
    // check net has Id
    if (!hasMemberOfType(flowNetConfig, "id", jsonInt)) {
        return;
    }

    // find flowTest with corresponding if exists
    for (json::iterator it = flowTestConfig.begin(); it != flowTestConfig.end(); ++it) {
        if (hasMemberOfType(it.value(), "id", jsonInt)) {
            // if match
            if (int(flowNetConfig["id"]) == int(it.value()["id"])) {
                // add it to flowNetConfig
                flowNetConfig.merge_patch(it.value());
                return;
            }
        }
    }
}

Flow *createFlow(json &flowNetConfig, json &flowTestConfig) {
    static map<string, FlowType> flowTypeMap = {
        {"basic", BasicFlowType},
        {"mbd", MBDFlowType},
        {"ndd", NDDFlowType},
#ifdef _TEST
        {"test", TestFlowType},
#endif /* _TEST */
    };

    if (!hasMemberOfType(flowNetConfig, "type", jsonString)) {
        throw runtime_error("Flow:\nNo string with name 'type'\n" + flowNetConfig.dump(4));
    }

    string flowTypeString = flowNetConfig["type"];
    FlowType flowType;
    try {
        flowType = flowTypeMap.at(flowTypeString);
    } catch (out_of_range&) {
        throw runtime_error("Flow:\nInvalid flow type: " + flowTypeString);
    }

    // combine with test object
    combineFlowConfigs(flowNetConfig, flowTestConfig);

    Flow *flow;

    switch (flowType) {
        case BasicFlowType:
            flow = new BasicFlow(flowNetConfig);
            break;
        case MBDFlowType:
            flow = new MBDFlow(flowNetConfig);
            break;
        case NDDFlowType:
            flow = new NDDFlow(flowNetConfig);
            break;
#ifdef _TEST
        case TestFlowType:
            flow = new TestFlow(flowNetConfig);
            break;
#endif /* _TEST */
        default:
            throw runtime_error("Flow:\nInvalid flow type: " + flowTypeString);
    }

    return flow;
}

//flowConfig already validated
Flow::Flow(json &flowConfig):
    NetworkObject(flowConfig["id"])
    {
    validateFlowConfig(flowConfig);

    this->rate = flowConfig["start_rate"];
    this->maxRate = 0; // will be corrected when the flow is initialized

    this->startTime = flowConfig["start_time"];
    this->endTime = flowConfig["end_time"];

    // create generator
    this->generator = createGenerator(flowConfig["generator"]);

    this->generator->setFlowId(flowConfig["id"]);

    this->sourceId = flowConfig["source_id"];
    this->destId = flowConfig["dest"];

    this->ttl = flowConfig["ttl"];

    this->packetsCreated = 0;
    this->packetsArrived = 0;
    this->acksArrived = 0;
    this->packetsTimedOut = 0;
    this->packetsDropped = 0;
    this->packetsErrored = 0;
    this->bytesArrived = 0;
    this->averageRTT = 0;
    this->averageHops = 0;
    this->outOfOrderCount = 0;
    this->throughput = 0;
    this->sentRate = this->rate;
    this->curSourcePId = 0;
    this->newestArrivedId = this->curSourcePId;
    this->curSinkPId = 0;
    this->running = false;
    this->sendingPacket = NULL;
    this->ackHeadSize = ACK_HEADER_SIZE;
    this->ackBodySize = ACK_BODY_SIZE;
}

void Flow::validateFlowConfig(json &flowConfig) {
    string message = "";

    if (!hasMemberOfType(flowConfig, "source_id", jsonInt)) {
        message += "No integer with name 'source_id'.\n";
    }

    if (!hasMemberOfType(flowConfig, "dest", jsonInt)) {
        message += "No integer with name 'dest'.\n";
    }

    if (!hasMemberOfType(flowConfig, "ttl", jsonInt)) {
        message += "No integer with name 'ttl'.\n";
    }

    if (!hasMemberOfType(flowConfig, "start_rate", jsonDouble)) {
        message += "No double with name 'start_rate'.\n";
    }

    if (!hasMemberOfType(flowConfig, "start_time", jsonDouble)) {
        message += "No double with name 'start_time'\n";
    }

    if (!hasMemberOfType(flowConfig, "end_time", jsonDouble)) {
        message += "No double with name 'end_time'\n";
    }

    // generator
    if (!hasMember(flowConfig, "generator")) {
        message += "No object with name 'generator'.\n";
    }

    if (!message.empty()) {
        message = "Flow:\n" + message + flowConfig.dump(4);
        throw runtime_error(message);
    }
}

Flow::~Flow() {
    for (auto it : sourcePackets) {
        delete it.second;
    }
    sourcePackets.clear();

    for (auto it : sinkPackets) {
        delete it.second;
    }
    sinkPackets.clear();

    delete generator;
}

void Flow::initializeFlow() {
    this->maxRate = getMaxRate();

    Switch *s = man.getSwitch(sourceId);
    if (s == NULL) {
        throw runtime_error("Switch id " + to_string(sourceId) + " doesn't exist");
    }
    this->minHops = int(s->costToDest(destId));
}

void Flow::removePacket(Packet *p) {
    this->bursts[p->getBurstId()].erase(p->getId());
    if (this->bursts[p->getBurstId()].empty()) {
        this->bursts.erase(p->getBurstId());
    } else if (p->isLastInBurst()) { // else not last packet of burst and was last packet
        // update next last in burst
        int nextLastId = *this->bursts[p->getBurstId()].rbegin();
        // if source check source
        if (p->isSourcePacket()) {
            sourcePackets[nextLastId]->setLastInBurst();
        } else {
            sinkPackets[nextLastId]->setLastInBurst();
        }
    }

    if (p->isSourcePacket()) {
        sourcePackets.erase(p->getId());
    } else {
        sinkPackets.erase(p->getId());
    }
}

bool Flow::addPacket(Packet *p) {
    this->bursts[p->getBurstId()].insert(p->getId());

    if (p->isSourcePacket()) {
        return sourcePackets.emplace(p->getId(), p).second;
    }
    return sinkPackets.emplace(p->getId(), p).second;
}

void Flow::packetArrived(Packet *p) {
    if (p->isSourcePacket()) {
        Flow::sourcePacketArrived(p);
    } else {
        Flow::sinkPacketArrived(p);
    }
}

void Flow::sourcePacketArrived(Packet *p) {
    bytesArrived += p->fullSize();

    packetsArrived++;

    this->averageHops += 1 / packetsArrived * (p->hopCount() - averageHops);

    int pId = p->getId();
    if (pId > this->newestArrivedId) {
        this->newestArrivedId = pId;
    } else {
        this->outOfOrderCount++;
    }

    removePacket(p);
    delete p;
}

void Flow::sinkPacketArrived(Packet *p) {
    acksArrived++;

    second_t packetRTT = man.time - p->getAckedSendTime();

    averageRTT += (packetRTT - averageRTT) / acksArrived;
    minRTT = min(minRTT, packetRTT);

    removePacket(p);
    delete p;
}

void Flow::packetDropped(Packet *p) {
    if (p->outOfTime()) {
        packetsTimedOut++;
    }
    packetsDropped++;
    removePacket(p);
    delete p;
}

void Flow::packetError(Packet *p) {
    packetsErrored++;
    removePacket(p);
    delete p;
}

int Flow::newPacketId(bool fromSource) {
    if (fromSource) {
        return curSourcePId++;
    }
    return curSinkPId++;
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
    if (man.getHandler(sourceId) == NULL) {
        fprintf(stderr, "Flow: endpoint of flow %d is missing\n", id);
        return false;
    }

    return true;
}

bool Flow::validateDest() {
    PacketHandler *handler = man.getHandler(destId);
    if (handler == NULL) {
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

Packet *Flow::getNextPacket(bool fromSource) {
    Generator::PacketData pData;

    if (!generator->getNextPacket(pData)) {
        return NULL;
    }

    int pId = newPacketId(fromSource);

    // create packet for this
    Packet *p = new Packet(pId, sourceId, destId, id, pData.burstId, pData.lastInBurst, NULL_DATA_ID, ttl,
            pData.headerSize, pData.bodySize, fromSource);

    if (!addPacket(p)) {
        delete p;
        return NULL;
    }

    packetsCreated++;

    return p;
}

second_t Flow::nextTxTime(Packet *p) {
    if (p == NULL) {
        return -1;
    }

    return man.time + (p->fullSizeBits() / rate);
}

double Flow::getMaxRate() {
    PacketHandler *h = man.getHandler(sourceId);
    if (h == NULL) {
        throw runtime_error("Flow: getMaxRate: could not find handler " + to_string(sourceId));
    }
    return h->getMaxOutputRate();
}

// returns the packet id of the last packet in the burst if known otherwise NULL_ID
int Flow::getBurstLastPacketId(int burstId) {
    if (this->bursts.find(burstId) == bursts.end()) {
        return NULL_ID;
    }

    return *this->bursts[burstId].rbegin();
}

/* BasicFlow */

BasicFlow::BasicFlow(json &flowConfig): Flow(validateBasicFlowConfig(flowConfig)) {
}

void BasicFlow::initializeFlow() {
    EventI *e1 = new Event<BasicFlow>(startTime, &BasicFlow::startFlow, this);
    man.pushEvent(e1);

    if (endTime != 0) {
        EventI *e2 = new Event<BasicFlow>(endTime, &BasicFlow::stopFlow, this);
        man.pushEvent(e2);
    }

    second_t nextRecord = man.time + man.miTime;
    EventI *e3 = new Event<BasicFlow>(nextRecord, &BasicFlow::recordData, this);
    man.pushEvent(e3);

    Flow::initializeFlow();
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

void BasicFlow::startFlow() {
    if (running) {
        return;
    }

    resetData();

    generator->startTraffic();

    running = true;

    sendingPacket = getNextPacket(true);

    second_t nextTx = nextTxTime(sendingPacket);
    if (nextTx >= 0) {
        EventI *e1 = new Event<BasicFlow>(nextTx, &BasicFlow::txPacketEvent, this);
        man.pushEvent(e1);
    }
}

void BasicFlow::stopFlow() {
    running = false;
}

void BasicFlow::txPacketEvent() {
    if (!running) {
        return;
    }

    PacketHandler *handler = man.getHandler(sourceId);
    if (handler == NULL) {
        throw runtime_error("BasicFlow: txPacketEvent: sourceId " + to_string(sourceId) + " does not exist");
    }

    man.logTxEvent(FLOW_STR, id, TX_PACKET_EVENT, sourceId, sendingPacket);

    man.logEvent("BasicFlow", this->id, "Flow Packet Tx", "Sent packet " + to_string(sendingPacket->getId()) +
                    " from flow " + to_string(this->id));
    // need to do before handler->txPacket as the packet may be deleted
    packetsSent++;
    bytesSent += sendingPacket->fullSize();

    handler->txPacket(sendingPacket);

    sendingPacket = getNextPacket(true); // TODO should this be true

    second_t nextTx = nextTxTime(sendingPacket);
    if (nextTx >= 0) {
        EventI *e = new Event<BasicFlow>(nextTx, &BasicFlow::txPacketEvent, this);
        man.pushEvent(e);
    }
}

Packet *BasicFlow::createAckPacket(Packet *toAck) {
    int pId = newPacketId(false);

    // create packet
    Packet *ackPacket = new Packet(pId, destId, sourceId, id, NULL_BURST_ID, true, NULL_DATA_ID, ttl, ackHeadSize, ackBodySize, false);

    // add state
    ackPacket->setAckData(toAck->getSendTime(), toAck->fullSize(), toAck->getId());

    return ackPacket;
}

void BasicFlow::txAck(Packet *toAckPacket) {
    PacketHandler *handler = man.getHandler(destId);

    // create
    Packet *ackPacket = createAckPacket(toAckPacket);

    // send
    handler->txPacket(ackPacket);

    man.logEvent("BasicFlow", this->id, "Flow Ack Tx", "Sent Ack " + to_string(ackPacket->getId()) +
                    " from flow " + to_string(this->id));
}

void BasicFlow::packetArrived(Packet *p) {
    if (p->isSourcePacket()) {
        sourcePacketArrived(p);
    }
    Flow::packetArrived(p);
}

void BasicFlow::sourcePacketArrived(Packet *p) {
    //txAck(p);
}

double BasicFlow::getAveragePacketSizeBytes() {
    return generator->getAveragePacketSizeBytes();
}

void BasicFlow::packetGenerationNotification() {
    if (sendingPacket == NULL) {
        sendingPacket = getNextPacket(true);

        second_t nextTx = nextTxTime(sendingPacket);
        if (nextTx >= 0) {
            EventI *e = new Event<BasicFlow>(nextTx, &BasicFlow::txPacketEvent, this);
            man.pushEvent(e);
        }
    }
}

void BasicFlow::resetData() {
    packetsSent = 0;
    packetsTimedOut = 0;
    packetsDropped = 0;
    bytesSent = 0;
    bytesArrived = 0;
    averageRTT = 0;
    averageHops = 0;
    outOfOrderCount = 0;
    minRTT = MAX_RTT;

    packetsArrived = 0;
    acksArrived = 0;
}

void BasicFlow::recordData() {
    // if nothing was sent and not running report nan
    bool reportNan = false;
    if (!running && bytesSent == 0) {
        reportNan = true;
    }

    throughput = bytesArrived * BITS_PER_BYTE / man.miTime;
    sentRate = bytesSent * BITS_PER_BYTE / man.miTime;
    double hop_ratio = averageHops / double(minHops);
    double outOfOrderRatio = outOfOrderCount / double(packetsArrived);

    observer.logFlowData(id, "AverageHops", averageHops, reportNan);
    observer.logFlowData(id, "HopRatio", hop_ratio, reportNan);
    observer.logFlowData(id, "Throughput", throughput, reportNan);
    observer.logFlowData(id, "AverageRTT", averageRTT, reportNan);
    observer.logFlowData(id, "MinRTT", minRTT, reportNan);
    observer.logFlowData(id, "SentRate", sentRate, reportNan);
    observer.logFlowData(id, "PacketsArrived", packetsArrived, reportNan);
    observer.logFlowData(id, "AcksArrived", acksArrived, reportNan);
    observer.logFlowData(id, "OutOfOrderRatio", outOfOrderRatio, reportNan);

    // packet counts
    observer.logFlowData(id, "SentPackets", packetsSent, reportNan);
    observer.logFlowData(id, "TimedOutPackets", packetsTimedOut, reportNan);
    observer.logFlowData(id, "CongestedPackets", packetsDropped - packetsTimedOut, reportNan);
    observer.logFlowData(id, "DroppedPackets", packetsDropped, reportNan);
    observer.logFlowData(id, "ErroredPackets", packetsErrored, reportNan);

    resetData();

    second_t nextRecord = man.time + man.miTime;
    EventI *e = new Event<BasicFlow>(nextRecord, &BasicFlow::recordData, this);
    man.pushEvent(e);
}

/* Manhattan Bandit Deflection Flow */

MBDFlow::MBDFlow(json &flowConfig): BasicFlow(flowConfig) {
}

Packet *MBDFlow::getNextPacket(bool fromSource) {
    Generator::PacketData pData;

    if (!generator->getNextPacket(pData)) {
        return NULL;
    }

    int pId = newPacketId(fromSource);

    MBDPacket *p = new MBDPacket(pId, sourceId, destId, id, pData.burstId, pData.lastInBurst, NULL_DATA_ID, ttl, pData.headerSize, pData.bodySize, fromSource);

    if (!addPacket(p)) {
        delete p;
        return NULL;
    }

    packetsCreated++;

    return p;
}

/* NDD Flow */

NDDFlow::NDDFlow(json &flowConfig): BasicFlow(flowConfig) {
}

Packet *NDDFlow::getNextPacket(bool fromSource) {
    Generator::PacketData pData;

    if (!generator->getNextPacket(pData)) {
        return NULL;
    }

    int pId = newPacketId(fromSource);

    NDDPacket *p = new NDDPacket(pId, sourceId, destId, id, pData.burstId, pData.lastInBurst, NULL_DATA_ID, ttl, pData.headerSize, pData.bodySize, fromSource);

    if (!addPacket(p)) {
        delete p;
        return NULL;
    }

    packetsCreated++;

    return p;
}

/* CUBIC Flow */
/*
CUBICFlow::CUBICFlow(json &flowConfig): Flow(flowConfig) {
}

CUBICFlow::initializeFlow() {
    CUBICInitialization();

    Flow::initializeFlow()
}

CUBICFlow::CUBICInitialization() {
    this->cwnd = ???;
    this->tcpFriendliness = 1;
    this->fastConvergence = 1;
    this->beta = 0.2; // 0.2 from paper, 0.7 from RFC 8312 Sec. 4.5
    this->C = 0.4; // 0.4 from paper and from RFC 8312 Sec. 4.5
    CUBICReset();
}

// ack packet arrived
void CUBICFlow::CUBICAckArrived() {
    if dMin then dMin <- min(dMin,RTT)
    else dMin <- RTT

    if cwnd <= ssthresh then cwnd <- cwnd + 1
    else
        cnt <- cubic_update()
}

// packet loss
void CUBICFlow::CUBICpacketLoss() {
    epoch_start <- 0
    if cwnd < Wlast_max and fast_convergance then
        Wlast_max <- cwnd * (2 - beta) / 2
    else Wlast_max <- cwnd
    wwthresh <- cwnd <- cwnd * (1 - beta)
}

// timeout
// queue function called
void CUBICFlow::timeout() {
    CUBICReset();
}

// cubic update
void CUBICFlow::CUBICUpdate() {
    ack_cnt <- ack_cnt + 1
    if epoch_start <= 0 then
        epoch_start <- tcp_time_stamp
        if cwnd < Wlast_time then
            K <- // cubic function
            origin_point <- Wlast_max
        else
            K <- 0
            origin_point <- cwnd
        ack_cnt <- 1
        Wtcp <- cwnd
    t <- tcp_time_stamp + dMin - epoch_start
    target <- origin_point + C(t - K)^3
    if target > cwnd then cnt <- cwnd/(target-cwnd)
    else cnt <- 100 * cwnd
    if tcp_friendliness then cubic_friendliness()
}

void CUBICFlow::CUBICTCPFriendliness() {
    Wtcp <- Wctp + (3 * beta) / (2 - beta) * ack_cnt/cwnd
    ack_cnt <- 0
    if Wtcp > cwnd then
        max_cnt <- cwnd / (Wtcp - cwnd)
        if cnt > max_cnt then cnt <- max_cnt
}

void CUBICFlow::CUBICReset() {
    Wlast_max <- 0
    epoch_start <- 0
    origin_point <- 0
    dMin <- 0
    Wtcp <- 0
    K <- 0
    ack_cnt <- 0
}
*/

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

second_t TestFlow::nextTxTime(Packet *p) {
    static const second_t min = 0.001, max = 0.01;

    return man.time + min + (float)rand() / (RAND_MAX / (max - min));
}

void TestFlow::startFlow() {
    if (running) {
        return;
    }

    second_t nextTx = nextTxTime(NULL);
    EventI *e1 = new Event<TestFlow>(nextTx, &TestFlow::txPacketEvent, this);
    man.pushEvent(e1);

    EventI *e2 = new Event<TestFlow>(man.time + man.miTime, &TestFlow::stepAgent, this);
    man.pushEvent(e2);

    generator->startTraffic();

    running = true;
}

void TestFlow::stepAgent() {
    //TODO: get state and reward
    double state = 0;
    double reward = 0;
    agent->step(state, reward,rate);
}

void TestFlow::txPacketEvent() {
    if (!running) {
        return;
    }

    static const int hMin = 10, hMax = 50;
    static const int bMin = 40, bMax = 120;
    static const int ttl = 15;

    int hSize = hMin + rand() % (hMax - hMin);
    int bSize = bMin + rand() % (bMax - bMin);

    Endpoint *endpoint = man.getEndpoint(sourceId);
    // todo test for error

    Packet *p = getNextPacket(true);

    man.logTxEvent(FLOW_STR, id, TX_PACKET_EVENT, sourceId, p);

    endpoint->txPacket(p);

    // set up next
    second_t nextTx = nextTxTime(NULL);
    EventI *e = new Event<TestFlow>(nextTx, &TestFlow::txPacketEvent, this);
    man.pushEvent(e);
}
#endif /* _TEST */
