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

        struct PacketData {
            int headerSize;
            int bodySize;
        };

        virtual bool startTraffic(); // return if first call
        PacketData getNextPacket();

        virtual void generatePacket();

    protected:
        int bufferMaxSize; // bytes
        int bufferCurSize; // bytes
        queue<PacketData> packetBuffer; // may want to switch to pointers

        second_t nextGenTime(int size, double rate);

        bool started; // TODO initialize

    private:
        void validateGeneratorConfig(json &generatorConfig);
};

// generates the same packets at a set interval
class BasicGenerator: public Generator {
    public:
        BasicGenerator(json &generatorConfig);

        void generatePacket();

    private:
        double rate;
        int headerSize;
        int bodySize;

        void validateBasicGeneratorConfig(json &generatorConfig);
};

// creates packets followiong a predefined cycle
class CycleGenerator: public Generator {
    public:
        CycleGenerator(json &generatorConfig);

        void generatePacket();

    private:
        void validateCycleGeneratorConfig(json &generatorConfig);

        struct cycleElement {
            int headerSize;
            int bodySize;
            double rate;
            int numPackets;
        };

        int getCycleHeaderSize();
        int getCycleBodySize();
        double getCycleRate();
        int getCycleNumPackets();

        // vector of tuples
        vector<cycleElement> cycleData;
        int packetsSent;
        int cyclePos;
};

Generator *createGenerator(json &generatorConfig);

#endif /* GENERATOR_H */
