#ifndef AGENT_H
#define AGENT_H

#include <string>

//TODO: Should we include headers for agent types here so they don't all have to be included in every file

using namespace std;

enum AgentType {
    SarsaAgent,
    ActorCriticAgent
};

class Agent {
    public:
        virtual ~Agent() = default;
        virtual pair<double, double> step(vector<double> state, double reward) = 0;
        virtual string getName() = 0;
        virtual vector<double> getWeights() = 0;

    protected:
        int flowId;
        static double sumIndices(vector<double> *vec, vector<int> *indices, int offset = 0);
};

#endif /* AGENT_H */
