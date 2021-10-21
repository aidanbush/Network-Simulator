#include "packet.h"
#include "flow.h"
#include "manager.h"

Packet::Packet(int id, int sourceId, int destId, int flowId, int dataId, int ttl,
        int headerSize, int bodySize, bool sourcePacket): NetworkObject(id) {
    this->sourceId = sourceId;
    this->destId = destId;
    this->flowId = flowId;
    this->dataId = dataId;
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
    this->dataId = p.dataId;
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
    return new Packet(id, sourceId, destId, dataId, flowId, ttl, headerSize, bodySize, sourcePacket);
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


ECNPacket::ECNPacket(int id, int sourceId, int destId, int flowId, int dataId, int ttl,
        int headerSize, int bodySize, bool sourcePacket): Packet(id, sourceId, destId, flowId, dataId, ttl,
            headerSize, bodySize, sourcePacket) {
    ECNBit = false;
    ECNScale = 0;
}

ECNPacket::ECNPacket(const Packet &p): Packet(p) {
    ECNBit = false;
    ECNScale = 0;
}

void ECNPacket::setECNBit() {
    ECNBit = true;
}

bool ECNPacket::getECNBit() {
    return ECNBit;
}

void ECNPacket::setECNScale(double scale) {
    ECNScale = max(ECNScale, scale);
}

double ECNPacket::getECNScale() {
    return ECNScale;
}

void ECNPacket::setAckData(second_t sendTime, int sizeBytes, bool ECNBit, double bufferOccupancy, int ackedId) {
    Packet::setAckData(sendTime, sizeBytes, ackedId);
    this->ackData.ECNBit = ECNBit;
    this->ackData.bufferOccupancy = bufferOccupancy;
}

ECNPacket::ackMetaData ECNPacket::getAckData() {
    return ackData;
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
