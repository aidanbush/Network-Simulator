#include <iostream>

#include "switch.h"
#include "manager.h"

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

void Switch::rxPacket(Packet *p) {
    // TODO: implement
}

#ifdef _TEST

void Switch::testOne() {
    cout << "Test Zero" << endl;
    EventI* e2 = new Event<Switch>(2, &Switch::testTwo, this);
    EventI* e3 = new Event<Switch>(3, &Switch::testThree, this);
    EventI* e4 = new Event<Switch>(4, &Switch::testFour, this);
    man.pushEvent(e3);
    man.pushEvent(e4);
    man.pushEvent(e2);
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

#endif // _TEST
