#ifndef PACKET_H
#define PACKET_H

#include "networkObject.h"

class Packet: public NetworkObject {
    public:
        Packet(int id, int sourceID, int destID, int ttl, int headerSize,
                int bodySize);

        Packet *clone();
        int getSize();

    private:
        int headerSize;
        int bodySize;
        int ttl;
        int sourceID;
        int destID;
        int flowID;
};

#endif // PACKET_H
