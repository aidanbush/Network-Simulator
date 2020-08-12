#ifndef SARSA_H
#define SARSA_H

#include <random>

#include "tilecoder.h"

class Sarsa: public Agent {
    public:
        Sarsa(vector<double> initialState, int flowId);

        pair<double, double> step(vector<double> state, double reward, double &rate);

        string getName();

    private:
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

        double totalReward = 0;

        Tilecoder *tilecoder;

        mt19937 generator;
};

#endif /* SARSA_H */
