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
        void logFlowDataBulk(int flowId, string type, vector<double> data);
        void logLinkData(int sourceId, int destId, string type, double data); //TODO implement

        void writeData();

        void deleteData();

    private:
        map<int, map<string, vector<double>>> flowData;
        map<pair<int, int>, map<string, vector<double>>> linkData;
        string filenamePrefix;

        void writeFile(string filename, vector<double> data);
        void setFilenamePrefix();
};

extern Observer observer;

#endif /* OBSERVER_H */
