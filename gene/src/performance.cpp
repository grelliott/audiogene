/*
 * Copyright 2020 Grant Elliott <grant@grantelliott.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "performance.hpp"

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <cstdlib>
#include <iostream>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "audience.hpp"
#include "individual.hpp"
#include "musician.hpp"
#include "midi.hpp"
#include "osc.hpp"
#include "population.hpp"
#include "spi.hpp"

namespace audiogene {

Performance::Performance(const YAML::Node _config): 
        logger(spdlog::get("log")), 
        config(_config) {
    seatAudience();
    assembleMusicians();
}

void Performance::seatAudience() {
    audiogene::Audience* audienceSource;
    YAML::Node inputNode;

    try {
        inputNode = config["input"];
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("No input source configured");
    }

    std::string inputType;
    try {
        inputType = inputNode["type"].as<std::string>();
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Input source misconfigured");
    }

    if (inputType == "midi") {
        logger->info("Input type is MIDI");
        const std::string inputName = (inputNode["name"]) ? inputNode["name"].as<std::string>() : "";
        const KeyMap mapping = inputNode["map"] ? inputNode["map"].as<KeyMap>() : KeyMap();
        audienceSource = new audiogene::MIDI(inputName, mapping);
    } else if (inputNode["type"].as<std::string>() == "midi") {
        logger->info("Input type is SPI");
        audienceSource = new audiogene::SPI();
    } else {
        throw std::runtime_error("Unknown input type " + inputType);
    }

    audience.reset(audienceSource);

    if (!audience->prepare()) {
        logger->error("Failed to prepare input!");
        throw std::runtime_error("Failed to prepare input");
    }
    logger->info("Input prepared");
}

void Performance::assembleMusicians() {
    std::string scAddr;
    std::string scPort;
    try {
        YAML::Node scNode(config["SuperCollider"]);
        scAddr = scNode["addr"].as<std::string>();
        scPort = scNode["port"].as<std::string>();
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Missing SuperCollider config");
    }

    std::string oscPort;
    try {
        YAML::Node oscNode = config["OSC"];
        oscPort = oscNode["port"].as<std::string>();
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Missing OSC config");
    }

    std::cout << "Initializing OSC" << std::endl;
    musician.reset(new audiogene::OSC(oscPort, scAddr, scPort));
    std::cout << "SC is ready" << std::endl;

    //musician->send("/notify", "1");
    //std::cout << "Sent notify"<<std::endl;
}

std::future<void> Performance::play() {
    return std::async(std::launch::async, [this] () {
        // The input is from an audience
        // So we want an audience to guide the presentation
        // An audience gives feedback on various criteria
        // The population takes that feedback and determines which of its individuals best represent that feedback
        // and the next attempt tries to meet these expectations
        KeyMap attributes;
        try {
            attributes = config["genes"].as<KeyMap>();
        } catch (const YAML::Exception& e) {
            throw std::runtime_error("Missing audience attributes");
        }
        // Initialize preferences of audience
        audience->initializePreferences(attributes);

        double mutationProbability;
        size_t populationSize;
        uint8_t topN;
        try {
            mutationProbability = config["mutationProb"].as<double>();
            populationSize = config["populationSize"].as<int>();
            topN = config["keepFittest"].as<int>();
        } catch (const YAML::Exception& e) {
            throw std::runtime_error("Genetics misconfigured");
        }
        audiogene::Individual seed(attributes);
        audiogene::Population pop(populationSize, seed, mutationProbability, audience);
        logger->info("Initial population: {}", pop);
        uint8_t i = 0;

        while (true) {
            // Make new generations
            std::cout << "loop " << +i++ << std::endl;
            logger->info("Getting new generation from top {} individuals", topN);
            pop.nextGeneration(topN);
            logger->info("New population: {}", pop);
            musician->setConductor(pop.fittest());
            musician->requestConductor();  // future should return when the thread finishes
        }
    });
}

}  // namespace audiogene
