#include <vector>

#include "agent.h"

using namespace std;

double Agent::sumIndices(vector<double> *vec, vector<int> *indices, int offset) {
    double s = 0;
    for (auto i: *indices) {
        s += vec->at(i + offset);
    }
    return s;
}
