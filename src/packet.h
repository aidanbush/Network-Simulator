#ifndef PACKET_H
#define PACKET_H

#define BITS_PER_BYTE       8
#define NULL_DATA_ID        -1
#define NULL_PACKET_SIZE    -1

#include "networkObject.h"
#include "flow.h"

class Flow;

class Packet: public NetworkObject {
    public:
        Packet(int id, int sourceId, int destId, int flowId, int dataId, int ttl,
                int headerSize, int bodySize, bool sourcePacket);
        Packet(const Packet &p);
        ~Packet() = default;
        Packet *clone();

        int getTTL() {return ttl; }
        int getFlow() {return flowId; }
        int getSource() {return sourceId; }
        int getDest() {return destId; }
        int getDataId() {return dataId; }
        int getAckedId() {return ackedId; }
        int getHeaderSize() {return headerSize; }
        int getBodySize() {return bodySize; }
        bool isSourcePacket() {return sourcePacket; }

        int fullSize() {return headerSize + bodySize; }
        int fullSizeBits() {return (headerSize + bodySize) * BITS_PER_BYTE; }
        int hopCount() {return initialTTL - ttl;}
        void decTTL() {ttl--; }
        bool outOfTime() { return ttl <= 0; }

        virtual void arrive();
        virtual void drop();
        void error();

        second_t getSendTime(); // creation not send time

        second_t getAckedSendTime();
        int getAckedFullSizeBits() {return ackedSizeBytes * BITS_PER_BYTE; }
        int getAckedFullSizeBytes() {return ackedSizeBytes; }

        void setAckData(second_t sendTime, int sizeBytes, int ackedId);

        bool validate();

#ifdef _TEST
        bool fullEqual(Packet *p);
#endif /* _TEST */

    protected:
        second_t ackedSendTime;
        int ackedSizeBytes;

        int headerSize;
        int bodySize;
        int initialTTL;
        int ttl;
        int sourceId;
        int destId;
        int flowId;

        int dataId;

        bool sourcePacket;

        int ackedId;

        second_t createTime;
        second_t arrivalTime; // currently not used
};

class MDCPacket: public Packet {
    public:
        MDCPacket(int id, int sourceId, int destId, int flowId, int dataId, int ttl,
                int headerSize, int bodySize, bool sourcePacket);

        void recordDeflection(int switchId, int interfaceId);

        void arrive();
        void drop();

    protected:
        vector<pair<int, int>> deflections; // switchId, interfaceId
        int ttlInitial;

        virtual void updateSwitches(double lostCost, double resendCost);
};

class MBDPacket: public MDCPacket {
    public:
        MBDPacket(int id, int sourceId, int destId, int flowId, int dataId, int ttl,
                int headerSize, int bodySize, bool sourcePacket);

        void recordAction(int switchId);

    protected:
        void updateSwitches(double lostCost, double resendCost);

        double shortestPath(int source, int dest);

        vector<int> switches;
};

#ifdef _TEST
int testPacket();
#endif /* _TEST */

#endif // PACKET_H
