#include <nlohmann/json.hpp>

#include "endpoint.h"

using namespace std;
using json = nlohmann::json;

Endpoint::Endpoint(int id) {
    this->id = id;
}
