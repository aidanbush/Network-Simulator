#include <stdlib.h>

#include "flow.h"
#include "manager.h"
#include "packet.h"
#include "endpoint.h"

using namespace std;

Flow::Flow(int id, Endpoint *endpoint, int destID) : NetworkObject(id) {
    this->endpoint = endpoint;
    this->sourceID = endpoint->getID();
    this->destID = destID;
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

#ifdef _TEST
second_t TestFlow::nextTxTime() {
    static const second_t min = 0.001, max = 0.01;

    return man.time + min + (float)rand() / (RAND_MAX / (max - min));
}

void TestFlow::StartFlow() {
    int nextTx = nextTxTime();
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
    // send p
    endpoint->txPacket(p);

    // set up next
    int nextTx = nextTxTime();
    EventI *e = new Event<TestFlow>(nextTx, &TestFlow::txPacketEvent, this);
    man.pushEvent(e);
}
#endif /* _TEST */
