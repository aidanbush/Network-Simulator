#include "endpoint.h"
#include "interface.h"
#include "packet.h"

using namespace std;

Endpoint::Endpoint(int id) {
    this->id = id;
}

void Endpoint::rxPacket(Packet *p) {
    p->arrive();
}

void Endpoint::txPacket(Packet *p) {
    // TODO: implement select interface and send on it
    ifaces[0]->rxHandler(p);
}
