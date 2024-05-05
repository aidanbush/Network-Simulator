#ifndef GENERATOR_H
#define GENERATOR_H

#include <queue>
#include <vector>
#include <nlohmann/json.hpp>

#include "manager.h"

using namespace std;
using json = nlohmann::json;

class Packet;

// currently creates UDP like traffic where packets are generated, should be modified to simulate TCP
class Generator {
    public:
        Generator(json &generatorConfig);
        virtual ~Generator() = default;

        bool setFlowId(int id);

        struct PacketData {
            int burstId;
            bool lastInBurst;
            int headerSize;
            int bodySize;
        };

        virtual bool startTraffic(); // return if first call
        void stopTraffic(); // return if first call
        bool getNextPacket(PacketData &data);

        virtual void generatePacket();

        virtual double getAveragePacketSizeBytes() = 0;

    protected:
        int bufferMaxSize; // bytes
        int bufferCurSize; // bytes
        queue<PacketData> packetBuffer; // may want to switch to pointers

        second_t nextGenTime(int size, double rate);

        bool running;

        int flowId;

        int curBurstId;

    private:
        void validateGeneratorConfig(json &generatorConfig);
};

// generates the same packets at a set interval
class BasicGenerator: public Generator {
    public:
        BasicGenerator(json &generatorConfig);

        void generatePacket();

        double getAveragePacketSizeBytes();

    private:
        double rate;
        int headerSize;
        int bodySize;

        void validateBasicGeneratorConfig(json &generatorConfig);
};

// Poisson model
class PoissonGenerator: public Generator {
    public:
        PoissonGenerator(json &generatorConfig);

        void generatePacket();

        double getAveragePacketSizeBytes();

    private:
        int getHeaderSize();
        int getBodySize();
        second_t nextGenTime();

        default_random_engine generator;
        exponential_distribution<double> distribution;

        int headerSize;
        int bodySize;

        void validatePoissonGeneratorConfig(json &generatorConfig);
};

// Compound Poisson
class CompoundPoissonGenerator: public Generator {
    public:
        CompoundPoissonGenerator(json &generatorConfig);

        bool startTraffic();
        void generatePacket();

        double getAveragePacketSizeBytes();
    private:
        int getHeaderSize();
        int getBodySize();

        void generateBurst();
        second_t nextBurstTime();
        second_t nextGenTime();

        default_random_engine generator;
        exponential_distribution<double> burstDelayDistribution;
        geometric_distribution<int> burstSizeDistribution;

        int headerSize;
        int bodySize;

        //int burstQueue; // TODO change to vector of burst sizes
        queue<int> burstQueue;
        double burstRate;

        void validateCompoundPoissonGeneratorConfig(json &generatorConfig);
};

Generator *createGenerator(json &generatorConfig);

#endif /* GENERATOR_H */
