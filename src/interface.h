#ifndef INTERFACE_H
#define INTERFACE_H

#include <nlohmann/json.hpp>
#include <queue>
#include <set>

#include "networkObject.h"
#include "link.h"

class Link;
class PacketHandler;
class Packet;

using json = nlohmann::json;

class Interface: public NetworkObject {
    public:
        Interface(int id, int handlerID, int linkBufSize, int handlerBufSize);
        void setLink(int linkID);

        void txLinkEvent();
        void txHandlerEvent();

        void rxLink(Packet *p);
        void rxHandler(Packet *p);

        int getLinkSpeed();
        second_t getLinkTxTime();

        int getHandlerID();
        set<int> getNeighbours();

#ifdef _TEST
        static int ifaceToIface();
#endif /* _TEST */
    private:
        int linkID;
        int handlerID;

        int linkBufSize; // bytes
        int handlerBufSize; // bytes

        queue<Packet*> linkBuffer;
        queue<Packet*> handlerBuffer;
};

#ifdef _TEST
int testInterface();
#endif /* _TEST */

#endif /* INTERFACE_H */
