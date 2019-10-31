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
#include <string>
#include <typeinfo>

template <typename In, typename Out>
static std::vector<Out> getExpectedOutputs(
    const std::vector<In>& inputs0,
    const std::vector<In>& inputs1,
    const BinaryFunc<In, Out>& verificationFunc)
{
    POTHOS_TEST_EQUAL(inputs0.size(), inputs1.size());
    const size_t numInputs = inputs0.size();

    std::vector<Out> expectedOutputs(numInputs);
    for(size_t elem = 0; elem < numInputs; ++elem)
    {
        expectedOutputs[elem] = verificationFunc(inputs0[elem], inputs1[elem]);
    }

    return expectedOutputs;
}

template <typename In, typename Out>
void testTwoToOneBlockCommon(
    const Pothos::Proxy& block,
    const BinaryFunc<In, Out>& verificationFunc)
{
    static const Pothos::DType inputDType(typeid(In));
    static const Pothos::DType outputDType(typeid(Out));

    static constexpr size_t numInputChannels = 2;

    std::vector<std::vector<In>> testInputs(numInputChannels);
    std::vector<Pothos::Proxy> feederSources(numInputChannels);
    Pothos::Proxy collectorSink;

    for(size_t chan = 0; chan < numInputChannels; ++chan)
    {
        testInputs[chan] = getTestInputs<In>();

        feederSources[chan] = Pothos::BlockRegistry::make(
                                  "/blocks/feeder_source",
                                  inputDType);
        feederSources[chan].call(
            "feedBuffer",
            stdVectorToBufferChunk<In>(
                inputDType,
                testInputs[chan]));
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
    if(nullptr != verificationFunc)
    {
        auto expectedOutputs = getExpectedOutputs(
                                   testInputs[0],
                                   testInputs[1],
                                   verificationFunc);
        testBufferChunk<Out>(
            output,
            expectedOutputs);
    }
}

template <typename T>
void testTwoToOneBlock(
    const std::string& blockRegistryPath,
    const BinaryFunc<T, T>& verificationFunc)
{
    static const Pothos::DType dtype(typeid(T));

    std::cout << "Testing " << blockRegistryPath << " (type: " << dtype.name() << ")" << std::endl;

    auto block = Pothos::BlockRegistry::make(
                     blockRegistryPath,
                     dtype);
    auto inputPortInfo = block.call<PortInfoVector>("inputPortInfo");
    auto outputPortInfo = block.call<PortInfoVector>("outputPortInfo");
    POTHOS_TEST_EQUAL(2, inputPortInfo.size());
    POTHOS_TEST_EQUAL(1, outputPortInfo.size());

    testTwoToOneBlockCommon<T, T>(
        block,
        verificationFunc);
}

template <typename In, typename Out>
void testTwoToOneBlock(
    const std::string& blockRegistryPath,
    const BinaryFunc<In, Out>& verificationFunc)
{
    static const Pothos::DType inputDType(typeid(In));
    static const Pothos::DType outputDType(typeid(Out));

    std::cout << "Testing " << blockRegistryPath
                            << " (types: " << inputDType.name() << " -> " << outputDType.name() << std::endl;

    auto block = Pothos::BlockRegistry::make(
                     blockRegistryPath,
                     inputDType,
                     outputDType);
    auto inputPortInfo = block.call<PortInfoVector>("inputPortInfo");
    auto outputPortInfo = block.call<PortInfoVector>("outputPortInfo");
    POTHOS_TEST_EQUAL(2, inputPortInfo.size());
    POTHOS_TEST_EQUAL(1, outputPortInfo.size());

    testTwoToOneBlockCommon<In, Out>(
        block,
        verificationFunc);
}

#define SPECIALIZE_TEMPLATE_TEST(T) \
    template \
    void testTwoToOneBlock<T>( \
        const std::string& blockRegistryPath, \
        const BinaryFunc<T, T>& verificationFunc);

/*
#define SPECIALIZE_COMPLEX_TEMPLATE_TEST(T) \
    template \
    void testTwoToOneBlock<T, std::complex<T>>( \
        const std::string& blockRegistryPath, \
        const BinaryFunc<T, std::complex<T>>& verificationFunc); \
    template \
    void testTwoToOneBlock<std::complex<T>, T>( \
        const std::string& blockRegistryPath, \
        const BinaryFunc<std::complex<T>, T>& verificationFunc);
*/

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

//SPECIALIZE_COMPLEX_TEMPLATE_TEST(float)
//SPECIALIZE_COMPLEX_TEMPLATE_TEST(double)
