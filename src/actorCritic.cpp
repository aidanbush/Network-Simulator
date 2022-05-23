#include <vector>
#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <random>
#include <stdexcept>
#include <boost/math/special_functions/digamma.hpp>
#include <nlohmann/json.hpp>

#include "agent.h"
#include "actorCritic.h"
#include "tilecoder.h"
#include "manager.h"
#include "config.h"

using namespace std;

using json = nlohmann::json;

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
#define NUM_PARAMS 7
#define CHOOSE_EPSILON 0.1
#define MODE Mult //Add, Mult, Both, or Choose
#define MULT_MODE Gamma
#define ADD_MODE Gaussian
#define GAUSSIAN_TANH_SCALE 1
#define BETA_MULT_SCALE 10 //TODO: can this and GAUSSIAN_TANH_SCALE be combined into one?
#define BETA_ADD_SCALE 100
#define REPORT_FLOW 1 // -1 for all flows

#define TILECODE_NUM_DIMS 1
#define TILECODE_DIM_RANGES {{0,1}}
#define TILECODE_PATTERNS {{0}}
#define TILECODE_TILES_PER_DIM_TILING {11}
#define TILECODE_NUM_TILINGS {1}

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

ActorCritic::ActorCritic(vector<double> initialState, int flowId, double initialRBar, json agentConfig) {
    this->flowId = flowId;
    this->rBar = initialRBar;

    tilecoder = new Tilecoder(TILECODE_NUM_DIMS, TILECODE_DIM_RANGES, TILECODE_PATTERNS,
            TILECODE_TILES_PER_DIM_TILING, TILECODE_NUM_TILINGS);

    stepnum = 0;

    double initialWeights;

    initialAlphaU = DEFAULT_ALPHA_U;
    initialAlphaV = DEFAULT_ALPHA_V;
    alphaR = DEFAULT_ALPHA_R;
    tau = DEFAULT_TAU;
    inac = DEFAULT_INAC;
    s = DEFAULT_S;
    initialWeights = DEFAULT_INITIAL_WEIGHTS;

    if (hasMemberOfType(agentConfig, "alpha_u", jsonDouble)) {
        initialAlphaU = agentConfig["alpha_u"];
    }

    if (hasMemberOfType(agentConfig, "alpha_v", jsonDouble)) {
        initialAlphaV = agentConfig["alpha_v"];
    }

    if (hasMemberOfType(agentConfig, "alpha_r", jsonDouble)) {
        alphaR = agentConfig["alpha_r"];
    }

    if (hasMemberOfType(agentConfig, "tau", jsonDouble)) {
        tau = agentConfig["tau"];
    }

    if (hasMemberOfType(agentConfig, "inac", jsonInt)) {
        inac = int(agentConfig["inac"]);
    }

    if (hasMemberOfType(agentConfig, "s", jsonInt)) {
        s = int(agentConfig["s"]);
    }

    if (hasMemberOfType(agentConfig, "initial_weights", jsonDouble)) {
        initialWeights = agentConfig["initial_weights"];
    }

    alphaU = (double)initialAlphaU / tilecoder->getNumTotalTilings();
    alphaV = (double)initialAlphaV / tilecoder->getNumTotalTilings();
    lambda = 1 - 1.0/tau;

    parameters = vector<double>(tilecoder->getNumTiles() * TILE_MULTIPLE, 0);
    for (int i = 0; i < (int)parameters.size(); i++) {
        switch (i / tilecoder->getNumTiles()) {
            case K_MUL_ORDER:
                parameters[i] = DEFAULT_INITIAL_K_PARAMETERS / tilecoder->getNumTotalTilings();
                break;
            case PHI_MUL_ORDER:
                parameters[i] = DEFAULT_INITIAL_PHI_PARAMETERS / tilecoder->getNumTotalTilings();
                break;
            case MU_ADD_ORDER:
                parameters[i] = DEFAULT_INITIAL_MU_PARAMETERS / tilecoder->getNumTotalTilings();
                break;
            case SIGMA_ADD_ORDER:
                parameters[i] = DEFAULT_INITIAL_SIGMA_PARAMETERS / tilecoder->getNumTotalTilings();
                break;
            case K_DIV_ORDER:
                parameters[i] = DEFAULT_INITIAL_K_PARAMETERS / tilecoder->getNumTotalTilings();
                break;
            case PHI_DIV_ORDER:
                parameters[i] = DEFAULT_INITIAL_PHI_PARAMETERS / tilecoder->getNumTotalTilings();
                break;
            case MU_SUB_ORDER:
                parameters[i] = DEFAULT_INITIAL_MU_PARAMETERS / tilecoder->getNumTotalTilings();
                break;
            case SIGMA_SUB_ORDER:
                parameters[i] = DEFAULT_INITIAL_SIGMA_PARAMETERS / tilecoder->getNumTotalTilings();
                break;
        }
    }

    if (mode == Choose) {
        this->criticWeights = vector<double>(tilecoder->getNumTiles() * TILE_MULTIPLE * 2, initialWeights);
    } else {
        this->criticWeights = vector<double>(tilecoder->getNumTiles() * TILE_MULTIPLE, initialWeights);
    }

    advantageParameters = vector<double>(tilecoder->getNumTiles() * TILE_MULTIPLE, 0);

    weightTrace = vector<double>(tilecoder->getNumTiles(), 0);
    parameterTrace = vector<double>(tilecoder->getNumTiles() * TILE_MULTIPLE, 0);

    oldState = initialState;
    oldTiles = tilecoder->tilecode(initialState);

    tiles = oldTiles; // TODO does this cause a bug, should we copy over?

    mode = MODE;
    multMode = MULT_MODE;
    addMode = ADD_MODE;

    generator = mt19937();
    generator.seed(man.random());

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

vector<double> ActorCritic::getWeights() {
    return criticWeights;
}

double ActorCritic::selectActionMult() {
    double action;

    switch (multMode) {
        case GaussianTanh:
            {
                if (SPLIT_DISTRIBUTION && splitDistributionDecrease) {
                    k = sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*K_DIV_ORDER);
                    phi = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*PHI_DIV_ORDER));
                } else {
                    k = sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*K_MUL_ORDER);
                    phi = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*PHI_MUL_ORDER));
                }

                normal_distribution distribution(k, phi);
                action = exp(GAUSSIAN_TANH_SCALE*tanh(distribution(generator)));
            }
            break;
        case Gamma:
            {
                if (SPLIT_DISTRIBUTION && splitDistributionDecrease) {
                    k = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*K_DIV_ORDER)) + 1;
                    phi = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*PHI_DIV_ORDER));
                } else {
                    k = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*K_MUL_ORDER)) + 1;
                    phi = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*PHI_MUL_ORDER));
                }

                gamma_distribution distribution(k, phi);
                 //TODO: Consider clipping the value to a pre defined range
                action = distribution(generator);
            }
            break;
        case BetaMult:
            {
                if (SPLIT_DISTRIBUTION && splitDistributionDecrease) {
                    k = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*K_DIV_ORDER)) + 1;
                    phi = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*PHI_DIV_ORDER)) + 1;
                } else {
                    k = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*K_MUL_ORDER)) + 1;
                    phi = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*PHI_MUL_ORDER)) + 1;
                }

                gamma_distribution distribution1(k, 1.0);
                gamma_distribution distribution2(phi, 1.0);
                double sample1 = distribution1(generator);
                double sample2 = distribution2(generator);
                // https://en.wikipedia.org/wiki/Beta_distribution#Generating_beta-distributed_random_variates
                action = sample1/(sample1 + sample2);
            }
            break;
    }

    return action;
}

double ActorCritic::selectActionAdd() {
    double action;

    switch (addMode) {
        case Gaussian:
            {
                if (SPLIT_DISTRIBUTION && splitDistributionDecrease) {
                    mu = sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*MU_SUB_ORDER);
                    sigma = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*SIGMA_SUB_ORDER));
                } else {
                    mu = sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*MU_ADD_ORDER);
                    sigma = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*SIGMA_ADD_ORDER));
                }
                normal_distribution distribution(mu, sigma);
                //TODO: Consider clipping the value to a pre defined range
                action = distribution(generator);
            }
            break;
        case BetaAdd:
            {
                if (SPLIT_DISTRIBUTION && splitDistributionDecrease) {
                    mu = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*MU_SUB_ORDER)) + 1;
                    sigma = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*SIGMA_SUB_ORDER)) + 1;
                } else {
                    mu = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*MU_ADD_ORDER)) + 1;
                    sigma = exp(sumIndices(&parameters, &tiles, tilecoder->getNumTiles()*SIGMA_ADD_ORDER)) + 1;
                }

                gamma_distribution distribution1(mu, 1.0);
                gamma_distribution distribution2(sigma, 1.0);
                double sample1 = distribution1(generator);
                double sample2 = distribution2(generator);
                // https://en.wikipedia.org/wiki/Beta_distribution#Generating_beta-distributed_random_variates
                action = sample1/(sample1 + sample2);
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
                    multValue += criticWeights.at(tiles[i] + CHOOSE_MUL_ORDER*tilecoder->getNumTiles());
                    addValue += criticWeights.at(tiles[i] + CHOOSE_ADD_ORDER*tilecoder->getNumTiles());
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

double ActorCritic::getMultVariance() {
    double variance;
    switch (multMode) {
        case GaussianTanh:
            variance = phi*phi;
            break;
        case Gamma:
            variance = k*phi*phi;
            break;
        case BetaMult:
            variance = k*phi/((k + phi)*(k + phi)*(k + phi + 1));
            break;
    }
    return variance;
}

double ActorCritic::getAddVariance() {
    double variance;
    switch (addMode) {
        case Gaussian:
            variance = sigma*sigma;
            break;
        case BetaAdd:
            variance = mu*sigma/((mu + sigma)*(mu + sigma)*(mu + sigma + 1));
            break;
    }
    return variance;
}

double ActorCritic::getVariance() {
    switch (mode) {
        case Mult:
            return getMultVariance();
        case Add:
            return getAddVariance();
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
            multNew += criticWeights.at(tiles[i] + CHOOSE_MUL_ORDER*tilecoder->getNumTiles());
            multOld += criticWeights.at(oldTiles[i] + CHOOSE_MUL_ORDER*tilecoder->getNumTiles());
            addNew += criticWeights.at(tiles[i] + CHOOSE_ADD_ORDER*tilecoder->getNumTiles());
            addOld += criticWeights.at(oldTiles[i] + CHOOSE_ADD_ORDER*tilecoder->getNumTiles());
        }
        diffSum = GAMMA*max(multNew, addNew) - max(multOld, addOld);
    } else {
        for (int i = 0; i < (int)tiles.size(); i++) {
            diffSum += GAMMA*criticWeights.at(tiles[i]) - criticWeights.at(oldTiles[i]);
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
    for (int i = 0; i < (int)criticWeights.size(); i++) {
        criticWeights[i] += weightTrace[i] * alphaV * delta;
    }
}

void ActorCritic::computeMultActionGradient(vector<double> &gradLog) {
    switch (multMode) {
        case GaussianTanh:
            for (int i = 0; i < (int)oldTiles.size(); i++) {
                if (SPLIT_DISTRIBUTION && splitDistributionDecrease) {
                    //grad log for k (mu)
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*K_DIV_ORDER] = (actionPair.first - k)/(phi*phi);
                    // grad log for phi (sigma)
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*PHI_DIV_ORDER] =\
                                                        (actionPair.first - k)*(actionPair.first - k)/(phi*phi) - 1;
                } else {
                    //grad log for k (mu)
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*K_MUL_ORDER] = (actionPair.first - k)/(phi*phi);
                    // grad log for phi (sigma)
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*PHI_MUL_ORDER] =\
                                                        (actionPair.first - k)*(actionPair.first - k)/(phi*phi) - 1;
                }
            }
            break;
        case Gamma:
            for (int i = 0; i < (int)oldTiles.size(); i++) {
                if (SPLIT_DISTRIBUTION && splitDistributionDecrease) {
                    //grad log for k
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*K_DIV_ORDER] =\
                                                        (k - 1)*(log(actionPair.first/phi) - boost::math::digamma(k));
                    // grad log for phi
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*PHI_DIV_ORDER] = actionPair.first/phi - k;
                } else {
                    //grad log for k
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*K_MUL_ORDER] =\
                                                        (k - 1)*(log(actionPair.first/phi) - boost::math::digamma(k));
                    // grad log for phi
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*PHI_MUL_ORDER] = actionPair.first/phi - k;
                }
            }
            break;
        case BetaMult:
            for (int i = 0; i < (int)oldTiles.size(); i++) {
                if (SPLIT_DISTRIBUTION && splitDistributionDecrease) {
                    //grad log for k
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*K_DIV_ORDER] =\
                            (k - 1)*(log(actionPair.first) + boost::math::digamma(k + phi) - boost::math::digamma(k));
                    // grad log for phi
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*PHI_DIV_ORDER] =\
                                            (phi - 1)*(log(1 - actionPair.first) + boost::math::digamma(k + phi) -\
                                            boost::math::digamma(phi));
                } else {
                    //grad log for k
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*K_MUL_ORDER] =\
                            (k - 1)*(log(actionPair.first) + boost::math::digamma(k + phi) - boost::math::digamma(k));
                    // grad log for phi
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*PHI_MUL_ORDER] =\
                                            (phi - 1)*(log(1 - actionPair.first) + boost::math::digamma(k + phi) -\
                                            boost::math::digamma(phi));
                }
            }
            break;
    }
}

void ActorCritic::computeAddActionGradient(vector<double> &gradLog) {
    switch (addMode) {
        case Gaussian:
            for (int i = 0; i < (int)oldTiles.size(); i++) {
                if (SPLIT_DISTRIBUTION && splitDistributionDecrease) {
                    //grad log for mu
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*MU_SUB_ORDER] =\
                                            (actionPair.second - mu)/(sigma*sigma);
                    // grad log for sigma
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*SIGMA_SUB_ORDER] =\
                                            (actionPair.second - mu)*(actionPair.second - mu)/(sigma*sigma) - 1;
                } else {
                    //grad log for mu
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*MU_ADD_ORDER] =\
                                            (actionPair.second - mu)/(sigma*sigma);
                    // grad log for sigma
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*SIGMA_ADD_ORDER] =\
                                            (actionPair.second - mu)*(actionPair.second - mu)/(sigma*sigma) - 1;
                }
            }
            break;
        case BetaAdd:
            for (int i = 0; i < (int)oldTiles.size(); i++) {
                if (SPLIT_DISTRIBUTION && splitDistributionDecrease) {
                    //grad log for mu
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*MU_SUB_ORDER] =\
                        (mu - 1)*(log(actionPair.second) + boost::math::digamma(mu + sigma) -\
                        boost::math::digamma(mu));
                    // grad log for sigma
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*SIGMA_SUB_ORDER] =\
                        (sigma - 1)*(log(1 - actionPair.second) + boost::math::digamma(mu + sigma) -\
                        boost::math::digamma(sigma));
                } else {
                    //grad log for mu
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*MU_ADD_ORDER] =\
                        (mu - 1)*(log(actionPair.second) + boost::math::digamma(mu + sigma) -\
                        boost::math::digamma(mu));
                    // grad log for sigma
                    gradLog[oldTiles[i] + tilecoder->getNumTiles()*SIGMA_ADD_ORDER] =\
                        (sigma - 1)*(log(1 - actionPair.second) + boost::math::digamma(mu + sigma) -\
                        boost::math::digamma(sigma));
                }
            }
            break;
    }
}

vector<double> ActorCritic::computeGradient() {
    vector<double> gradLog = vector<double>(tilecoder->getNumTiles() * TILE_MULTIPLE, 0);

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

pair<double, double> ActorCritic::step(vector<double> state, double reward) {
    totalReward += reward;
    splitDistributionDecrease = (state[0] > DEC_THRESHOLD);
    // Tilecode
    tiles = tilecoder->tilecode(state);

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
        for (auto it: criticWeights) {
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

    //Transform actions from values chosen in distributions to values used to control flow
    double multAction = actionPair.first;
    if (multMode == BetaMult) {
        multAction *= BETA_MULT_SCALE;
        //TODO: BETA_MULT_SCALE sets the upper bound, do we want to shift so the lower bound is above zero?
    }
    if (SPLIT_DISTRIBUTION) {
        multAction++; // Change range from (0, inf) to (1, inf) so it is always increasing
        if (splitDistributionDecrease) {
            multAction = 1/multAction; // If a decrease is needed, invert it
        }
    }
    double addAction = actionPair.second;
    if (SPLIT_DISTRIBUTION) {
        if (splitDistributionDecrease) {
            if (addMode == BetaAdd) {
                addAction *= -BETA_ADD_SCALE;
            } else {
                addAction = min(addAction, 0.0);
            }
        } else {
            if (addMode == BetaAdd) {
                addAction *= BETA_ADD_SCALE;
            } else {
                addAction = max(addAction, 0.0);
            }
        }
    } else {
        if (addMode == BetaAdd) {
            addAction -= 0.5; // Range is now (-0.5, 0.5)
            addAction *= 2*BETA_ADD_SCALE; //Range is now (-BETA_ADD_SCALE, BETA_ADD_SCALE)
        }
    }
    // TODO: should this be if statement, it would remove duplicated code in case Both, but it might be better
    //          to keep it as a switch since it is going over the values of an enum
    switch (mode) {
        case Mult:
            actionPair.second = 0;
            break;
        case Add:
            actionPair.first = 1;
            break;
        case Both:
            break;
        case Choose:
            if (actionPair.first == 0) {
                // if 0 then not taking a multiplicative action
                actionPair.first = 1;
            } else {
                // else not taking addative action
                actionPair.second = 0;
            }
            break;
    }

    return actionPair;
}

pair<double, double> ActorCritic::getMultMeanStdev() {
    double mean;
    double stdev = sqrt(getMultVariance());
    switch (multMode) {
        case GaussianTanh:
            mean = k;
            break;
        case Gamma:
            mean = k * phi;
            break;
        case BetaMult:
            mean = k/(k + phi);
            break;
    }

    return pair<double, double>(mean, stdev);;
}

pair<double, double> ActorCritic::getAddMeanStdev() {
    double mean;
    double stdev = sqrt(getAddVariance());
    switch (addMode) {
        case Gaussian:
            mean = mu;
            break;
        case BetaAdd:
            mean = mu/(mu + sigma);
            break;
    }
    return pair<double, double>(mean, stdev);
}
