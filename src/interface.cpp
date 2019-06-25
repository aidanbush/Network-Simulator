#include <nlohmann/json.hpp>

#include "interface.h"
#include "packetHandler.h"
#include "manager.h"
#include "link.h"
#include "packet.h"

using namespace std;

using json = nlohmann::json;

Interface::Interface(int id, int handlerID, int linkBufSize, int handlerBufSize):
        NetworkObject(id) {
    this->linkBufSize = linkBufSize;
    this->handlerBufSize = handlerBufSize;
    this->handlerID = handlerID;
}

void Interface::setLink(int linkID) {
    this->linkID = linkID;
}

int Interface::getLinkSpeed() {
    Link *link = man.getLink(linkID);
    return link->getSpeed();
}

second_t Interface::getLinkTxTime() {
    Link *link = man.getLink(linkID);
    return link->getTxTime();
}

void Interface::txLinkEvent() {
    Packet *p = linkBuffer.front();
    linkBuffer.pop();

    linkBufSize += p->fullSize();

    Link *link = man.getLink(linkID);

    link->txPacket(p, id);

    if (!linkBuffer.empty()) {
        second_t nextTx = man.time + double(linkBuffer.front()->fullSizeBits()) / link->getSpeed();
        EventI *e = new Event<Interface>(nextTx, &Interface::txLinkEvent, this);
        man.pushEvent(e);
    }
}

void Interface::txHandlerEvent() {
    Packet *p = handlerBuffer.front();
    handlerBuffer.pop();

    handlerBufSize += p->fullSize();

    PacketHandler *handler = man.getHandler(handlerID);

    handler->rxPacket(p);

    if (!handlerBuffer.empty()) {
        second_t nextTx = man.time + double(p->fullSizeBits()) / handler->getInternalSpeed();
        EventI *e = new Event<Interface>(nextTx, &Interface::txHandlerEvent, this);
        man.pushEvent(e);
    }
}

void Interface::rxLink(Packet *p) {
    if (handlerBufSize - p->fullSize() >= 0) {
        // TODO: drop packet
    }

    handlerBufSize -= p->fullSize();
    handlerBuffer.push(p);

    if (handlerBuffer.size() == 1) {
        PacketHandler *handler = man.getHandler(handlerID);

        second_t nextTx = man.time + double(p->fullSizeBits()) / handler->getInternalSpeed();
        EventI *e = new Event<Interface>(nextTx, &Interface::txHandlerEvent, this);
        man.pushEvent(e);
    }
}

void Interface::rxHandler(Packet *p) {
    if (linkBufSize - p->fullSize() >= 0) {
        // TODO: drop packet
    }

    linkBufSize -= p->fullSize();
    linkBuffer.push(p);

    // if only one element add event
    if (linkBuffer.size() == 1) {
        Link *link = man.getLink(linkID);

        second_t nextTx = man.time + double(p->fullSizeBits()) / link->getSpeed();
        EventI *e = new Event<Interface>(nextTx, &Interface::txLinkEvent, this);
        man.pushEvent(e);
    }
}

#ifdef _TEST
#include <assert.h>

#include "endpoint.h"

int Interface::ifaceToIface() {
    const int l1ID = 1,
          l1Speed = 1600000;
    const second_t l1TxTime = 0.01;
    const int i1ID = 1,
          i1LBuf = 1000,
          i1HBuf = 1000;
    const int i2ID = 2,
          i2LBuf = 1000,
          i2HBuf = 1000;
    const int p1ID = 1,
          p1SID = 1,
          p1DID = 2,
          p1TTL = 10,
          p1HSize = 50,
          p1BSize = 100;
    const int p2ID = 2,
          p2SID = 1,
          p2DID = 2,
          p2TTL = 10,
          p2HSize = 40,
          p2BSize = 120;
    const int e1ID = 1,
          e1Speed = 1;
    const int e2ID = 2,
          e2Speed = 1;

    Link *l1;
    Interface *i1, *i2;
    Packet *p1, *p2;
    Flow *f1;
    Endpoint *e1, *e2;
    EventI *e;

    // setup network
    e1 = new Endpoint(e1ID, e1Speed);
    e2 = new Endpoint(e2ID, e2Speed);

    man.addEndpoint(e1);
    man.addEndpoint(e2);

    i1 = new Interface(i1ID, e1ID, i1LBuf, i1HBuf);
    i2 = new Interface(i2ID, e2ID, i2LBuf, i2HBuf);

    man.addInterface(i1);
    man.addInterface(i2);

    l1 = new Link(l1ID, l1Speed, l1TxTime);

    l1->addDest(i1ID);
    l1->addDest(i2ID);

    i1->setLink(l1ID);
    i2->setLink(l1ID);

    man.addLink(l1);

    f1 = NULL;

    p1 = new Packet(p1ID, p1SID, p1DID, f1, p1TTL, p1HSize, p1BSize);
    p2 = new Packet(p2ID, p2SID, p2DID, f1, p2TTL, p2HSize, p2BSize);

    // add p1
    i1->rxHandler(p1);

    // check packet is in buffer
    assert(i1->linkBufSize == i1LBuf - p1->fullSize());
    assert(i1->linkBuffer.size() == 1);
    assert(i1->linkBuffer.front() == p1);

    // check event exists and is correct
    assert(man.numEvents() == 1);

    // add p2
    i1->rxHandler(p2);

    // check packet was added into buffer
    assert(i1->linkBufSize == i1LBuf - (p1->fullSize() + p2->fullSize()));
    assert(i1->linkBuffer.size() == 2);
    assert(i1->linkBuffer.back() == p2);

    // no new events
    assert(man.numEvents() == 1);

    // pop and evaluate event
    e = man.popEvent();
    e->call();
    delete e;

    // two events one transit other queued
    assert(man.numEvents() == 2);

    // check that buffer only holds p2
    assert(i1->linkBufSize == i1LBuf - p2->fullSize());
    assert(i1->linkBuffer.size() == 1);
    assert(i1->linkBuffer.front() == p2);

    // move p2 onto link
    e = man.popEvent();
    e->call();
    delete e;

    assert(man.numEvents() == 1);

    // move p1 from l1 to i2
    e = man.popEvent();
    e->call();
    delete e;

    // check packet arrived
    assert(i2->handlerBufSize == i2HBuf - p1->fullSize());
    assert(i2->handlerBuffer.size() == 1);
    assert(i2->handlerBuffer.front() == p1);

    // move p2 from l1 to i2
    e = man.popEvent();
    e->call();
    delete e;

    // check packet arrived
    assert(i2->handlerBufSize == i2HBuf - (p1->fullSize() + p2->fullSize()));
    assert(i2->handlerBuffer.size() == 2);
    assert(i2->handlerBuffer.back() == p2);

    assert(man.numEvents() == 1);

    // clean up last event
    delete man.popEvent();

    assert(man.numEvents() == 0);

    delete p1;
    delete p2;

    man.deleteNetwork();

    return 1;
}

int testInterface() {
    const int l1ID = 1;
    const int s1ID = 1;
    const int i1ID = 1,
          i1LBuf = 48000,
          i1HBuf = 32000;

    Interface *i1 = new Interface(i1ID, s1ID, i1LBuf, i1HBuf);
    i1->setLink(l1ID);

    delete i1;

    return Interface::ifaceToIface();
}

#endif /* _TEST */
