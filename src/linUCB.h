#ifndef LINUCB_H
#define LINUCB_H

#include <vector>
#include <random>
#include <utility>

#include <torch/torch.h>

using namespace std;

class LinUCB {
    public:
        LinUCB(int observeDims, int numActions, double regularizer, double delta, int seed);

        void updateAgent(vector<double> observation, int action, double reward);
        pair<int, double> selectAction(vector<double> observation, vector<int> available_actions);

    private:

        int observeDims;
        int numActions;
        double regularizer;
        double delta;

        vector<torch::Tensor> V;
        vector<torch::Tensor> theta;
        vector<torch::Tensor> b;

        int timestep;

        // helpers
        void updateTheta(torch::Tensor oldContext, int action, double reward);

        default_random_engine generator;
};

#endif /* LINUCB_H */
