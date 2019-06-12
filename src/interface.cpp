#include "interface.h"
#include "packetHandler.h"
#include "manager.h"

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

    link->txPacket(p, id);

    if (!linkBuffer.empty()) {
        second_t nextTx = man.time + double(linkBuffer.front()->fullSizeBits()) / linkSpeed;
        EventI *e = new Event<Interface>(nextTx, &Interface::txLinkEvent, this);
        man.pq.push(e);
    }
}

void Interface::txHandlerEvent() {
    Packet *p = handlerBuffer.front();
    handlerBuffer.pop();

    handler->handlePacket(p);

    if (!handlerBuffer.empty()) {
        second_t nextTx = man.time + double(p->fullSizeBits()) / handlerSpeed;
        EventI *e = new Event<Interface>(nextTx, &Interface::txHandlerEvent, this);
        man.pq.push(e);
    }
}

void Interface::rxLink(Packet *p) {
    handlerBuffer.push(p);

    if (handlerBuffer.size() == 1) {
        second_t nextTx = man.time + double(p->fullSizeBits()) / handlerSpeed;
        EventI *e = new Event<Interface>(nextTx, &Interface::txHandlerEvent, this);
        man.pq.push(e);
    }
}

void Interface::rxHandler(Packet *p) {
    // add packet to link buffer
    linkBuffer.push(p);

    // if only one element add event
    if (linkBuffer.size() == 1) {
        second_t nextTx = man.time + double(p->fullSizeBits()) / linkSpeed;
        EventI *e = new Event<Interface>(nextTx, &Interface::txLinkEvent, this);
        man.pq.push(e);
    }
}

#ifdef _TEST
#include <assert.h>

#include "switch.h"

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

    return 1;
}

#endif /* _TEST */
