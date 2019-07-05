#ifndef NETWORK_OBJECT_H
#define NETWORK_OBJECT_H
#include <nlohmann/json.hpp>

using namespace std;

using json = nlohmann::json;

class NetworkObject {
    public:
        NetworkObject(int id);
        virtual ~NetworkObject() = default;

        int getID();
        int getId();

        virtual bool validate() = 0;

    protected:
        int id;
};

#endif // NETWORK_OBJECT_H
