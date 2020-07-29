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
#define GAMMA 1 //Always 1 for continuing case
#define DEFAULT_INITIAL_WEIGHTS 0.1
#define DEFAULT_INITIAL_K_PARAMETERS 1
#define DEFAULT_INITIAL_PHI_PARAMETERS -1
#define DEFAULT_INITIAL_MU_PARAMETERS 0
#define DEFAULT_INITIAL_SIGMA_PARAMETERS 0
#define NUM_PARAMS 6
#define CHOOSE_EPSILON 0.1
#define MODE Mult //Add, Mult, Both, or Choose
#define MULT_MODE Gamma
#define GAUSSIAN_TANH_SCALE 1
#define REPORT_FLOW 1 // -1 for all flows
#define SPLIT_DISTRIBUTION 1
#define DEC_THRESHOLD 0.1
#endif // __has_include

#if SPLIT_DISTRIBUTION > 0
#define TILE_MULTIPLE 8
#else
#define TILE_MULTIPLE 4
#endif
//TODO: better names for these constants
#define K_MUL_ORDER 0
#define PHI_MUL_ORDER 1
#define MU_ADD_ORDER 2
#define SIGMA_ADD_ORDER 3
#define K_DIV_ORDER 4
#define PHI_DIV_ORDER 5
#define MU_SUB_ORDER 6
#define SIGMA_SUB_ORDER 7
#define CHOOSE_MUL_ORDER 0
#define CHOOSE_ADD_ORDER 1

vector<double> ActorCritic::initializeWeights() {
    double initialWeights;
    if (!man.getInitialWeights(&initialWeights)) {
        initialWeights = DEFAULT_INITIAL_WEIGHTS;
    }

    return vector<double>(Tilecoder::getNumTiles(), initialWeights);
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
            case K_MUL_ORDER:
                parameters[i] = DEFAULT_INITIAL_K_PARAMETERS / Tilecoder::getNumTilings();
                break;
            case PHI_MUL_ORDER:
                parameters[i] = DEFAULT_INITIAL_PHI_PARAMETERS / Tilecoder::getNumTilings();
                break;
            case MU_ADD_ORDER:
                parameters[i] = DEFAULT_INITIAL_MU_PARAMETERS / Tilecoder::getNumTilings();
                break;
            case SIGMA_ADD_ORDER:
                parameters[i] = DEFAULT_INITIAL_SIGMA_PARAMETERS / Tilecoder::getNumTilings();
                break;
            case K_DIV_ORDER:
                parameters[i] = DEFAULT_INITIAL_K_PARAMETERS / Tilecoder::getNumTilings();
                break;
            case PHI_DIV_ORDER:
                parameters[i] = DEFAULT_INITIAL_PHI_PARAMETERS / Tilecoder::getNumTilings();
                break;
            case MU_SUB_ORDER:
                parameters[i] = DEFAULT_INITIAL_MU_PARAMETERS / Tilecoder::getNumTilings();
                break;
            case SIGMA_SUB_ORDER:
                parameters[i] = DEFAULT_INITIAL_SIGMA_PARAMETERS / Tilecoder::getNumTilings();
                break;
        }
    }

    criticWeights = weights;

    advantageParameters = vector<double>(Tilecoder::getNumTiles() * TILE_MULTIPLE, 0);

    weightTrace = vector<double>(Tilecoder::getNumTiles(), 0);
    parameterTrace = vector<double>(Tilecoder::getNumTiles() * TILE_MULTIPLE, 0);

    oldState = initialState;
    oldTiles = Tilecoder::tilecode(initialState);

    tiles = oldTiles; // TODO does this cause a bug, should we copy over?

    mode = MODE;
    multMode = MULT_MODE;

    generator = mt19937();
    if (mode == Choose) {
        //In choose mode, we need a second copy of the critic weights (one for each action type)
        criticWeights->insert(criticWeights->end(), weights->begin(), weights->end());
    }
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
    double action;

    switch (multMode) {
        case GaussianTanh:
            {
                if (SPLIT_DISTRIBUTION && decreaseRate) {
                    k = sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*K_DIV_ORDER);
                    phi = exp(sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*PHI_DIV_ORDER));
                } else {
                    k = sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*K_MUL_ORDER);
                    phi = exp(sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*PHI_MUL_ORDER));
                }

                normal_distribution distribution(k, phi);
                action = exp(GAUSSIAN_TANH_SCALE*tanh(distribution(generator)));
            }
            break;
        case Gamma:
            {
                if (SPLIT_DISTRIBUTION && decreaseRate) {
                    k = exp(sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*K_DIV_ORDER)) + 1;
                    phi = exp(sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*PHI_DIV_ORDER));
                } else {
                    k = exp(sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*K_MUL_ORDER)) + 1;
                    phi = exp(sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*PHI_MUL_ORDER));
                }

                gamma_distribution distribution(k, phi);
                 //TODO: Consider clipping the value to a pre defined range
                action = distribution(generator);
            }
            break;
    }

    if (SPLIT_DISTRIBUTION) {
        action++; // Change range from (0, inf) to (1, inf) so it is always increasing
        if (decreaseRate) {
            action = 1/action; // If a decrease is needed, invert it
        }
    }
    return action;
}

double ActorCritic::selectActionAdd() {
    if (SPLIT_DISTRIBUTION && decreaseRate) {
        mu = sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*MU_SUB_ORDER);
        sigma = exp(sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*SIGMA_SUB_ORDER));
    } else {
        mu = sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*MU_ADD_ORDER);
        sigma = exp(sumIndices(&parameters, &tiles, Tilecoder::getNumTiles()*SIGMA_ADD_ORDER));
    }
    normal_distribution distribution(mu, sigma);
    //TODO: Consider clipping the value to a pre defined range
    double action = distribution(generator);
    if (SPLIT_DISTRIBUTION) {
        if (decreaseRate) {
            action = min(action, 0.0);
        } else {
            action = max(action, 0.0);
        }
    }
    return action;
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
                    multValue += criticWeights->at(tiles[i] + CHOOSE_MUL_ORDER*Tilecoder::getNumTiles());
                    addValue += criticWeights->at(tiles[i] + CHOOSE_ADD_ORDER*Tilecoder::getNumTiles());
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
            double variance;
            switch (multMode) {
                case GaussianTanh:
                    variance = phi*phi;
                    break;
                case Gamma:
                    variance = k*phi*phi;
                    break;
            }
            return variance;
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
            multNew += criticWeights->at(tiles[i] + CHOOSE_MUL_ORDER*Tilecoder::getNumTiles());
            multOld += criticWeights->at(oldTiles[i] + CHOOSE_MUL_ORDER*Tilecoder::getNumTiles());
            addNew += criticWeights->at(tiles[i] + CHOOSE_ADD_ORDER*Tilecoder::getNumTiles());
            addOld += criticWeights->at(oldTiles[i] + CHOOSE_ADD_ORDER*Tilecoder::getNumTiles());
        }
        diffSum = GAMMA*max(multNew, addNew) - max(multOld, addOld);
    } else {
        for (int i = 0; i < (int)tiles.size(); i++) {
            diffSum += GAMMA*criticWeights->at(tiles[i]) - criticWeights->at(oldTiles[i]);
        }
    }
    return diffSum;
}

void ActorCritic::updateWeightTrace() {
    for (int i = 0; i < (int)weightTrace.size(); i++) {
        weightTrace[i] *= GAMMA * lambda;
    }

    for (auto xi: oldTiles) {
        weightTrace[xi]++;
    }
}

void ActorCritic::updateCriticWeights(double delta) {
    for (int i = 0; i < (int)criticWeights->size(); i++) {
        (*criticWeights)[i] += weightTrace[i] * alphaV * delta;
    }
}

void ActorCritic::computeMultActionGradient(vector<double> &gradLog) {
    switch (multMode) {
        case GaussianTanh:
            for (int i = 0; i < (int)oldTiles.size(); i++) {
                if (SPLIT_DISTRIBUTION && decreaseRate) {
                    //grad log for k (mu)
                    gradLog[oldTiles[i] + Tilecoder::getNumTiles()*K_DIV_ORDER] = (actionPair.first - k)/(phi*phi);
                    // grad log for phi (sigma)
                    gradLog[oldTiles[i] + Tilecoder::getNumTiles()*PHI_DIV_ORDER] =\
                                                        (actionPair.first - k)*(actionPair.first - k)/(phi*phi) - 1;
                } else {
                    //grad log for k (mu)
                    gradLog[oldTiles[i] + Tilecoder::getNumTiles()*K_MUL_ORDER] = (actionPair.first - k)/(phi*phi);
                    // grad log for phi (sigma)
                    gradLog[oldTiles[i] + Tilecoder::getNumTiles()*PHI_MUL_ORDER] =\
                                                        (actionPair.first - k)*(actionPair.first - k)/(phi*phi) - 1;
                }
            }
            break;
        case Gamma:
            for (int i = 0; i < (int)oldTiles.size(); i++) {
                if (SPLIT_DISTRIBUTION && decreaseRate) {
                    //grad log for k
                    gradLog[oldTiles[i] + Tilecoder::getNumTiles()*K_DIV_ORDER] =\
                                                        (k - 1)*(log(actionPair.first/phi) - boost::math::digamma(k));
                    // grad log for phi
                    gradLog[oldTiles[i] + Tilecoder::getNumTiles()*PHI_DIV_ORDER] = actionPair.first/phi - k;
                } else {
                    //grad log for k
                    gradLog[oldTiles[i] + Tilecoder::getNumTiles()*K_MUL_ORDER] =\
                                                        (k - 1)*(log(actionPair.first/phi) - boost::math::digamma(k));
                    // grad log for phi
                    gradLog[oldTiles[i] + Tilecoder::getNumTiles()*PHI_MUL_ORDER] = actionPair.first/phi - k;
                }
            }
            break;
    }
}

void ActorCritic::computeAddActionGradient(vector<double> &gradLog) {
    for (int i = 0; i < (int)oldTiles.size(); i++) {
        if (SPLIT_DISTRIBUTION && decreaseRate) {
            //grad log for mu
            gradLog[oldTiles[i] + Tilecoder::getNumTiles()*MU_SUB_ORDER] = (actionPair.second - mu)/(sigma*sigma);
            // grad log for sigma
            gradLog[oldTiles[i] + Tilecoder::getNumTiles()*SIGMA_SUB_ORDER] =\
                                                (actionPair.second - mu)*(actionPair.second - mu)/(sigma*sigma) - 1;
        } else {
            //grad log for mu
            gradLog[oldTiles[i] + Tilecoder::getNumTiles()*MU_ADD_ORDER] = (actionPair.second - mu)/(sigma*sigma);
            // grad log for sigma
            gradLog[oldTiles[i] + Tilecoder::getNumTiles()*SIGMA_ADD_ORDER] =\
                                                (actionPair.second - mu)*(actionPair.second - mu)/(sigma*sigma) - 1;
        }
    }
}

vector<double> ActorCritic::computeGradient() {
    vector<double> gradLog = vector<double>(Tilecoder::getNumTiles() * TILE_MULTIPLE, 0);

    if (mode == Mult || mode == Both || (mode == Choose && actionPair.first != 0)) {
        // If in Choose mode andthe multiplicative action (actionPair.first) is 0,
        // that means an additive action was selected
        computeMultActionGradient(gradLog);
    }

    if (mode == Add || mode == Both || (mode == Choose && actionPair.first == 0)) {
        computeAddActionGradient(gradLog);
    }

    return gradLog;
}

void ActorCritic::updateParameterTrace(vector<double> gradLog) {
    for (int i = 0; i < (int)parameterTrace.size(); i++) {
        parameterTrace[i] = GAMMA*lambda*parameterTrace[i] + gradLog[i];
    }
}

void ActorCritic::updateAdvantageParameters(vector<double> gradLog, double delta) {
    double gradLogDot = 0;
    //Get gradlog dot gradlog
    for (int i = 0; i < (int)gradLog.size(); i++) {
        gradLogDot += gradLog[i]*gradLog[i];
    }

    //Update advantage parameters (w)
    for (int i = 0; i < (int)advantageParameters.size(); i++) {
        advantageParameters[i] += alphaV*(delta*parameterTrace[i] - gradLogDot*advantageParameters[i]);
    }
}

void ActorCritic::updateParametersINAC() {
    for (int i = 0; i < (int)advantageParameters.size(); i++) {
        parameters[i] += alphaU*advantageParameters[i] * (s ? getVariance() : 1);
    }
}

void ActorCritic::updateParameters(double delta) {
    for (int i = 0; i < (int)parameters.size(); i++) {
        parameters[i] += alphaU*delta*parameterTrace[i] * (s ? getVariance() : 1);
    }
}

pair<double, double> ActorCritic::step(double state, double reward, double &rate) {
    totalReward += reward;
    decreaseRate = (state > DEC_THRESHOLD);
    // Tilecode
    tiles = Tilecoder::tilecode(state);

    double delta = reward - rBar + computeDiffSum();

    rBar += alphaR*delta;

    if ((flowId == REPORT_FLOW || REPORT_FLOW == -1) && !man.getSuppressOutput(AGENT_VALS)) {
        printf("\nflowId %d step %d\n", flowId, stepnum++);
        printf(" reward %f\n", reward);
        printf(" rBar %f\n", rBar);
        printf(" delta %f\n", delta);
    }

    // update update critic trace using oldTiles
    updateWeightTrace();

    // update citic weights using trace
    updateCriticWeights(delta);

    // compute gradients using oldTiles and actionPair
    vector<double> gradLog = computeGradient();

    updateParameterTrace(gradLog);

    if (inac) {
        // update advantage parameters (w)
        updateAdvantageParameters(gradLog, delta);

        // Update parameters (u) using advantageParameters
        updateParametersINAC();
    } else {
        //Update parameters (u) using parametersTrace
        updateParameters(delta);
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

    return actionPair;
}

pair<double, double> ActorCritic::getMultMeanStdev() {
    pair<double, double> meanStdev;

    switch (multMode) {
        case GaussianTanh:
            meanStdev = pair<double, double>(k, phi);
            break;
        case Gamma:
            meanStdev = pair<double, double>(k * phi, sqrt(k * pow(phi, 2)));
            break;
    }

    return meanStdev;
}

pair<double, double> ActorCritic::getAddMeanStdev() {
    return pair<double, double>(mu, sigma);
}
