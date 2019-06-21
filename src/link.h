#ifndef LINK_H
#define LINK_H
#include <nlohmann/json.hpp>

#include "networkObject.h"

using namespace std;
using json = nlohmann::json;

class Link: public NetworkObject {
    public:
        Link(json linkConfig);
    private:
        int speed; // bits/second
        int transmissionTime; // microseconds
        int sourceInterfaceId;
        int destInterfaceId;
};

#endif // LINK_H
