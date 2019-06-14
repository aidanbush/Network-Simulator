#ifndef PACKET_H
#define PACKET_H

#define BITS_PER_BYTE   8

#include "networkObject.h"
#include "flow.h"

class Flow;

class Packet: public NetworkObject {
    public:
        Packet(int id, int sourceID, int destID, int flowID, int ttl,
                int headerSize, int bodySize);

        Packet *clone();

        int getTTL() {return ttl; }
        int getFlow() {return flowID; }
        int getSource() {return sourceID; }
        int getDest() {return destID; }

        int fullSize() {return headerSize + bodySize; }
        int fullSizeBits() {return (headerSize + bodySize) * BITS_PER_BYTE; }
        void decTTL() {ttl--; }

        void arrive();
        void drop();
        void error();

#ifdef _TEST
        bool fullEqual(Packet *p);
#endif /* _TEST */

    private:
        int headerSize;
        int bodySize;
        int ttl;
        int sourceID;
        int destID;
        int flowID;

        Flow *flow;
};

#ifdef _TEST
int testPacket();
#endif /* _TEST */

#endif // PACKET_H
