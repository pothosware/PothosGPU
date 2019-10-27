// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <typeinfo>

static std::random_device rd;
static std::mt19937 g(rd());

template <typename T>
static EnableIfInteger<T, std::vector<T>> getTestInputs()
{
    static constexpr T minValue = std::is_same<T, std::int8_t>::value ? T(-5) : T(-25);
    static constexpr size_t numInputs = std::is_same<T, std::int8_t>::value ? 11 : 51;

    auto testParams = getIntTestParams<T>(minValue, T(1), numInputs);
    std::shuffle(testParams.begin(), testParams.end(), g);

    return testParams;
}

template <typename T>
static EnableIfUnsignedInt<T, std::vector<T>> getTestInputs()
{
    static constexpr T minValue = std::is_same<T, std::uint8_t>::value ? T(5) : T(25);
    static constexpr size_t numInputs = std::is_same<T, std::uint8_t>::value ? 9 : 76;

    auto testParams = getIntTestParams<T>(minValue, T(1), numInputs);
    std::shuffle(testParams.begin(), testParams.end(), g);

    return testParams;
}

template <typename T>
static EnableIfFloat<T, std::vector<T>> getTestInputs()
{
    // To not have nice even numbers
    static constexpr size_t numInputs = 123;

    auto testParams = linspace<T>(-20.0f, 20.0f, numInputs);
    std::shuffle(testParams.begin(), testParams.end(), g);

    return testParams;
}

template <typename T>
static EnableIfComplex<T, std::vector<T>> getTestInputs()
{
    using Scalar = typename T::value_type;

    // To not have nice even numbers
    static constexpr size_t numInputs = 246;

    auto testParams = toComplexVector(linspace<Scalar>(10.0f, 20.0f, numInputs));
    std::shuffle(testParams.begin(), testParams.end(), g);

    return testParams;
}

using PortInfoVector = std::vector<Pothos::PortInfo>;

template <typename T>
void testOneToOneBlock(
    const std::string& blockRegistryPath,
    size_t numChannels,
    const UnaryFunc<T>& verificationFunc)
{
    static const Pothos::DType dtype(typeid(T));

    std::cout << "Testing " << blockRegistryPath << " (type: " << dtype.name()
                            << ", " << "chans: " << numChannels << ")" << std::endl;

    auto block = Pothos::BlockRegistry::make(
                     blockRegistryPath,
                     dtype,
                     numChannels);
    auto inputPortInfo = block.call<PortInfoVector>("inputPortInfo");
    auto outputPortInfo = block.call<PortInfoVector>("outputPortInfo");
    POTHOS_TEST_EQUAL(numChannels, inputPortInfo.size());
    POTHOS_TEST_EQUAL(numChannels, outputPortInfo.size());

    std::vector<std::vector<T>> testInputs(numChannels);
    std::vector<Pothos::Proxy> feederSources;
    std::vector<Pothos::Proxy> collectorSinks;

    for(size_t chan = 0; chan < numChannels; ++chan)
    {
        testInputs[chan] = getTestInputs<T>();

        feederSources.emplace_back(
            Pothos::BlockRegistry::make(
                "/blocks/feeder_source",
                dtype));
        feederSources.back().call(
            "feedBuffer",
            stdVectorToBufferChunk<T>(
                dtype,
                testInputs[chan]));

        collectorSinks.emplace_back(
            Pothos::BlockRegistry::make(
                "/blocks/collector_sink",
                dtype));
    }

    // Execute the topology.
    {
        Pothos::Topology topology;
        for(size_t chan = 0; chan < numChannels; ++chan)
        {
            topology.connect(
                feederSources[chan],
                0,
                block,
                chan);
            topology.connect(
                block,
                chan,
                collectorSinks[chan],
                0);
        }

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.05));
    }

    // Make sure the blocks output data and, if the caller provided a
    // verification function, that the outputs are valid.
    for(size_t chan = 0; chan < numChannels; ++chan)
    {
        const auto& chanInputs = testInputs[chan];
        const size_t numInputs = chanInputs.size();

        auto chanOutputs = collectorSinks[chan].call<Pothos::BufferChunk>("getBuffer");
        POTHOS_TEST_EQUAL(
            numInputs,
            chanOutputs.elements());
        if(nullptr != verificationFunc)
        {
            std::vector<T> expectedOutputs;
            std::transform(
                chanInputs.begin(),
                chanInputs.end(),
                std::back_inserter(expectedOutputs),
                verificationFunc);

            testBufferChunk<T>(
                chanOutputs,
                expectedOutputs);
        }
    }
}

#define SPECIALIZE_TEMPLATE_TESTS(T) \
    template \
    void testOneToOneBlock<T>( \
        const std::string& blockRegistryPath, \
        size_t numChannels, \
        const UnaryFunc<T>& verificationFunc);

SPECIALIZE_TEMPLATE_TESTS(std::int8_t)
SPECIALIZE_TEMPLATE_TESTS(std::int16_t)
SPECIALIZE_TEMPLATE_TESTS(std::int32_t)
SPECIALIZE_TEMPLATE_TESTS(std::int64_t)
SPECIALIZE_TEMPLATE_TESTS(std::uint8_t)
SPECIALIZE_TEMPLATE_TESTS(std::uint16_t)
SPECIALIZE_TEMPLATE_TESTS(std::uint32_t)
SPECIALIZE_TEMPLATE_TESTS(std::uint64_t)
SPECIALIZE_TEMPLATE_TESTS(float)
SPECIALIZE_TEMPLATE_TESTS(double)
SPECIALIZE_TEMPLATE_TESTS(std::complex<float>)
SPECIALIZE_TEMPLATE_TESTS(std::complex<double>)

// TODO: auto-generate
POTHOS_TEST_BLOCK("/arrayfire/tests", test_block_execution)
{
    testOneToOneBlock<float>(
        "/arrayfire/arith/abs",
        1,
        [](const float& val){return std::abs(val);});
    testOneToOneBlock<float>(
        "/arrayfire/arith/abs",
        3,
        [](const float& val){return std::abs(val);});
}
