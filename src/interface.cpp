#include <vector>
#include <nlohmann/json.hpp>

#include "interface.h"
#include "packetHandler.h"
#include "manager.h"
#include "link.h"
#include "packet.h"
#include "config.h"

#define IFACE_STR               "Interface"
#define TX_LINK_EVENT_STR       "interface link tx"
#define TX_HANDLER_EVENT_STR    "interface handler tx"
#define RX_LINK_EVENT_STR       "interface link rx"
#define RX_HANDLER_EVENT_STR    "interface handler rx"

#define DEFAULT_ECN_THRESHOLD 0.1

using namespace std;

using json = nlohmann::json;

Interface::Interface(json &interfaceConfig): NetworkObject(validateInterfaceConfig(interfaceConfig)) {
    this->outBufTotalSize = interfaceConfig["out_buf_size"];
    this->inBufTotalSize = interfaceConfig["in_buf_size"];
    this->outBufSize = interfaceConfig["out_buf_size"];
    this->inBufSize = interfaceConfig["in_buf_size"];
    this->handlerId = interfaceConfig["handler_id"];
    this->ECNThreshold = DEFAULT_ECN_THRESHOLD;
}

int Interface::validateInterfaceConfig(json &interfaceConfig) {
    string message = "";
    if (!hasMemberOfType(interfaceConfig, "id", jsonInt)) {
        message += "No integer with name 'id'.\n";
    }

    if (!hasMemberOfType(interfaceConfig, "out_buf_size", jsonInt)) {
        message += "No integer with name 'out_buf_size'.\n";
    }

    if (!hasMemberOfType(interfaceConfig, "in_buf_size", jsonInt)) {
        message += "No integer with name 'in_buf_size'.\n";
    }

    if (!hasMemberOfType(interfaceConfig, "handler_id", jsonInt)) {
        message += "No integer with name 'handler_id'.\n";
    }

    if (!message.empty()) {
        message = "Interface:\n" + message + interfaceConfig.dump(4);
        throw runtime_error(message);
    }
    return interfaceConfig["id"];
}

int Interface::getLinkId() {
    return linkId;
}

bool Interface::setLink(int linkId, vector<int> neighbours) {
    this->linkId = linkId;

    // For every neighbouring interface add the neighbour's handler as a neighbour to this interface's handler
    PacketHandler *packetHandler = man.getHandler(handlerId);
    for (int i: neighbours) {
        if (i != id) {
            Interface *neighbour = man.getInterface(i);
            if (neighbour == NULL) {
                return false;
            }

            if (!packetHandler->addInterface(neighbour->getHandlerId(), id)) {
                return false;
            }
        }
    }
    return true;
}

int Interface::getLinkSpeed() {
    Link *netLink = man.getLink(linkId);
    return netLink->getSpeed();
}

second_t Interface::getLinkTxTime() {
    Link *netLink = man.getLink(linkId);
    return netLink->getTxTime();
}

void Interface::txLinkEvent() {
    Packet *p = outBuffer.front();
    outBuffer.pop();

    man.logTxEvent(IFACE_STR, id, TX_LINK_EVENT_STR, linkId, p);

    outBufSize += p->fullSize();

    Link *netLink = man.getLink(linkId);
    netLink->txPacket(p, id);

    if (!outBuffer.empty()) {
        second_t nextTx = man.time + double(outBuffer.front()->fullSizeBits()) / netLink->getSpeed();
        EventI *e = new Event<Interface>(nextTx, &Interface::txLinkEvent, this);
        man.pushEvent(e);
    }
}

void Interface::txHandlerEvent() {
    Packet *p = inBuffer.front();
    inBuffer.pop();

    man.logTxEvent(IFACE_STR, id, TX_HANDLER_EVENT_STR, linkId, p);

    inBufSize += p->fullSize();

    PacketHandler *handler = man.getHandler(handlerId);
    handler->rxPacket(p);

    if (!inBuffer.empty()) {
        second_t nextTx = man.time + double(inBuffer.front()->fullSizeBits())
            / handler->getInternalSpeed();
        EventI *e = new Event<Interface>(nextTx, &Interface::txHandlerEvent, this);
        man.pushEvent(e);
    }
}

void Interface::tagPacket(Packet *p, int bufferCurrentSize, int bufferFullSize) {
    ECNPacket *ecnP = dynamic_cast<ECNPacket*>(p);
    if (ecnP != NULL) {
        if ((double)bufferCurrentSize / bufferFullSize > ECNThreshold) {
            ecnP->setECN();
        }
    }
}

void Interface::tagPacketIn(Packet *p) {
    tagPacket(p, inBufTotalSize - inBufSize, inBufTotalSize);
}

void Interface::tagPacketOut(Packet *p) {
    tagPacket(p, outBufTotalSize - outBufSize, outBufTotalSize);
}

void Interface::rxLink(Packet *p) {
    if (inBufSize - p->fullSize() < 0) {
        p->drop();
        man.logEvent(IFACE_STR, id, RX_LINK_EVENT_STR, "Packet dropped");
        return;
    }
    
    inBufSize -= p->fullSize();
    inBuffer.push(p);
    tagPacketIn(p);

    if (inBuffer.size() == 1) { //TODO: what if buffer size is larger than 1
        PacketHandler *handler = man.getHandler(handlerId);

        second_t nextTx = man.time + double(p->fullSizeBits()) / handler->getInternalSpeed();
        EventI *e = new Event<Interface>(nextTx, &Interface::txHandlerEvent, this);
        man.pushEvent(e);
    }
}

void Interface::rxHandler(Packet *p) {
    if (outBufSize - p->fullSize() < 0) {
        p->drop();
        man.logEvent(IFACE_STR, id, RX_HANDLER_EVENT_STR, "Packet dropped");
        return;
    }

    outBufSize -= p->fullSize();
    outBuffer.push(p);
    tagPacketOut(p);

    // if only one element add event
    if (outBuffer.size() == 1) {
        Link *netLink = man.getLink(linkId);

        second_t nextTx = man.time + double(p->fullSizeBits()) / netLink->getSpeed();
        EventI *e = new Event<Interface>(nextTx, &Interface::txLinkEvent, this);
        man.pushEvent(e);
    }
}

int Interface::getHandlerId() {
    return handlerId;
}

set<int> Interface::getNeighbours() {
    set<int> neighbours;
    Link *netLink = man.getLink(linkId);

    neighbours = netLink->getNeighbours();
    neighbours.erase(handlerId);

    return neighbours;
}

int Interface::getInBufferTotalSize() {
    return inBufTotalSize;
}
int Interface::getOutBufferTotalSize() {
    return outBufTotalSize;
}

int Interface::getInBufferCurrentSize() {
    return inBufTotalSize - inBufSize;
}

int Interface::getOutBufferCurrentSize() {
    return outBufTotalSize - outBufSize;
}

bool Interface::validateHandler() {
    PacketHandler *handler = man.getHandler(handlerId);
    if (handler == NULL) {
        fprintf(stderr, "Interface: handler %d of interface %d is missing\n",
                handlerId, id);
        return false;
    }

    if (!handler->hasInterface(id)) {
        fprintf(stderr, "Interface: handler %d does not know of interface %d\n",
                linkId, id);
        return false;
    }

    return true;
}

bool Interface::validateLink() {
    Link *netLink = man.getLink(linkId);
    if (netLink == NULL) {
        fprintf(stderr, "Interface: link %d of interface %d is missing\n",
                linkId, id);
        return false;
    }

    bool valid = true;

    if (!netLink->hasInterface(id)) {
        fprintf(stderr, "Interface: link %d does not know of interface %d\n",
                linkId, id);
        valid = false;
    }

    return valid;
}

bool Interface::validateVariables() {
    bool valid = true;

    if (outBufSize <= 0) {
        fprintf(stderr, "Interface: %d has invalid link buffer size %d\n",
                id, outBufSize);
        valid = false;
    }

    if (inBufSize <= 0) {
        fprintf(stderr, "Interface: %d has invalid handler buffer size %d\n",
                id, outBufSize);
        valid = false;
    }

    return valid;
}

bool Interface::validate() {
    bool valid = true;

    // check handler and link
    if (!validateHandler()) {
        valid = false;
    }

    if (!validateLink()) {
        valid = false;
    }

    // check variables
    if (!validateVariables()) {
        valid = false;
    }

    return valid;
}

#ifdef _TEST
#include "endpoint.h"
#include "tests/throwAssert.h"

int Interface::interfaceToInterface() {
    const int l1Id = 1,
          l1Speed = 1600000;
    const second_t l1TxTime = 0.01;
    const int i1Id = 1,
          i1LBuf = 1000,
          i1HBuf = 1000;
    const int i2Id = 2,
          i2LBuf = 1000,
          i2HBuf = 1000;
    const int p1Id = 1,
          p1SId = 1,
          p1DId = 2,
          p1TTL = 10,
          p1HSize = 50,
          p1BSize = 100;
    const int p2Id = 2,
          p2SId = 1,
          p2DId = 2,
          p2TTL = 10,
          p2HSize = 40,
          p2BSize = 120;
    const int e1Id = 1,
          e1Speed = 1;
    const int e2Id = 2,
          e2Speed = 1;
    const int f1Id = 1;

    Link *l1;
    Interface *i1, *i2;
    Packet *p1, *p2;
    Endpoint *e1, *e2;
    EventI *e;

    // setup network
    json endpointJson1 = {
        {"id", e1Id},
        {"internal_speed", e1Speed}
    };
    json endpointJson2 = {
        {"id", e2Id},
        {"internal_speed", e2Speed}
    };
    e1 = new Endpoint(endpointJson1);
    e2 = new Endpoint(endpointJson2);

    man.addEndpoint(e1);
    man.addEndpoint(e2);

    json interfaceJson1 = {
        {"id", i1Id},
        {"handler_id", e1Id},
        {"out_buf_size", i1LBuf},
        {"in_buf_size", i1HBuf}
    };
    json interfaceJson2 = {
        {"id", i2Id},
        {"handler_id", e2Id},
        {"out_buf_size", i2LBuf},
        {"in_buf_size", i2HBuf}
    };
    i1 = new Interface(interfaceJson1);
    i2 = new Interface(interfaceJson2);

    man.addInterface(i1);
    man.addInterface(i2);

    json linkJson = {
        {"id", l1Id},
        {"speed", l1Speed},
        {"time", l1TxTime},
        {"interfaces", {i1Id, i2Id}}
    };
    l1 = new Link(linkJson);
    l1->addToInterfaces();

    man.addLink(l1);

    p1 = new Packet(p1Id, p1SId, p1DId, f1Id, p1TTL, p1HSize, p1BSize);
    p2 = new Packet(p2Id, p2SId, p2DId, f1Id, p2TTL, p2HSize, p2BSize);

    // add p1
    i1->rxHandler(p1);

    // check packet is in buffer
    throwAssert(i1->outBufSize == i1LBuf - p1->fullSize());
    throwAssert(i1->outBuffer.size() == 1);
    throwAssert(i1->outBuffer.front() == p1);

    // check event exists and is correct
    throwAssert(man.numEvents() == 1);

    // add p2
    i1->rxHandler(p2);

    // check packet was added into buffer
    throwAssert(i1->outBufSize == i1LBuf - (p1->fullSize() + p2->fullSize()));
    throwAssert(i1->outBuffer.size() == 2);
    throwAssert(i1->outBuffer.back() == p2);

    // no new events
    throwAssert(man.numEvents() == 1);

    // pop and evaluate event
    e = man.popEvent();
    e->call();
    delete e;

    // two events one transit other queued
    throwAssert(man.numEvents() == 2);

    // check that buffer only holds p2
    throwAssert(i1->outBufSize == i1LBuf - p2->fullSize());
    throwAssert(i1->outBuffer.size() == 1);
    throwAssert(i1->outBuffer.front() == p2);

    // move p2 onto link
    e = man.popEvent();
    e->call();
    delete e;

    throwAssert(man.numEvents() == 1);

    // move p1 from l1 to i2
    e = man.popEvent();
    e->call();
    delete e;

    // check packet arrived
    throwAssert(i2->inBufSize == i2HBuf - p1->fullSize());
    throwAssert(i2->inBuffer.size() == 1);
    throwAssert(i2->inBuffer.front() == p1);

    // move p2 from l1 to i2
    e = man.popEvent();
    e->call();
    delete e;

    // check packet arrived
    throwAssert(i2->inBufSize == i2HBuf - (p1->fullSize() + p2->fullSize()));
    throwAssert(i2->inBuffer.size() == 2);
    throwAssert(i2->inBuffer.back() == p2);

    throwAssert(man.numEvents() == 1);

    // clean up last event
    delete man.popEvent();

    throwAssert(man.numEvents() == 0);

    delete p1;
    delete p2;

    man.deleteNetwork();

    return 1;
}

int testInterface() {
    const int s1Id = 1;
    const int i1Id = 1,
          i1LBuf = 48000,
          i1HBuf = 32000;

    json interfaceJson = {
        {"id", i1Id},
        {"handler_id", s1Id},
        {"out_buf_size", i1LBuf},
        {"in_buf_size", i1HBuf}
    };
    Interface *i1 = new Interface(interfaceJson);

    delete i1;

    return Interface::interfaceToInterface();
}

#endif /* _TEST */
