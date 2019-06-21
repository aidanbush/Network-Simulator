#include <nlohmann/json.hpp>

#include "interface.h"

using namespace std;
using json = nlohmann::json;

Interface::Interface(json interfaceConfig) {
    this->id = interfaceConfig["id"];
    this->packetHandlerId = interfaceConfig["phId"];
    this->neighbourId = interfaceConfig["neighbourId"];
}

void Interface::setOutgoingLink(int outLinkId) {
    this->outgoingLinkId = outLinkId;
}

void Interface::setIncomingLink(int inLinkId) {
    this->incomingLinkId = inLinkId;
}
