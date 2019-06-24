#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H

#include <vector>
#include <map>
#include <nlohmann/json.hpp>

#include "networkObject.h"

using namespace std;

using json = nlohmann::json;

class Interface;
class Packet;

class PacketHandler: public NetworkObject {
    public:
        PacketHandler(int id, int speed);
        
        vector<int> getInterfaces();
        
        bool addInterface(int destId, int interfaceId);
        void removeInterface(int interfaceId);
        
        vector<int> getNeighbours();
        int getInternalSpeed();
        
        virtual void rxPacket(Packet *p) = 0;
        
    protected:
        map<int, int> interfaces; // map neighbour id to interface id
        
        int internalSpeed;
};

#ifdef _TEST
class TestHandler: public PacketHandler {
    public:
        TestHandler(int id, int internalSpeed): PacketHandler(id, internalSpeed) {; }

        void rxPacket(Packet *) {; }
};
#endif /* _TEST */

#endif /* PACKET_HANDLER_H */
