#include <map>
#include <random>

#include "generator.h"
#include "manager.h"
#include "config.h"
#include "packet.h"

using namespace std;

/* Traffic generator */
enum GeneratorType {
    BasicGeneratorType,
    CycleGeneratorType,
    PoissonGeneratorType,
};

Generator *createGenerator(json &generatorConfig) {
    static map<string, GeneratorType> generatorTypeMap = {
        {"basic", BasicGeneratorType},
        {"cycle", CycleGeneratorType},
        {"poisson", PoissonGeneratorType},
    };

    if (!hasMemberOfType(generatorConfig, "type", jsonString)) {
        throw runtime_error("Generator:\nNo string with name 'type'\n" + generatorConfig.dump(4));
    }

    string generatorTypeString = generatorConfig["type"];
    GeneratorType generatorType;
    try {
        generatorType = generatorTypeMap.at(generatorTypeString);
    } catch (out_of_range&) {
        throw runtime_error("Generator:\nInvalid generator type: " + generatorTypeString);
    }

    Generator *generator;

    switch (generatorType) {
        case BasicGeneratorType:
            generator = new BasicGenerator(generatorConfig);
            break;
        case CycleGeneratorType:
            generator = new CycleGenerator(generatorConfig);
            break;
        case PoissonGeneratorType:
            generator = new PoissonGenerator(generatorConfig);
            break;
        default:
            throw runtime_error("Generator:\nInvalid generator type: " + generatorTypeString);
    }

    return generator;
}

Generator::Generator(json &generatorConfig) {
    validateGeneratorConfig(generatorConfig);

    this->bufferMaxSize = generatorConfig["buffer_size"];
    this->flowId = -1;
    this->bufferCurSize = 0;
    this->running = false;
}

bool Generator::setFlowId(int id) {
    if (flowId == -1) {
        flowId = id;
        return true;
    }

    return false;
}

void Generator::validateGeneratorConfig(json &generatorConfig) {
    string message = "";

    if (!hasMemberOfType(generatorConfig, "buffer_size", jsonInt)) {
        message += "No integer with name 'buffer_size'.\n";
    }

    if (!message.empty()) {
        message = "Generator:\n" + message + generatorConfig.dump(4);
        throw runtime_error(message);
    }
}

void Generator::generatePacket() {
    Flow *f = man.getFlow(flowId);
    f->packetGenerationNotification();
}

bool Generator::startTraffic() {
    if (running) {
        // TODO Log error
        return false;
    }

    running = true;

    EventI *e = new Event<Generator>(man.time, &Generator::generatePacket, this);
    man.pushEvent(e);

    return true;
}

void Generator::stopTraffic() {
    running = false;
}

bool Generator::getNextPacket(Generator::PacketData &data) {
    // if has room
    if (packetBuffer.empty()) {
        return false;
    }

    Generator::PacketData p = packetBuffer.front();
    packetBuffer.pop();

    bufferCurSize -= (p.headerSize + p.bodySize);

    // copy over values
    data.headerSize = p.headerSize;
    data.bodySize = p.bodySize;

    return true;
}

second_t Generator::nextGenTime(int size, double rate) {
    return man.time + (size * BITS_PER_BYTE / rate);
}

/* Basic traffic generator */

BasicGenerator::BasicGenerator(json &generatorConfig):
    Generator(generatorConfig) {
    validateBasicGeneratorConfig(generatorConfig);

    this->rate = generatorConfig["rate"];
    this->headerSize = generatorConfig["header"];
    this->bodySize = generatorConfig["body"];
}

void BasicGenerator::validateBasicGeneratorConfig(json &generatorConfig) {
    string message = "";

    if (!hasMemberOfType(generatorConfig, "rate", jsonDouble)) {
        message += "No integer with name 'rate'.\n";
    }

    if (!hasMemberOfType(generatorConfig, "header", jsonInt)) {
        message += "No integer with name 'header'.\n";
    }

    if (!hasMemberOfType(generatorConfig, "body", jsonInt)) {
        message += "No integer with name 'body'.\n";
    }

    if (!message.empty()) {
        message = "BasicGenerator:\n" + message + generatorConfig.dump(4);
        throw runtime_error(message);
    }
}

double BasicGenerator::getAveragePacketSizeBytes() {
    return headerSize + bodySize;
}

void BasicGenerator::generatePacket() {
    if (!running) {
        return;
    }

    // create packet, if there is room, else wait
    int packetSize = headerSize + bodySize;
    if (bufferCurSize + packetSize <= bufferMaxSize) {
        packetBuffer.push({headerSize, bodySize});
        bufferCurSize += packetSize;
    }

    man.logEvent("BasicGenerator", 0, "generatePacket", "Generated Packet of size" + to_string(packetSize));
    Generator::generatePacket();

    EventI *e = new Event<BasicGenerator>(nextGenTime(packetSize, rate), &BasicGenerator::generatePacket, this);
    man.pushEvent(e);
}

/* Poisson traffic generator */

PoissonGenerator::PoissonGenerator(json &generatorConfig):
    Generator(generatorConfig) {
    validatePoissonGeneratorConfig(generatorConfig);

    // get lambda
    int rate = generatorConfig["bitrate"];

    this->headerSize = generatorConfig["header"];
    this->bodySize = generatorConfig["body"];

    double lambda = rate / ((this->headerSize + this->bodySize) * BITS_PER_BYTE);

    // create distribution
    distribution = exponential_distribution(lambda);
    // seed generator
    generator.seed(man.random()); // use man random
}

void PoissonGenerator::validatePoissonGeneratorConfig(json &generatorConfig) {
    string message = "";

    if (!hasMemberOfType(generatorConfig, "bitrate", jsonInt)) {
        message += "No integer with name 'bitrate'.\n";
    }

    if (!hasMemberOfType(generatorConfig, "header", jsonInt)) {
        message += "No integer with name 'header'.\n";
    }

    if (!hasMemberOfType(generatorConfig, "body", jsonInt)) {
        message += "No integer with name 'body'.\n";
    }

    if (!message.empty()) {
        message = "PoissonGenerator:\n" + message + generatorConfig.dump(4);
        throw runtime_error(message);
    }
}

int PoissonGenerator::getHeaderSize() {
    return headerSize;
}

int PoissonGenerator::getBodySize() {
    return bodySize;
}

second_t PoissonGenerator::nextGenTime() {
    second_t interArrivalTime = distribution(generator);
    return man.time + interArrivalTime;
}

void PoissonGenerator::generatePacket() {
    if (!running) {
        return;
    }

    // create packet, if there is room, else wait
    int headerSize = getHeaderSize();
    int bodySize = getBodySize();

    int packetSize = headerSize + bodySize;
    if (bufferCurSize + packetSize <= bufferMaxSize) {
        packetBuffer.push({headerSize, bodySize});
        bufferCurSize += packetSize;
    }

    man.logEvent("PoissonGenerator", 0, "generatePacket", "Generated Packet of size" + to_string(packetSize));
    Generator::generatePacket();

    EventI *e = new Event<PoissonGenerator>(nextGenTime(), &PoissonGenerator::generatePacket, this);
    man.pushEvent(e);
}

double PoissonGenerator::getAveragePacketSizeBytes() {
    return headerSize + bodySize;
}
