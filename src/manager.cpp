#include <map>
#include <vector>
#include <iostream>

#include "manager.h"
#include "packetHandler.h"
#include "switch.h"
#include "endpoint.h"
#include "interface.h"
#include "link.h"
#include "packet.h"
#include "flow.h"

Manager::Manager() {
    time = 0;
    logFile = stdout;
}

bool Manager::addHandler(PacketHandler *handler) {
    int id = handler->getId();

    if (handler == NULL) {
        return false;
    }

    return packetHandlers.emplace(id, handler).second;
}

PacketHandler *Manager::getHandler(int id) {
    auto handler = packetHandlers.find(id);
    if (handler == packetHandlers.end()) {
        return NULL;
    }

    return handler->second;
}

// switch
bool Manager::addSwitch(Switch *netSwitch) {
    return addHandler(netSwitch);
}

Switch *Manager::getSwitch(int id) {
    auto handlerIt = packetHandlers.find(id);
    if (handlerIt == packetHandlers.end()) {
        return NULL;
    }

    return dynamic_cast<Switch *>(handlerIt->second);
}

// endpoint
bool Manager::addEndpoint(Endpoint *endpoint) {
    return addHandler(endpoint);
}

Endpoint *Manager::getEndpoint(int id) {
    auto handlerIt = packetHandlers.find(id);
    if (handlerIt == packetHandlers.end()) {
        return NULL;
    }

    return dynamic_cast<Endpoint *>(handlerIt->second);
}

vector<int> Manager::getEndpoints() {
    vector<int> endpoints;

    for (auto &it : packetHandlers) {
        if (dynamic_cast<Endpoint *>(it.second) != NULL) {
            endpoints.push_back(it.first);
        }
    }

    return endpoints;
}

// interface
bool Manager::addInterface(Interface *interface) {
    int id = interface->getId();

    if (interface == NULL) {
        return false;
    }

    return interfaces.emplace(id, interface).second;
}

Interface *Manager::getInterface(int id) {
    auto interface = interfaces.find(id);
    if (interface == interfaces.end()) {
        return NULL;
    }

    return interface->second;
}

// link
bool Manager::addLink(Link *netLink) {
    int id = netLink->getId();

    if (netLink == NULL) {
        return false;
    }

    return links.emplace(id, netLink).second;
}

Link *Manager::getLink(int id) {
    auto netLink = links.find(id);
    if (netLink == links.end()) {
        return NULL;
    }

    return netLink->second;
}

// flow
bool Manager::addFlow(Flow *flow) {
    int id = flow->getId();

    if (flow == NULL) {
        return false;
    }

    return flows.emplace(id, flow).second;
}

void Manager::removeFlow(int id) {
    Flow *f = getFlow(id);
    totalReward += f->getTotalReward();
    flows.erase(id);
    if (flows.size() == 0) {
        // All flows finished, end simulation
        if (!suppressOutput) {
            for (auto p: params) {
                cout << p << ",";
            }
            cout << initialWeights << ",";
        }
        cout << totalReward << endl;
        exit(0);
    }
}

Flow *Manager::getFlow(int id) {
    auto flow = flows.find(id);
    if (flow == flows.end()) {
        return NULL;
    }

    return flow->second;
}

EventI *Manager::popEvent() {
    EventI *e = pq.top();
    pq.pop();

    if (time > e->time) {
        // TODO THIS IS BAD
    }

    time = e->time;
    return e;
}

void Manager::deleteHandlers() {
    for (auto [id, handler] : packetHandlers) {
        delete handler;
    }

    packetHandlers.clear();
}

void Manager::deleteInterfaces() {
    for (auto [id, interface] : interfaces) {
        delete interface;
    }

    interfaces.clear();
}

void Manager::deleteLinks() {
    for (auto [id, netLink] : links) {
        delete netLink;
    }

    links.clear();
}

void Manager::deleteFlows() {
    for (auto [id, flow] : flows) {
        delete flow;
    }

    flows.clear();
}

void Manager::deleteEvents() {
    EventI *e;

    while (!pq.empty()) {
        e = pq.top();
        pq.pop();
        delete e;
    }
}

void Manager::deleteNetwork() {
    deleteHandlers();
    deleteInterfaces();
    deleteLinks();
    deleteFlows();

    deleteEvents();

    if (logFile != stdout) {
        if (fclose(logFile)) {
            perror("fclose");
        }
    }
}

bool Manager::linkHandlers() {
    for (auto const& it : packetHandlers) {
        if (!it.second->connectNeighbours()) {
        return false;
        }
    }

    return true;
}

bool Manager::validateNetwork() {
    bool valid = true;

    // validate each handler
    for (auto& it : packetHandlers) {
        if (!it.second->validate()) {
            valid = false;
        }
    }

    // validate each interface
    for (auto& it : packetHandlers) {
        if (!it.second->validate()) {
            valid = false;
        }
    }

    // validate each link
    for (auto& it : packetHandlers) {
        if (!it.second->validate()) {
            valid = false;
        }
    }

    return valid;
}

bool Manager::startSimulator() {
    for (auto &it : packetHandlers) {
        Switch *netSwitch = dynamic_cast<Switch *>(it.second);
        if (netSwitch != NULL) {
            if (!netSwitch->initSwitch()) {
                return false;
            }
        }
    }

    for (auto& it : flows) {
        it.second->startFlow();
    }

    return true;
}

void Manager::setParameters(vector<double> params, double initialWeights) {
    parametersSet = true;
    this->params = params;
    this->initialWeights = initialWeights;
}

bool Manager::getParameters(vector<double> *params, int numParams) {
    if (parametersSet && numParams == (int)this->params.size()) {
        // TODO: print warning if incorrect number of parameters
        *params = this->params;
        return true;
    } else {
        return false;
    }
}

bool Manager::getInitialWeights(double *initialWeights) {
    if (parametersSet) {
        *initialWeights = this->initialWeights;
        return true;
    } else {
        return false;
    }
}

void Manager::setSuppressOutput(int value) {
    suppressOutput = value;
}

int Manager::getSuppressOutput() {
    return suppressOutput;
}

bool Manager::setLogFile(string filename) {
    FILE *newLog = fopen(filename.c_str(), (char *)"w");

    if (newLog == NULL) {
        perror("fopen");
        return false;
    }

    logFile = newLog;
    return true;
}

bool Manager::setCSVFilename(string filename) {
    if (CSVFilename.empty()) {
        CSVFilename = filename;
        return true;
    }

    return false;
}

string Manager::getCSVFilename() {
    return CSVFilename;
}

void Manager::logTxEvent(string objName, int objId, string eventName, int destId, Packet *p) {
    string message = "dest: " + to_string(destId) + " packet: " + to_string(p->getId()) + " flow: "
        + to_string(p->getFlow());
    logEvent(objName, objId, eventName, message);
}

void Manager::logEvent(string objName, int objId, string eventName, string message) {
    if (suppressOutput < 1) {
        fprintf(logFile, "time: %f %s: %d event: %s message: %s\n", time,
            objName.c_str(), objId, eventName.c_str(), message.c_str());
    }
}
