#ifndef INTERFACE_H
#define INTERFACE_H

#include <nlohmann/json.hpp>
#include <queue>
#include <set>
#include <vector>

#include "networkObject.h"
#include "link.h"

class Link;
class PacketHandler;
class Packet;

using json = nlohmann::json;

class Interface: public NetworkObject {
    public:
        Interface(json &interfaceConfig);
        bool setLink(int linkId, vector<int> neighbours);
        int getLinkId();

        void txLinkEvent();
        void txHandlerEvent();

        void rxLink(Packet *p);
        void rxHandler(Packet *p);

        int getLinkSpeed();
        second_t getLinkTxTime();

        int getHandlerId();
        set<int> getNeighbours();

        bool validate();

#ifdef _TEST
        static int interfaceToInterface();
#endif /* _TEST */
    private:
        static int validateInterfaceConfig(json &interfaceConfig);

        bool validateHandler();
        bool validateLink();
        bool validateVariables();

        int linkId;
        int handlerId;

        int outBufSize; // bytes
        int inBufSize; // bytes

        queue<Packet*> outBuffer;
        queue<Packet*> inBuffer;
};

#ifdef _TEST
int testInterface();
#endif /* _TEST */

#endif /* INTERFACE_H */
