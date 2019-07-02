#ifndef MANAGER_H
#define MANAGER_H

#include <queue>
#include <map>

//TODO: why not include the headers?
class PacketHandler;
class Endpoint;
class Switch;
class Interface;
class Link;
class Packet;

using namespace std;

typedef double second_t;

struct EventI {
    second_t time;

    virtual ~EventI() = default;
    virtual void call() const {}
};

template <typename T> struct Event: EventI {
    void (T::*fcnPtr)();
    T* obj;
    Event(second_t t, void (T::*fp)(), T* o) {
        this->time = t;
        this->obj = o;
        this->fcnPtr = fp;
    }
    void call() const {
        (obj->*fcnPtr)();
    }
};

struct EventQueueComparator {
    bool operator()(const EventI* lhs, const EventI* rhs) const {
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
        int addSwitch(Switch *netSwitch);

        Endpoint *getEndpoint(int id);
        int addEndpoint(Endpoint *endpoint);

        Interface *getInterface(int id);
        int addInterface(Interface *interface);

        Link *getLink(int id);
        int addLink(Link *link);

        void pushEvent(EventI *e) {pq.push(e); }
        EventI *popEvent();
        priority_queue<EventI*, vector<EventI*>, EventQueueComparator>::size_type
            numEvents() {return pq.size(); }

        bool linkHandlers();
        bool validateNetwork();
        void deleteNetwork();

        bool setLogFile(string filename);

        void logTxEvent(string objName, int objID, string eventName, int destID, Packet *p);
        void logEvent(string objName, int objID, string eventName, string message);

    private:
        int addHandler(PacketHandler *handler);

        void deleteHandlers();
        void deleteInterfaces();
        void deleteLinks();

        void deleteEvents();

        priority_queue<EventI*, vector<EventI*>, EventQueueComparator> pq;
        map<int, PacketHandler*> packetHandlers;
        map<int, Interface*> interfaces;
        map<int, Link*> links;

        FILE *logFile;
};

extern Manager man;

#endif // MANAGER_H
