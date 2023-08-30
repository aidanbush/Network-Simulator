#include <cstdio>
#include <map>

#include "linUCB.h"

using namespace std;

LinUCB::LinUCB(int observeDims, int numActions, double regularizer, double delta, double discountFactor, string setAlgType, int seed) {
    static map<string, AlgType> algTypeMap = {
        {"original", OriginalAlg},
        {"slide", SlideAlg},
        {"D-LinUCB", D_linAlg},
    };
    if (algTypeMap.find(setAlgType) == algTypeMap.end()) {
        throw runtime_error("algorithm " + setAlgType + " type does not exist");
    }
    this->algType = algTypeMap[setAlgType];

    this->observeDims = observeDims;
    this->numActions = numActions;
    this->regularizer = regularizer;
    this->delta = delta;
    if (algType == D_linAlg) {
        this->discountFactor = discountFactor;//0.99999;
    }

    for (int i = 0; i < numActions; i++) {
        torch::manual_seed(seed);
        this->theta.push_back(torch::rand(this->observeDims));
        this->V.push_back(torch::eye(this->observeDims) * this->regularizer);
        if (algType == D_linAlg) {
            this->VAprox.push_back(torch::eye(this->observeDims) * this->regularizer); // D_LIN_UCB_UPDATE
        }
        this->b.push_back(torch::zeros(this->observeDims));
    }

    /*
    // modify V for drop action
    torch::Tensor t = torch::ones(this->numActions * this->observeDims);
    for (int i = (this->numActions-1) * this->observeDims; i < this->numActions * this->observeDims; i++ ) {
        t[i] *= 5;
    }
    this->V = this->V * t;
    */

    this->timestep = 1;

    generator.seed(seed);
}

void LinUCB::updateAgent(vector<double> observation, int action, double reward) {
    torch::Tensor obsTensor = torch::tensor(observation);
    this->timestep++;

    updateTheta(obsTensor, action, reward);
}

pair<int, double> LinUCB::selectAction(vector<double> observation, vector<int> available_actions) {
    torch::Tensor obsTensor = torch::tensor(observation);

    double beta;

    switch (algType) {
        case OriginalAlg:
            beta = 1 + sqrt(log(2 / this->delta) / 2);
            break;
        case SlideAlg:
            beta = sqrt(this->regularizer) + sqrt(2 * log(1 / this->delta) + this->numActions *
                    log(1 + (this->timestep-1) / (this->regularizer * this->numActions)));
            break;
        case D_linAlg:
            beta = sqrt(this->regularizer) +
                sqrt(2 * log(1 / this->delta) + this->numActions *
                        log((1 + (1 - pow(this->discountFactor, 2 * (this->timestep - 1)))) /
                            (this->regularizer * this->numActions * (1 - pow(this->discountFactor, 2)))));
            break;
        default:
            throw runtime_error("can't calculate beta, no valid algorithm type set");
    }

    torch::Tensor ucb = torch::zeros(this->numActions);
    torch::Tensor ucbReward = torch::zeros(this->numActions);
    torch::Tensor inverseV;

    for (int action : available_actions) {
        if (algType == D_linAlg) {
            inverseV = torch::matmul(torch::matmul(torch::inverse(V[action]), VAprox[action]), torch::inverse(V[action]));
        } else {
            inverseV = torch::inverse(V[action]);
        }
        ucbReward[action] = obsTensor.dot(this->theta[action]);
        ucb[action] = ucbReward[action] + beta *
            torch::sqrt(torch::matmul(obsTensor, inverseV).dot(obsTensor));
    }

    // select highest ucb value
    vector<int> actions = {available_actions[0]};
    double actionValue = ucb[actions[0]].item().to<double>();
    vector<double> expectedRewards = {ucbReward[actions[0]].item().to<double>()};

    for (int i = 1; i < available_actions.size(); i++) {
        int action = available_actions[i];
        double tmpActionValue = ucb[action].item().to<double>();
        if (tmpActionValue > actionValue) {
            actions = {action};
            expectedRewards = {ucbReward[action].item().to<double>()};
            actionValue = tmpActionValue;
        } else if (tmpActionValue == actionValue) {
            expectedRewards.push_back(ucbReward[action].item().to<double>());
            actions.push_back(action);
        }
    }

    int selectedAction = generator() % actions.size();
    return pair<int, double>{actions[selectedAction], expectedRewards[selectedAction]};
}

void LinUCB::updateTheta(torch::Tensor context, int action, double reward) {
    if (algType == D_linAlg) {
        this->V[action] = this->discountFactor * this->V[action] + torch::outer(context, context) +
            (1 - this->discountFactor) * this->regularizer * torch::eye(this->observeDims);
        this->VAprox[action] = pow(this->discountFactor, 2) * this->V[action] + torch::outer(context, context) +
            (1 - pow(this->discountFactor, 2)) * this->regularizer * torch::eye(this->observeDims);

        this->b[action] = this->discountFactor * this->b[action] + reward * context;
    } else {
        this->V[action] = this->V[action] + torch::outer(context, context);

        this->b[action] = this->b[action] + reward * context;
    }

    if (algType == SlideAlg || algType == D_linAlg) {
        for (int i = 0; i < numActions; i++) {
            this->theta[i] = torch::matmul(torch::inverse(this->V[i]), this->b[i]);
        }
    } else {
        this->theta[action] = torch::matmul(torch::inverse(this->V[action]), this->b[action]);
    }

}
