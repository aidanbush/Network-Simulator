#include <iostream>

#include "switch.h"
#include "simulator.h"

using namespace std;

Switch::Switch(int id) {
    this->id = id;
}

int Switch::getInternalSpeed() {
    return internalSpeed;
}

void Switch::setInternalSpeed(int speed) {
    internalSpeed = speed;
}

void Switch::testOne() {
    cout << "Test Zero" << endl;
    EventI* e2 = new Event<Switch>(2,&Switch::testTwo,this);
    EventI* e3 = new Event<Switch>(3,&Switch::testThree,this);
    EventI* e4 = new Event<Switch>(4,&Switch::testFour,this);
    pq.push(e3);
    pq.push(e4);
    pq.push(e2);
    cout << "Test One" << endl;
}

void Switch::testTwo() {
    cout << "Test Two" << endl;
}

void Switch::testThree() {
    cout << "Test Three" << endl;
}

void Switch::testFour() {
    cout << "Test Four" << endl;
}
