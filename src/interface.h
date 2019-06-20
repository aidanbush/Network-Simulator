#ifndef INTERFACE_H
#define INTERFACE_H

#include <queue>

#include "networkObject.h"
#include "link.h"

class Link;
class PacketHandler;
class Packet;

class Interface: public NetworkObject {
    public:
        Interface(int id, Link *link, int linkBufSize, int handlerBufSize);
        int addHandler(PacketHandler *handler);

        void txLinkEvent();
        void txHandlerEvent();

        void rxLink(Packet *p);
        void rxHandler(Packet *p);

        int getLinkSpeed() {return link->getSpeed(); }
        second_t getLinkTxTime() {return link->getTxTime(); }

#ifdef _TEST
        static int ifaceToIface();
#endif /* _TEST */
    private:
        Link *link;
        PacketHandler *handler;

        int linkBufSize; // bytes
        int handlerBufSize; // bytes

        queue<Packet*> linkBuffer;
        queue<Packet*> handlerBuffer;
};

#ifdef _TEST
int testInterface();
#endif /* _TEST */

#endif /* INTERFACE_H */
