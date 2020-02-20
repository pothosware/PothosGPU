// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <typeinfo>

namespace PothosArrayFireTests
{

template <typename In, typename Out>
static std::vector<Out> getExpectedOutputs(
    const std::vector<std::vector<In>>& inputs,
    const BinaryFunc<In, Out>& verificationFunc)
{
    const size_t numInputs = inputs[0].size();

    std::vector<Out> expectedOutputs = inputs[0];
    for(size_t chan = 1; chan < inputs.size(); ++chan)
    {
        const auto& chanInputs = inputs[chan];
        POTHOS_TEST_EQUAL(chanInputs.size(), expectedOutputs.size());
        for(size_t elem = 0; elem < numInputs; ++elem)
        {
            expectedOutputs[elem] = verificationFunc(
                                        expectedOutputs[elem],
                                        chanInputs[elem]);
        }
    }

    return expectedOutputs;
}

// TODO: TwoToOneBlocks can likely use this code
template <typename In, typename Out>
static void testNToOneBlockCommon(
    const Pothos::Proxy& block,
    size_t numInputChannels,
    const BinaryFunc<In, Out>& verificationFunc,
    bool removeZerosInBuffer1)
{
    static const Pothos::DType inputDType(typeid(In));
    static const Pothos::DType outputDType(typeid(Out));

    std::vector<std::vector<In>> testInputs(numInputChannels);
    std::vector<Pothos::Proxy> feederSources(numInputChannels);
    Pothos::Proxy collectorSink;

    for(size_t chan = 0; chan < numInputChannels; ++chan)
    {
        testInputs[chan] = getTestInputs<In>();

        feederSources[chan] = Pothos::BlockRegistry::make(
                                  "/blocks/feeder_source",
                                  inputDType);
    }

    // If specified, remove any zeros from the second buffer, which
    // ends up being a denominator. Resize the numerator to match.
    if(removeZerosInBuffer1)
    {
        static const In Zero(0);

        auto& denom = testInputs[1];
        if(denom.end() != std::find(std::begin(denom), std::end(denom), Zero))
        {
            testInputs[1].erase(
                std::remove(
                    std::begin(denom),
                    std::end(denom),
                    Zero));
            testInputs[0].resize(denom.size());
        }
    }

    POTHOS_TEST_TRUE(!testInputs[0].empty());
    POTHOS_TEST_EQUAL(testInputs[0].size(), testInputs[1].size());

    for(size_t chan = 0; chan < numInputChannels; ++chan)
    {
        feederSources[chan].call(
            "feedBuffer",
            stdVectorToBufferChunk<In>(testInputs[chan]));
    }

    collectorSink = Pothos::BlockRegistry::make(
                        "/blocks/collector_sink",
                        outputDType);

    // Execute the topology.
    {
        Pothos::Topology topology;
        for(size_t chan = 0; chan < numInputChannels; ++chan)
        {
            topology.connect(
                feederSources[chan],
                0,
                block,
                chan);
        }

        topology.connect(
            block,
            0,
            collectorSink,
            0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.05));
    }

    // Make sure the blocks output data and, if the caller provided a
    // verification function, that the outputs are valid.
    auto output = collectorSink.call<Pothos::BufferChunk>("getBuffer");
    POTHOS_TEST_EQUAL(
        testInputs[0].size(),
        output.elements());
    if((nullptr != verificationFunc) && ("CPU" == block.call<std::string>("getArrayFireBackend")))
    {
        auto expectedOutputs = getExpectedOutputs(
                                   testInputs,
                                   verificationFunc);
        testBufferChunk<Out>(
            output,
            expectedOutputs);
    }
}

template <typename T>
void testNToOneBlock(
    const std::string& blockRegistryPath,
    size_t numInputChannels,
    const BinaryFunc<T, T>& verificationFunc)
{
    static const Pothos::DType dtype(typeid(T));

    std::cout << "Testing " << blockRegistryPath
              << " (type: " << dtype.name()
              << ", nchans: " << numInputChannels
              << ")" << std::endl;

    auto block = Pothos::BlockRegistry::make(
                     blockRegistryPath,
                     "Auto",
                     dtype,
                     numInputChannels);
    auto inputs = block.call<InputPortVector>("inputs");
    auto outputs = block.call<OutputPortVector>("outputs");
    POTHOS_TEST_EQUAL(numInputChannels, inputs.size());
    POTHOS_TEST_EQUAL(1, outputs.size());

    testNToOneBlockCommon<T, T>(
        block,
        numInputChannels,
        verificationFunc,
        false /*removeZerosInBuffer1*/);
}

#define SPECIALIZE_TEMPLATE_TEST(T) \
    template \
    void testNToOneBlock<T>( \
        const std::string& blockRegistryPath, \
        size_t numChannels, \
        const BinaryFunc<T, T>& verificationFunc);

SPECIALIZE_TEMPLATE_TEST(std::int8_t)
SPECIALIZE_TEMPLATE_TEST(std::int16_t)
SPECIALIZE_TEMPLATE_TEST(std::int32_t)
SPECIALIZE_TEMPLATE_TEST(std::int64_t)
SPECIALIZE_TEMPLATE_TEST(std::uint8_t)
SPECIALIZE_TEMPLATE_TEST(std::uint16_t)
SPECIALIZE_TEMPLATE_TEST(std::uint32_t)
SPECIALIZE_TEMPLATE_TEST(std::uint64_t)
SPECIALIZE_TEMPLATE_TEST(float)
SPECIALIZE_TEMPLATE_TEST(double)
SPECIALIZE_TEMPLATE_TEST(std::complex<float>)
SPECIALIZE_TEMPLATE_TEST(std::complex<double>)

}
