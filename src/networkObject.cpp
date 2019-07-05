#include <nlohmann/json.hpp>
#include <iostream>

#include "networkObject.h"
#include "config.h"
#include "manager.h"

using namespace std;

using json = nlohmann::json;

NetworkObject::NetworkObject(int id) {
    this->id = id;
}

int NetworkObject::getID() {
    return id;
}

//TODO: It seems that most suggestions are to not capitalize the d, I left the other version to avoid breaking things
int NetworkObject::getId() {
    return id;
}
