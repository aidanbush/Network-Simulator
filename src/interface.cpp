#include "interface.h"
#include "packetHandler.h"
#include "manager.h"

using namespace std;

void Interface::txLinkEvent() {
    Packet *p = linkBuffer.front();
    linkBuffer.pop();

    link->txPacket(p, id);

    if (!linkBuffer.empty()) {
        second_t nextTx = man.time + double(linkBuffer.front()->getSize()) / linkSpeed;
        EventI *e = new Event<Interface>(nextTx, &Interface::txLinkEvent, this);
        man.pq.push(e);
    }
}

void Interface::txHandlerEvent() {
    Packet *p = handlerBuffer.front();
    handlerBuffer.pop();

    handler->handlePacket(p);

    if (!handlerBuffer.empty()) {
        second_t nextTx = man.time + double(p->getSize()) / handlerSpeed;
        EventI *e = new Event<Interface>(nextTx, &Interface::txHandlerEvent, this);
        man.pq.push(e);
    }
}

void Interface::rxLink(Packet *p) {
    handlerBuffer.push(p);

    if (handlerBuffer.size() == 1) {
        second_t nextTx = man.time + double(p->getSize()) / handlerSpeed;
        EventI *e = new Event<Interface>(nextTx, &Interface::txHandlerEvent, this);
        man.pq.push(e);
    }
}

void Interface::rxHandler(Packet *p) {
    // add packet to link buffer
    linkBuffer.push(p);

    // if only one element add event
    if (linkBuffer.size() == 1) {
        second_t nextTx = man.time + double(p->getSize()) / linkSpeed;
        EventI *e = new Event<Interface>(nextTx, &Interface::txLinkEvent, this);
        man.pq.push(e);
    }
}
