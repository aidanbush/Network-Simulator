#ifndef INTERFACE_H
#define INTERFACE_H

#include <nlohmann/json.hpp>

#include "networkObject.h"

using json = nlohmann::json;

class Interface: public NetworkObject {
    public:
        Interface(json interfaceConfig);
        void setOutgoingLink(int outLinkId);
        void setIncomingLink(int inLinkId);
    private:
        int packetHandlerId;
        int neighbourId;
        int outgoingLinkId;
        int incomingLinkId;
};

#endif // INTERFACE_H
