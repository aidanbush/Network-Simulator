#include "switch.h"

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
