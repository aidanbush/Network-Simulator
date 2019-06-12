#ifndef INTERFACE_H
#define INTERFACE_H

#include <queue>

#include "networkObject.h"
#include "packetHandler.h"
#include "packet.h"
#include "link.h"
#include "manager.h"

class Link;
class PacketHandler;

class Interface: public NetworkObject {
    public:
        Interface(int id, Link *link, int linkBufSize, int handlerBufSize);
        int addHandler(PacketHandler *handler);

        void txLinkEvent();
        void txHandlerEvent();

        void rxLink(Packet *p);
        void rxHandler(Packet *p);

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
