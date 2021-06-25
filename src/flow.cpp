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
#include "networkObject.h"
#include "config.h"
#include "agent.h"
#include "actorCritic.h"
#include "sarsa.h"
#include "generator.h"
#include "observer.h"

#define FLOW_STR        "Flow"
#define TX_PACKET_EVENT "flow create packet event"

// All following defines can be overridden in flowDefines.h, which can be modified for local testing
#if __has_include("flowDefines.h")
#include "flowDefines.h"
#else
#define MIN_RATE 500.0
#define NUM_AGENT_STEPS 100
#define MI_TIME 10
#define PACKET_HEADER_SIZE 20
#define PACKET_BODY_SIZE 236
#define STAT_FILE_DIRECTORY "results"
#define NO_PACKET_WAIT 0.01

#define DEFAULT_REWARD_TYPE BasicReward

#define INITIAL_STATE {0}
#endif // __has_include

#define MAX_RTT 1

using namespace std;

enum FlowType {
    BasicFlowType,
    ECNFlowType,
#ifdef _TEST
    TestFlowType,
#endif /* _TEST */
};

Flow *createFlow(json &flowConfig) {
    static map<string, FlowType> flowTypeMap = {
        {"basic", BasicFlowType},
        {"ecn", ECNFlowType},
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
        case ECNFlowType:
            flow = new ECNFlow(flowConfig);
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

// TODO move out of Flow
double Flow::initialAverageReward() {
    double rBar = 0;
    double expectedPackets = floor(this->rate*MI_TIME/(PACKET_HEADER_SIZE + PACKET_BODY_SIZE)/8); // new

    switch (DEFAULT_REWARD_TYPE) {
        case OffsetReward:
        case ECNReward:
        case BasicReward:
            rBar = expectedPackets;
            break;
        case AdvancedReward:
        case AdvancedPenaltyReward:
        case RateReward:
            rBar = expectedPackets / pow(this->rate, 0.5);
            break;
        case LogReward:
            rBar = expectedPackets;
            if (rBar != 0) {
                rBar = log(rBar) + 1;
            }
            break;
        case NegativeReward:
            rBar = 0;
            break;
        case ExpertReward:
            rBar = 1;
            break;
        case ThroughputReward:
        case ThroughputDemandReward:
            rBar = 0;
            break;
        case LogThroughput:
            rBar = log(rate);
            break;
        case REMY1:
            rBar = log(rate); // ignore RTT for now, average reward should increase
            break;
        case REMY2:
            rBar = -1/rate;
            break;
    }

    return rBar;
}

//flowConfig already validated
Flow::Flow(json &flowConfig):
    NetworkObject(flowConfig["id"])
    //TODO: Second parameter is initial state, should it be something other than 0?
    //,agent(new AGENT_TYPE(man.getEndpoint(flowConfig["source_id"])->getWeights(), 0))
    {
    validateFlowConfig(flowConfig);

    this->rate = flowConfig["start_rate"];

    this->startTime = flowConfig["start_time"];
    this->endTime = flowConfig["end_time"];

    // create generator
    this->generator = createGenerator(flowConfig["generator"]);

    // TODO move agents into ECN Flow
    vector<double> initialState = INITIAL_STATE;
    switch (AGENT_TYPE) {
        case ActorCriticAgent:
            {
                double rBar = initialAverageReward();

                agent = new ActorCritic(initialState, flowConfig["id"], rBar);
                break;
            }
        case SarsaAgent:
            agent = new Sarsa(initialState, flowConfig["id"]);
            break;
    }
    this->sourceId = flowConfig["source_id"];
    this->destId = flowConfig["dest"];

    this->packetsCreated = 0;
    this->packetsArrived = 0;
    this->acksArrived = 0;
    this->packetsDropped = 0;
    this->packetsErrored = 0;
    this->bytesArrived = 0;
    this->throughput = 0;
    this->oldThroughput = 0;
    this->sentRate = this->rate;
    this->curSourcePId = 0;
    this->curSinkPId = 0;
    this->miTime = MI_TIME;
    this->ttl = 15; // TODO use define
    this->running = false;
    this->ackHeadSize = ACK_HEADER_SIZE;
    this->ackBodySize = ACK_BODY_SIZE;

    this->rewardType = DEFAULT_REWARD_TYPE;
}

void Flow::validateFlowConfig(json &flowConfig) {
    string message = "";

    if (!hasMemberOfType(flowConfig, "source_id", jsonInt)) {
        message += "No integer with name 'source_id'.\n";
    }

    if (!hasMemberOfType(flowConfig, "dest", jsonInt)) {
        message += "No integer with name 'dest'.\n";
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

    for (auto it : sinkPackets) {
        delete it.second;
    }

    delete agent;
    delete generator;
}

void Flow::removePacket(Packet *p) {
    if (p->isSourcePacket()) {
        sourcePackets.erase(p->getId());
    } else {
        sinkPackets.erase(p->getId());
    }
}

bool Flow::addPacket(Packet *p) {
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

Packet *Flow::getNextPacket(bool fromSource) {
    Generator::PacketData pData;

    if (!generator->getNextPacket(pData)) {
        return NULL;
    }

    int pId = newPacketId(fromSource);

    // create packet for this
    Packet *p = new Packet(pId, sourceId, destId, id, ttl, pData.headerSize, pData.bodySize, fromSource);

    if (!addPacket(p)) {
        delete p;
        return NULL;
    }
    packetsCreated++;

    return p;
}

second_t Flow::nextTxTime(Packet *p) {
    if (p == NULL) {
        return man.time + NO_PACKET_WAIT;
    }

    return man.time + (p->fullSizeBits() / rate);
}

double Flow::getMaxRate() {
    Endpoint *e = man.getEndpoint(sourceId);
    return e->getMaxOutputRate();
}

double Flow::getTotalReward() {
    return totalReward;
}

BasicFlow::BasicFlow(json &flowConfig): Flow(validateBasicFlowConfig(flowConfig)) {
}

void BasicFlow::initializeFlow() {
    EventI *e = new Event<BasicFlow>(startTime, &BasicFlow::startFlow, this);
    man.pushEvent(e);

    if (endTime != 0) {
        EventI *e = new Event<BasicFlow>(endTime, &BasicFlow::stopFlow, this);
        man.pushEvent(e);
    }
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

    second_t nextTx = nextTxTime(NULL);
    EventI *e1 = new Event<BasicFlow>(nextTx, &BasicFlow::txPacketEvent, this);
    man.pushEvent(e1);

    second_t nextRecord = man.time + miTime;
    EventI *e = new Event<BasicFlow>(nextRecord, &BasicFlow::recordData, this);
    man.pushEvent(e);
}

void BasicFlow::stopFlow() {
    running = false;
}

void BasicFlow::stepAgent() {
}

void BasicFlow::txPacketEvent() {
    if (!running) {
        return;
    }

    Endpoint *endpoint = man.getEndpoint(sourceId);
    // TODO test for error

    Packet *p = getNextPacket(true); // TODO should this be true

    // only tx if a packet exists
    if (p != NULL) {
        man.logTxEvent(FLOW_STR, id, TX_PACKET_EVENT, sourceId, p);

        endpoint->txPacket(p);

        packetsSent++;
        bytesSent += p->fullSize();
    }

    second_t nextTx = nextTxTime(NULL);
    EventI *e = new Event<BasicFlow>(nextTx, &BasicFlow::txPacketEvent, this);
    man.pushEvent(e);
}

Packet *BasicFlow::createAckPacket(Packet *toAck) {
    int pId = newPacketId(false);

    // create packet
    Packet *ackPacket = new Packet(pId, destId, sourceId, id, ttl, ackHeadSize, ackBodySize, false);

    // add state
    ackPacket->setAckData(toAck->getSendTime(), toAck->fullSize(), toAck->getId());

    return ackPacket;
}

void BasicFlow::txAck(Packet *toAckPacket) {
    Endpoint *endpoint = man.getEndpoint(destId);

    // create
    Packet *ackPacket = createAckPacket(toAckPacket);

    // send
    endpoint->txPacket(ackPacket);

    man.logEvent("ECNFlow", this->id, "Flow Ack Tx", "Sent Ack " + to_string(ackPacket->getId()) +
                    " from flow " + to_string(this->id));
}

void BasicFlow::packetArrived(Packet *p) {
    if (p->isSourcePacket()) {
        sourcePacketArrived(p);
    }
    Flow:: packetArrived(p);
}

void BasicFlow::sourcePacketArrived(Packet *p) {
    txAck(p);
}

double BasicFlow::getAveragePacketSizeBytes() {
    return generator->getAveragePacketSizeBytes();
}

void BasicFlow::resetData() {
    packetsSent = 0;
    packetsDropped = 0;
    bytesSent = 0;
    bytesArrived = 0;
    averageRTT = 0;
    minRTT = MAX_RTT;

    packetsArrived = 0;
    acksArrived = 0;
}

void BasicFlow::recordData() {
    throughput = bytesArrived / miTime;
    sentRate = bytesSent * BITS_PER_BYTE / miTime;

    observer.logFlowData(id, "Throughput", throughput);
    observer.logFlowData(id, "AverageRTT", averageRTT);
    observer.logFlowData(id, "MinRTT", minRTT);
    observer.logFlowData(id, "SentRate", sentRate);
    observer.logFlowData(id, "PacketsArrived", packetsArrived);
    observer.logFlowData(id, "AcksArrived", acksArrived);

    if (packetsSent == 0) {
        observer.logFlowData(id, "DroppedPackets", 0);
    } else {
        observer.logFlowData(id, "DroppedPackets", packetsDropped/packetsSent);
    }

    resetData();

    if (!running) {
        return;
    }

    second_t nextRecord = man.time + miTime;
    EventI *e = new Event<BasicFlow>(nextRecord, &BasicFlow::recordData, this);
    man.pushEvent(e);
}

/* Explicit congestion notification flow */

ECNFlow::ECNFlow(json &flowConfig): Flow(validateECNFlowConfig(flowConfig)) {
    this->packetsUntagged = 0;
    this->packetsSent = 0;
    this->bytesSent = 0;
    this->bytesArrived = 0;
    this->averageRTT = 0;
    this->minRTT = MAX_RTT;
    this->averageECN = 0;
    this->maxRate = getMaxRate();
    this->numSteps = 0;
    this->maxSteps = NUM_AGENT_STEPS;

    Sarsa *agent = dynamic_cast<Sarsa *>(this->agent);
    if (agent != NULL) {
        agent->setAveragePacketSizeBytes(getAveragePacketSizeBytes());
    }
}

json &ECNFlow::validateECNFlowConfig(json &flowConfig) {
    string message = "";

    if (!hasMemberOfType(flowConfig, "id", jsonInt)) {
        message += "No integer with name 'id'.\n";
    }

    if (!message.empty()) {
        message = "ECN Flow:\n" + message + flowConfig.dump(4);
        throw runtime_error(message);
    }

    return flowConfig;
}

ECNPacket *ECNFlow::getNextPacket(bool fromSource) {
    Generator::PacketData pData;

    if (!generator->getNextPacket(pData)) {
        return NULL;
    }

    int pId = newPacketId(fromSource);

    int packetSourceId = sourceId;
    int packetDestId = destId;

    if (!fromSource) {
        packetSourceId = destId;
        packetDestId = sourceId;
    }

    // create packet for this
    ECNPacket *p = new ECNPacket(pId, packetSourceId, packetDestId, id, ttl, pData.headerSize, pData.bodySize,
            fromSource);

    if (!addPacket(p)) {
        delete p;
        return NULL;
    }
    packetsCreated++;

    return p;
}

ECNPacket *ECNFlow::createAckPacket(ECNPacket *toAck) {
    int pId = newPacketId(false);

    // create packet
    ECNPacket *ackPacket = new ECNPacket(pId, destId, sourceId, id, ttl, ackHeadSize, ackBodySize, false);

    // add state
    ackPacket->setAckData(toAck->getSendTime(), toAck->fullSize(), toAck->getECNBit(), toAck->getECNScale(),
            toAck->getId());

    return ackPacket;
}

void ECNFlow::initializeFlow() {
    EventI *e = new Event<ECNFlow>(startTime, &ECNFlow::startFlow, this);
    man.pushEvent(e);

    if (endTime != 0) {
        EventI *e = new Event<ECNFlow>(endTime, &ECNFlow::stopFlow, this);
        man.pushEvent(e);
    }
}

void ECNFlow::startFlow() {
    if (running) {
        return;
    }

    // create txPacket event
    second_t nextTx = nextTxTime(NULL);
    EventI *e1 = new Event<ECNFlow>(nextTx, &ECNFlow::txPacketEvent, this);
    man.pushEvent(e1);

    EventI *e2 = new Event<ECNFlow>(man.time + miTime, &ECNFlow::stepAgent, this);
    man.pushEvent(e2);

    // start generator
    generator->startTraffic();

    running = true;
}

void ECNFlow::stopFlow() {
    running = false;
}

vector<double> ECNFlow::getState() {
    double averageECNFeature = averageECN;
    double throughputFeature = throughput / maxRate;
    double rateSelectedFeature = rate / maxRate;
    double dropRateFeature = packetsSent == 0 ? 0 : packetsDropped / packetsSent; // TODO figure out bug
    double averageRTTFeature = averageRTT / MAX_RTT;
    double queueDelayFeature = (averageRTT - minRTT) / MAX_RTT;
    double rateSentFeature = sentRate / maxRate;

    // average max buffer occupancy
    return {averageECNFeature, throughputFeature, rateSelectedFeature, dropRateFeature, averageRTTFeature,
        queueDelayFeature, rateSentFeature};
}

double ECNFlow::getReward() {
    switch (rewardType) {
        case BasicReward:
            return packetsUntagged;
        case RateReward:
            return packetsUntagged / pow(rate, 0.5);
        case LogReward:
            if (packetsUntagged == 0) {
                return 0;
            }
            return log(packetsUntagged) + 1;
        case AdvancedReward:
            return ((1 - averageECN)*packetsSent) / pow(rate, 0.5);
            /*
            // (untagged - tagged) / sqrt(rate)
            return (2*packetsUntagged - packetsSent) / pow(rate, 0.5);
            */
        case AdvancedPenaltyReward:
            {
                double r = ((1 - averageECN)*packetsSent) / pow(rate, 0.5);
                /*
                // (untagged - tagged) / sqrt(rate)
                double r = (2*packetsUntagged - packetsSent) / pow(rate, 0.5);
                */
                if (rate == MIN_RATE || rate == maxRate) {
                    r -= 1;
                }
                return r;
            }
        case NegativeReward:
            return packetsUntagged - packetsSent;
        case OffsetReward:
            return 2*packetsUntagged - packetsSent; // +1 if untagged, -1 if tagged
        case ECNReward:
            return (1 - averageECN)*packetsSent;
        case ExpertReward:
            if ((oldECN > 0.1) != (rate > oldRate)) { // (ECN greater than threshold) XOR (rate has increased)
                //Either ECN is low and rate increased or ECN is high and rate decreased
                return 1.0;
            } else {
                //Either ECN is low and rate decreased or ECN is high and rate increased
                return -1.0;
            }
        case ThroughputReward:
            if (oldThroughput < throughput) {
                return 1;
            } else if (oldThroughput > throughput) {
                return -1;
            }
            return 0;
        case ThroughputDemandReward:
            {
                double r = 0;
                if (oldThroughput < throughput) {
                    r = 1;
                } else if (oldThroughput > throughput) {
                    r = -1;
                } else {
                    r = 0;
                }
                if (throughput > 50000) {
                    //r -= 1;
                    //r -= 0.5;
                    r = -1;
                }

                return r;
            }
        case LogThroughput:
            return log(throughput);
        case REMY1:
            return log(throughput) - log(averageRTT);
        case REMY2:
            return -1/throughput;
    }
    throw runtime_error("Invalid reward specified\n");
}

void ECNFlow::resetState() {
    packetsUntagged = 0;
    packetsSent = 0;
    averageECN = 0;
    packetsDropped = 0;
    bytesSent = 0;
    bytesArrived = 0;
    averageRTT = 0;
    minRTT = MAX_RTT;

    packetsArrived = 0;
    acksArrived = 0;
}

void ECNFlow::updateStats() {
    totalPacketsUntagged += packetsUntagged;
    totalPacketsSent += packetsSent;
    totalPacketsDropped += packetsDropped;
    throughput = bytesArrived / miTime;
    sentRate = bytesSent * BITS_PER_BYTE / miTime;

    observer.logFlowData(id, "Reward", getReward());
    observer.logFlowData(id, "Rates", rate);
    observer.logFlowData(id, "Throughput", throughput);
    observer.logFlowData(id, "AverageRTT", averageRTT);
    observer.logFlowData(id, "MinRTT", minRTT);
    observer.logFlowData(id, "SentRate", sentRate);
    observer.logFlowData(id, "PacketsArrived", packetsArrived);
    observer.logFlowData(id, "AcksArrived", acksArrived);
    observer.logFlowData(id, "ECNAverages", averageECN);

    if (packetsSent == 0) {
        observer.logFlowData(id, "DroppedPackets", 0);
    } else {
        observer.logFlowData(id, "DroppedPackets", packetsDropped/packetsSent);
    }
}

void ECNFlow::updateStatsPostStep(pair<double, double> action) {
    observer.logFlowData(id, "MultActions", action.first);
    observer.logFlowData(id, "AddAction", action.second);

    // Add distribution data
    ActorCritic *actorCriticAgent = dynamic_cast<ActorCritic*>(agent);
    if (actorCriticAgent != NULL) {
        pair<double, double> multMeanStdev = actorCriticAgent->getMultMeanStdev();
        observer.logFlowData(id, "MultMean", multMeanStdev.first);
        observer.logFlowData(id, "MultStdev", multMeanStdev.second);

        pair<double, double> addMeanStdev = actorCriticAgent->getAddMeanStdev();
        observer.logFlowData(id, "AddMean", addMeanStdev.first);
        observer.logFlowData(id, "AddStdev", addMeanStdev.second);
    }
}

void ECNFlow::stepAgent() {
    numSteps++;

    updateStats();
    vector<double> state = getState();

    double reward = getReward();
    totalReward += reward;

    pair<double, double> action = agent->step(state, reward, rate);
    oldRate = rate;
    oldECN = averageECN;
    oldThroughput = throughput;

    updateStatsPostStep(action);

    resetState();

    rate = min(maxRate, max(MIN_RATE, rate));
    if (numSteps < maxSteps && running) {
        EventI *e = new Event<ECNFlow>(man.time + miTime, &ECNFlow::stepAgent, this);
        man.pushEvent(e);
    } else {
        man.logEvent("ECNFlow", this->id, "End of program", "Acheived reward " + to_string(totalReward) +\
                        " with final rate of " + to_string(rate) + " and " + to_string(totalPacketsUntagged) +\
                        " out of " + to_string(totalPacketsSent) + " packets untagged.");

        observer.logFlowDataBulk(id, "Weights", agent->getWeights());

        generator->stopTraffic();
        // TODO stop sending new packets
        running = false;
    }
    man.logEvent("ECNFlow", this->id, "Agent Step", "Agent called with state " + string (state.begin(), state.end())
            + " and reward " + to_string(reward) + " and took action " + to_string(action.first) + ", "
            + to_string(action.second) + ", setting rate to " + to_string(rate));
}

void ECNFlow::txPacketEvent() {
    if (!running) {
        return;
    }

    Endpoint *endpoint = man.getEndpoint(sourceId);

    // create packet
    ECNPacket *p = getNextPacket(true); // TODO should this be true

    if (p != NULL) {
        endpoint->txPacket(p);
        // track sent packets
        packetsSent++;
        bytesSent += p->fullSize();
    }

    second_t nextTx = nextTxTime(p);

    EventI *e = new Event<ECNFlow>(nextTx, &ECNFlow::txPacketEvent, this);
    man.pushEvent(e);

    if (p!= NULL) {
        man.logEvent("ECNFlow", this->id, "Flow Packet Tx", "Sent packet " + to_string(p->getId()) +
                        " from flow " + to_string(this->id));
    }
}

void ECNFlow::txAck(ECNPacket *toAckPacket) {
    Endpoint *endpoint = man.getEndpoint(destId);

    // create
    ECNPacket *ackPacket = createAckPacket(toAckPacket);

    // send
    endpoint->txPacket(ackPacket);

    man.logEvent("ECNFlow", this->id, "Flow Ack Tx", "Sent Ack " + to_string(ackPacket->getId()) +
                    " from flow " + to_string(this->id));
}

double ECNFlow::getAveragePacketSizeBytes() {
    return generator->getAveragePacketSizeBytes();
}

void ECNFlow::packetArrived(Packet *p) {
    if (p->isSourcePacket()) {
        sourcePacketArrived(p);
    } else {
        sinkPacketArrived(p);
    }
    Flow::packetArrived(p);
}

void ECNFlow::sourcePacketArrived(Packet *p) {
    ECNPacket *ecnP = dynamic_cast<ECNPacket *>(p);
    if (ecnP != NULL) {
        man.logEvent("ECNFlow", this->id, "Flow Packet Arrived", "Source packet " + to_string(p->getId()) +
                        " arrived at destination.");
    } else {
        man.logEvent("ECNFlow", this->id, "Flow Packet Arrived", "Packet arrived but was null");
        // TODO oh no this is bad, really bad!
    }

    // create and send ack packet
    txAck(ecnP);
}

void ECNFlow::sinkPacketArrived(Packet *p) {
    ECNPacket *ecnP = dynamic_cast<ECNPacket *>(p);
    if (ecnP != NULL) {
        man.logEvent("ECNFlow", this->id, "Flow Packet Arrived", "Ack packet " + to_string(p->getId()) +
                        " arrived at destination.");

        ECNPacket::ackMetaData ackData = ecnP->getAckData();
        // TODO update stats
        if (!ackData.ECNBit) {
            packetsUntagged++;
        }
        if (packetsSent > 0) { // TODO shouldnt use packetsSent
            averageECN += ackData.bufferOccupancy / packetsSent;
            averageECN *= (double)packetsSent / (packetsSent + 1);
        } else {
            averageECN = ackData.bufferOccupancy;
        }
    } else {
        man.logEvent("ECNFlow", this->id, "Flow Packet Arrived", "Packet arrived but was null");
        // TODO oh no this is bad, really bad!
    }
}

void ECNFlow::packetDropped(Packet *p) {
    // TODO track acks separatly
    Flow::packetDropped(p);
}

void ECNFlow::packetError(Packet *p) {
    // TODO track acks separatly
    Flow::packetError(p);
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

    EventI *e2 = new Event<TestFlow>(man.time + miTime, &TestFlow::stepAgent, this);
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
