#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <random>
#include <map>

#include "NDDAgents.h"
#include "manager.h"
#include "config.h"

using namespace std;

using json = nlohmann::json;

enum NDDAgentType {
    RandType,
    QLearningType,
};

NDDAgent *createNDDAgent(json &NDDAgentConfig) {
    static map<string, NDDAgentType> agentTypeMap = {
        {"rand",RandType},
        {"Q-learning", QLearningType},
    };

    if (!hasMemberOfType(NDDAgentConfig, "NDD_type", jsonString)) {
        throw runtime_error("NDDAgent:\nNo string with name 'NDD_type'\n" + NDDAgentConfig.dump(4));
    }

    string NDDAgentTypeString = NDDAgentConfig["NDD_type"];
    NDDAgentType agentType;
    try {
        agentType = agentTypeMap.at(NDDAgentTypeString);
    } catch (out_of_range&) {
        throw runtime_error("NDDAgent:\nInvalid agenttype: " + NDDAgentTypeString);
    }

    NDDAgent *agent;

    switch (agentType) {
        case RandType:
            agent = new RandNDDAgent(NDDAgentConfig);
            break;
        case QLearningType:
            agent = new TabQLearning(NDDAgentConfig);
            break;
        default:
            throw runtime_error("Generator:\nInvalid generator type: " + NDDAgentTypeString);
    }

    return agent;
}

NDDAgent::NDDAgent(json &NDDAgentConfig) {
    generator.seed(man.random());
}

void NDDAgent::init(vector<int> stateRanges, int maxNumActions) {
}

int NDDAgent::selectAction(vector<int> state, vector<int> availableActions, bool explore) {
}

void NDDAgent::update(vector<int> state, int action, double reward, vector<int> actionSet) {
}

/* Rand algorithm - used for testing */

RandNDDAgent::RandNDDAgent(json &NDDAgentConfig): NDDAgent(NDDAgentConfig) {
}

void RandNDDAgent::init(vector<int> stateRanges, int maxNumActions) {
}

int RandNDDAgent::selectAction(vector<int> obs, vector<int> availableActions, bool explore) {
    return availableActions[generator() % availableActions.size()];
}

void RandNDDAgent::update(vector<int> state, int action, double reward, vector<int> actionSet) {
    return;
}

/* Q-Learning algorithm */

TabQLearning::TabQLearning(json &NDDAgentConfig):
    NDDAgent(validateTabQLearningConfig(NDDAgentConfig)) {
    //double alpha, double epsilon, double gamma, int seed
    this->alpha = NDDAgentConfig["alpha"];
    // TODO is epsilon 1 / what is the value?
    this->epsilon = NDDAgentConfig["epsilon"];
    this->gamma = NDDAgentConfig["gamma"];
}

json &TabQLearning::validateTabQLearningConfig(json &NDDAgentConfig) {
    string message = "";
    if (!hasMemberOfType(NDDAgentConfig, "alpha", jsonDouble)) {
        message += "No double with name 'alpha'.\n";
    }

    if (!hasMemberOfType(NDDAgentConfig, "epsilon", jsonDouble)) {
        message += "No double with name 'epsilon'.\n";
    }

    if (!hasMemberOfType(NDDAgentConfig, "gamma", jsonDouble)) {
        message += "No double with name 'gamma'.\n";
    }

    if (!message.empty()) {
        message = "TabQLearning:\n" + message + NDDAgentConfig.dump(4);
        throw runtime_error(message);
    }
    return NDDAgentConfig;
}

void TabQLearning::init(vector<int> stateRanges, int maxNumActions) {
    this->stateRanges = stateRanges;

    int numStates = 1;//product of stateRanges
    for (int r : stateRanges) {
        numStates *= (r); // +1 to account for 0
    }

    this->Q.resize(numStates);

    for (int i = 0; i < numStates; i++) {
        this->Q[i].resize(maxNumActions, 0);
    }
}

int TabQLearning::obsToState(vector<int> obs) {
    int state = obs[0];
    int offset = this->stateRanges[0];

    for (int i = 1; i < obs.size(); i++) {
        state += obs[i] * offset;
        offset *= stateRanges[i];
    }

    return state;
}

int TabQLearning::selectAction(vector<int> obs, vector<int> availableActions, bool explore) {
    if (explore && (double)generator()/(generator.max() - generator.min()) < this->epsilon) {
        // TODO are only random actions taken
        return availableActions[generator() % availableActions.size()];
    }

    int state = obsToState(obs);

    vector<int> bestActions = {availableActions[0]};
    if (state > Q.size()) {
        throw runtime_error("TabQLearning:\nstate > state size\n");
    }

    double bestValue = Q[state][bestActions[0]];
    for (int i = 1; i < availableActions.size(); i++) {
        int action = availableActions[i];
        double value = Q[state][action];

        if (value > bestValue) {
            bestActions = {action};
            bestValue = value;
        } else if (value == bestValue) {
            bestActions.push_back(action);
        }
    }

    if (bestActions.size() > 1) {
        int selectedAction = generator() % bestActions.size();
        return bestActions[selectedAction];
    }

    return bestActions[0];
}

void TabQLearning::update(vector<int> obs, int action, double reward, vector<int> actionSet) {
    // this is fake Q-Learning use the same state for s'?
    int state = this->obsToState(obs);

    double maxQ = this->Q[state][actionSet[0]];
    // each state has a different number of actions
    for (int a: actionSet) {
        if (this->Q[state][a] > maxQ) {
            maxQ = this->Q[state][a];
        }
    }

    this->Q[state][action] += this->alpha * (reward + this->gamma * maxQ - this->Q[state][action]);
}
