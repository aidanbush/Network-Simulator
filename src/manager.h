#ifndef MANAGER_H
#define MANAGER_H

#include <queue>
#include <map>
#include <vector>

//TODO: why not include the headers?
class PacketHandler;
class Endpoint;
class Switch;
class Interface;
class Link;
class Packet;
class Flow;

using namespace std;

typedef double second_t;

struct EventI {
    second_t time;

    virtual ~EventI() = default;
    virtual void call() const {}
};

template <typename T> struct Event: EventI {
    void (T::*fcnPtr)();
    T *obj;
    Event(second_t t, void (T::*fp)(), T *o) {
        this->time = t;
        this->obj = o;
        this->fcnPtr = fp;
    }
    void call() const {
        (obj->*fcnPtr)();
    }
};

struct EventQueueComparator {
    bool operator()(const EventI *lhs, const EventI *rhs) const {
        return lhs->time > rhs->time; // Lower time is higher priority
    }
};

class Manager {
    public:
        Manager();

        second_t time;
        // global stats

        PacketHandler *getHandler(int id);

        Switch *getSwitch(int id);
        bool addSwitch(Switch *netSwitch);

        Endpoint *getEndpoint(int id);
        bool addEndpoint(Endpoint *endpoint);
        vector<int> getEndpoints();

        Interface *getInterface(int id);
        bool addInterface(Interface *interface);

        Link *getLink(int id);
        bool addLink(Link *netLink);

        Flow *getFlow(int id);
        bool addFlow(Flow *flow);
        void removeFlow(int id);

        void pushEvent(EventI *e) {pq.push(e); }
        EventI *popEvent();
        priority_queue<EventI*, vector<EventI*>, EventQueueComparator>::size_type
            numEvents() {return pq.size(); }

        bool linkHandlers();
        bool validateNetwork();
        void deleteNetwork();

        bool setLogFile(string filename);

        bool setCSVFilename(string filename);
        string getCSVFilename();

        void setParameters(vector<double> params, double initialWeights);
        bool getParameters(vector<double> *params, int numParams);
        bool getInitialWeights(double *initialWeights);

        void setSuppressOutput(int);

        int getSuppressOutput();

        void logTxEvent(string objName, int objId, string eventName, int destId, Packet *p);
        void logEvent(string objName, int objId, string eventName, string message);

        bool startSimulator();

    private:
        bool addHandler(PacketHandler *handler);

        void deleteHandlers();
        void deleteInterfaces();
        void deleteLinks();
        void deleteFlows();

        void deleteEvents();

        bool parametersSet = false;
        vector<double> params;
        double initialWeights;

        double totalReward = 0;

        int suppressOutput = 0;

        priority_queue<EventI*, vector<EventI*>, EventQueueComparator> pq;
        map<int, PacketHandler*> packetHandlers;
        map<int, Interface*> interfaces;
        map<int, Link*> links;
        map<int, Flow*> flows;

        FILE *logFile;

        string CSVFilename;
};

extern Manager man;

#endif // MANAGER_H
