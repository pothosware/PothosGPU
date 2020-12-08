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
static void testBitwiseNot()
{
    const Pothos::DType dtype(typeid(T));

    auto input = getTestInputs<T>();
    Pothos::BufferChunk expectedOutput(typeid(T), input.elements());
    for (size_t elem = 0; elem < expectedOutput.elements(); ++elem)
    {
        expectedOutput.template as<T*>()[elem] = ~input.template as<const T*>()[elem];
    }

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto notBlock = Pothos::BlockRegistry::make("/gpu/array/bitwise_not", "Auto", dtype);
    auto sink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    source.call("feedBuffer", input);

    {
        Pothos::Topology topology;

        topology.connect(source, 0, notBlock, 0);
        topology.connect(notBlock, 0, sink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    std::cout << " * Testing Not..." << std::endl;
    GPUTests::testBufferChunk(
        expectedOutput,
        sink.call<Pothos::BufferChunk>("getBuffer"));
}

template <typename T>
static void testBitwiseArray()
{
    const Pothos::DType dtype(typeid(T));
    constexpr size_t numInputs = 3;

    std::cout << "Testing " << dtype.name() << "..." << std::endl;

    testBitwiseNot<T>();

    std::vector<Pothos::BufferChunk> inputs;
    for (size_t i = 0; i < numInputs; ++i) inputs.emplace_back(getTestInputs<T>());

    Pothos::BufferChunk expectedAndOutput(dtype, bufferLen);
    Pothos::BufferChunk expectedOrOutput(dtype, bufferLen);
    Pothos::BufferChunk expectedXOrOutput(dtype, bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        expectedAndOutput.template as<T*>()[elem] = inputs[0].template as<T*>()[elem] & inputs[1].template as<T*>()[elem] & inputs[2].template as<T*>()[elem];
        expectedOrOutput.template as<T*>()[elem] = inputs[0].template as<T*>()[elem] | inputs[1].template as<T*>()[elem] | inputs[2].template as<T*>()[elem];
        expectedXOrOutput.template as<T*>()[elem] = inputs[0].template as<T*>()[elem] ^ inputs[1].template as<T*>()[elem] ^ inputs[2].template as<T*>()[elem];
    }

    std::vector<Pothos::Proxy> sources(numInputs);
    for (size_t input = 0; input < numInputs; ++input)
    {
        sources[input] = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
        sources[input].call("feedBuffer", inputs[input]);
    }

    auto andBlock = Pothos::BlockRegistry::make(
                        "/gpu/array/bitwise",
                        "Auto",
                        "And",
                        dtype,
                        numInputs);
    auto orBlock = Pothos::BlockRegistry::make(
                       "/gpu/array/bitwise",
                       "Auto",
                       "Or",
                       dtype,
                       numInputs);
    auto xorBlock = Pothos::BlockRegistry::make(
                        "/gpu/array/bitwise",
                        "Auto",
                        "XOr",
                        dtype,
                        numInputs);

    auto andSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto orSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto xorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        for (size_t input = 0; input < numInputs; ++input)
        {
            topology.connect(sources[input], 0, andBlock, input);
            topology.connect(sources[input], 0, orBlock, input);
            topology.connect(sources[input], 0, xorBlock, input);
        }

        topology.connect(andBlock, 0, andSink, 0);
        topology.connect(orBlock, 0, orSink, 0);
        topology.connect(xorBlock, 0, xorSink, 0);

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

    std::cout << " * Testing XOr..." << std::endl;
    GPUTests::testBufferChunk(
        expectedXOrOutput,
        xorSink.call<Pothos::BufferChunk>("getBuffer"));
}

template <typename T>
static void testBitwiseScalar()
{
    const Pothos::DType dtype(typeid(T));

    std::cout << "Testing " << dtype.name() << "..." << std::endl;

    auto input = getTestInputs<T>();
    const auto scalar = getRandomValue<T>();

    Pothos::BufferChunk expectedAndOutput(dtype, bufferLen);
    Pothos::BufferChunk expectedOrOutput(dtype, bufferLen);
    Pothos::BufferChunk expectedXOrOutput(dtype, bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        expectedAndOutput.template as<T*>()[elem] = input.template as<T*>()[elem] & scalar;
        expectedOrOutput.template as<T*>()[elem] = input.template as<T*>()[elem] | scalar;
        expectedXOrOutput.template as<T*>()[elem] = input.template as<T*>()[elem] ^ scalar;
    }

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    source.call("feedBuffer", input);

    auto andBlock = Pothos::BlockRegistry::make(
                        "/gpu/scalar/bitwise",
                        "Auto",
                        "And",
                        dtype,
                        scalar);
    POTHOS_TEST_EQUAL(
        scalar,
        andBlock.template call<T>("scalar"));

    auto orBlock = Pothos::BlockRegistry::make(
                       "/gpu/scalar/bitwise",
                       "Auto",
                       "Or",
                       dtype,
                       scalar);
    POTHOS_TEST_EQUAL(
        scalar,
        orBlock.template call<T>("scalar"));

    auto xorBlock = Pothos::BlockRegistry::make(
                        "/gpu/scalar/bitwise",
                        "Auto",
                        "XOr",
                        dtype,
                        scalar);
    POTHOS_TEST_EQUAL(
        scalar,
        xorBlock.template call<T>("scalar"));

    auto andSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto orSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto xorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        topology.connect(source, 0, andBlock, 0);
        topology.connect(source, 0, orBlock, 0);
        topology.connect(source, 0, xorBlock, 0);

        topology.connect(andBlock, 0, andSink, 0);
        topology.connect(orBlock, 0, orSink, 0);
        topology.connect(xorBlock, 0, xorSink, 0);

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

    std::cout << " * Testing XOr..." << std::endl;
    GPUTests::testBufferChunk(
        expectedXOrOutput,
        xorSink.call<Pothos::BufferChunk>("getBuffer"));
}

template <typename T>
static void testBitShift()
{
    const Pothos::DType dtype(typeid(T));

    std::cout << "Testing " << dtype.name() << "..." << std::endl;

    auto input = getTestInputs<T>();
    constexpr size_t leftShiftSize = ((sizeof(T) * 8) / 2);
    constexpr size_t rightShiftSize = ((sizeof(T) * 8) - 1);

    Pothos::BufferChunk expectedLeftShiftOutput(dtype, bufferLen);
    Pothos::BufferChunk expectedRightShiftOutput(dtype, bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        expectedLeftShiftOutput.template as<T*>()[elem] = input.template as<T*>()[elem] << leftShiftSize;
        expectedRightShiftOutput.template as<T*>()[elem] = input.template as<T*>()[elem] >> rightShiftSize;
    }

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    source.call("feedBuffer", input);

    auto leftShift = Pothos::BlockRegistry::make(
                         "/gpu/scalar/bitshift",
                         "Auto",
                         dtype,
                         "Left Shift",
                         leftShiftSize);
    auto leftShiftSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    POTHOS_TEST_EQUAL(leftShiftSize, leftShift.call<size_t>("shiftSize"));

    auto rightShift = Pothos::BlockRegistry::make(
                          "/gpu/scalar/bitshift",
                          "Auto",
                          dtype,
                          "Right Shift",
                          rightShiftSize);
    auto rightShiftSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    POTHOS_TEST_EQUAL(rightShiftSize, rightShift.call<size_t>("shiftSize"));

    {
        Pothos::Topology topology;

        topology.connect(source, 0, leftShift, 0);
        topology.connect(leftShift, 0, leftShiftSink, 0);

        topology.connect(source, 0, rightShift, 0);
        topology.connect(rightShift, 0, rightShiftSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    std::cout << " * Testing Left Shift..." << std::endl;
    GPUTests::testBufferChunk(
        expectedLeftShiftOutput,
        leftShiftSink.call<Pothos::BufferChunk>("getBuffer"));

    std::cout << " * Testing Right Shift..." << std::endl;
    GPUTests::testBufferChunk(
        expectedRightShiftOutput,
        rightShiftSink.call<Pothos::BufferChunk>("getBuffer"));
}

//
// Tests
//

POTHOS_TEST_BLOCK("/gpu/tests", test_array_bitwise)
{
    testBitwiseArray<short>();
    testBitwiseArray<int>();
    testBitwiseArray<long long>();
    testBitwiseArray<unsigned char>();
    testBitwiseArray<unsigned short>();
    testBitwiseArray<unsigned>();
    testBitwiseArray<unsigned long long>();
}

POTHOS_TEST_BLOCK("/gpu/tests", test_scalar_bitwise)
{
    testBitwiseScalar<short>();
    testBitwiseScalar<int>();
    testBitwiseScalar<long long>();
    testBitwiseScalar<unsigned char>();
    testBitwiseScalar<unsigned short>();
    testBitwiseScalar<unsigned>();
    testBitwiseScalar<unsigned long long>();
}

POTHOS_TEST_BLOCK("/gpu/tests", test_bitshift)
{
    testBitShift<short>();
    testBitShift<int>();
    testBitShift<unsigned char>();
    testBitShift<unsigned short>();
    testBitShift<unsigned>();
    testBitShift<unsigned long long>();
}
