#ifndef SWITCH_H
#define SWITCH_H

#include "packetHandler.h"
#include "networkObject.h"

using namespace std;

class Switch: public PacketHandler {
    public:
        Switch(int id);
        
        int getInternalSpeed();
        
        void setInternalSpeed(int speed);
        
        void testOne(); // Temporary, delete
        void testTwo(); // Temporary, delete
        void testThree(); // Temporary, delete
        void testFour(); // Temporary, delete
        
    private:
        int internalSpeed;
};

#endif // SWITCH_H
