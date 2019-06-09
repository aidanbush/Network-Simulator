#include "packet.h"

Packet::Packet(int id, int sourceID, int destID, int ttl, int headerSize,
        int bodySize) {
    this->id = id;
    this->sourceID = sourceID;
    this->destID = destID;
    this->ttl = ttl;
    this->headerSize = headerSize;
    this->bodySize = bodySize;
}

Packet *Packet::clone() {
    return new Packet(id, sourceID, destID, ttl, headerSize, bodySize);
}

int Packet::getSize() {
    return headerSize + bodySize;
}
