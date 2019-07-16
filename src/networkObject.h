#ifndef NETWORK_OBJECT_H
#define NETWORK_OBJECT_H
using namespace std;

class NetworkObject {
    public:
        NetworkObject(int id);
        virtual ~NetworkObject() = default;

        int getId();

        virtual bool validate() = 0;

    protected:
        int id;
};

#endif // NETWORK_OBJECT_H
