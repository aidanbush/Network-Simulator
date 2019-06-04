#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H

#include <vector>

#include "interface.h"

using namespace std;

class PacketHandler: public NetworkObject {
    public:
        vector<Interface> getInterfaces();
        
        void addInterface(Interface interface);
        
        void removeInterface(int interfaceId);
        
        vector<PacketHandler> getNeighbours();
    
    protected:
        vector<Interface> interfaces;
        
        int id;
};

#endif // PACKET_HANDLER_H
