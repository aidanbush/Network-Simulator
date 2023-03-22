#include "observer.h"
#include "manager.h"

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cmath>

#define DEFAULT_STAT_DIR "results"

using namespace std;

Observer::Observer() {
}

void Observer::initializeObserver() {
    setFilenamePrefix();
}

void Observer::setFilenamePrefix() {
    string fileDir = man.getCSVDir();
    if (fileDir.empty()) {
        fileDir = string(DEFAULT_STAT_DIR);
    }

    // create directory if it doesnt exist
    if (!filesystem::exists(fileDir)) {
        filesystem::create_directory(fileDir);
    }

    filenamePrefix = man.getCSVFilename();

    if (filenamePrefix.empty()) {
        time_t currentTime;
        time(&currentTime);
        tm *currentTm = localtime(&currentTime);

        char date[13];
        strftime(date, 13, "%Y%m%d%H%M", currentTm);

        filenamePrefix = fileDir + "/_" + date;
    } else {
        filenamePrefix = fileDir + "/" + filenamePrefix;
    }
}

void Observer::logFlowData(int flowId, string type, double data, bool reportNan) {
    if (reportNan) {
        flowData[flowId][type].push_back(NAN);
    } else {
        flowData[flowId][type].push_back(data);
    }
}

void Observer::logFlowDataBulk(int flowId, string type, vector<double> data) {
    flowData[flowId][type] = data;
}

void Observer::logLinkData(int sourceId, int destId, string type, double data) {
    linkData[pair<int,int>(sourceId, destId)][type].push_back(data);
}

void Observer::logSwitchData(int switchId, string type, double data) {
    switchData[switchId][type].push_back(data);
}

void Observer::writeData() {
    if (man.getSuppressOutput(CSV)) {
        fprintf(stderr, "suppressing observer output\n");
        return;
    }

    // flow data, flow Id's
    for (auto& it : flowData) {
        string filename = filenamePrefix + "_Flow" + to_string(it.first);
        // data elements
        for (auto& itt : it.second) {
            writeFile(filename + "_" + itt.first + ".csv", itt.second);
        }
    }

    // link data
    for (auto& it : linkData) {
        string filename = filenamePrefix + "_link" + to_string(it.first.first) + "-" + to_string(it.first.second);
        for (auto& itt : it.second) {
            writeFile(filename + "_" + itt.first + ".csv", itt.second);
        }
    }

    // switch data
    for (auto& it : switchData) {
        string filename = filenamePrefix + "_switch_" + to_string(it.first);
        for (auto& itt : it.second) {
            writeFile(filename + "_" + itt.first + ".csv", it.second);
        }
    }
}

void Observer::writeFile(string filename, vector<double> data) {
    ofstream ofs;
    ofs.open(filename, ofstream::trunc);

    if (data.size() >= 1) {
        ofs << data[0];
        for (int i = 1; i < (int)data.size(); i++) {
            ofs << "," << data[i];
        }
    }

    ofs.close();
}

void Observer::deleteData() {
    flowData.clear();
}
