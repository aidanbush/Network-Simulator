#include <nlohmann/json.hpp>
#include <stdio.h>

#include "endpoint.h"
#include "interface.h"
#include "packet.h"
#include "packetHandler.h"
#include "config.h"

// All following defines can be overridden in endpointDefines.h, which can be modified for local testing
#if __has_include("endpointDefines.h")
#include "endpointDefines.h"
#else
#endif // __has_include

using namespace std;
using json = nlohmann::json;

Endpoint::Endpoint(json &endpointConfig): PacketHandler(validateEndpointConfig(endpointConfig)) {
}

json &Endpoint::validateEndpointConfig(json &endpointConfig) {
    string message = "";
    if (!hasMemberOfType(endpointConfig, "id", jsonInt)) {
        message += "No integer with name 'id'.\n";
    }

    if (!hasMemberOfType(endpointConfig, "internal_speed", jsonInt)) {
        message += "No integer with name 'internal_speed'.\n";
    }

    if (!message.empty()) {
        message = "Endpoint:\n" + message + endpointConfig.dump(4);
        throw runtime_error(message);
    }
    return endpointConfig;
}

void Endpoint::rxPacket(Packet *p, int sourceInterfaceId) {
    p->arrive();
}

void Endpoint::txPacket(Packet *p) {
    auto interfaceIt = interfaces.begin();
    if (interfaceIt == interfaces.end()) {
        throw runtime_error("Endpoint: txPacket: no interface on endpoint to send packet");
    }

    Interface *interface = man.getInterface(interfaceIt->second);

    interface->rxHandler(p);
}

bool Endpoint::validate() {
    bool valid = true;

    if (!validateHandler()) {
        valid = false;
    }

    return valid;
}

#ifdef _TEST
#include <stdio.h>

#include "link.h"
#include "tests/throwAssert.h"

int Endpoint::endpointToEndpoint() {
    const int e1Id = 1,
          e1Speed = 1000,
          e2Id = 2,
          e2Speed = 1000;
    const int i1Id = 1,
          i1LBuf = 1000,
          i1HBuf = 1000;
    const int i2Id = 2,
          i2LBuf = 1000,
          i2HBuf = 1000;
    const int l1Id = 1,
          l1Speed = 1600000;
    const second_t l1TxTime = 0.01;
    const int p1Id = 1,
          p1SId = 1,
          p1DId = 2,
          p1TTL = 10,
          p1HSize = 50,
          p1BSize = 100;
    const int f1Id = 1,
          f1SId = e1Id,
          f1DId = e2Id;

    json f1C = {
        {"type", "test"},
        {"id", f1Id},
        {"source_id", f1SId},
        {"dest", f1DId},
    };

    Endpoint *e1, *e2;
    Interface *i1, *i2;
    Link *l1;
    Flow *f1;
    Packet *p1;
    EventI *e;

    // setup network
    json endpointJson1 = {
        {"id", e1Id},
        {"internal_speed", e1Speed}
    };
    json endpointJson2 = {
        {"id", e2Id},
        {"internal_speed", e2Speed}
    };
    e1 = new Endpoint(endpointJson1);
    e2 = new Endpoint(endpointJson2);

    throwAssert(man.addEndpoint(e1));
    throwAssert(man.addEndpoint(e2));

    json interfaceJson1 = {
        {"id", i1Id},
        {"handler_id", e1Id},
        {"out_buf_size", i1LBuf},
        {"in_buf_size", i1HBuf}
    };
    json interfaceJson2 = {
        {"id", i2Id},
        {"handler_id", e2Id},
        {"out_buf_size", i2LBuf},
        {"in_buf_size", i2HBuf}
    };
    i1 = new Interface(interfaceJson1);
    i2 = new Interface(interfaceJson2);

    throwAssert(man.addInterface(i1));
    throwAssert(man.addInterface(i2));

    json linkJson = {
        {"id", l1Id},
        {"speed", l1Speed},
        {"time", l1TxTime},
        {"interfaces", {i1Id, i2Id}}
    };
    l1 = new Link(linkJson);
    l1->addToInterfaces();

    throwAssert(man.addLink(l1));

    f1 = createFlow(f1C);

    throwAssert(man.addFlow(f1));

    p1 = new Packet(p1Id, p1SId, p1DId, f1Id, p1TTL, p1HSize, p1BSize);

    e1->addInterface(e2Id, i1Id);
    e2->addInterface(e1Id, i2Id);

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

    man.deleteNetwork();

    return 1;
}

int testEndpoint() {
    const int e1Id = 1,
          e1Speed = 120;
    json endpointJson = {
        {"id", e1Id},
        {"internal_speed", e1Speed}
    };
    Endpoint *e1 = new Endpoint(endpointJson);

    throwAssert(e1->getId() == e1Id);
    throwAssert(e1->getInternalSpeed() == e1Speed);

    delete e1;

    return Endpoint::endpointToEndpoint();
}

#endif /* _TEST */
