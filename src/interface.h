#ifndef INTERFACE_H
#define INTERFACE_H

#include <queue>

#include "networkObject.h"
#include "packetHandler.h"
#include "packet.h"
#include "link.h"

class Link;
class PacketHandler;

class Interface: public NetworkObject {
    public:
        void txLinkEvent();
        void txHandlerEvent();

        void rxLink(Packet *p);
        void rxHandler(Packet *p);

    private:
        Link *link;
        PacketHandler *handler;

        queue<Packet*> linkBuffer;
        queue<Packet*> handlerBuffer;

        int linkSpeed;
        int handlerSpeed;
};

#endif // INTERFACE_H
