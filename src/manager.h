#ifndef MANAGER_H
#define MANAGER_H

#include <queue>

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

struct comparator {
    bool operator()(const EventI* lhs, const EventI* rhs) const {
        return lhs->time > rhs->time; // Lower time is higher priority
    }
};

class Manager {
    public:
        Manager();

        priority_queue<EventI*, vector<EventI*>, comparator> pq;
        second_t time;
        // global stats
        // reference to all
        // switches
        // endpoints
        // links
        // flow
};

extern Manager man;

#endif // MANAGER_H
