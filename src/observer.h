#ifndef OBSERVER_H
#define OBSERVER_H

#include <string>
#include <vector>
#include <map>

using namespace std;

class Observer {
    public:
        Observer();

        void initializeObserver();

        void logFlowData(int flowid, string type, double data, bool reportNan);
        void logLinkData(int sourceId, int destId, string type, double data);
        void logSwitchData(int switchId, string type, double data);

        void writeData();

        void deleteData();

    private:
        map<string, map<int, vector<double>>> flowData; // metric:[flow_id]
        map<string, map<pair<int, int>, vector<double>>> linkData; // metric:[(id1,id2)]
        map<string, map<int, vector<double>>> switchData; // metric:[switch_id]
        string filenamePrefix;

        /*
        template <typename KeyType>
            string switchIDToString(KeyType key);
        template <typename KeyType>
            string flowIDToString(KeyType key);
        template <typename KeyType>
            string linkIDToString(KeyType key);
        */

        template <typename KeyType>
        void writeMetrics(map<string, map<KeyType, vector<double>>> &data, string(*key_to_string)(KeyType));

        void writeFile(string filename, vector<double> data);
        void setFilenamePrefix();
};

extern Observer observer;

#endif /* OBSERVER_H */
