#include "packet.h"
#include "flow.h"

Packet::Packet(int id, int sourceId, int destId, int flowId, int ttl,
        int headerSize, int bodySize): NetworkObject(id) {
    this->sourceId = sourceId;
    this->destId = destId;
    this->flowId = flowId;
    this->ttl = ttl;
    this->headerSize = headerSize;
    this->bodySize = bodySize;
}

Packet *Packet::clone() {
    return new Packet(id, sourceId, destId, flowId, ttl, headerSize, bodySize);
}

void Packet::arrive() {
    Flow *f = man.getFlow(flowId);
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

bool Packet::validate() {
    // TODO implement
    return true;
}

#ifdef _TEST
#include <assert.h>

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
    assert(p1->fullSize() == p1FullSize);
    assert(p1->getTTL() == p1TTL);
    assert(p1->getId() == p1Id);
    assert(p1->getSource() == p1SId);
    assert(p1->getDest() == p1DId);

    // test clone
    Packet *p2 = p1->clone();

    // test equal to orignal
    assert(p1->fullEqual(p2));

    // test dec TTL
    p1->decTTL();
    assert(p1->getTTL() == p1TTL - 1);

    delete p1;
    delete p2;

    return 1;
}

#endif /* _TEST */
