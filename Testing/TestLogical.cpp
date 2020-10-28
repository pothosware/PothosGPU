// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <Poco/RandomStream.h>

#include <iostream>
#include <vector>

//
// Utility code
//

static constexpr size_t bufferLen = 4096;

template <typename T>
static Pothos::BufferChunk getTestInputs()
{
    Pothos::BufferChunk bufferChunk(typeid(T), bufferLen);
    Poco::RandomBuf randomBuf;
    randomBuf.readFromDevice(bufferChunk, bufferChunk.length);

    return bufferChunk;
}

template <typename T>
static T getRandomValue()
{
    T constant;
    Poco::RandomBuf randomBuf;
    randomBuf.readFromDevice((char*)&constant, sizeof(constant));

    return constant;
}

//
// Test implementations
//

template <typename T>
static void testLogicalArray()
{
    const Pothos::DType dtype(typeid(T));
    constexpr size_t numInputs = 3;

    std::cout << "Testing " << dtype.name() << "..." << std::endl;

    std::vector<Pothos::BufferChunk> inputs;
    for (size_t i = 0; i < numInputs; ++i) inputs.emplace_back(getTestInputs<T>());

    Pothos::BufferChunk expectedAndOutput("int8", bufferLen);
    Pothos::BufferChunk expectedOrOutput("int8", bufferLen);
    Pothos::BufferChunk expectedXOrOutput("int8", bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        expectedAndOutput.template as<std::int8_t*>()[elem] = (inputs[0].template as<T*>()[elem] &&
                                                               inputs[1].template as<T*>()[elem] &&
                                                               inputs[2].template as<T*>()[elem]) ? 1 : 0;
        expectedOrOutput.template as<std::int8_t*>()[elem]  = (inputs[0].template as<T*>()[elem] ||
                                                               inputs[1].template as<T*>()[elem] ||
                                                               inputs[2].template as<T*>()[elem]) ? 1 : 0;
    }

    std::vector<Pothos::Proxy> sources(numInputs);
    for (size_t input = 0; input < numInputs; ++input)
    {
        sources[input] = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
        sources[input].call("feedBuffer", inputs[input]);
    }

    auto andBlock = Pothos::BlockRegistry::make(
                        "/gpu/array/logical",
                        "Auto",
                        "And",
                        dtype,
                        numInputs);
    auto orBlock = Pothos::BlockRegistry::make(
                       "/gpu/array/logical",
                       "Auto",
                       "Or",
                       dtype,
                       numInputs);

    auto andSink = Pothos::BlockRegistry::make("/blocks/collector_sink", "int8");
    auto orSink = Pothos::BlockRegistry::make("/blocks/collector_sink", "int8");

    {
        Pothos::Topology topology;

        for (size_t input = 0; input < numInputs; ++input)
        {
            topology.connect(sources[input], 0, andBlock, input);
            topology.connect(sources[input], 0, orBlock, input);
        }

        topology.connect(andBlock, 0, andSink, 0);
        topology.connect(orBlock, 0, orSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    std::cout << " * Testing And..." << std::endl;
    GPUTests::testBufferChunk(
        expectedAndOutput,
        andSink.call<Pothos::BufferChunk>("getBuffer"));

    std::cout << " * Testing Or..." << std::endl;
    GPUTests::testBufferChunk(
        expectedOrOutput,
        orSink.call<Pothos::BufferChunk>("getBuffer"));
}

template <typename T>
static void testLogicalScalar()
{
    const Pothos::DType dtype(typeid(T));

    std::cout << "Testing " << dtype.name() << "..." << std::endl;

    auto input = getTestInputs<T>();
    const auto scalar = getRandomValue<T>();

    Pothos::BufferChunk expectedAndOutput("int8", bufferLen);
    Pothos::BufferChunk expectedOrOutput("int8", bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        expectedAndOutput.template as<std::int8_t*>()[elem] = (input.template as<T*>()[elem] && scalar) ? 1 : 0;
        expectedOrOutput.template as<std::int8_t*>()[elem]  = (input.template as<T*>()[elem] || scalar) ? 1 : 0;
    }

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    source.call("feedBuffer", input);

    auto andBlock = Pothos::BlockRegistry::make(
                        "/gpu/scalar/logical",
                        "Auto",
                        "And",
                        dtype,
                        scalar);
    POTHOS_TEST_EQUAL(
        scalar,
        andBlock.template call<T>("scalar"));

    auto orBlock = Pothos::BlockRegistry::make(
                       "/gpu/scalar/logical",
                       "Auto",
                       "Or",
                       dtype,
                       scalar);
    POTHOS_TEST_EQUAL(
        scalar,
        orBlock.template call<T>("scalar"));

    auto andSink = Pothos::BlockRegistry::make("/blocks/collector_sink", "int8");
    auto orSink = Pothos::BlockRegistry::make("/blocks/collector_sink", "int8");

    {
        Pothos::Topology topology;

        topology.connect(source, 0, andBlock, 0);
        topology.connect(source, 0, orBlock, 0);

        topology.connect(andBlock, 0, andSink, 0);
        topology.connect(orBlock, 0, orSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    std::cout << " * Testing And..." << std::endl;
    GPUTests::testBufferChunk(
        expectedAndOutput,
        andSink.call<Pothos::BufferChunk>("getBuffer"));

    std::cout << " * Testing Or..." << std::endl;
    GPUTests::testBufferChunk(
        expectedOrOutput,
        orSink.call<Pothos::BufferChunk>("getBuffer"));
}

//
// Tests
//

POTHOS_TEST_BLOCK("/gpu/tests", test_array_logical)
{
    testLogicalArray<std::int16_t>();
    testLogicalArray<std::int32_t>();
    testLogicalArray<std::int64_t>();
    testLogicalArray<std::uint8_t>();
    testLogicalArray<std::uint16_t>();
    testLogicalArray<std::uint32_t>();
    testLogicalArray<std::uint64_t>();
}

POTHOS_TEST_BLOCK("/gpu/tests", test_scalar_logical)
{
    testLogicalScalar<std::int16_t>();
    testLogicalScalar<std::int32_t>();
    testLogicalScalar<std::int64_t>();
    testLogicalScalar<std::uint8_t>();
    testLogicalScalar<std::uint16_t>();
    testLogicalScalar<std::uint32_t>();
    testLogicalScalar<std::uint64_t>();
}
