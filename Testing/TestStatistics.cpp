// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"

#include <Poco/Thread.h>
#include <Poco/Timestamp.h>

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

//
// Get expected values for the labels.
//

template <typename T>
static T max(const std::vector<T>& inputs)
{
    auto maxElementIter = std::max_element(inputs.begin(), inputs.end());
    POTHOS_TEST_TRUE(inputs.end() != maxElementIter);

    return *maxElementIter;
}

template <typename T>
static T min(const std::vector<T>& inputs)
{
    auto minElementIter = std::min_element(inputs.begin(), inputs.end());
    POTHOS_TEST_TRUE(inputs.end() != minElementIter);

    return *minElementIter;
}

template <typename T>
static T mean(const std::vector<T>& inputs)
{
    return std::accumulate(
               inputs.begin(),
               inputs.end(),
               T(0)) / static_cast<T>(inputs.size());
}

template <typename T>
static T median(const std::vector<T>& inputs)
{
    std::vector<T> sortedInputs(inputs);
    std::sort(sortedInputs.begin(), sortedInputs.end());
    auto sortedIndex = size_t(std::floor(inputs.size()/2));

    return sortedInputs[sortedIndex];
}

template <typename T>
static T stddev(const std::vector<T>& inputs)
{
    const T inputMean = mean(inputs);
    const size_t size = inputs.size();

    const T oneDivSizeMinusOne = 1.0 / static_cast<T>(size-1);

    std::vector<T> diffs;
    diffs.reserve(size);
    std::transform(
        inputs.begin(),
        inputs.end(),
        std::back_inserter(diffs),
        [&inputMean, &size](T input)
        {
            return std::pow((input - inputMean), T(2));
        });

    const T addedDiffs = std::accumulate(diffs.begin(), diffs.end(), T(0));

    return std::sqrt(oneDivSizeMinusOne * addedDiffs);
}

template <typename T>
static T variance(const std::vector<T>& inputs)
{
    return std::pow(stddev(inputs), T(2));
}

template <typename T>
static T medAbsDev(const std::vector<T>& inputs)
{
    const T med = median(inputs);
    std::vector<T> diffs;

    std::transform(
        inputs.begin(),
        inputs.end(),
        std::back_inserter(diffs),
        [&med](T input){return std::abs<T>(input-med);});

    return median(diffs);
}

//
// Get test values.
//

static std::vector<double> getExpectedOutputs(const std::vector<double>& inputs)
{
    return
    {
        max(inputs),
        min(inputs),
        mean(inputs),
        median(inputs),
        stddev(inputs),
        variance(inputs),
        medAbsDev(inputs)
    };
}

static void waitUntilMessagesReceived(const std::vector<Pothos::Proxy>& collectorSinks)
{
    constexpr Poco::Int64 timeoutUs = 2e6;
    Poco::Timestamp timestamp;
    bool allReceived = false;

    while(!allReceived && (timestamp.elapsed() < timeoutUs))
    {
        allReceived = std::all_of(
            collectorSinks.begin(),
            collectorSinks.end(),
            [](const Pothos::Proxy& collectorSink)
            {
                return !collectorSink.call<Pothos::ObjectVector>("getMessages").empty();
            });

        Poco::Thread::sleep(100 /*ms*/);
    }
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_statistics)
{
    using namespace PothosArrayFireTests;

    std::random_device rd;
    std::mt19937 g(rd());

    std::vector<double> inputs = linspace<double>(-10, 10, 50);
    inputs.emplace_back(0.0); // To test PTP
    std::shuffle(inputs.begin(), inputs.end(), g);

    const auto dtype = Pothos::DType("float64");

    // NaN functions will be tested elsewhere.
    const std::vector<Pothos::Proxy> arrayFireBlocks =
    {
        Pothos::BlockRegistry::make("/arrayfire/algorithm/max", "Auto", dtype),
        Pothos::BlockRegistry::make("/arrayfire/algorithm/min", "Auto", dtype),
        Pothos::BlockRegistry::make("/arrayfire/statistics/mean", "Auto", dtype),
        Pothos::BlockRegistry::make("/arrayfire/statistics/median", "Auto", dtype),
        Pothos::BlockRegistry::make("/arrayfire/statistics/stdev", "Auto", dtype),
        Pothos::BlockRegistry::make("/arrayfire/statistics/var", "Auto", dtype, false),
        Pothos::BlockRegistry::make("/arrayfire/statistics/medabsdev", "Auto", dtype),
    };
    const size_t numBlocks = arrayFireBlocks.size();

    auto periodicTrigger = Pothos::BlockRegistry::make("/blocks/periodic_trigger");
    periodicTrigger.call("setRate", 1.25);

    // We need separate vector sources because Pothos doesn't support
    // connecting to multiple blocks with custom input buffer managers.
    std::vector<Pothos::Proxy> vectorSources;
    std::vector<Pothos::Proxy> slotToMessages;
    std::vector<Pothos::Proxy> collectorSinks;
    for(size_t blockIndex = 0; blockIndex < numBlocks; ++blockIndex)
    {
        vectorSources.emplace_back(Pothos::BlockRegistry::make(
                                       "/blocks/vector_source",
                                       dtype));
        vectorSources.back().call("setMode", "ONCE");
        vectorSources.back().call("setElements", inputs);

        slotToMessages.emplace_back(Pothos::BlockRegistry::make(
                                        "/blocks/slot_to_message",
                                        "lastValue"));

        collectorSinks.emplace_back(Pothos::BlockRegistry::make(
                                        "/blocks/collector_sink",
                                        dtype));
    }

    // Execute the topology.
    {
        Pothos::Topology topology;

        for(size_t blockIndex = 0; blockIndex < numBlocks; ++blockIndex)
        {
            topology.connect(
                vectorSources[blockIndex],
                0,
                arrayFireBlocks[blockIndex],
                0);
            topology.connect(
                arrayFireBlocks[blockIndex],
                0,
                collectorSinks[blockIndex],
                0);

            topology.connect(
                periodicTrigger,
                "triggered",
                arrayFireBlocks[blockIndex],
                "probeLastValue");
            topology.connect(
                arrayFireBlocks[blockIndex],
                "lastValueTriggered",
                slotToMessages[blockIndex],
                "lastValue");
            topology.connect(
                slotToMessages[blockIndex],
                0,
                collectorSinks[blockIndex],
                0);
        }

        topology.commit();

        /*
         * Messages are asynchronous, so waitInactive() won't wait
         * for them to arrive. ArrayFire blocks are slowed down in
         * the beginning because of internal setup, so we need to
         * put in extra checks for messages.
         */
        waitUntilMessagesReceived(collectorSinks);
        POTHOS_TEST_TRUE(topology.waitInactive());
    }


    const auto expectedOutputs = getExpectedOutputs(inputs);
    POTHOS_TEST_EQUAL(expectedOutputs.size(), numBlocks);

    for(size_t blockIndex = 0; blockIndex < numBlocks; ++blockIndex)
    {
        const auto blockName = arrayFireBlocks[blockIndex].call<std::string>("getName");
        
        const bool isStdOrVar = ("/arrayfire/statistics/stdev" == blockName) ||
                                ("/arrayfire/statistics/var" == blockName);
        
        std::cout << "Testing " << blockName << "..." << std::endl;

        // Make sure the buffer was forwarded as expected.
        testBufferChunk<double>(
            collectorSinks[blockIndex].call<Pothos::BufferChunk>("getBuffer"),
            inputs);

        const auto messages = collectorSinks[blockIndex].call<Pothos::ObjectVector>("getMessages");
        POTHOS_TEST_EQUAL(1, messages.size());

        auto output = messages[0];
        POTHOS_TEST_TRUE(output.type() == typeid(Pothos::Object));
        POTHOS_TEST_TRUE(output.extract<Pothos::Object>().type() == typeid(double));

        POTHOS_TEST_CLOSE(
            expectedOutputs[blockIndex],
            output.extract<Pothos::Object>().extract<double>(),
            isStdOrVar ? 1.0 : 1e-6);
    }
}
