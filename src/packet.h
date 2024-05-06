#ifndef PACKET_H
#define PACKET_H

#define BITS_PER_BYTE       8
#define NULL_DATA_ID        -1
#define NULL_BURST_ID       -1
#define NULL_PACKET_SIZE    -1

#include "networkObject.h"
#include "flow.h"

class Flow;

class Packet: public NetworkObject {
    public:
        Packet(int id, int sourceId, int destId, int flowId, int burstId, bool lastInBurst, int dataId, int ttl,
                int headerSize, int bodySize, bool sourcePacket);
        Packet(const Packet &p);
        ~Packet() = default;
        Packet *clone();

        int getTTL() {return ttl; }
        int getFlow() {return flowId; }
        int getSource() {return sourceId; }
        int getDest() {return destId; }
        int getBurstId() {return burstId; }
        int getDataId() {return dataId; }
        int getAckedId() {return ackedId; }
        int getHeaderSize() {return headerSize; }
        int getBodySize() {return bodySize; }
        bool isSourcePacket() {return sourcePacket; }
        bool isLastInBurst() {return lastInBurst; }
        void setLastInBurst() {this->lastInBurst = true; }

        int fullSize() {return headerSize + bodySize; }
        int fullSizeBits() {return (headerSize + bodySize) * BITS_PER_BYTE; }
        int hopCount() {return initialTTL - ttl;}
        void decTTL() {ttl--; }
        void setTimedOut() { this->ttl = 0; } // used if prematurely timed out
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
        int burstId;
        bool lastInBurst;

        int dataId;

        bool sourcePacket;

        int ackedId;

        second_t createTime;
        second_t arrivalTime; // currently not used
};

class RandDeflectPacket: public Packet {
    public:
        RandDeflectPacket(int id, int sourceId, int destId, int flowId, int burstId, bool lastInBurst, int dataId,
                int ttl, int headerSize, int bodySize, bool sourcePacket, int deflectionsRemaining);

        void recordDeflection() {this->remainingDeflections--; }
        int deflectionsRemaining() {return this->remainingDeflections; }
    protected:
        int remainingDeflections;
};

class MBDPacket: public Packet {
    public:
        MBDPacket(int id, int sourceId, int destId, int flowId, int burstId, bool lastInBurst, int dataId,
                int ttl, int headerSize, int bodySize, bool sourcePacket, int deflectionsRemaining);

        void arrive();
        void drop();

        void recordAction(int switchId);
        void recordDeflection() {this->remainingDeflections--; }
        int deflectionsRemaining() {return this->remainingDeflections; }

    protected:
        int remainingDeflections;
        vector<int> switches;

        void updateSwitches(bool arrived);
};

class NDDPacket: public Packet {
    public:
        NDDPacket(int id, int sourceId, int destId, int flowId, int burstId, bool lastInBurst, int dataId,
                int ttl, int headerSize, int bodySize, bool sourcePacket);

        bool deflect(int deflectionId, int deflectingSwitchId, int initialDHC);

        int getDeflectionId() {return this->deflectionId; }
        int getDeflectingSwitchId() {return this->deflectingSwitchId; }
        int getDHC() {return this->DHC; }
        void incrementDHC() {this->DHC++; }

    private:
        int deflectionId;
        int DHC;
        int deflectingSwitchId;
};

#ifdef _TEST
int testPacket();
#endif /* _TEST */

#endif // PACKET_H
