#include <cstdio>

#include "linUCB.h"

using namespace std;

LinUCB::LinUCB(int observeDims, int numActions, double regularizer, double delta, int seed) {
    this->observeDims = observeDims;
    this->numActions = numActions;
    this->regularizer = regularizer;
    this->delta = delta;

    for (int i = 0; i < numActions; i++) {
        torch::manual_seed(seed);
        this->theta.push_back(torch::rand(this->observeDims));
        this->V.push_back(torch::eye(this->observeDims) * this->regularizer);
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

int LinUCB::selectAction(vector<double> observation, vector<int> available_actions) {
    torch::Tensor obsTensor = torch::tensor(observation);

    double beta = sqrt(this->regularizer) + sqrt(2 * log(1 / this->delta) + this->numActions *
            log(1 + (this->timestep-1) / (this->regularizer * this->numActions)));

    torch::Tensor ucb = torch::zeros(this->numActions);

    for (int action : available_actions) {
        torch::Tensor inverseV = torch::inverse(V[action]);
        ucb[action] = obsTensor.dot(this->theta[action]) + beta *
            torch::sqrt(torch::matmul(obsTensor, inverseV).dot(obsTensor));
    }

    // select highest ucb value
    vector<int> actions = {available_actions[0]};
    double actionValue = ucb[actions[0]].item().to<double>();

    for (int i = 1; i < available_actions.size(); i++) {
        int action = available_actions[i];
        double tmpActionValue = ucb[action].item().to<double>();
        if (tmpActionValue > actionValue) {
            actions = {action};
            actionValue = tmpActionValue;
        } else if (tmpActionValue == actionValue) {
            actions.push_back(action);
        }
    }

    return actions[generator() % actions.size()];
}

void LinUCB::updateTheta(torch::Tensor context, int action, double reward) {
    this->V[action] = this->V[action] + torch::outer(context, context);

    this->b[action] = this->b[action] + reward * context;

    for (int i = 0; i < numActions; i++) {
        this->theta[i] = torch::matmul(torch::inverse(this->V[i]), this->b[i]);
    }
}
