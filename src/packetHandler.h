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
        bool addInterfaceConfig(int ifaceID);

        bool hasInterface(int ifaceID);

        void removeInterface(int interfaceId);

        vector<int> getNeighbours();
        int getInternalSpeed();

        bool connectNeighbours();

        virtual void rxPacket(Packet *p) = 0;

    protected:
        bool validateHandler();

        map<int, int> interfaces; // map neighbour id to interface id

        int internalSpeed;

    private:
        bool validateInterfaces();
        bool validateVariables();
};

#ifdef _TEST
class TestHandler: public PacketHandler {
    public:
        TestHandler(int id, int internalSpeed): PacketHandler(id, internalSpeed) {; }

        void rxPacket(Packet *) {; }

        bool validate() {return true; }
};
#endif /* _TEST */

#endif /* PACKET_HANDLER_H */
