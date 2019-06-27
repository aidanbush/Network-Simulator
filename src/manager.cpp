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

// interface
int Manager::addInterface(Interface* interface) {
    return interfaces.emplace(interface->getId(), interface).second;
}

Interface* Manager::getInterface(int id) {
    auto interface = interfaces.find(id);
    if (interface == interfaces.end()) {
        //TODO: perhaps this should be an error instead of returning null
        //We can use map.at() instead of map.find() to do this automatically
        return NULL;
    }
    
    return interface->second;
}

// link
int Manager::addLink(Link* linck) {
    return links.emplace(linck->getId(), linck).second;
}

Link* Manager::getLink(int id) {
    auto linck = links.find(id); //TODO: what should switches and links be called for variable names
    if (linck == links.end()) {
        //TODO: perhaps this should be an error instead of returning null
        //We can use map.at() instead of map.find() to do this automatically
        return NULL;
    }
    
    return linck->second;
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
