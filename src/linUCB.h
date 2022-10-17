#ifndef LINUCB_H
#define LINUCB_H

#include <vector>
#include <random>

#include <torch/torch.h>

using namespace std;

class LinUCB {
    public:
        LinUCB(int observeDims, int numActions, double regularizer, double delta, int seed);

        void updateAgent(vector<double> observation, int action, double reward);
        int selectAction(vector<double> observation, vector<int> available_actions);

    private:

        int observeDims;
        int numActions;
        double regularizer;
        double delta;

        torch::Tensor V;
        torch::Tensor theta;
        torch::Tensor b;

        int timestep;

        // helpers
        torch::Tensor createActionContext(torch::Tensor observation, int action);

        void updateTheta(torch::Tensor oldActionContext, double reward);

        default_random_engine generator;
};

#endif /* LINUCB_H */
