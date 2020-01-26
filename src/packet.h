#ifndef PACKET_H
#define PACKET_H

#define BITS_PER_BYTE   8

#include "networkObject.h"
#include "flow.h"

class Flow;

class Packet: public NetworkObject {
    public:
        Packet(int id, int sourceId, int destId, int flowId, int ttl,
                int headerSize, int bodySize);
        ~Packet() = default;

        Packet *clone();

        int getTTL() {return ttl; }
        int getFlow() {return flowId; }
        int getSource() {return sourceId; }
        int getDest() {return destId; }

        int fullSize() {return headerSize + bodySize; }
        int fullSizeBits() {return (headerSize + bodySize) * BITS_PER_BYTE; }
        void decTTL() {ttl--; }

        void arrive();
        void drop();
        void error();

        bool validate();

#ifdef _TEST
        bool fullEqual(Packet *p);
#endif /* _TEST */

    private:
        int headerSize;
        int bodySize;
        int ttl;
        int sourceId;
        int destId;
        int flowId;
};

class ECNPacket: public Packet {
    public:
        ECNPacket(int id, int sourceId, int destId, int flowId, int ttl,
                int headerSize, int bodySize); // TODO add ack data
        ~ECNPacket() = default;

        void setECN();
        bool ECNSet();

    private:
        bool ECNBit;
};

#ifdef _TEST
int testPacket();
#endif /* _TEST */

#endif // PACKET_H
