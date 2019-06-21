#ifndef INTERFACE_H
#define INTERFACE_H

#include <nlohmann/json.hpp>
#include <queue>

#include "networkObject.h"
#include "link.h"

class Link;
class PacketHandler;
class Packet;

using json = nlohmann::json;

class Interface: public NetworkObject {
    public:
        Interface(json interfaceConfig);
        void setLink(int linkId);
        
        void txLinkEvent();
        void txHandlerEvent();
        
        void rxLink(Packet *p);
        void rxHandler(Packet *p);
        
        int getLinkSpeed();
        second_t getLinkTxTime();
        
#ifdef _TEST
        static int ifaceToIface();
#endif /* _TEST */
    private:
        int linkId;
        int packetHandlerId;
        
        int linkBufSize; // bytes
        int handlerBufSize; // bytes
        
        queue<Packet*> linkBuffer;
        queue<Packet*> handlerBuffer;
};

#ifdef _TEST
int testInterface();
#endif /* _TEST */

#endif /* INTERFACE_H */
