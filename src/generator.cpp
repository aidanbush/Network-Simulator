#include <map>

#include "generator.h"
#include "manager.h"
#include "config.h"
#include "packet.h"

using namespace std;

/* Traffic generator */
enum GeneratorType {
    BasicGeneratorType,
    CycleGeneratorType,
};

Generator *createGenerator(json &generatorConfig) {
    static map<string, GeneratorType> generatorTypeMap = {
        {"basic", BasicGeneratorType},
        {"cycle", CycleGeneratorType},
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
        default:
            throw runtime_error("Generator:\nInvalid generator type: " + generatorTypeString);
    }

    return generator;
}

Generator::Generator(json &generatorConfig) {
    validateGeneratorConfig(generatorConfig);

    this->bufferMaxSize = generatorConfig["buffer_size"];
    this->bufferCurSize = 0;
    this->started = false;
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
}

bool Generator::startTraffic() {
    if (started) {
        // TODO Log error
        return false;
    }

    EventI *e = new Event<Generator>(man.time, &Generator::generatePacket, this);
    man.pushEvent(e);

    return true;
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

void BasicGenerator::generatePacket() {
    // create packet, if there is room, else wait
    int packetSize = headerSize + bodySize;
    if (bufferCurSize + packetSize <= bufferMaxSize) {
        packetBuffer.push({headerSize, bodySize});
        bufferCurSize += packetSize;
    }

    EventI *e = new Event<BasicGenerator>(nextGenTime(packetSize, rate), &BasicGenerator::generatePacket, this);
    man.pushEvent(e);
}

/* Cycle traffic generator */

CycleGenerator::CycleGenerator(json &generatorConfig):
    Generator(generatorConfig) {
    validateCycleGeneratorConfig(generatorConfig);

    // go though cycles and store them
    for (auto it: generatorConfig["cycle"].items()) {
        // go through cycle elements
        cycleElement elem = {
            it.value()[0], // header size
            it.value()[1], // body size
            it.value()[2], // rate
            it.value()[3], // number of packets
        };

        cycleData.push_back(elem);
    }

    this->packetsSent = 0;
    this->cyclePos = 0;
}

void CycleGenerator::validateCycleGeneratorConfig(json &generatorConfig) {
    string message = "";

    // check if has cycle list
    if (!hasMemberOfType(generatorConfig, "cycle", jsonArray)) {
        message += "No array with name 'cycle'.\n";
    } else {
        // go through list and check count of elements and type
        for (auto it: generatorConfig["cycle"].items()) {
            // check if array
            if (!checkConfigObjType(it.value(), jsonArray)) {
                message += "No array type in 'cycle'.\n";
            } else {
                if (!checkConfigObjType(it.value()[0], jsonInt)) {
                    message += "No int type in 'cycle' array at index 0.\n";
                }

                if (!checkConfigObjType(it.value()[1], jsonInt)) {
                    message += "No int type in 'cycle' array at index 0.\n";
                }

                if (!checkConfigObjType(it.value()[2], jsonDouble)) {
                    message += "No double type in 'cycle' array at index 0.\n";
                }

                if (!checkConfigObjType(it.value()[3], jsonInt)) {
                    message += "No int type in 'cycle' array at index 0.\n";
                }
            }
        }
    }

    if (!message.empty()) {
        message = "Cycle:\n" + message + generatorConfig.dump(4);
        throw runtime_error(message);
    }
}

int CycleGenerator::getCycleHeaderSize() {
    return cycleData[cyclePos].headerSize;
}

int CycleGenerator::getCycleBodySize() {
    return cycleData[cyclePos].bodySize;
}

double CycleGenerator::getCycleRate() {
    return cycleData[cyclePos].rate;
}

int CycleGenerator::getCycleNumPackets() {
    return cycleData[cyclePos].numPackets;
}

void CycleGenerator::generatePacket() {
    // create packet, if there is room, else wait
    int headerSize = getCycleHeaderSize();
    int bodySize = getCycleBodySize();
    double rate = getCycleRate();

    int packetSize = headerSize + bodySize;
    if (bufferCurSize + packetSize <= bufferMaxSize) {
        packetBuffer.push({headerSize, bodySize});
        bufferCurSize += packetSize;

        packetsSent++;

        // if done cycle go to next
        if (packetsSent >= getCycleNumPackets()) {
            cyclePos++;
            cyclePos %= cycleData.size();
        }
    }

    EventI *e = new Event<CycleGenerator>(nextGenTime(packetSize, rate), &CycleGenerator::generatePacket, this);
    man.pushEvent(e);
}
