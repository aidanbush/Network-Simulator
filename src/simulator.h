#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <queue>

using namespace std;

struct EventI {
    int time;
    virtual void call() const {}
};

template <typename T> struct Event: EventI {
    void (T::*fcnPtr)();
    T* obj;
    Event(int t, void (T::*fp)(), T* o) {
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

extern priority_queue<EventI*, vector<EventI*>, comparator> pq;

#endif // SIMULATOR_H
