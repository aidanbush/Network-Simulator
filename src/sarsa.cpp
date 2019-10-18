#include <vector>
#include <stdlib.h>

#include "sarsa.h"
#include "tilecoder.h"

using namespace std;

#define ALPHA 0.2
#define LAMBDA 0.9
#define GAMMA 1
#define EPSILON 0.1
#define INITIAL_WEIGHTS 0.1
#define NUM_ACTIONS 2

vector<double> Sarsa::initializeWeights() {
    return vector<double>(Tilecoder::getNumTiles() * NUM_ACTIONS, INITIAL_WEIGHTS);
}

Sarsa::Sarsa(vector<double> *weights, double initialState) {
    //TODO: Seed?
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
        return pair<int, double>(ind, val);
    } else {
        double best;
        int bestInd;
        for (int i = 0; i < NUM_ACTIONS; i++) {
            double val = sumIndices(weights, tiles, i*Tilecoder::getNumTiles());
            if (val > best || i == 0) {
                best = val;
                bestInd = i;
            }
        }
        return pair<int, double>(bestInd, best);
    }
}

//TODO: Might be able to remove oldAction variable
int Sarsa::step(double state, double reward) {
    // Tilecode
    vector<int> tiles = Tilecoder::tilecode(state);
    
    // Select action
    pair<int, double> actionValue = selectAction(tiles);
    int action = actionValue.first;
    double value = actionValue.second;
    
    // Update values
    double newOldValue = selectAction(oldTiles).second;
    double delta = reward + GAMMA*value - newOldValue;
    for (int i = 0; i < Tilecoder::getNumTiles() * NUM_ACTIONS; i++) {
        (*weights)[i] += ALPHA*(delta + value - oldValue)*trace[i];
        trace[i] *= GAMMA*LAMBDA;
    }
    for (auto tile: tiles) {
        (*weights)[action*Tilecoder::getNumTiles() + tile] -= ALPHA*(value - oldValue);
        trace[action*Tilecoder::getNumTiles() + tile] += (1 - ALPHA);
    }
    
    oldAction = action;
    oldValue = value;
    oldTiles = tiles;
    
    return action;
}
