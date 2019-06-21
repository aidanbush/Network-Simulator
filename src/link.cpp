#include <nlohmann/json.hpp>

#include "link.h"

using namespace std;
using json = nlohmann::json;

Link::Link(json linkConfig) {
    this->id = linkConfig["id"];
    this->sourceInterfaceId = linkConfig["src"];
    this->destInterfaceId = linkConfig["dest"];
}