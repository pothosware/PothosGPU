// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

namespace PothosArrayFireTests
{

//
// Get expected values for the labels.
//

template <typename T>
static T max(const std::vector<T>& inputs, size_t* pPosition)
{
    auto maxElementIter = std::max_element(inputs.begin(), inputs.end());
    POTHOS_TEST_TRUE(inputs.end() != maxElementIter);

    *pPosition = std::distance(inputs.begin(), maxElementIter);
    return *maxElementIter;
}

template <typename T>
static T min(const std::vector<T>& inputs, size_t* pPosition)
{
    auto minElementIter = std::min_element(inputs.begin(), inputs.end());
    POTHOS_TEST_TRUE(inputs.end() != minElementIter);

    *pPosition = std::distance(inputs.begin(), minElementIter);
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
static T median(const std::vector<T>& inputs, size_t* pPosition)
{
    std::vector<T> sortedInputs(inputs);
    std::sort(sortedInputs.begin(), sortedInputs.end());

    auto sortedIndex = size_t(std::floor(inputs.size()/2));
    auto unsortedIter = std::find(
                            inputs.begin(),
                            inputs.end(),
                            sortedInputs[sortedIndex]);
    POTHOS_TEST_TRUE(unsortedIter != inputs.end());

    *pPosition = std::distance(inputs.begin(), unsortedIter);

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
    size_t _;

    const T med = median(inputs, &_);
    std::vector<T> diffs;

    std::transform(
        inputs.begin(),
        inputs.end(),
        std::back_inserter(diffs),
        [&med](T input){return std::abs<T>(input-med);});

    return median(diffs, &_);
}

//
// Make sure that blocks that post labels post the labels we expect.
//

static std::vector<Pothos::Label> getExpectedLabels(const std::vector<double>& inputs)
{
    size_t expectedMaxPosition = 0;
    size_t expectedMinPosition = 0;
    size_t expectedMedianPosition = 0;

    const auto expectedMax = max(inputs, &expectedMaxPosition);
    const auto expectedMin = min(inputs, &expectedMinPosition);
    const auto expectedMean = mean(inputs);
    const auto expectedMedian = median(inputs, &expectedMedianPosition);
    const auto expectedStdDev = stddev(inputs);
    const auto expectedVariance = variance(inputs);
    const auto expectedMedAbsDev = medAbsDev(inputs);

    return std::vector<Pothos::Label>
    ({
        Pothos::Label("MAX", expectedMax, expectedMaxPosition),
        Pothos::Label("MIN", expectedMin, expectedMinPosition),
        Pothos::Label("MEAN", expectedMean, 0),
        Pothos::Label("MEDIAN", expectedMedian, expectedMedianPosition),
        Pothos::Label("STDDEV", expectedStdDev, 0),
        Pothos::Label("VAR", expectedVariance, 0),
        Pothos::Label("MEDABSDEV", expectedMedAbsDev, 0),
    });
}

}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_labels)
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

    // We need separate vector sources because Pothos doesn't support
    // connecting to multiple blocks with custom input buffer managers.
    std::vector<Pothos::Proxy> vectorSources;
    std::vector<Pothos::Proxy> collectorSinks;
    for(size_t blockIndex = 0; blockIndex < numBlocks; ++blockIndex)
    {
        vectorSources.emplace_back(Pothos::BlockRegistry::make(
                                       "/blocks/vector_source",
                                       dtype));
        vectorSources.back().call("setMode", "ONCE");
        vectorSources.back().call("setElements", inputs);

        collectorSinks.emplace_back(Pothos::BlockRegistry::make(
                                        "/blocks/collector_sink",
                                        dtype));
    }

    // Execute the topology.
    {
        auto topology = Pothos::Topology::make();

        for(size_t blockIndex = 0; blockIndex < numBlocks; ++blockIndex)
        {
            topology->connect(
                vectorSources[blockIndex],
                0,
                arrayFireBlocks[blockIndex],
                0);
            topology->connect(
                arrayFireBlocks[blockIndex],
                0,
                collectorSinks[blockIndex],
                0);
        }

        topology->commit();
        POTHOS_TEST_TRUE(topology->waitInactive(0.01, 0.0));
    }

    auto expectedLabels = getExpectedLabels(inputs);
    POTHOS_TEST_EQUAL(expectedLabels.size(), numBlocks);

    for(size_t labelIndex = 0; labelIndex < numBlocks; ++labelIndex)
    {
        const auto& expectedLabel = expectedLabels[labelIndex];
        const bool isStdOrVar = ("VAR" == expectedLabel.id) || ("STDDEV" == expectedLabel.id);

        auto blockLabels = collectorSinks[labelIndex].call<std::vector<Pothos::Label>>("getLabels");
        POTHOS_TEST_EQUAL(1, blockLabels.size());
        const auto& blockLabel = blockLabels[0];

        std::cout << "Testing label " << expectedLabel.id << std::endl;

        std::cout << " * Buffer..." << std::endl;
        testBufferChunk<double>(
            collectorSinks[labelIndex].call<Pothos::BufferChunk>("getBuffer"),
            inputs);
        std::cout << " * ID..." << std::endl;
        POTHOS_TEST_EQUAL(
            expectedLabel.id,
            blockLabel.id);
        std::cout << " * Index..." << std::endl;
        testEqual(
            expectedLabel.index,
            blockLabel.index);
        std::cout << " * Data..." << std::endl;
        POTHOS_TEST_CLOSE(
            expectedLabel.data.convert<double>(),
            blockLabel.data.convert<double>(),
            isStdOrVar ? 1.0 : 1e-6);
    }
}
