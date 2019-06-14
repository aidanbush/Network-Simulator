#ifndef SWITCH_H
#define SWITCH_H

#include "packetHandler.h"

using namespace std;

class Switch: public PacketHandler {
    public:
        Switch(int id);

        int getInternalSpeed();
        void setInternalSpeed(int speed);

        void rxPacket(Packet *p);

#ifdef _TEST
        void testOne(); // Temporary, delete
        void testTwo(); // Temporary, delete
        void testThree(); // Temporary, delete
        void testFour(); // Temporary, delete
#endif // _TEST

    private:
        int internalSpeed;
};

#endif // SWITCH_H
