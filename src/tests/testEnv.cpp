#include <iostream>
#include <vector>
#include <algorithm>

#include "../sarsa.h"

#define NUM_EPISODES 100

using namespace std;

/*
NOTE: To run this, set STATE_MIN to -10.0 and STATE_MAX to 10.0 in tilecoder.cpp, and NUM_ACTIONS to 2 in sarsa.cpp
*/

double getReward(double state) {
    if (state > 9.9) {
        return 10.0;
    } else if (state > 7.9) {
        return -1.0;
    } else {
        return 0.0;
    }
}

int main() {
    vector<double> weights = Sarsa::initializeWeights();
    cout << NUM_EPISODES << endl;
    for (int i = 0; i < NUM_EPISODES; i++) {
        Sarsa agent = Sarsa(&weights, 0);
        double state = 0.0;
        double reward = 0.0;
        bool done = false;
        int numSteps = 0;
        while (true) {
            if (state > 9.9) {
                done = true;
            }
            if (agent.step(state, reward)) {
                // Action 1
                state = min(state + 1.0, 10.0);
            } else {
                //Action 0
                state = max(state - 1.0, -10.0);
            }
            reward = getReward(state);
            if (done) {
                break;
            }
            numSteps++;
            //cout << endl << "Step: " << numSteps << " State: " << state << endl;//'\r' << flush;
            // if (numSteps >= 50) {
            //     break;
            // }
        }
        cout << i << " " << numSteps << endl;
    }
    return 0;
}