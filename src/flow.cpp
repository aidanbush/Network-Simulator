#include <stdlib.h>
#include <typeinfo>
#include <typeindex>
#include <map>
#include <ctime>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <filesystem>

#include "flow.h"
#include "manager.h"
#include "packet.h"
#include "endpoint.h"
#include "networkObject.h"
#include "config.h"
#include "agent.h"
#include "actorCritic.h"
#include "sarsa.h"

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
    }

    return rBar;
}

//flowConfig already validated
Flow::Flow(json &flowConfig):
    NetworkObject(flowConfig["id"])
    //TODO: Second parameter is initial state, should it be something other than 0?
    //,agent(new AGENT_TYPE(man.getEndpoint(flowConfig["source_id"])->getWeights(), 0))
    {
    Endpoint *end = man.getEndpoint(flowConfig["source_id"]);
    this->rate = flowConfig["start_rate"];

    // TODO move agents into ECN Flow
    vector<double> initialState = INITIAL_STATE;
    switch (end->getAgentType()) {
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
    this->curSourcePId = 0;
    this->curSinkPId = 0;
    this->miTime = MI_TIME;
    this->maxTime = NUM_AGENT_STEPS*miTime;

    this->rewardType = DEFAULT_REWARD_TYPE;
}

Flow::~Flow() {
    for (auto it : sourcePackets) {
        delete it.second;
    }
    for (auto it : sinkPackets) {
        delete it.second;
    }
    delete agent;
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
    bytesArrived += p->fullSizeBits();
    // TODO update to actual RTT not single direction
    packetsArrived++;

    second_t packetRTT = p->getTravelTime();

    averageRTT += (packetRTT - averageRTT) / packetsArrived;
    minRTT = min(minRTT, packetRTT);

    removePacket(p);
    delete p;
}

void Flow::sinkPacketArrived(Packet *p) {
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

Packet *Flow::createPacket(int ttl, int headSize, int bodySize, bool fromSource) {
    int pId = newPacketId(fromSource);

    Packet *p = new Packet(pId, sourceId, destId, id, ttl, headSize, bodySize, fromSource);

    if (!addPacket(p)) {
        delete p;
        return NULL;
    }
    packetsCreated++;

    return p;
}

double Flow::getMaxRate() {
    Endpoint *e = man.getEndpoint(sourceId);
    return e->getMaxOutputRate();
}

double Flow::getTotalReward() {
    return totalReward;
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
}

void BasicFlow::stepAgent() {
}

void BasicFlow::txPacketEvent() {
    Endpoint *endpoint = man.getEndpoint(sourceId);
    // TODO test for error

    Packet *p = createPacket(ttl, headSize, bodySize, true); // TODO should this be true

    man.logTxEvent(FLOW_STR, id, TX_PACKET_EVENT, sourceId, p);

    endpoint->txPacket(p);

    second_t nextTx = nextTxTime();
    EventI *e = new Event<BasicFlow>(nextTx, &BasicFlow::txPacketEvent, this);
    man.pushEvent(e);
}

double BasicFlow::getAveragePacketSizeBytes() {
    return double(headSize + bodySize);
}

/* Explicit congestion notification flow */

ECNFlow::ECNFlow(json &flowConfig): Flow(validateECNFlowConfig(flowConfig)) {
    this->packetsUntagged = 0;
    this->packetsSent = 0;
    this->throughput = 0;
    this->averageRTT = 0;
    this->minRTT = MAX_RTT;
    this->averageECN = 0;
    this->headSize = PACKET_HEADER_SIZE;
    this->bodySize = PACKET_BODY_SIZE;
    this->ackHeadSize = PACKET_HEADER_SIZE; // TODO change to be separate
    this->ackBodySize = PACKET_BODY_SIZE;
    this->ttl = 15;
    this->maxRate = getMaxRate();

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

    if (!hasMemberOfType(flowConfig, "source_id", jsonInt)) {
        message += "No integer with name 'source_id'.\n";
    }

    if (!hasMemberOfType(flowConfig, "dest", jsonInt)) {
        message += "No integer with name 'dest'.\n";
    }

    if (!hasMemberOfType(flowConfig, "start_rate", jsonDouble)) {
        message += "No double with name 'start_rate'.\n";
    }

    if (!message.empty()) {
        message = "Basic Flow:\n" + message + flowConfig.dump(4);
        throw runtime_error(message);
    }
    return flowConfig;
}

ECNPacket *ECNFlow::createPacket(int ttl, int headSize, int bodySize, bool fromSource) {
    int pId = newPacketId(fromSource);

    int packetSourceId = sourceId;
    int packetDestId = destId;

    if (!fromSource) {
        packetSourceId = destId;
        packetDestId = sourceId;
    }

    ECNPacket *p = new ECNPacket(pId, packetSourceId, packetDestId, id, ttl, headSize, bodySize, fromSource);

    if (!addPacket(p)) {
        delete p;
        return NULL;
    }
    packetsCreated++;

    return p;
}

// TODO source and dest are backwards???
ECNPacket *ECNFlow::createAckPacket(ECNPacket *toAck) {
    // create packet
    ECNPacket *ackPacket = createPacket(ttl, ackHeadSize, ackBodySize, false); // TODO refactor the source and dest are for the wrong direction

    // add state
    ackPacket->setAckData(toAck->getECNBit(), toAck->getECNScale(), toAck->getId());

    return ackPacket;
}

second_t ECNFlow::nextTxTime() {
    return man.time + ((headSize + bodySize) * BITS_PER_BYTE / rate);
}

void ECNFlow::startFlow() {
    // create txPacket event
    second_t nextTx = nextTxTime();
    EventI *e1 = new Event<ECNFlow>(nextTx, &ECNFlow::txPacketEvent, this);
    man.pushEvent(e1);

    EventI *e2 = new Event<ECNFlow>(man.time + miTime, &ECNFlow::stepAgent, this);
    man.pushEvent(e2);
}

vector<double> ECNFlow::getState() {
    double averageECNFeature = averageECN;
    double throughputFeature = throughput / maxRate;
    double rateFeature = rate / maxRate;
    double dropRateFeature = packetsSent == 0 ? 0 : packetsDropped / packetsSent; // TODO figure out bug
    double averageRTTFeature = averageRTT / MAX_RTT;
    double queueDelayFeature = (averageRTT - minRTT) / MAX_RTT;

    // average max buffer occupancy
    return {averageECNFeature, throughputFeature, rateFeature, dropRateFeature, averageRTTFeature, queueDelayFeature};
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
    }
    throw runtime_error("Invalid reward specified\n");
}

void ECNFlow::resetState() {
    packetsUntagged = 0;
    packetsSent = 0;
    averageECN = 0;
    packetsDropped = 0;
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

    rewardList.push_back(getReward());
    rateList.push_back(rate);
    throughputList.push_back(throughput);
    averageRTTList.push_back(averageRTT);
    minRTTList.push_back(minRTT);

    packetsArrivedList.push_back(packetsArrived);
    acksArrivedList.push_back(acksArrived);

    averageECNList.push_back(averageECN);
    if (packetsSent == 0) {
        packetsDroppedList.push_back(0);
    } else {
        packetsDroppedList.push_back(packetsDropped/packetsSent);
    }
}

void ECNFlow::updateStatsPostStep(pair<double, double> action) {
    actionMultList.push_back(action.first);
    actionAddList.push_back(action.second);

    // Add distribution data
    ActorCritic *actorCriticAgent = dynamic_cast<ActorCritic*>(agent);
    if (actorCriticAgent != NULL) {
        pair<double, double> multMeanStdev = actorCriticAgent->getMultMeanStdev();
        multMeanList.push_back(multMeanStdev.first);
        multStdevList.push_back(multMeanStdev.second);

        pair<double, double> addMeanStdev = actorCriticAgent->getAddMeanStdev();
        addMeanList.push_back(addMeanStdev.first);
        addStdevList.push_back(addMeanStdev.second);
    }
}

void ECNFlow::printCSV(string filename, vector<double> vec) {
    if (man.getSuppressOutput(CSV)) {
        //Don't create CSV files if in q mode
        return;
    }

    // TODO clean up placement of setting path
    if (!filesystem::exists(STAT_FILE_DIRECTORY)) {
        filesystem::create_directory(STAT_FILE_DIRECTORY);
    }

    ofstream ofs;
    ofs.open(filename, ofstream::trunc);
    if (vec.size() >= 1) {
        ofs << vec[0];
        for (int i = 1; i < (int)vec.size(); i++) {
            ofs << "," << vec[i];
        }
    }
    ofs.close();
}

void ECNFlow::stepAgent() {
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
    if (man.time + miTime <= maxTime) {
        EventI *e = new Event<ECNFlow>(man.time + miTime, &ECNFlow::stepAgent, this);
        man.pushEvent(e);
    } else {
        man.logEvent("ECNFlow", this->id, "End of program", "Acheived reward " + to_string(totalReward) +\
                        " with final rate of " + to_string(rate) + " and " + to_string(totalPacketsUntagged) +\
                        " out of " + to_string(totalPacketsSent) + " packets untagged.");
        time_t currentTime;
        time(&currentTime);
        tm *currentTm = localtime(&currentTime);
        char date[13];
        strftime(date, 13, "%Y%m%d%H%M", currentTm);

        string fileDir = man.getCSVDir();
        if (fileDir.empty()) {
            fileDir = string(STAT_FILE_DIRECTORY);
        }

        string filePathExceptSuffix = man.getCSVFilename();
        if (filePathExceptSuffix.empty()) {
            filePathExceptSuffix = fileDir + "/" + agent->getName() + "_" + date;
        } else {
            filePathExceptSuffix = fileDir + "/" + filePathExceptSuffix;
        }
        filePathExceptSuffix += "_Flow" + to_string(id);

        printCSV(filePathExceptSuffix + "_Rewards.csv", rewardList);
        printCSV(filePathExceptSuffix + "_Rates.csv", rateList);
        printCSV(filePathExceptSuffix + "_ECNAverages.csv", averageECNList);
        printCSV(filePathExceptSuffix + "_MultActions.csv", actionMultList);
        printCSV(filePathExceptSuffix + "_AddActions.csv", actionAddList);
        printCSV(filePathExceptSuffix + "_DroppedPackets.csv", packetsDroppedList);
        printCSV(filePathExceptSuffix + "_Throughput.csv", throughputList);
        printCSV(filePathExceptSuffix + "_AverageRTT.csv", averageRTTList);
        printCSV(filePathExceptSuffix + "_MinRTT.csv", minRTTList);

        printCSV(filePathExceptSuffix + "_PacketsArrived.csv", packetsArrivedList);
        printCSV(filePathExceptSuffix + "_AcksArrived.csv", acksArrivedList);

        if (dynamic_cast<ActorCritic*>(agent) != NULL) {
            printCSV(filePathExceptSuffix + "_MultMean.csv", multMeanList);
            printCSV(filePathExceptSuffix + "_MultStdev.csv", multStdevList);

            printCSV(filePathExceptSuffix + "_AddMean.csv", addMeanList);
            printCSV(filePathExceptSuffix + "_AddStdev.csv", addStdevList);
        }

        // print weights
        printCSV(filePathExceptSuffix + "_Weights", agent->getWeights());

        man.removeFlow(id);
        delete this;
    }
    // man.logEvent("ECNFlow", this->id, "Agent Step", "Agent called with state " + to_string(state) + " and reward "
    //                 + to_string(reward) + " and took action " + to_string(action)
    //                     + ", setting rate to " +to_string(rate));
}

void ECNFlow::txPacketEvent() {
    Endpoint *endpoint = man.getEndpoint(sourceId);

    // create packet
    ECNPacket *p = createPacket(ttl, headSize, bodySize, true); // TODO should this be true

    endpoint->txPacket(p);

    second_t nextTx = nextTxTime();
    if (nextTx < maxTime) {
        EventI *e = new Event<ECNFlow>(nextTx, &ECNFlow::txPacketEvent, this);
        man.pushEvent(e);
    }
    man.logEvent("ECNFlow", this->id, "Flow Packet Tx", "Sent packet " + to_string(p->getId()) + 
                    " from flow " + to_string(this->id));
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
    return double(headSize + bodySize);
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
        packetsSent++;
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
        acksArrived++;
        man.logEvent("ECNFlow", this->id, "Flow Packet Arrived", "Ack packet " + to_string(p->getId()) +
                        " arrived at destination.");

        ECNPacket::ackMetaData ackData = ecnP->getAckData();
        // TODO update stats
        if (!ackData.ECNBit) {
            packetsUntagged++;
        }
        if (packetsSent > 0) {
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
    packetsSent++;
    Flow::packetDropped(p);
}

void ECNFlow::packetError(Packet *p) {
    // TODO track acks separatly
    packetsSent++;
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
    //TODO: get state and reward
    double state = 0;
    double reward = 0;
    agent->step(state, reward,rate);
}

void TestFlow::txPacketEvent() {
    static const int hMin = 10, hMax = 50;
    static const int bMin = 40, bMax = 120;
    static const int ttl = 15;

    int hSize = hMin + rand() % (hMax - hMin);
    int bSize = bMin + rand() % (bMax - bMin);

    Endpoint *endpoint = man.getEndpoint(sourceId);
    // todo test for error

    Packet *p = createPacket(ttl, hSize, bSize, true);

    man.logTxEvent(FLOW_STR, id, TX_PACKET_EVENT, sourceId, p);

    endpoint->txPacket(p);

    // set up next
    second_t nextTx = nextTxTime();
    EventI *e = new Event<TestFlow>(nextTx, &TestFlow::txPacketEvent, this);
    man.pushEvent(e);
}
#endif /* _TEST */
