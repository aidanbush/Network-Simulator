#include "interface.h"
#include "packetHandler.h"
#include "manager.h"
#include "link.h"
#include "packet.h"

using namespace std;

Interface::Interface(int id, Link *link, int linkBufSize, int handlerBufSize) {
    this->id = id;
    this->link = link;
    this->linkBufSize = linkBufSize;
    this->handlerBufSize = handlerBufSize;
    this->handler = NULL;
}

int Interface::addHandler(PacketHandler *handler) {
    if (this->handler != NULL) {
        return 0;
    }

    this->handler = handler;

    return 1;
}

void Interface::txLinkEvent() {
    Packet *p = linkBuffer.front();
    linkBuffer.pop();

    linkBufSize += p->fullSize();
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
        second_t nextTx = man.time + double(p->fullSizeBits()) / link->getSpeed();
        EventI *e = new Event<Interface>(nextTx, &Interface::txLinkEvent, this);
        man.pushEvent(e);
    }
}

#ifdef _TEST
#include <assert.h>

#include "switch.h"

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
    const int h1Speed = 1;

    Link *l1;
    Interface *i1, *i2;
    Packet *p1, *p2;
    EventI *e;
    TestHandler *h1;
    Flow *f1;

    // setup network
    l1 = new Link(l1ID, l1Speed, l1TxTime);

    i1 = new Interface(i1ID, l1, i1LBuf, i1HBuf);
    i2 = new Interface(i2ID, l1, i2LBuf, i2HBuf);

    l1->addDest(i1);
    l1->addDest(i2);

    h1 = new TestHandler(h1Speed);

    i2->addHandler(h1);

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

    // clean up last events
    delete man.popEvent();

    delete l1;
    delete i1;
    delete i2;
    delete p1;
    delete p2;
    delete h1;

    return 1;
}

#define L_ID        1
#define L_SPEED     80000
#define L_TX_TIME   ((second_t)0.001)

#define I_ID        1
#define I_LB_SIZE   48000
#define I_HB_SIZE   32000

#define S_ID        1

int testInterface() {
    Link *l1 = new Link(L_ID, L_SPEED, L_TX_TIME);

    Interface *i1 = new Interface(I_ID, l1, I_LB_SIZE, I_HB_SIZE);

    Switch *s1 = new Switch(S_ID);

    assert(i1->addHandler(s1));
    assert(!i1->addHandler(s1));

    delete s1;
    delete i1;
    delete l1;

    return Interface::ifaceToIface();
}

#endif /* _TEST */
