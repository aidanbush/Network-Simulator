#ifndef MANAGER_H
#define MANAGER_H

#define NULL_TIME   -1.0
#define NULL_ID     -1

#include <queue>
#include <map>
#include <vector>
#include <random>

//TODO: why not include the headers?
class PacketHandler;
class Endpoint;
class Switch;
class Interface;
class Link;
class Packet;
class Flow;

class Manager;

extern Manager man;

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

enum LogLevel {
    CSV=1,
    PARAMS=2,
    SIM_EVENTS=4,
    AGENT_VALS=8
};

class Manager {
    public:
        Manager();

        struct EventQueueComparator {
            bool operator()(const EventI *lhs, const EventI *rhs) const {
                if (lhs->time == rhs->time) {
                    return man.random() % 2;
                }
                return lhs->time > rhs->time; // Lower time is higher priority
            }
        };

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

        bool setCSVDir(string filename);
        string getCSVDir();

        void setSuppressOutput(int value);
        int getSuppressOutput(LogLevel level);

        void logTxEvent(string objName, int objId, string eventName, int destId, Packet *p);
        void logEvent(string objName, int objId, string eventName, string message);

        bool startSimulator();

        void setExitTime(second_t time);
        bool checkExitTime();

        void seed(int seed);
        int random();

    private:
        bool addHandler(PacketHandler *handler);

        void deleteHandlers();
        void deleteInterfaces();
        void deleteLinks();
        void deleteFlows();

        void deleteEvents();

        bool parametersSet = false;
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
        string CSVDir;

        mt19937 generator;

        second_t exitTime;
};

#endif // MANAGER_H
