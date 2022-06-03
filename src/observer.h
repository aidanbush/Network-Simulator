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

        void logFlowData(int flowid, string type, double data);
        void logFlowDataBulk(int flowId, string type, vector<double> data);

        void writeData();

    private:
        map<int, map<string, vector<double>>> flowData;
        string filenamePrefix;

        void writeFile(string filename, vector<double> data);
        void setFilenamePrefix();
};

extern Observer observer;

#endif /* OBSERVER_H */
