// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <iostream>
#include <limits>
#include <string>
#include <vector>

// TODO: complex overload

template <typename T>
static AFTests::EnableIfAnyInt<T, Pothos::BufferChunk> getIsXTestInputs()
{
    const std::vector<T> testInputs =
    {
        0,
        std::numeric_limits<T>::max(),
        std::numeric_limits<T>::min(),
        5,
        std::numeric_limits<T>::max() / 2,
    };
    return AFTests::stdVectorToBufferChunk(testInputs);
}

template <typename T>
static AFTests::EnableIfFloat<T, Pothos::BufferChunk> getIsXTestInputs()
{
    const std::vector<T> testInputs =
    {
        0.0,
        std::numeric_limits<T>::infinity(),
        -std::numeric_limits<T>::infinity(),
        5.0,
        std::numeric_limits<T>::quiet_NaN(),
    };
    return AFTests::stdVectorToBufferChunk(testInputs);
}

template <typename T>
static void testIsX(
    const std::string& blockRegistryPath,
    const std::vector<std::int8_t>& expectedOutput)
{
    static const Pothos::DType dtype(typeid(T));
    static const Pothos::DType Int8DType(typeid(std::int8_t));

    std::cout << "Testing " << dtype.name() << std::endl;

    auto feederSource = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    feederSource.call("feedBuffer", getIsXTestInputs<T>());

    auto testBlock = Pothos::BlockRegistry::make(
                         blockRegistryPath,
                         "Auto",
                         dtype);

    auto collectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", Int8DType);

    {
        Pothos::Topology topology;

        topology.connect(feederSource, 0, testBlock, 0);
        topology.connect(testBlock, 0, collectorSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.05));
    }

    AFTests::testBufferChunk(
        collectorSink.call("getBuffer"),
        expectedOutput);
}

template <typename T>
static void testIsXBlockFailsForType(const std::string& blockRegistryPath)
{
    static const Pothos::DType dtype(typeid(T));

    POTHOS_TEST_THROWS(
        Pothos::BlockRegistry::make(
            blockRegistryPath,
            "Auto",
            dtype),
    Pothos::ProxyExceptionMessage);
}

static void testFloatOnlyBlock(
    const std::string& blockRegistryPath,
    const std::vector<std::int8_t>& expectedOutput)
{
    testIsX<float>(blockRegistryPath, expectedOutput);
    testIsX<double>(blockRegistryPath, expectedOutput);

    testIsXBlockFailsForType<std::int16_t>(blockRegistryPath);
    testIsXBlockFailsForType<std::int32_t>(blockRegistryPath);
    testIsXBlockFailsForType<std::int64_t>(blockRegistryPath);
    testIsXBlockFailsForType<std::uint8_t>(blockRegistryPath);
    testIsXBlockFailsForType<std::uint16_t>(blockRegistryPath);
    testIsXBlockFailsForType<std::uint32_t>(blockRegistryPath);
    testIsXBlockFailsForType<std::uint64_t>(blockRegistryPath);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_isinf)
{
    AFTests::setupTestEnv();

    testFloatOnlyBlock(
        "/arrayfire/arith/isinf",
        {0,1,1,0,0});
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_isnan)
{
    AFTests::setupTestEnv();

    testFloatOnlyBlock(
        "/arrayfire/arith/isnan",
        {0,0,0,0,1});
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_iszero)
{
    AFTests::setupTestEnv();

    const std::string blockRegistryPath = "/arrayfire/arith/iszero";
    const std::vector<std::int8_t> signedOutput = {1,0,0,0,0};
    const std::vector<std::int8_t> unsignedOutput = {1,0,1,0,0};

    // TODO: test complex
    testIsX<std::int16_t>(blockRegistryPath, signedOutput);
    testIsX<std::int32_t>(blockRegistryPath, signedOutput);
    testIsX<std::int64_t>(blockRegistryPath, signedOutput);
    testIsX<std::uint8_t>(blockRegistryPath, unsignedOutput);
    testIsX<std::uint16_t>(blockRegistryPath, unsignedOutput);
    testIsX<std::uint32_t>(blockRegistryPath, unsignedOutput);
    testIsX<std::uint64_t>(blockRegistryPath, unsignedOutput);
    testIsX<float>(blockRegistryPath, signedOutput);
    testIsX<double>(blockRegistryPath, signedOutput);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_sign)
{
    AFTests::setupTestEnv();

    const std::string blockRegistryPath = "/arrayfire/arith/sign";
    const std::vector<std::int8_t> expectedOutput = {0,0,1,0,0};

    // TODO: test complex
    testIsX<std::int16_t>(blockRegistryPath, expectedOutput);
    testIsX<std::int32_t>(blockRegistryPath, expectedOutput);
    testIsX<std::int64_t>(blockRegistryPath, expectedOutput);
    testIsX<float>(blockRegistryPath, expectedOutput);
    testIsX<double>(blockRegistryPath, expectedOutput);
}
