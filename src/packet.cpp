#include "packet.h"
#include "flow.h"

Packet::Packet(int id, int sourceID, int destID, Flow *flow, int ttl,
        int headerSize, int bodySize) {
    this->id = id;
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

#ifdef _TEST
#include <assert.h>

#include "endpoint.h"

#define P_ID    1
#define S_ID    1
#define S_SPEED 1
#define D_ID    1
#define F_ID    1
#define TTL     10
#define H_SIZE  15
#define B_SIZE  200
#define SIZE    ((H_SIZE) + (B_SIZE))

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
    Endpoint *e1 = new Endpoint(S_ID, S_SPEED);
    Flow *f1 = new TestFlow(F_ID, e1, D_ID);
    // test creation packet
    Packet *p1 = new Packet(P_ID, S_ID, D_ID, f1, TTL, H_SIZE, B_SIZE);

    // test values
    assert(p1->fullSize() == SIZE);
    assert(p1->getTTL() == TTL);
    assert(p1->getID() == P_ID);
    assert(p1->getSource() == S_ID);
    assert(p1->getDest() == D_ID);
    assert(p1->getFlow() == F_ID);

    // test clone
    Packet *p2 = p1->clone();

    // test equal to orignal
    assert(p1->fullEqual(p2));

    // test dec TTL
    p1->decTTL();
    assert(p1->getTTL() == TTL - 1);

    delete p1;
    delete p2;

    delete e1;
    delete f1;

    return 1;
}

#endif /* _TEST */
