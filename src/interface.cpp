#include "interface.h"

using namespace std;

void Interface::txLinkEvent() {
    // enqueue onto link which will create an event
    // pop packet
    // link.txpacket
    // if queue not empty create new txLinkEvent := cur time + size/speed
}

void Interface::txHandlerEvent() {
    // if switch check which interface to move onto and enqueue onto thats linkBuffer
    // if endpoint consume packet
}

void Interface::rxLink(Packet *p) {
    // add packet to handler buffer
    // if only one element add event
}

void Interface::rxHandler(Packet *p) {
    // add packet to link buffer
    // if only one element add event
}
