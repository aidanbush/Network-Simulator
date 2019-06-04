#include "packet.h"

Packet::Packet(int id, int sourceID, int destID, int ttl) {
    this->id = id;
    this->sourceID = sourceID;
    this->destID = destID;
    this->ttl = ttl;
}

int Packet::size() {
    return headerSize + bodySize;
}
