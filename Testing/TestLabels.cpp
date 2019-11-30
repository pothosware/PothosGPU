// Copyright (c) 2019 Nicholas Corgan
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

/*
template <typename T>
static T median(const std::vector<T>& inputs, size_t* pPosition)
{
    std::vector<T> sortedInputs(inputs);
    std::sort(sortedInputs.begin(), sortedInputs.end());

    *pPosition = size_t(std::floor(inputs[inputs.size()/2]));

    return sortedInputs[*pPosition];
}
*/

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

//
// Make sure that blocks that post labels post the labels we expect.
//

static std::vector<Pothos::Label> getExpectedLabels(const std::vector<double>& inputs)
{
    size_t expectedMaxPosition = 0;
    size_t expectedMinPosition = 0;
    //size_t expectedMedianPosition = 0;

    const auto expectedMax = max(inputs, &expectedMaxPosition);
    const auto expectedMin = min(inputs, &expectedMinPosition);
    const auto expectedMean = mean(inputs);
    //const auto expectedMedian = median(inputs, &expectedMedianPosition);
    const auto expectedStdDev = stddev(inputs);
    const auto expectedVariance = variance(inputs);

    return std::vector<Pothos::Label>
    ({
        Pothos::Label("MAX", expectedMax, expectedMaxPosition),
        Pothos::Label("MIN", expectedMin, expectedMinPosition),
        Pothos::Label("MEAN", expectedMean, 0),
        //Pothos::Label("MEDIAN", expectedMedian, expectedMedianPosition),
        Pothos::Label("STDDEV", expectedStdDev, 0),
        Pothos::Label("VAR", expectedVariance, 0),
    });
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_labels)
{
    std::random_device rd;
    std::mt19937 g(rd());

    std::vector<double> inputs = linspace<double>(-10, 10, 50);
    inputs.emplace_back(0.0); // To test PTP
    std::shuffle(inputs.begin(), inputs.end(), g);

    const auto dtype = Pothos::DType("float64");

    auto vectorSource = Pothos::BlockRegistry::make(
                            "/blocks/vector_source",
                            dtype);
    vectorSource.call("setMode", "ONCE");
    vectorSource.call("setElements", inputs);

    // NaN functions will be tested elsewhere.
    const std::vector<Pothos::Proxy> arrayFireBlocks =
    {
        Pothos::BlockRegistry::make("/arrayfire/algorithm/max", dtype, 1),
        Pothos::BlockRegistry::make("/arrayfire/algorithm/min", dtype, 1),
        Pothos::BlockRegistry::make("/arrayfire/statistics/mean", dtype, 1),
        //Pothos::BlockRegistry::make("/arrayfire/statistics/median", dtype, 1),
        Pothos::BlockRegistry::make("/arrayfire/statistics/stdev", dtype, 1),
        Pothos::BlockRegistry::make("/arrayfire/statistics/var", dtype, false, 1),
    };

    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             dtype);

    // Execute the topology.
    {
        auto topology = Pothos::Topology::make();

        for(const auto& block: arrayFireBlocks)
        {
            topology->connect(
                vectorSource,
                0,
                block,
                0);
        }
        for(const auto& block: arrayFireBlocks)
        {
            topology->connect(
                block,
                0,
                collectorSink,
                0);
        }

        topology->commit();
        POTHOS_TEST_TRUE(topology->waitInactive(0.01, 0.0));
    }

    auto expectedLabels = getExpectedLabels(inputs);
    const auto collectorSinkLabels = collectorSink.call<std::vector<Pothos::Label>>("getLabels");
    POTHOS_TEST_EQUAL(expectedLabels.size(), collectorSinkLabels.size());

    // TODO: use separate collector sinks to check expected indices
    for(const auto& expectedLabel: expectedLabels)
    {
        auto collectorSinkLabelIter = std::find_if(
            collectorSinkLabels.begin(),
            collectorSinkLabels.end(),
            [&expectedLabel](const Pothos::Label& collectorSinkLabel)
            {
                return (expectedLabel.id == collectorSinkLabel.id);
            });
        std::cout << "Testing label " << expectedLabel.id << std::endl;
        POTHOS_TEST_TRUE(collectorSinkLabels.end() != collectorSinkLabelIter);

        // Allow greater variance due to floating-point precision issues
        // propagating over many operations.
        POTHOS_TEST_CLOSE(
            expectedLabel.data.extract<double>(),
            collectorSinkLabelIter->data.extract<double>(),
            0.1);
    }
}
