// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <cstring>
#include <iostream>
#include <vector>

template <typename T>
static void getTestValues(
    Pothos::BufferChunk* pInputs,
    Pothos::BufferChunk* pExpectedRoundOutputs,
    Pothos::BufferChunk* pExpectedCeilOutputs,
    Pothos::BufferChunk* pExpectedFloorOutputs,
    Pothos::BufferChunk* pExpectedTruncOutputs)
{
    const auto inputs = std::vector<T>
    {
        -1.0, -0.75, -0.5, -0.25, 0.0, 0.25, 0.5, 1.0
    };
    const auto expectedRoundOutputs = std::vector<T>
    {
        -1.0, -1.0, -1.0, 0.0, 0.0, 0.0, 1.0, 1.0
    };
    const auto expectedCeilOutputs = std::vector<T>
    {
        -1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0
    };
    const auto expectedFloorOutputs = std::vector<T>
    {
        -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 0.0, 1.0
    };
    const auto expectedTruncOutputs = std::vector<T>
    {
        -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0
    };

    *pInputs = GPUTests::stdVectorToBufferChunk(inputs);
    *pExpectedRoundOutputs = GPUTests::stdVectorToBufferChunk(expectedRoundOutputs);
    *pExpectedCeilOutputs = GPUTests::stdVectorToBufferChunk(expectedCeilOutputs);
    *pExpectedFloorOutputs = GPUTests::stdVectorToBufferChunk(expectedFloorOutputs);
    *pExpectedTruncOutputs = GPUTests::stdVectorToBufferChunk(expectedTruncOutputs);
}

template <typename T>
static void testRoundBlocks()
{
    const Pothos::DType dtype(typeid(T));

    std::cout << " * Testing " << dtype.name() << "..." << std::endl;

    Pothos::BufferChunk inputs;
    Pothos::BufferChunk expectedRoundOutputs;
    Pothos::BufferChunk expectedCeilOutputs;
    Pothos::BufferChunk expectedFloorOutputs;
    Pothos::BufferChunk expectedTruncOutputs;
    getTestValues<T>(
        &inputs,
        &expectedRoundOutputs,
        &expectedCeilOutputs,
        &expectedFloorOutputs,
        &expectedTruncOutputs);

    auto feederSource = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    feederSource.call("feedBuffer", inputs);

    auto round = Pothos::BlockRegistry::make("/gpu/arith/round", "Auto", dtype);
    auto ceil  = Pothos::BlockRegistry::make("/gpu/arith/ceil", "Auto", dtype);
    auto floor = Pothos::BlockRegistry::make("/gpu/arith/floor", "Auto", dtype);
    auto trunc = Pothos::BlockRegistry::make("/gpu/arith/trunc", "Auto", dtype);

    auto roundCollectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto ceilCollectorSink  = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto floorCollectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto truncCollectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        topology.connect(feederSource, 0, round, 0);
        topology.connect(feederSource, 0, ceil, 0);
        topology.connect(feederSource, 0, floor, 0);
        topology.connect(feederSource, 0, trunc, 0);

        topology.connect(round, 0, roundCollectorSink, 0);
        topology.connect(ceil, 0, ceilCollectorSink, 0);
        topology.connect(floor, 0, floorCollectorSink, 0);
        topology.connect(trunc, 0, truncCollectorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    std::cout << "   * Testing /gpu/arith/round..." << std::endl;
    GPUTests::testBufferChunk(
        expectedRoundOutputs,
        roundCollectorSink.call("getBuffer"));

    std::cout << "   * Testing /gpu/arith/ceil..." << std::endl;
    GPUTests::testBufferChunk(
        expectedCeilOutputs,
        ceilCollectorSink.call("getBuffer"));

    std::cout << "   * Testing /gpu/arith/floor..." << std::endl;
    GPUTests::testBufferChunk(
        expectedFloorOutputs,
        floorCollectorSink.call("getBuffer"));

    std::cout << "   * Testing /gpu/arith/trunc..." << std::endl;
    GPUTests::testBufferChunk(
        expectedTruncOutputs,
        truncCollectorSink.call("getBuffer"));
}

POTHOS_TEST_BLOCK("/gpu/tests", test_round_blocks)
{
    testRoundBlocks<float>();
    testRoundBlocks<double>();
}
