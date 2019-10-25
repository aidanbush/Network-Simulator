#include <vector>
#include <stdlib.h>
#include <iostream>

#include "sarsa.h"
#include "tilecoder.h"

using namespace std;

#define ALPHA 0.2
#define LAMBDA 0.9
#define GAMMA 0.9
#define EPSILON 0.1
#define INITIAL_WEIGHTS 0.1
#define NUM_ACTIONS 4

vector<double> Sarsa::initializeWeights() {
    return vector<double>(Tilecoder::getNumTiles() * NUM_ACTIONS, INITIAL_WEIGHTS);
}

Sarsa::Sarsa(vector<double> *weights, double initialState) {
    this->weights = weights;
    alpha = (double)ALPHA/Tilecoder::getNumTilings();
    trace = vector<double>(Tilecoder::getNumTiles() * NUM_ACTIONS, 0);
    oldState = initialState;
    oldTiles = Tilecoder::tilecode(initialState);
}

double sumIndices(vector<double> *vec, vector<int> indices, int offset = 0) {
    double s = 0;
    for (auto i: indices) {
        s += (*vec)[i + offset];
    }
    return s;
}

pair<int, double> Sarsa::selectAction(vector<int> tiles) {
    if ((double)random()/RAND_MAX < EPSILON) {
        int ind = random()%NUM_ACTIONS;
        double val = sumIndices(weights, tiles, ind*Tilecoder::getNumTiles());
        //cout << "Exploring:\nAction: " << ind << " Value: " << val << endl;
        return pair<int, double>(ind, val);
    } else {
        double best;
        int bestInd;
        for (int i = 0; i < NUM_ACTIONS; i++) {
            double val = sumIndices(weights, tiles, i*Tilecoder::getNumTiles());
            //cout << "Action: " << i << " Value: " << val << endl;
            if (val > best || i == 0) {
                best = val;
                bestInd = i;
            }
        }
        return pair<int, double>(bestInd, best);
    }
}

int Sarsa::step(double state, double reward) {
    // Tilecode
    vector<int> tiles = Tilecoder::tilecode(state);
    //for (auto t: tiles) {
        //cout << t << " ";
    //}
    //cout << endl;
    
    // Select action
    pair<int, double> actionValue = selectAction(tiles);
    int action = actionValue.first;
    double value = actionValue.second;
    
    // Update values
    double newOldValue = sumIndices(weights, oldTiles, oldAction*Tilecoder::getNumTiles());
    double delta = reward + GAMMA*value - newOldValue;
    //cout << "Q: " << newOldValue << " Q\': " << value << " Q_old: " << oldValue << endl;
    //cout << "Alpha: " << alpha << " Delta: " << delta << endl;
    // if (value > 100 || newOldValue > 100 || oldValue > 100) {
    //     exit(0);
    // }
    // Update trace
    for (int i = 0; i < Tilecoder::getNumTiles() * NUM_ACTIONS; i++) {
        trace[i] *= GAMMA*LAMBDA;
    }
    for (auto tile: tiles) {
        trace[action*Tilecoder::getNumTiles() + tile] += (1 - alpha);
    }
    // Update Weights
    for (int i = 0; i < Tilecoder::getNumTiles() * NUM_ACTIONS; i++) {
        (*weights)[i] += alpha*(delta + newOldValue - oldValue)*trace[i];
    }
    //cout << alpha*(delta + newOldValue - oldValue) << endl;
    for (auto tile: tiles) {
        (*weights)[action*Tilecoder::getNumTiles() + tile] -= alpha*(newOldValue - oldValue);
    }
    //cout << alpha*(newOldValue - oldValue) << endl;
    
    oldAction = action;
    oldValue = value;
    oldTiles = tiles;
    
    return action;
}
