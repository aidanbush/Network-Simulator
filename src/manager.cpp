#include "manager.h"
#include "packetHandler.h"

#include <map>

Manager::Manager() {
    time = 0;
}

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

EventI *Manager::popEvent() {
    EventI *e = pq.top();
    pq.pop();

    if (time > e->time) {
        // TODO THIS IS BAD
    }

    time = e->time;
    return e;
}
