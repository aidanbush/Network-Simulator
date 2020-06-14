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

#define NUM_ACTIONS 4

vector<double> Sarsa::initializeWeights() {
    double initialWeights;
    if (!man.getInitialWeights(&initialWeights)) {
        initialWeights = DEFAULT_INITIAL_WEIGHTS;
    }
    return vector<double>(Tilecoder::getNumTiles() * NUM_ACTIONS, initialWeights);
}

Sarsa::Sarsa(vector<double> *weights, double initialState, int flowId) {
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
    this->weights = weights;
    alpha = (double)initialAlpha/Tilecoder::getNumTilings();
    trace = vector<double>(Tilecoder::getNumTiles() * NUM_ACTIONS, 0);
    oldState = initialState;
    oldTiles = Tilecoder::tilecode(initialState);
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
        double val = sumIndices(weights, &tiles, ind*Tilecoder::getNumTiles());
        //cout << "Exploring:\nAction: " << ind << " Value: " << val << endl;
        return pair<int, double>(ind, val);
    } else {
        double best;
        int bestInd;
        for (int i = 0; i < NUM_ACTIONS; i++) {
            double val = sumIndices(weights, &tiles, i*Tilecoder::getNumTiles());
            //cout << "Action: " << i << " Value: " << val << endl;
            if (val > best || i == 0) {
                best = val;
                bestInd = i;
            }
        }
        return pair<int, double>(bestInd, best);
    }
}

void Sarsa::step(double state, double reward, double &rate) {
    totalReward += reward;
    // Tilecode
    tiles = Tilecoder::tilecode(state);
    //for (auto t: tiles) {
        //cout << t << " ";
    //}
    //cout << endl;
    
    // Select action
    pair<int, double> actionValue = selectAction();
    int action = actionValue.first;
    double value = actionValue.second;
    
    // Update values
    double newOldValue = sumIndices(weights, &oldTiles, oldAction*Tilecoder::getNumTiles());
    double delta = reward + gamma*value - newOldValue;
    //cout << "Q: " << newOldValue << " Q\': " << value << " Q_old: " << oldValue << endl;
    //cout << "Alpha: " << alpha << " Delta: " << delta << endl;
    // if (value > 100 || newOldValue > 100 || oldValue > 100) {
    //     exit(0);
    // }
    // Update trace
    for (int i = 0; i < Tilecoder::getNumTiles() * NUM_ACTIONS; i++) {
        trace[i] *= gamma*lambda;
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
    
    //update rate
    switch (action) {
        case 0:
            rate *= 2;
            break;
        case 1:
            rate /= 2;
            break;
        case 2:
            rate++;
            break;
        case 3:
            rate--;
            break;
    }
}
