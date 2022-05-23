#ifndef SARSA_H
#define SARSA_H

#include <random>
#include <nlohmann/json.hpp>

#include "tilecoder.h"

using json = nlohmann::json;

class Sarsa: public Agent {
    public:
        Sarsa(vector<double> initialState, int flowId, json agentConfig);

        pair<double, double> step(vector<double> state, double reward);

        string getName();

        vector<double> getWeights();

        void setAveragePacketSizeBytes(double size);

    private:
        enum RewardMode {
            averageReward,
            discountedReward,
        };

        RewardMode rewardMode;

        pair<int, double> selectAction();

        vector<double> weights;
        vector<double> trace;
        double oldValue = 0;
        vector<double> oldState;
        /*TODO: this sets what the agent considers to be its previous action at the start of the episode
                it should be changed to choose the first action during setup (outside of the loop over time steps)
        */
        int oldAction = 1;
        vector<int> tiles;
        vector<int> oldTiles;

        double initialAlpha;
        double alpha;
        double lambda;
        double gamma;
        double epsilon;
        double beta;

        double rBar;

        double totalReward = 0;

        double avgPacketSizeBytes;

        Tilecoder *tilecoder;

        mt19937 generator;
};

#endif /* SARSA_H */
