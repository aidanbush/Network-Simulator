#include "packet.h"
#include "flow.h"

Packet::Packet(int id, int sourceID, int destID, Flow *flow, int ttl,
        int headerSize, int bodySize): NetworkObject(id) {
    this->sourceID = sourceID;
    this->destID = destID;
    this->flow = flow;
    this->ttl = ttl;
    this->headerSize = headerSize;
    this->bodySize = bodySize;
}

Packet *Packet::clone() {
    return new Packet(id, sourceID, destID, flow, ttl, headerSize, bodySize);
}

void Packet::arrive() {
    flow->packetArrived(this);
}

void Packet::drop() {
    flow->packetDropped(this);
}

void Packet::error() {
    flow->packetError(this);
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
        sourceID == p->sourceID &&
        destID == p->destID &&
        flow == p->flow;
}

int testPacket() {
    const int p1ID = 1,
          p1TTL = 15,
          p1HSize = 50,
          p1BSize = 200,
          p1FullSize = p1HSize + p1BSize,
          p1SID = 1,
          p1DID = 2;
    Flow *p1F;

    Packet *p1 = new Packet(p1ID, p1SID, p1DID, p1F, p1TTL, p1HSize, p1BSize);

    // test values
    assert(p1->fullSize() == p1FullSize);
    assert(p1->getTTL() == p1TTL);
    assert(p1->getID() == p1ID);
    assert(p1->getSource() == p1SID);
    assert(p1->getDest() == p1DID);

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
