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

namespace GPUTests
{

// TODO: some of this can likely be consolidated with NToOne
template <typename In, typename Out>
void testTwoToOneBlockCommon(
    const Pothos::Proxy& block,
    bool removeZerosInBuffer1)
{
    static const Pothos::DType inputDType(typeid(In));
    static const Pothos::DType outputDType(typeid(Out));

    static constexpr size_t numInputChannels = 2;

    std::vector<Pothos::BufferChunk> testInputs(numInputChannels);
    std::vector<Pothos::Proxy> feederSources(numInputChannels);
    Pothos::Proxy collectorSink;

    for(size_t chan = 0; chan < numInputChannels; ++chan)
    {
        testInputs[chan] = getTestInputs(inputDType.toString());

        feederSources[chan] = Pothos::BlockRegistry::make(
                                  "/blocks/feeder_source",
                                  inputDType);
    }

    // If specified, remove any zeros from the second buffer.
    if(removeZerosInBuffer1)
    {
        static const In Zero(0);
        static const In One(1);

        auto& denom = testInputs[1];
        In* denomBuffer = denom.as<In*>();
        for(size_t i = 0; i < denom.elements(); ++i)
        {
            if(denomBuffer[i] == Zero) denomBuffer[i] = One;
        }
    }

    for(size_t chan = 0; chan < numInputChannels; ++chan)
    {
        feederSources[chan].call(
            "feedBuffer",
            testInputs[chan]);
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

    // Make sure the block outputs data.
    auto output = collectorSink.call<Pothos::BufferChunk>("getBuffer");
    POTHOS_TEST_EQUAL(
        testInputs[0].elements(),
        output.elements());
}

template <typename T>
void testTwoToOneBlock(
    const std::string& blockRegistryPath,
    bool removeZerosInBuffer1)
{
    static const Pothos::DType dtype(typeid(T));

    std::cout << "Testing " << blockRegistryPath << " (type: " << dtype.name() << ")" << std::endl;

    auto block = Pothos::BlockRegistry::make(
                     blockRegistryPath,
                     "Auto",
                     dtype);
    auto inputs = block.call<InputPortVector>("inputs");
    auto outputs = block.call<OutputPortVector>("outputs");
    POTHOS_TEST_EQUAL(2, inputs.size());
    POTHOS_TEST_EQUAL(1, outputs.size());

    testTwoToOneBlockCommon<T, T>(
        block,
        removeZerosInBuffer1);
}

template <typename T>
void testTwoToOneBlockF2C(
    const std::string& blockRegistryPath,
    bool removeZerosInBuffer1)
{
    static const Pothos::DType floatDType(typeid(T));
    static const Pothos::DType complexDType(typeid(std::complex<T>));

    std::cout << "Testing " << blockRegistryPath
                            << " (types: " << floatDType.name() << " -> " << complexDType.name() << ")" << std::endl;

    auto block = Pothos::BlockRegistry::make(
                     blockRegistryPath,
                     "Auto",
                     floatDType);
    auto inputs = block.call<InputPortVector>("inputs");
    auto outputs = block.call<OutputPortVector>("outputs");
    POTHOS_TEST_EQUAL(2, inputs.size());
    POTHOS_TEST_EQUAL(1, outputs.size());

    testTwoToOneBlockCommon<T, std::complex<T>>(
        block,
        removeZerosInBuffer1);
}

#define SPECIALIZE_TEMPLATE_TEST(T) \
    template \
    void testTwoToOneBlock<T>(const std::string&, bool);

#define SPECIALIZE_F2C_TEMPLATE_TEST(T) \
    template \
    void testTwoToOneBlockF2C<T>(const std::string&, bool);

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

SPECIALIZE_F2C_TEMPLATE_TEST(float)
SPECIALIZE_F2C_TEMPLATE_TEST(double)

}
