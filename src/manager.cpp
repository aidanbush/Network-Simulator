#include <map>

#include "manager.h"
#include "packetHandler.h"
#include "switch.h"
#include "endpoint.h"
#include "interface.h"
#include "link.h"
#include "packet.h"

Manager::Manager() {
    time = 0;
    logFile = stdout;
}

bool Manager::addHandler(PacketHandler *handler) {
    int id = handler->getID();

    if (handler == NULL) {
        return false;
    }

    return packetHandlers.emplace(id, handler).second;
}

PacketHandler *Manager::getHandler(int id) {
    auto handler = packetHandlers.find(id);
    if (handler == packetHandlers.end()) {
        return NULL;
    }

    return handler->second;
}

// switch
bool Manager::addSwitch(Switch *netSwitch) {
    return addHandler(netSwitch);
}

Switch *Manager::getSwitch(int id) {
    auto handlerIt = packetHandlers.find(id);
    if (handlerIt == packetHandlers.end()) {
        return NULL;
    }

    return dynamic_cast<Switch *>(handlerIt->second);
}

// endpoint
bool Manager::addEndpoint(Endpoint *endpoint) {
    return addHandler(endpoint);
}

Endpoint *Manager::getEndpoint(int id) {
    auto handlerIt = packetHandlers.find(id);
    if (handlerIt == packetHandlers.end()) {
        return NULL;
    }

    return dynamic_cast<Endpoint *>(handlerIt->second);
}

// interface
bool Manager::addInterface(Interface *interface) {
    int id = interface->getID();

    if (interface == NULL) {
        return false;
    }

    return interfaces.emplace(id, interface).second;
}

Interface *Manager::getInterface(int id) {
    auto interface = interfaces.find(id);
    if (interface == interfaces.end()) {
        return NULL;
    }

    return interface->second;
}

// link
bool Manager::addLink(Link *link) {
    int id = link->getID();

    if (link == NULL) {
        return false;
    }

    return links.emplace(id, link).second;
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

void Manager::deleteHandlers() {
    for (auto [id, handler] : packetHandlers) {
        delete handler;
    }

    packetHandlers.clear();
}

void Manager::deleteInterfaces() {
    for (auto [id, iface] : interfaces) {
        delete iface;
    }

    interfaces.clear();
}

void Manager::deleteLinks() {
    for (auto [id, link] : links) {
        delete link;
    }

    links.clear();
}

void Manager::deleteEvents() {
    EventI *e;

    while (!pq.empty()) {
        e = pq.top();
        pq.pop();
        delete e;
    }
}

void Manager::deleteNetwork() {
    deleteHandlers();
    deleteInterfaces();
    deleteLinks();

    deleteEvents();

    if (logFile != stdout) {
        if (fclose(logFile)) {
            perror("fclose");
        }
    }
}

bool Manager::linkHandlers() {
    for (auto const& it : packetHandlers) {
        if (!it.second->connectNeighbours()) {
        return false;
        }
    }

    return true;
}

bool Manager::validateNetwork() {
    bool valid = true;

    // validate each handler
    for (auto& it : packetHandlers) {
        if (!it.second->validate()) {
            valid = false;
        }
    }

    // validate each interface
    for (auto& it : packetHandlers) {
        if (!it.second->validate()) {
            valid = false;
        }
    }

    // validate each link
    for (auto& it : packetHandlers) {
        if (!it.second->validate()) {
            valid = false;
        }
    }

    return valid;
}

bool Manager::setLogFile(string filename) {
    FILE *newLog = fopen(filename.c_str(), (char *)"w");

    if (newLog == NULL) {
        perror("fopen");
        return false;
    }

    logFile = newLog;
    return true;
}

void Manager::logTxEvent(string objName, int objID, string eventName, int destID, Packet *p) {
    string message = "dest: " + to_string(destID) + " packet: " + to_string(p->getID()) + " flow: "
        + to_string(p->getFlow());
    logEvent(objName, objID, eventName, message);
}

void Manager::logEvent(string objName, int objID, string eventName, string message) {
    fprintf(logFile, "time: %f %s: %d event: %s message: %s\n", time,
            objName.c_str(), objID, eventName.c_str(), message.c_str());
}

void Manager::setConfigInvalid() {
    configValid = false;
}

bool Manager::isConfigValid() {
    return configValid;
}