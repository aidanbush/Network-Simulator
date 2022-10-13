#include <cstdio>

#include "linUCB.h"

using namespace std;

LinUCB::LinUCB(int observeDims, int numActions, double regularizer, double delta, int seed) {
    this->observeDims = observeDims;
    this->numActions = numActions;
    this->regularizer = regularizer;
    this->delta = delta;

    this->theta = torch::rand(this->observeDims * this->numActions);
    this->V = torch::eye(this->observeDims * this->numActions) * this->regularizer;
    this->b = torch::zeros(this->observeDims * this->numActions);

    this->timestep = 1;

    generator.seed(seed);
}

void LinUCB::updateAgent(vector<double> observation, int action, double reward) {
    torch::Tensor obsTensor= torch::tensor(observation);

    this->timestep++;

    torch::Tensor prevActionContext = createActionContext(obsTensor, action);
    updateTheta(prevActionContext, reward);
}

int LinUCB::selectAction(vector<double> observation) {
    torch::Tensor obsTensor= torch::tensor(observation);

    double beta = sqrt(this->regularizer) + sqrt(2 * log(1 / this->delta) + this->numActions *
            log(1 + (this->timestep-1) / (this->regularizer * this->numActions)));

    torch::Tensor ucb = torch::zeros(this->numActions);
    torch::Tensor inverseV = torch::inverse(this->V);

    for (int action = 0; action < this->numActions; action++) {
        torch::Tensor actionContext = createActionContext(obsTensor, action);
        ucb[action] = actionContext.dot(this->theta) + beta *
            torch::sqrt(torch::matmul(actionContext, inverseV).dot(actionContext));
    }

    // select highest ucb value
    // TODO ramdomly select equal actions
    vector<int> actions = {0};
    double actionValue = ucb[0].item().to<double>();
    for (int i = 1; i < this->numActions; i++) {
        double tmpActionValue = ucb[i].item().to<double>();
        if (tmpActionValue > actionValue) {
            actions = {i};
            actionValue = tmpActionValue;
        } else if (tmpActionValue == actionValue) {
            actions.push_back(i);
        }
    }

    return actions[generator() % actions.size()];
}

torch::Tensor LinUCB::createActionContext(torch::Tensor observation, int action) {
    // auto options = torch::TensorOptions().device(torch::kCPU);
    // torch::Tensor actionContext = torch::Tensor({}, 0, options);
    torch::Tensor actionContext = torch::empty(0);

    for (int i = 0; i < this->numActions; i++) {
        if (i == action) {
            actionContext = torch::cat({actionContext, observation}, 0);
        } else {
            actionContext = torch::cat({actionContext, torch::zeros(this->observeDims)}, 0);
        }
    }

    return actionContext;
}

void LinUCB::updateTheta(torch::Tensor oldActionContext, double reward) {
    this->V = this->V + torch::outer(oldActionContext, oldActionContext);

    this->b = this->b + reward * oldActionContext;

    this->theta = torch::matmul(torch::inverse(this->V), this->b);
}
