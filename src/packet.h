#ifndef PACKET_H
#define PACKET_H

#define BITS_PER_BYTE   8
#define NULL_DATA_ID    -1

#include "networkObject.h"
#include "flow.h"

class Flow;

class Packet: public NetworkObject {
    public:
        Packet(int id, int sourceId, int destId, int flowId, int dataId, int ttl,
                int headerSize, int bodySize, bool sourcePacket);
        ~Packet() = default;

        Packet *clone();

        int getTTL() {return ttl; }
        int getFlow() {return flowId; }
        int getSource() {return sourceId; }
        int getDest() {return destId; }
        int getDataId() {return dataId; }
        int getHeaderSize() {return headerSize; }
        int getBodySize() {return bodySize; }
        bool isSourcePacket() {return sourcePacket; }

        int fullSize() {return headerSize + bodySize; }
        int fullSizeBits() {return (headerSize + bodySize) * BITS_PER_BYTE; }
        void decTTL() {ttl--; }

        void arrive();
        void drop();
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

    private:
        int headerSize;
        int bodySize;
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

class ECNPacket: public Packet {
    public:
        struct ackMetaData {
            bool ECNBit;
            double bufferOccupancy; // ECN Scale
        };

        ECNPacket(int id, int sourceId, int destId, int flowId, int dataId, int ttl,
                int headerSize, int bodySize, bool sourcePacket); // TODO add ack data
        ~ECNPacket() = default;

        void setECNBit();
        bool getECNBit();
        void setECNScale(double scale);
        double getECNScale();

        // TODO set to bool and only allow setting once
        void setAckData(second_t sendTime, int sizeBytes, bool ECNBit, double bufferOccupancy, int ackedId);
        ackMetaData getAckData();

    private:
        bool ECNBit;
        double ECNScale;
        ackMetaData ackData;
};

#ifdef _TEST
int testPacket();
#endif /* _TEST */

#endif // PACKET_H
