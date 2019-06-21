#include <map>

#include "manager.h"
#include "packetHandler.h"
#include "switch.h"
#include "endpoint.h"

Manager::Manager() {
    time = 0;
}

// manager
int Manager::addHandler(PacketHandler *handler) {
    int id = handler->getID();

    if (handler == NULL || packetHandlers.find(id) != packetHandlers.end()) {
        return 0;
    }

    packetHandlers.insert({id, handler});
    return 1;
}

PacketHandler *Manager::getHandler(int id) {
    auto handler = packetHandlers.find(id);
    if (handler == packetHandlers.end()) {
        return NULL;
    }

    return handler->second;
}

// switch
int Manager::addSwitch(Switch *netSwitch) {
    int id = netSwitch->getID();

    if (netSwitch == NULL || switches.find(id) != switches.end()) {
        return 0;
    }

    switches.insert({id, netSwitch});
    return 1;
}

Switch *Manager::getSwitch(int id) {
    auto netSwitch = switches.find(id);
    if (netSwitch == switches.end()) {
        return NULL;
    }

    return netSwitch->second;
}

// endpoint
int Manager::addEndpoint(Endpoint *endpoint) {
    int id = endpoint->getID();

    if (endpoint == NULL || endpoints.find(id) != endpoints.end()) {
        return 0;
    }

    endpoints.insert({id, endpoint});
    return 1;
}

Endpoint *Manager::getEndpoint(int id) {
    auto endpoint = endpoints.find(id);
    if (endpoint == endpoints.end()) {
        return NULL;
    }

    return endpoint->second;
}

EventI *Manager::popEvent() {
    EventI *e = pq.top();
    pq.pop();

    if (time > e->time) {
        // TODO THIS IS BAD
    }

    time = e->time;
    return e;
}
