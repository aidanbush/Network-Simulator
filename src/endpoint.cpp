#include <nlohmann/json.hpp>
#include <stdio.h>

#include "endpoint.h"
#include "interface.h"
#include "packet.h"
#include "packetHandler.h"

using namespace std;
using json = nlohmann::json;

Endpoint::Endpoint(int id, int speed): PacketHandler(id, speed) {
}

void Endpoint::rxPacket(Packet *p) {
    p->arrive();
}

int Endpoint::txPacket(Packet *p) {
    auto ifaceIt = interfaces.begin();
    if (ifaceIt == interfaces.end()) {
        fprintf(stderr, "interface not found to transmit packet on\n");
        return 0;
    }

    Interface *iface = man.getInterface(ifaceIt->second);

    iface->rxHandler(p);
    return 1;
}

bool Endpoint::validate() {
    bool valid = true;

    if (!validateHandler()) {
        valid = false;
    }

    return valid;
}

#ifdef _TEST
#include <assert.h>
#include <stdio.h>

#include "link.h"

int Endpoint::endpointToEndpoint() {
    const int e1ID = 1,
          e1Speed = 1000,
          e2ID = 2,
          e2Speed = 1000;
    const int i1ID = 1,
          i1LBuf = 1000,
          i1HBuf = 1000;
    const int i2ID = 2,
          i2LBuf = 1000,
          i2HBuf = 1000;
    const int l1ID = 1,
          l1Speed = 1600000;
    const second_t l1TxTime = 0.01;
    const int p1ID = 1,
          p1SID = 1,
          p1DID = 2,
          p1TTL = 10,
          p1HSize = 50,
          p1BSize = 100;
    const int f1ID = 1;

    Endpoint *e1, *e2;
    Interface *i1, *i2;
    Link *l1;
    Flow *f1;
    Packet *p1;
    EventI *e;

    // setup network
    e1 = new Endpoint(e1ID, e1Speed);
    e2 = new Endpoint(e2ID, e2Speed);

    assert(man.addEndpoint(e1));
    assert(man.addEndpoint(e2));

    i1 = new Interface(i1ID, e1ID, i1LBuf, i1HBuf);
    i2 = new Interface(i2ID, e2ID, i2LBuf, i2HBuf);

    assert(man.addInterface(i1));
    assert(man.addInterface(i2));

    l1 = new Link(l1ID, l1Speed, l1TxTime);

    l1->addDest(i1ID);
    l1->addDest(i2ID);

    i1->setLink(l1ID);
    i2->setLink(l1ID);

    assert(man.addLink(l1));

    f1 = new TestFlow(f1ID, e1, e2ID);

    p1 = new Packet(p1ID, p1SID, p1DID, f1, p1TTL, p1HSize, p1BSize);

    e1->addInterface(e2ID, i1ID);
    e2->addInterface(e1ID, i2ID);

    // add to endpoint 1
    e1->txPacket(p1);

    // move packet through network
    // i1 to l1, l1 to i2, i2 to e2
    for (int i = 0; i < 3; i++) {
        e = man.popEvent();
        e->call();
        delete e;
    }

    // no more events

    delete f1;

    man.deleteNetwork();

    return 1;
}

int testEndpoint() {
    const int e1ID = 1,
          e1Speed = 120;
    Endpoint *e1 = new Endpoint(e1ID, e1Speed);

    assert(e1->getID() == e1ID);
    assert(e1->getInternalSpeed() == e1Speed);

    delete e1;

    return Endpoint::endpointToEndpoint();
}

#endif /* _TEST */
