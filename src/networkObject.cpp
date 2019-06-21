#include "networkObject.h"

using namespace std;

NetworkObject::NetworkObject(int id) {
    this->id = id;
}

int NetworkObject::getID() {
    return id;
}
