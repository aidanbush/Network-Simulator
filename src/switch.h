#ifndef SWITCH_H
#define SWITCH_H

#include "packetHandler.h"

using namespace std;

class Switch: public PacketHandler {
    public:
        Switch(int id);
        
        int getInternalSpeed();
        
        void setInternalSpeed(int speed);
        
    private:
        int internalSpeed;
};

#endif // SWITCH_H
