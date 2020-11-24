// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <Poco/TemporaryFile.h>

#include <arrayfire.h>

#include <iostream>
#include <string>
#include <vector>

namespace GPUTests
{

static const std::vector<std::string> AllTypes =
{
    "int16",
    "int32",
    "int64",
    "uint8",
    "uint16",
    "uint32",
    "uint64",
    "float32",
    "float64",
    "complex_float32",
    "complex_float64"
};
struct TestData
{
    Pothos::DType dtype;

    std::string oneDimKey;
    std::string twoDimKey;
    af::array oneDimArray;
    af::array twoDimArray;
};

static std::string generateTestFile(const std::vector<TestData>& allTestData)
{
    Poco::TemporaryFile tempFile;
    tempFile.keepUntilExit();

    const std::string& path = tempFile.path();
    for(const auto& testData: allTestData)
    {
        af::saveArray(
            testData.oneDimKey.c_str(),
            testData.oneDimArray,
            path.c_str(),
            true /*append*/);
        af::saveArray(
            testData.twoDimKey.c_str(),
            testData.twoDimArray,
            path.c_str(),
            true /*append*/);
    }

    return path;
}

static void testFileSource1D(
    const std::string& filepath,
    const TestData& testData)
{
    std::cout << "Testing " << testData.dtype.name()
              << " (chans: 1)..." << std::endl;

    auto oneDimBlock = Pothos::BlockRegistry::make(
                           "/gpu/array/file_source",
                           filepath,
                           testData.oneDimKey,
                           false /*repeat*/);
    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             testData.dtype);

    POTHOS_TEST_EQUAL(
        filepath,
        oneDimBlock.call<std::string>("filepath"));
    POTHOS_TEST_EQUAL(
        testData.oneDimKey,
        oneDimBlock.call<std::string>("key"));
    POTHOS_TEST_FALSE(oneDimBlock.call<bool>("repeat"));
    POTHOS_TEST_EQUAL(0, oneDimBlock.call<InputPortVector>("inputs").size());

    const auto outputs = oneDimBlock.call<OutputPortVector>("outputs");
    POTHOS_TEST_EQUAL(1, outputs.size());
    POTHOS_TEST_EQUAL(
        testData.dtype.name(),
        outputs[0]->dtype().name());

    // Execute the topology.
    {
        Pothos::Topology topology;
        topology.connect(
            oneDimBlock,
            0,
            collectorSink,
            0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.05));
    }

    const auto bufferChunk = collectorSink.call<Pothos::BufferChunk>("getBuffer");
    compareAfArrayToBufferChunk(
        testData.oneDimArray,
        bufferChunk);
}

static void testFileSource2D(
    const std::string& filepath,
    const TestData& testData)
{
    const auto nchans = static_cast<size_t>(testData.twoDimArray.dims(0));

    std::cout << "Testing " << testData.dtype.name()
              << " (chans: " << nchans << ")..." << std::endl;

    auto twoDimBlock = Pothos::BlockRegistry::make(
                           "/gpu/array/file_source",
                           filepath,
                           testData.twoDimKey,
                           false /*repeat*/);
    std::vector<Pothos::Proxy> collectorSinks;

    POTHOS_TEST_EQUAL(
        filepath,
        twoDimBlock.call<std::string>("filepath"));
    POTHOS_TEST_EQUAL(
        testData.twoDimKey,
        twoDimBlock.call<std::string>("key"));
    POTHOS_TEST_FALSE(twoDimBlock.call<bool>("repeat"));
    POTHOS_TEST_EQUAL(0, twoDimBlock.call<InputPortVector>("inputs").size());

    const auto outputs = twoDimBlock.call<OutputPortVector>("outputs");
    POTHOS_TEST_EQUAL(nchans, outputs.size());
    for(const auto* output: outputs)
    {
        POTHOS_TEST_EQUAL(
            testData.dtype.name(),
            output->dtype().name());

        collectorSinks.emplace_back(Pothos::BlockRegistry::make(
                                        "/blocks/collector_sink",
                                        testData.dtype));
    }

    // Execute the topology.
    {
        Pothos::Topology topology;
        for(size_t chan = 0; chan < nchans; ++chan)
        {
            topology.connect(
                twoDimBlock,
                chan,
                collectorSinks[chan],
                0);
        }

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.05));
    }

    for(size_t chan = 0; chan < nchans; ++chan)
    {
        const auto bufferChunk = collectorSinks[chan].call<Pothos::BufferChunk>("getBuffer");
        compareAfArrayToBufferChunk(
            testData.twoDimArray.row(chan),
            bufferChunk);
    }
}

}

POTHOS_TEST_BLOCK("/gpu/tests", test_file_source)
{
    using namespace GPUTests;

    setupTestEnv();

    constexpr dim_t numChannels = 4;
    constexpr dim_t numElements = 50;

    std::vector<TestData> allTestData;
    for(const std::string& type: AllTypes)
    {
        auto afDType = Pothos::Object(type).convert<af::dtype>();

        TestData testData{
            Pothos::DType(type),
            ("1d_" + type),
            ("2d_" + type),
            af::randu(numElements, afDType),
            af::randu(numChannels, numElements, afDType)
        };
        GPUTests::addMinMaxToAfArray(
            testData.oneDimArray,
            type);
        GPUTests::addMinMaxToAfArray(
            testData.twoDimArray,
            type);

        allTestData.emplace_back(std::move(testData));
    }

    const auto testDataFilepath = generateTestFile(allTestData);

    for(const auto& testData: allTestData)
    {
        testFileSource1D(
            testDataFilepath,
            testData);
        testFileSource2D(
            testDataFilepath,
            testData);
    }
}
