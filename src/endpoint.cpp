#include "endpoint.h"

using namespace std;

void Endpoint::handlePacket(Packet *p) {
    // TODO: implement
}

void Endpoint::txPacket(Packet *p) {
    // TODO: implement select interface and send on it
    interfaces[0]->rxHandler(p);
}
