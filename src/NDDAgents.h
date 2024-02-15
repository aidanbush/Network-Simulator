#ifndef NDD_AGENTS_H
#define NDD_AGENTS_H

#include <nlohmann/json.hpp>
#include <vector>
#include <random>

using namespace std;

using json = nlohmann::json;

class NDDAgent {
    public:
        NDDAgent(json &NDDAgentConfig);

        virtual void init(int numStates, int maxNumActions);

        virtual int selectAction(vector<int> state, vector<int> availableActions, bool explore);
        virtual void update(vector<int> state, int action, double reward, vector<int> actionSet);

    protected:
        default_random_engine generator;
};

class RandNDDAgent: public NDDAgent {
    public:
        RandNDDAgent(json &NDDAgentConfig);

        void init(int numStates, int maxNumActions);

        int selectAction(vector<int> state, vector<int> availableActions, bool explore);
        void update(vector<int> state, int action, double reward, vector<int> actionSet);

    protected:
};

class TabQLearning: public NDDAgent {
    public:
        TabQLearning(json &NDDAgentConfig);
        void init(int numStates, int maxNumActions);

        int selectAction(vector<int> state, vector<int> availableActions, bool explore);
        void update(vector<int> state, int action, double reward, vector<int> actionSet);

    protected:
        int obsToState(vector<int> obs);

        vector<vector<double>> Q;

        double alpha;
        double epsilon;
        double gamma;

    private:
        json &validateTabQLearningConfig(json &NDDAgentConfig);
};

NDDAgent *createNDDAgent(json &NDDAgentConfig);

#endif /* NDD_AGENTS_H */
