#include "observer.h"
#include "manager.h"

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <utility>

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
        flowData[type][flowId].push_back(NAN);
    } else {
        flowData[type][flowId].push_back(data);
    }
}

void Observer::logLinkData(int sourceId, int destId, string type, double data) {
    linkData[type][pair<int,int>(sourceId, destId)].push_back(data);
}

void Observer::logSwitchData(int switchId, string type, double data) {
    switchData[type][switchId].push_back(data);
}

template <typename KeyType>
string switchIDToString(KeyType key) {
    int intKey = int(key);
    return "switch_" + to_string(intKey);
}

template <typename KeyType>
string flowIDToString(KeyType key) {
    int intKey = int(key);
    return "flow_" + to_string(intKey);
}

template <typename KeyType>
string linkIDToString(KeyType key) {
    pair<int, int> pair_key = key;
    return "link_" + to_string(pair_key.first) + "_" + to_string(pair_key.second);
}

template <typename KeyType>
void Observer::writeMetrics(map<string, map<KeyType, vector<double>>> &data, string(*key_to_string)(KeyType)) {
    for (auto& metric_it : data) { // for each metric
        string filename = filenamePrefix + "_" + metric_it.first + ".csv";
        FILE *file = fopen(filename.c_str(), "w");

        // for each index into ids
        int row_len = metric_it.second.size();
        vector<double> row(row_len, NAN);

        int max_data_count = 0;
        int column_count = 0;
        for (auto& itt : metric_it.second) {
            if (itt.second.size() > max_data_count) {
                max_data_count = itt.second.size();
            }
            column_count++;
        }

        {
            fprintf(file, "Time,");
            int i = 0;
            for (auto& itt : metric_it.second) {
                string column_name = key_to_string(itt.first);
                fprintf(file, "%s", column_name.c_str());
                if (i < column_count -1) {
                    fprintf(file, ",");
                }
                i++;
            }
            fprintf(file, "\n");
        }

        for (int i = 0; i < max_data_count; i++) {
            int row_i = 0;

            for (auto& itt : metric_it.second) { // for each id
                row[row_i] = NAN;
                if (i < itt.second.size()) { // if data add else nan
                    row[row_i] = itt.second[i];
                }
                row_i++;
            }

            fprintf(file, "%d,", i);
            for (int j = 0; j < row_len; j++) {
                fprintf(file, "%f", row[j]);
                if (j != row_len -1) {
                    fprintf(file, ",");
                }
            }
            fprintf(file, "\n");
        }
        fclose(file);
    }
}

void Observer::writeData() {
    if (man.getSuppressOutput(CSV)) {
        fprintf(stderr, "suppressing observer output\n");
        return;
    }

    // flow data, flow Id's
    writeMetrics(flowData, flowIDToString);

    // link data
    writeMetrics(linkData, linkIDToString);

    // switch data
    writeMetrics(switchData, switchIDToString);
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
