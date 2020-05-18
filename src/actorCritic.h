#ifndef ACTOR_CRITIC_H
#define ACTOR_CRITIC_H

#include <vector>
#include <random>

class ActorCritic: public Agent {
    public:
        ActorCritic(vector<double> *weights, double initialState, int flowId);
        void step(double state, double reward, double &rate);
        static vector<double> initializeWeights();
        string getName();
    
    private:
        enum Mode {
            Add,
            Mult,
            Both,
            Choose
        };
        Mode mode;
        default_random_engine generator;
        
        pair<double, double> selectAction();
        double selectActionMult();
        double selectActionAdd();
        
        double getVariance();
        
        double oldValue = 0;
        double oldState;
        /*TODO: this sets what the agent considers to be its previous action at the start of the episode
                it should be changed to choose the first action during setup (outside of the loop over time steps)
        */
        pair<double, double> actionPair;
        vector<int> tiles;
        vector<int> oldTiles;
        vector<double> parameters; // u
        vector<double> *criticWeights; // v
        vector<double> actorWeights; // w
        vector<double> weightTrace; // ev
        vector<double> parameterTrace; // eu
        double rBar = 0;
        double k, phi, mu, sigma;
        

        double initialAlphaU;
        double initialAlphaV;
        double alphaU;
        double alphaV;
        double tau;
        double lambda;
        double gamma;
        bool inac;
        bool s;

        double totalReward = 0;
};

#endif /* ACTOR_CRITIC_H */
