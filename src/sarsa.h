#ifndef SARSA_H
#define SARSA_H

class Sarsa: public Agent {
    public:
        Sarsa(vector<double> *weights, double initialState, int flowId);
        void step(double state, double reward, double &rate);
        static vector<double> initializeWeights();
        string getName();
    
    private:
        pair<int, double> selectAction();
        vector<double> *weights;
        vector<double> trace;
        double oldValue = 0;
        double oldState;
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
};

#endif /* SARSA_H */
