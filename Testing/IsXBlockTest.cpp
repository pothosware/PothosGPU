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
static EnableIfAnyInt<T, Pothos::BufferChunk> getIsXTestInputs()
{
    const std::vector<T> testInputs =
    {
        0,
        std::numeric_limits<T>::max(),
        std::numeric_limits<T>::min(),
        5,
        std::numeric_limits<T>::max() / 2,
    };
    return GPUTests::stdVectorToBufferChunk(testInputs);
}

template <typename T>
static EnableIfFloat<T, Pothos::BufferChunk> getIsXTestInputs()
{
    const std::vector<T> testInputs =
    {
        0.0,
        std::numeric_limits<T>::infinity(),
        -std::numeric_limits<T>::infinity(),
        5.0,
        std::numeric_limits<T>::quiet_NaN(),
    };
    return GPUTests::stdVectorToBufferChunk(testInputs);
}

template <typename T>
static void testIsX(
    const std::string& blockRegistryPath,
    const std::vector<char>& expectedOutput)
{
    static const Pothos::DType dtype(typeid(T));
    static const Pothos::DType Int8DType(typeid(char));

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

    GPUTests::testBufferChunk(
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
    const std::vector<char>& expectedOutput)
{
    testIsX<float>(blockRegistryPath, expectedOutput);
    testIsX<double>(blockRegistryPath, expectedOutput);

    testIsXBlockFailsForType<short>(blockRegistryPath);
    testIsXBlockFailsForType<int>(blockRegistryPath);
    testIsXBlockFailsForType<long long>(blockRegistryPath);
    testIsXBlockFailsForType<unsigned char>(blockRegistryPath);
    testIsXBlockFailsForType<unsigned short>(blockRegistryPath);
    testIsXBlockFailsForType<unsigned>(blockRegistryPath);
    testIsXBlockFailsForType<unsigned long long>(blockRegistryPath);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_isinf)
{
    GPUTests::setupTestEnv();

    testFloatOnlyBlock(
        "/gpu/arith/isinf",
        {0,1,1,0,0});
}

POTHOS_TEST_BLOCK("/gpu/tests", test_isnan)
{
    GPUTests::setupTestEnv();

    testFloatOnlyBlock(
        "/gpu/arith/isnan",
        {0,0,0,0,1});
}

POTHOS_TEST_BLOCK("/gpu/tests", test_iszero)
{
    GPUTests::setupTestEnv();

    const std::string blockRegistryPath = "/gpu/arith/iszero";
    const std::vector<char> signedOutput = {1,0,0,0,0};
    const std::vector<char> unsignedOutput = {1,0,1,0,0};

    // TODO: test complex
    testIsX<short>(blockRegistryPath, signedOutput);
    testIsX<int>(blockRegistryPath, signedOutput);
    testIsX<long long>(blockRegistryPath, signedOutput);
    testIsX<unsigned char>(blockRegistryPath, unsignedOutput);
    testIsX<unsigned short>(blockRegistryPath, unsignedOutput);
    testIsX<unsigned>(blockRegistryPath, unsignedOutput);
    testIsX<unsigned long long>(blockRegistryPath, unsignedOutput);
    testIsX<float>(blockRegistryPath, signedOutput);
    testIsX<double>(blockRegistryPath, signedOutput);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_sign)
{
    GPUTests::setupTestEnv();

    const std::string blockRegistryPath = "/gpu/arith/sign";
    const std::vector<char> expectedOutput = {0,0,1,0,0};

    // TODO: test complex
    testIsX<short>(blockRegistryPath, expectedOutput);
    testIsX<int>(blockRegistryPath, expectedOutput);
    testIsX<long long>(blockRegistryPath, expectedOutput);
    testIsX<float>(blockRegistryPath, expectedOutput);
    testIsX<double>(blockRegistryPath, expectedOutput);
}
