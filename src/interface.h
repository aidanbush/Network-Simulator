#ifndef INTERFACE_H
#define INTERFACE_H

#include <queue>

#include "networkObject.h"
#include "packet.h"
#include "link.h"

class Link;

class Interface: public NetworkObject {
    public:
        void txLinkEvent();
        void txHandlerEvent();

        void rxLink(Packet *p);
        void rxHandler(Packet *p);

    private:
        Link *link;
        int nextLinkTx;
        int nextHandlerTx;
        queue<Packet*> linkBuffer;
        queue<Packet*> handlerBuffer;
};

#endif // INTERFACE_H
