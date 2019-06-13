#include "endpoint.h"

using namespace std;

void Endpoint::rxPacket(Packet *p) {
    // TODO: implement
}

void Endpoint::txPacket(Packet *p) {
    // TODO: implement select interface and send on it
    ifaces[0]->rxHandler(p);
}
