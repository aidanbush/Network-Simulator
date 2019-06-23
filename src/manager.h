#ifndef MANAGER_H
#define MANAGER_H

#include <queue>
#include <map>

class PacketHandler;
class Endpoint;
class Switch;

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
        // reference to all
        // switches
        // endpoints
        // links
        // flow

        PacketHandler *getHandler(int id);
        int addHandler(PacketHandler *handler);

        Switch *getSwitch(int id);
        int addSwitch(Switch *netSwitch);

        Endpoint *getEndpoint(int id);
        int addEndpoint(Endpoint *endpoint);

        void pushEvent(EventI *e) {pq.push(e); }
        EventI *popEvent();
        priority_queue<EventI*, vector<EventI*>, EventQueueComparator>::size_type
            numEvents() {return pq.size(); }

    private:
        priority_queue<EventI*, vector<EventI*>, EventQueueComparator> pq;
        map<int, PacketHandler*> packetHandlers;
        map<int, Switch*> switches;
        map<int, Endpoint*> endpoints;
};

extern Manager man;

#endif // MANAGER_H
