#ifndef ACTOR_CRITIC_H
#define ACTOR_CRITIC_H

#include <vector>
#include <random>

#include "tilecoder.h"

class ActorCritic: public Agent {
    public:
        ActorCritic(vector<double> initialState, int flowId, double initialRBar);

        pair<double, double> step(vector<double> state, double reward, double &rate);

        string getName();
        void seed(int seed);

        pair<double, double> getMultMeanStdev();
        pair<double, double> getAddMeanStdev();

    private:
        enum Mode {
            Add,
            Mult,
            Both,
            Choose
        };
        Mode mode;

        enum MultMode {
            Gamma,
            GaussianTanh,
            BetaMult,
        };
        MultMode multMode;

        enum AddMode {
            Gaussian,
            BetaAdd,
        };
        AddMode addMode;

        Tilecoder *tilecoder;

        mt19937 generator;

        pair<double, double> selectAction();
        double selectActionMult();
        double selectActionAdd();

        double getMultVariance();
        double getAddVariance();
        double getVariance();
        double computeDiffSum();

        void updateWeightTrace();
        void updateCriticWeights(double delta);

        vector<double> computeGradient();
        void computeMultActionGradient(vector<double> &gradLog);
        void computeAddActionGradient(vector<double> &gradLog);

        void updateParameterTrace(vector<double> gradLog);
        void updateParameters(double delta);

        void updateAdvantageParameters(vector<double> gradlog, double delta);
        void updateParametersINAC();

        double oldValue = 0;
        vector<double> oldState;
        /*TODO: this sets what the agent considers to be its previous action at the start of the episode
                it should be changed to choose the first action during setup (outside of the loop over time steps)
        */
        pair<double, double> actionPair;
        vector<int> tiles;
        vector<int> oldTiles;
        vector<double> parameters; // u
        vector<double> criticWeights; // v
        vector<double> advantageParameters; // w
        vector<double> weightTrace; // ev
        vector<double> parameterTrace; // eu
        double rBar = 0;
        double k = 0;
        double phi = 0;
        double mu = 0;
        double sigma = 0;

        int stepnum = 0;

        bool splitDistributionDecrease;

        double initialAlphaU;
        double initialAlphaV;
        double alphaU;
        double alphaV;
        double alphaR;
        double tau;
        double lambda;
        double gamma;
        bool inac;
        bool s;

        double totalReward = 0;
};

#endif /* ACTOR_CRITIC_H */
