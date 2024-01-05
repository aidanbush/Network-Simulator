#include "packet.h"
#include "flow.h"
#include "manager.h"
#include "switch.h"

#include <cstdio>

Packet::Packet(int id, int sourceId, int destId, int flowId, int burstId, int dataId, int ttl,
        int headerSize, int bodySize, bool sourcePacket): NetworkObject(id) {
    this->sourceId = sourceId;
    this->destId = destId;
    this->flowId = flowId;
    this->burstId = burstId;
    this->dataId = dataId;
    this->initialTTL = ttl;
    this->ttl = ttl;
    this->headerSize = headerSize;
    this->bodySize = bodySize;
    this->createTime = man.time;
    this->arrivalTime = 0;
    this->sourcePacket = sourcePacket;

    this->ackedSendTime = NULL_TIME;
    this->ackedSizeBytes = NULL_PACKET_SIZE;
    this->ackedId = NULL_DATA_ID;
}

Packet::Packet(const Packet &p): NetworkObject(p.id) {
    this->sourceId = p.sourceId;
    this->destId = p.destId;
    this->flowId = p.flowId;
    this->burstId = p.burstId;
    this->dataId = p.dataId;
    this->initialTTL = p.initialTTL;
    this->ttl = p.ttl;
    this->headerSize = p.headerSize;
    this->bodySize = p.bodySize;
    this->createTime = p.createTime;
    this->arrivalTime = p.arrivalTime;
    this->sourcePacket = p.sourcePacket;

    this->ackedSendTime = p.ackedSendTime;
    this->ackedSizeBytes = p.ackedSizeBytes;
    this->ackedId = p.ackedId;
}

Packet *Packet::clone() {
    Packet *p = new Packet(id, sourceId, destId, flowId, burstId, dataId, ttl, headerSize, bodySize, sourcePacket);

    p->createTime = this->createTime;
    p->arrivalTime = this->arrivalTime;

    p->ackedSendTime = this->ackedSendTime;
    p->ackedSizeBytes = this->ackedSizeBytes;
    p->ackedId = this->ackedId;

    return p;
}

void Packet::arrive() {
    Flow *f = man.getFlow(flowId);
    arrivalTime = man.time;
    f->packetArrived(this);
}

void Packet::drop() {
    Flow *f = man.getFlow(flowId);
    f->packetDropped(this);
}

void Packet::error() {
    Flow *f = man.getFlow(flowId);
    f->packetError(this);
}

second_t Packet::getSendTime() {
    return createTime;
}

second_t Packet::getAckedSendTime() {
    return ackedSendTime;
}

void Packet::setAckData(second_t sendTime, int sizeBytes, int ackedId) {
    this->ackedSendTime = sendTime;
    this->ackedSizeBytes = sizeBytes;
    this->ackedId = ackedId;
}

bool Packet::validate() {
    // TODO implement
    return true;
}

/* ManhattanBanditDeflectionPacket */
MBDPacket::MBDPacket(int id, int sourceId, int destId, int flowId, int burstId, int dataId, int ttl, int headerSize,
        int bodySize, bool sourcePacket): Packet(id, sourceId, destId, flowId, burstId, dataId, ttl, headerSize,
            bodySize, sourcePacket) {
}

void MBDPacket::arrive() {
    // update switches
#ifndef ONE_HOP_REWARD
    updateSwitches(true);
#endif /* ONE_HOP_REWARD */

    Packet::arrive();
}

void MBDPacket::drop() {
    // update switches
    MBDFlow *f = dynamic_cast<MBDFlow *>(man.getFlow(flowId));
#ifndef ONE_HOP_REWARD
    updateSwitches(false);
#endif /* ONE_HOP_REWARD */

    Packet::drop();
}

void MBDPacket::recordAction(int switchId) {
    switches.push_back(switchId);
}

// TODO move into manager
double MBDPacket::shortestPath(int source, int dest) {
    // assume manhattan
    int size = 3;
    int n = size + 1;
    int sourceX = (source / n) % n; // x
    int sourceY = source % n; // y
    int destX = (dest / n) % n; // x
    int destY = dest % n; // y

    return double(abs(sourceX - destX) + abs(sourceY - destY));
}

void MBDPacket::updateSwitches(bool arrived) {
    /*double dropCost = (lostCost + resendCost);
    //TODO lost cost is too small - not cacluated properly
    if (dropCost != 0) {
        dropCost = dropCost / shortestPath(sourceId, destId);
    }*/

    // these updates are not correct if a switch is in deflection twice as those updates will be out of order
    /*if (dropCost != 0) {
        fprintf(stderr, "id %d drop %f lost %f resend %f shortest path %f\n", id, dropCost, lostCost, resendCost, shortestPath(sourceId, destId));
    }*/

    double hops = 0;
    for (int i = switches.size() - 1; i >= 0; i--) {
        ManhattanBanditDeflectionSwitch *s = dynamic_cast<ManhattanBanditDeflectionSwitch *>(man.getSwitch(switches[i]));
        hops += 1;

        // calc reward
        // total = ttlInitial - ttl
        double minHops = shortestPath(switches[i], destId);
        //double reward = (ttlInitial - ttl - minHops) / minHops + dropCost;
        //double reward = hops / minHops + dropCost;
        double reward = 0;
        if (arrived) {
            reward = minHops / hops;
        }

        /*if (dropCost != 0) {
            fprintf(stderr, "hops %f min hops %f hop cost %f reward %f\n", hops, minHops, hops / minHops, reward);
        }*/

        //s->rewardAction(id, -(reward));
        s->rewardAction(id, reward);
    }
}

#ifdef _TEST
#include "tests/throwAssert.h"

bool Packet::fullEqual(Packet *p) {
    return id == p->id &&
        headerSize == p->headerSize &&
        bodySize == p->bodySize &&
        ttl == p->ttl &&
        sourceId == p->sourceId &&
        destId == p->destId &&
        flowId == p->flowId;
}

int testPacket() {
    const int p1Id = 1,
          p1TTL = 15,
          p1HSize = 50,
          p1BSize = 200,
          p1FullSize = p1HSize + p1BSize,
          p1SId = 1,
          p1DId = 2,
          p1FId = 1;

    Packet *p1 = new Packet(p1Id, p1SId, p1DId, p1FId, p1TTL, p1HSize, p1BSize);

    // test values
    throwAssert(p1->fullSize() == p1FullSize);
    throwAssert(p1->getTTL() == p1TTL);
    throwAssert(p1->getId() == p1Id);
    throwAssert(p1->getSource() == p1SId);
    throwAssert(p1->getDest() == p1DId);

    // test clone
    Packet *p2 = p1->clone();

    // test equal to orignal
    throwAssert(p1->fullEqual(p2));

    // test dec TTL
    p1->decTTL();
    throwAssert(p1->getTTL() == p1TTL - 1);

    delete p1;
    delete p2;

    return 1;
}

#endif /* _TEST */
