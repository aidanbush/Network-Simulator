#include <map>

#include "manager.h"
#include "packetHandler.h"
#include "switch.h"
#include "endpoint.h"
#include "interface.h"
#include "link.h"

Manager::Manager() {
    time = 0;
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

// interface
int Manager::addInterface(Interface *interface) {
    int id = interface->getID();

    if (interface == NULL || interfaces.find(id) != interfaces.end()) {
        return 0;
    }

    interfaces.insert({id, interface});
    return 1;
}

Interface *Manager::getInterface(int id) {
    auto interface = interfaces.find(id);
    if (interface == interfaces.end()) {
        return NULL;
    }

    return interface->second;
}

// link
int Manager::addLink(Link *link) {
    int id = link->getID();

    if (link == NULL || links.find(id) != links.end()) {
        return 0;
    }

    links.insert({id, link});
    return 1;
}

Link *Manager::getLink(int id) {
    auto link = links.find(id);
    if (link == links.end()) {
        return NULL;
    }

    return link->second;
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
