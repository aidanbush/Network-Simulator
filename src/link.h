#ifndef LINK_H
#define LINK_H

#include "networkObject.h"

using namespace std;

class Link: public NetworkObject {
    public:
    
    private:
        int speed; // bits/second
        int transmissionTime; // microseconds
};

#endif // LINK_H
