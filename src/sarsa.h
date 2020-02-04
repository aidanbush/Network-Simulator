#ifndef SARSA_H
#define SARSA_H

#include "tilecoder.h"

class Sarsa {
    public:
        Sarsa(vector<double> *weights, double initialState);
        int step(double state, double reward);
        static vector<double> initializeWeights();
    
    private:
        pair<int, double> selectAction(vector<int> tiles);
        double alpha;
        vector<double> *weights;
        vector<double> trace;
        //Tilecoder tilecoder;
        double oldValue = 0;
        double oldState;
        /*TODO: this sets what the agent considers to be its previous action at the start of the episode
                it should be changed to choose the first action during setup (outside of the loop over time steps)
        */
        int oldAction = 1;
        vector<int> oldTiles;
};

#endif /* SARSA_H */
