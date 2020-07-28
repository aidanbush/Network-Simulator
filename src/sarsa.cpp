#include <vector>
#include <stdlib.h>
#include <iostream>

#include "agent.h"
#include "sarsa.h"
#include "tilecoder.h"
#include "manager.h"

using namespace std;

// All following defines can be overridden in sarsaDefines.h, which can be modified for local testing
#if __has_include("sarsaDefines.h")
#include "sarsaDefines.h"
#else
#define DEFAULT_ALPHA 0.9
#define DEFAULT_LAMBDA 0.975
#define DEFAULT_GAMMA 0.9
#define DEFAULT_EPSILON 0.01
#define DEFAULT_INITIAL_WEIGHTS 0.1
#define NUM_PARAMS 4
#endif // __has_include

#define TILECODE_NUM_DIMS 1
#define TILECODE_DIM_RANGES {{0,1}}
#define TILECODE_PATTERNS {{0}}
#define TILESCODE_TILES_PER_DIM_TILING {11}
#define TILECODE_NUM_TILIGS {1}

#define NUM_ACTIONS 4

vector<double> Sarsa::initializeWeights() {
    double initialWeights;
    if (!man.getInitialWeights(&initialWeights)) {
        initialWeights = DEFAULT_INITIAL_WEIGHTS;
    }
    return vector<double>(tilecoder->getNumTiles() * NUM_ACTIONS, initialWeights);
}

Sarsa::Sarsa(vector<double> initialState, int flowId) {
    this->flowId = flowId;

    vector<double> params;
    if (!man.getParameters(&params, NUM_PARAMS)) {
        initialAlpha = DEFAULT_ALPHA;
        lambda = DEFAULT_LAMBDA;
        gamma = DEFAULT_GAMMA;
        epsilon = DEFAULT_EPSILON;
    } else {
        initialAlpha = params[0];
        lambda = params[1];
        gamma = params[2];
        epsilon = params[3];
    }

    this->weights = initializeWeights();

    tilecoder = new Tilecoder(TILECODE_NUM_DIMS, TILECODE_DIM_RANGES, TILECODE_PATTERNS,
            TILESCODE_TILES_PER_DIM_TILING, TILECODE_NUM_TILIGS);

    alpha = (double)initialAlpha / tilecoder->getNumTotalTilings();
    trace = vector<double>(tilecoder->getNumTiles() * NUM_ACTIONS, 0);
    oldState = initialState;
    oldTiles = tilecoder->tilecode(initialState);
}

string Sarsa::getName() {
    return "Sarsa";
}

void Sarsa::seed(int seed) {
    generator.seed(seed);
}

pair<int, double> Sarsa::selectAction() {
    if ((double)generator()/(generator.max() - generator.min()) < epsilon) {
        int ind = (int)(generator()%NUM_ACTIONS);
        double val = sumIndices(&weights, &tiles, ind * tilecoder->getNumTiles());
        //cout << "Exploring:\nAction: " << ind << " Value: " << val << endl;
        return pair<int, double>(ind, val);
    } else {
        double best;
        int bestInd;
        for (int i = 0; i < NUM_ACTIONS; i++) {
            double val = sumIndices(&weights, &tiles, i * tilecoder->getNumTiles());
            //cout << "Action: " << i << " Value: " << val << endl;
            if (val > best || i == 0) {
                best = val;
                bestInd = i;
            }
        }
        return pair<int, double>(bestInd, best);
    }
}

pair<double, double> Sarsa::step(vector<double> state, double reward, double &rate) {
    totalReward += reward;
    // Tilecode
    tiles = tilecoder->tilecode(state);

    // Select action
    pair<int, double> actionValue = selectAction();
    int action = actionValue.first;
    double value = actionValue.second;

    // Update values
    double newOldValue = sumIndices(&weights, &oldTiles, oldAction * tilecoder->getNumTiles());
    double delta = reward + gamma*value - newOldValue;
    //cout << "Q: " << newOldValue << " Q\': " << value << " Q_old: " << oldValue << endl;
    //cout << "Alpha: " << alpha << " Delta: " << delta << endl;
    // if (value > 100 || newOldValue > 100 || oldValue > 100) {
    //     exit(0);
    // }
    // Update trace
    for (int i = 0; i < tilecoder->getNumTiles() * NUM_ACTIONS; i++) {
        trace[i] *= gamma*lambda;
    }
    for (auto tile: tiles) {
        trace[action * tilecoder->getNumTiles() + tile] += (1 - alpha);
    }
    // Update Weights
    for (int i = 0; i < tilecoder->getNumTiles() * NUM_ACTIONS; i++) {
        weights[i] += alpha*(delta + newOldValue - oldValue)*trace[i];
    }
    //cout << alpha*(delta + newOldValue - oldValue) << endl;
    for (auto tile: tiles) {
        weights[action * tilecoder->getNumTiles() + tile] -= alpha*(newOldValue - oldValue);
    }
    //cout << alpha*(newOldValue - oldValue) << endl;
    
    oldAction = action;
    oldValue = value;
    oldTiles = tiles;

    pair<double, double> actionPair;

    //update rate
    switch (action) {
        case 0:
            rate *= 2;
            actionPair = pair<double, double>(2, 0);
            break;
        case 1:
            rate /= 2;
            actionPair = pair<double, double>(.5, 0);
            break;
        case 2:
            rate++;
            actionPair = pair<double, double>(0, 1);
            break;
        case 3:
            rate--;
            actionPair = pair<double, double>(0, -1);
            break;
    }

    return actionPair;
}
