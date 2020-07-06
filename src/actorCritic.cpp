#include <vector>
#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <random>
#include <stdexcept>
#include <boost/math/special_functions/digamma.hpp>

#include "agent.h"
#include "actorCritic.h"
#include "tilecoder.h"
#include "manager.h"

using namespace std;

// All following defines can be overridden in actorCriticDefines.h, which can be modified for local testing
#if __has_include("actorCriticDefines.h")
#include "actorCriticDefines.h"
#else
#define DEFAULT_ALPHA_U 0.005 //Add: 1, Mult: 0.005, Both: 0.005
#define DEFAULT_ALPHA_V 0.01 //Add: 0.5, Mult: 0.005, Both: 0.001
#define DEFAULT_ALPHA_R 0.1 //Add: 0.005, Mult: 0.0001, Both: 0.01
#define DEFAULT_TAU 32 //Add: 32, Mult: 1, Both: 2
#define DEFAULT_INAC false //Add: false, Mult: false, Both: false
#define DEFAULT_S false //Add: true, Mult: false, Both: false
#define DEFAULT_INITIAL_WEIGHTS 0.1
#define GAMMA 1 //Always 1 for continuing case
#define DEFAULT_INITIAL_WEIGHTS 0.1
#define DEFAULT_INITIAL_K_PARAMETERS 1
#define DEFAULT_INITIAL_PHI_PARAMETERS -1
#define DEFAULT_INITIAL_MU_PARAMETERS 0
#define DEFAULT_INITIAL_SIGMA_PARAMETERS 0
#define NUM_PARAMS 6
#define CHOOSE_EPSILON 0.1
#define MODE Mult //Add, Mult, Both, or Choose
#define REPORT_FLOW 1 // -1 for all flows
#endif // __has_include

#define TILE_MULTIPLE 4
//TODO: better names for these constants
#define K_ORDER 0
#define PHI_ORDER 1
#define MU_ORDER 2
#define SIGMA_ORDER 3

vector<double> ActorCritic::initializeWeights() {
    double initialWeights;
    if (!man.getInitialWeights(&initialWeights)) {
        initialWeights = DEFAULT_INITIAL_WEIGHTS;
    }

    return vector<double>(Tilecoder::getNumTiles() * TILE_MULTIPLE, initialWeights);
}

ActorCritic::ActorCritic(vector<double> *weights, double initialState, int flowId, double initialRBar) {
    this->flowId = flowId;
    this->rBar = initialRBar;

    stepnum = 0;

    vector<double> params;
    if (!man.getParameters(&params, NUM_PARAMS)) {
        initialAlphaU = DEFAULT_ALPHA_U;
        initialAlphaV = DEFAULT_ALPHA_V;
        alphaR = DEFAULT_ALPHA_R;
        tau = DEFAULT_TAU;
        inac = DEFAULT_INAC;
        s = DEFAULT_S;
    } else {
        initialAlphaU = params[0];
        initialAlphaV = params[1];
        alphaR = params[2];
        tau = params[3];
        //TODO: ensure this cast works
        inac = params[4];
        s = params[5];
    }
    alphaU = (double)initialAlphaU/Tilecoder::getNumTilings();
    alphaV = (double)initialAlphaV/Tilecoder::getNumTilings();
    lambda = 1 - 1.0/tau;

    parameters = vector<double>(Tilecoder::getNumTiles() * TILE_MULTIPLE, 0);
    for (int i = 0; i < (int)parameters.size(); i++) {
        switch (i / Tilecoder::getNumTiles()) {
            case K_ORDER:
                parameters[i] = DEFAULT_INITIAL_K_PARAMETERS / Tilecoder::getNumTilings();
                break;
            case PHI_ORDER:
                parameters[i] = DEFAULT_INITIAL_PHI_PARAMETERS / Tilecoder::getNumTilings();
                break;
            case MU_ORDER:
                parameters[i] = DEFAULT_INITIAL_MU_PARAMETERS / Tilecoder::getNumTilings();
                break;
            case SIGMA_ORDER:
                parameters[i] = DEFAULT_INITIAL_SIGMA_PARAMETERS / Tilecoder::getNumTilings();
                break;
        }
    }

    criticWeights = weights;

    actorWeights = vector<double>(Tilecoder::getNumTiles() * TILE_MULTIPLE, 0);

    weightTrace = vector<double>(Tilecoder::getNumTiles() * TILE_MULTIPLE, 0);
    parameterTrace = vector<double>(Tilecoder::getNumTiles() * TILE_MULTIPLE, 0);

    oldState = initialState;
    oldTiles = Tilecoder::tilecode(initialState);

    tiles = oldTiles; // TODO does this cause a bug, should we copy over?

    mode = MODE;
    generator = mt19937();
    if ((mode == Both || mode ==Choose) && s) {
        //TODO: add support for this, see Williams, R. 1992. Simple Statistical Gradient-Following
        //                                  Algorithms for Connectionist Reinforcement Learning
        // According to Wolfram, for gamma, sigma^2 = alpha*theta^2 which is k*phi^2 in the variable used here
        // If using this, it will mean having a different step size for different parts of the parameters
        throw runtime_error("S parameter can only be set to true in Add or Mult modes");
    }

    actionPair = selectAction();
}

string ActorCritic::getName() {
    switch (mode) {
        case Add:
            return "ACAdd";
        case Mult:
            return "ACMult";
        case Both:
            return "ACBoth";
        case Choose:
            return "ACChoose";
        default:
            return "AC";
    }
}

void ActorCritic::seed(int seed) {
    generator.seed(seed);
}

double ActorCritic::selectActionMult() {
    k = exp(sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*K_ORDER)) + 1;
    phi = exp(sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*PHI_ORDER));

    gamma_distribution distribution(k, phi);
    //TODO: Consider clipping the value to a pre defined range
    return distribution(generator);
}

double ActorCritic::selectActionAdd() {
    mu = sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*MU_ORDER);
    sigma = exp(sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*SIGMA_ORDER));
    normal_distribution distribution(mu, sigma);
    //TODO: Consider clipping the value to a pre defined range
    return distribution(generator);
}

pair<double, double> ActorCritic::selectAction() {
    pair<double, double> actionPair;
    switch (mode) {
        case Mult:
            actionPair = pair<double, double>(selectActionMult(), 0.0);
            break;
        case Add:
            actionPair = pair<double, double>(0.0, selectActionAdd());
            break;
        case Both:
            actionPair = pair<double, double>(selectActionMult(), selectActionAdd());
            break;
        case Choose:
            bool doMultAction;
            if (generator()/generator.max() < CHOOSE_EPSILON) {
                doMultAction = generator()/generator.max() < 0.5;
            } else {
                double multValue = 0, addValue = 0;
                for (int i = 0; i < (int)tiles.size(); i++) {
                    multValue += criticWeights->at(tiles[i] + K_ORDER*Tilecoder::getNumTiles());
                    multValue += criticWeights->at(tiles[i] + PHI_ORDER*Tilecoder::getNumTiles());
                    addValue += criticWeights->at(tiles[i] + MU_ORDER*Tilecoder::getNumTiles());
                    addValue += criticWeights->at(tiles[i] + SIGMA_ORDER*Tilecoder::getNumTiles());
                }
                doMultAction = multValue > addValue;
            }
            if (doMultAction) {
                actionPair = pair<double, double>(selectActionMult(), 0.0);
            } else {
                actionPair = pair<double, double>(0.0, selectActionAdd());
            }
            break;
    }
    return actionPair;
}

double ActorCritic::getVariance() {
    switch (mode) {
        case Mult:
            return k*phi*phi;
        case Add:
            return sigma*sigma;
        default:
            throw runtime_error("Getting variance only supported in Add and Mult modes");
    }
}

double ActorCritic::computeDiffSum() {
    double diffSum = 0;
    if (mode == Choose) {
        double multNew = 0, multOld = 0, addNew = 0, addOld = 0; 
        for (int i = 0; i < (int)tiles.size(); i++) {
            //TODO: For multOld and addOld consider reusing the computation in the last selectAction call
            multNew += criticWeights->at(tiles[i] + K_ORDER*Tilecoder::getNumTiles());
            multNew += criticWeights->at(tiles[i] + PHI_ORDER*Tilecoder::getNumTiles());
            multOld += criticWeights->at(oldTiles[i] + K_ORDER*Tilecoder::getNumTiles());
            multOld += criticWeights->at(oldTiles[i] + PHI_ORDER*Tilecoder::getNumTiles());
            addNew += criticWeights->at(tiles[i] + MU_ORDER*Tilecoder::getNumTiles());
            addNew += criticWeights->at(tiles[i] + SIGMA_ORDER*Tilecoder::getNumTiles());
            addOld += criticWeights->at(oldTiles[i] + MU_ORDER*Tilecoder::getNumTiles());
            addOld += criticWeights->at(oldTiles[i] + SIGMA_ORDER*Tilecoder::getNumTiles());
        }
        diffSum = GAMMA*max(multNew, addNew) - max(multOld, addOld);
    } else {
        for (int i = 0; i < (int)tiles.size(); i++) {
            for (int j = 0; j < TILE_MULTIPLE; j++) {
                diffSum += GAMMA*criticWeights->at(tiles[i] + j*Tilecoder::getNumTiles()) -
                            criticWeights->at(oldTiles[i] + j*Tilecoder::getNumTiles());
            }
        }
    }
    return diffSum;
}

void ActorCritic::step(double state, double reward, double &rate) {
    totalReward += reward;
    // Tilecode
    tiles = Tilecoder::tilecode(state);

    // Update weights
    double delta = reward - rBar + computeDiffSum();
    rBar += alphaR*delta;

    if ((flowId == REPORT_FLOW || REPORT_FLOW == -1) && !man.getSuppressOutput(AGENT_VALS)) {
        printf("\nflowId %d step %d\n", flowId, stepnum++);
        printf(" reward %f\n", reward);
        printf(" rBar %f\n", rBar);
        printf(" delta %f\n", delta);
    }

    for (int i = 0; i < (int)weightTrace.size(); i++) {
        weightTrace[i] *= GAMMA*lambda;
    }

    for (auto i: oldTiles) {
        for (int j = 0; j < TILE_MULTIPLE; j++) {
            weightTrace[i + j*Tilecoder::getNumTiles()]++;
        }
    }

    for (int i = 0; i < (int)criticWeights->size(); i++) {
        (*criticWeights)[i] += weightTrace[i]*alphaV*delta;
    }

    // Update parameters
    vector<double> gradLog = vector<double>(Tilecoder::getNumTiles() * TILE_MULTIPLE, 0);
    // Compute gradLog
    if (mode == Mult || mode == Both || (mode == Choose && actionPair.first != 0)) {
        // If in Choose mode andthe multiplicative action (actionPair.first) is 0,
        // that means an additive action was selected
        for (int i = 0; i < (int)oldTiles.size(); i++) {
            //grad log for k
            gradLog[oldTiles[i] + Tilecoder::getNumTiles()*K_ORDER] =\
                                                (k - 1)*(log(actionPair.first/phi) - boost::math::digamma(k));
            // grad log for phi
            gradLog[oldTiles[i] + Tilecoder::getNumTiles()*PHI_ORDER] = actionPair.first/phi - k;
        }
    }
    if (mode == Add || mode == Both || (mode == Choose && actionPair.first == 0)) {
        for (int i = 0; i < (int)oldTiles.size(); i++) {
            //grad log for mu
            gradLog[oldTiles[i] + Tilecoder::getNumTiles()*MU_ORDER] = (actionPair.second - mu)/(sigma*sigma);
            // grad log for sigma
            gradLog[oldTiles[i] + Tilecoder::getNumTiles()*SIGMA_ORDER] =\
                                                (actionPair.second - mu)*(actionPair.second - mu)/(sigma*sigma) - 1;
        }
    }
    // End Compute gradLog

    for (int i = 0; i < (int)parameterTrace.size(); i++) {
        parameterTrace[i] = GAMMA*lambda*parameterTrace[i] + gradLog[i];
    }

    if (inac) {
        double gradLogDot = 0;
        //Get gradlog dot gradlog
        for (int i = 0; i < (int)gradLog.size(); i++) {
            gradLogDot += gradLog[i]*gradLog[i];
        }
        //Update actor weights (w)
        for (int i = 0; i < (int)actorWeights.size(); i++) {
            actorWeights[i] += alphaV*(delta*parameterTrace[i] - gradLogDot*actorWeights[i]);
        }
        //Update parameters (u)
        for (int i = 0; i < (int)parameters.size(); i++) {
            parameters[i] += alphaU*actorWeights[i]*(s ? getVariance() : 1);
        }
    } else {
        //Update parameters (u)
        for (int i = 0; i < (int)parameters.size(); i++) {
            parameters[i] += alphaU*delta*parameterTrace[i]*(s ? getVariance() : 1);
        }
    }

    if ((flowId == REPORT_FLOW || REPORT_FLOW == -1) && !man.getSuppressOutput(AGENT_VALS)) {
        // tiles
        printf(" tiles:\n");
        for (auto it: tiles) {
            printf(" %d", it);
        }

        // gradient
        printf("\ngradLog\n");
        for (auto it: gradLog) {
            printf(" %e", it);
        }
        // actor trace
        printf("\nparameterTrace\n");
        for (auto it: parameterTrace) {
            printf(" %e", it);
        }
        // parameters
        printf("\nparameters\n");
        for (auto it: parameters) {
            printf(" %e", it);
        }
        // critic trace
        printf("\nweightTrace\n");
        for (auto it: weightTrace) {
            printf(" %e", it);
        }
        // weights
        printf("\ncriticWeights\n");
        for (auto it: *criticWeights) {
            printf(" %e", it);
        }
        printf("\n");
    }

    // set oldTiles to current tiles
    oldTiles = tiles;

    // select action
    actionPair = selectAction();

    if ((flowId == REPORT_FLOW || REPORT_FLOW == -1) && !man.getSuppressOutput(AGENT_VALS)) {
        printf("action:\n");
        printf(" k %e\n", k);
        printf(" phi %e\n", phi);
        printf(" mu %e\n", mu);
        printf(" sigma %e\n", sigma);
        printf(" action: multi: %f add: %f\n", actionPair.first, actionPair.second);// action
    }

    // TODO: should this be if statement, it would remove duplicated code in case Both, but it might be better
    //          to keep it as a switch since it is going over the values of an enum
    switch (mode) {
        case Mult:
            rate *= actionPair.first;
            break;
        case Add:
            rate += actionPair.second;
            break;
        case Both:
            rate *= actionPair.first;
            rate += actionPair.second;
            break;
        case Choose:
            if (actionPair.first == 0) {
                rate += actionPair.second;
            } else {
                rate *= actionPair.first;
            }
            break;
    }
}
