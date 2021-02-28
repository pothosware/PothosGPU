// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"
#include "Utility.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <arrayfire.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

static constexpr const char* blockRegistryPath = "/gpu/arith/clamp";
static constexpr const char* pluginPath = "/blocks/gpu/arith/clamp";

#define GET_MINMAX_OBJECTS(typeStr, cType) \
    if(inputs.dtype.name() == typeStr) \
    { \
        auto inputsCopy = GPUTests::bufferChunkToStdVector<cType>(inputs); \
        std::sort(inputsCopy.begin(), inputsCopy.end()); \
 \
        (*pMinObjectOut) = Pothos::Object(inputsCopy[static_cast<size_t>(inputsCopy.size() * 0.25)]); \
        (*pMaxObjectOut) = Pothos::Object(inputsCopy[static_cast<size_t>(inputsCopy.size() * 0.75)]); \
    }

static void getMinMaxObjects(
    const Pothos::BufferChunk& inputs,
    Pothos::Object* pMinObjectOut,
    Pothos::Object* pMaxObjectOut)
{
    assert(nullptr != pMinObjectOut);
    assert(nullptr != pMaxObjectOut);

    GET_MINMAX_OBJECTS("int8", char)
    GET_MINMAX_OBJECTS("int16", short)
    GET_MINMAX_OBJECTS("int32", int)
    GET_MINMAX_OBJECTS("int64", long long)
    GET_MINMAX_OBJECTS("uint8", unsigned char)
    GET_MINMAX_OBJECTS("uint16", unsigned short)
    GET_MINMAX_OBJECTS("uint32", unsigned)
    GET_MINMAX_OBJECTS("uint64", unsigned long long)
    GET_MINMAX_OBJECTS("float32", float)
    GET_MINMAX_OBJECTS("float64", double)
    // This block has no complex implementation.
}

template <typename T>
static void testClampBlockOutput(
    const T* output,
    size_t numElems,
    T min,
    T max)
{
    for(size_t elem = 0; elem < numElems; ++elem)
    {
        POTHOS_TEST_GE(output[elem], min);
        POTHOS_TEST_LE(output[elem], max);
    }
}

#define TEST_OUTPUT_FOR_TYPE(typeStr, cType) \
    if(output.dtype.name() == typeStr) \
        testClampBlockOutput<cType>(output, output.elements(), minObject, maxObject);

static void testClampBlockOutput(
    const Pothos::BufferChunk& output,
    const Pothos::Object& minObject,
    const Pothos::Object& maxObject)
{
    TEST_OUTPUT_FOR_TYPE("int8", char)
    TEST_OUTPUT_FOR_TYPE("int16", short)
    TEST_OUTPUT_FOR_TYPE("int32", int)
    TEST_OUTPUT_FOR_TYPE("int64", long long)
    TEST_OUTPUT_FOR_TYPE("uint8", unsigned char)
    TEST_OUTPUT_FOR_TYPE("uint16", unsigned short)
    TEST_OUTPUT_FOR_TYPE("uint32", unsigned)
    TEST_OUTPUT_FOR_TYPE("uint64", unsigned long long)
    TEST_OUTPUT_FOR_TYPE("float32", float)
    TEST_OUTPUT_FOR_TYPE("float64", double)
    // This block has no complex implementation.
}

static void testClampBlockForType(const Pothos::DType& type)
{
    std::cout << "Testing " << blockRegistryPath
              << " (type: " << type.name() << ")" << std::endl;

    Pothos::Object minObject(0);
    Pothos::Object maxObject(0);

    Pothos::DType dtype(type);
    if(isDTypeComplexFloat(dtype))
    {
        POTHOS_TEST_THROWS(
            Pothos::BlockRegistry::make(
                blockRegistryPath,
                "Auto",
                type,
                minObject,
                maxObject),
        Pothos::ProxyExceptionMessage);
    }
    else
    {
        auto block = Pothos::BlockRegistry::make(
                         blockRegistryPath,
                         "Auto",
                         type,
                         minObject,
                         maxObject);

        auto testInputs = GPUTests::getTestInputs(type.name());
        getMinMaxObjects(testInputs, &minObject, &maxObject);

        block.call("setMaxValue", maxObject);
        block.call("setMinValue", minObject);

        auto feederSource = Pothos::BlockRegistry::make(
                                "/blocks/feeder_source",
                                dtype);
        feederSource.call("feedBuffer", testInputs);

        auto collectorSink = Pothos::BlockRegistry::make(
                                 "/blocks/collector_sink",
                                 dtype);

        // Execute the topology.
        {
            Pothos::Topology topology;
            topology.connect(feederSource, 0, block, 0);
            topology.connect(block, 0, collectorSink, 0);

            topology.commit();
            POTHOS_TEST_TRUE(topology.waitInactive(0.05));
        }

        auto output = collectorSink.call<Pothos::BufferChunk>("getBuffer");
        POTHOS_TEST_EQUAL(
            testInputs.elements(),
            output.elements());
        testClampBlockOutput(output, minObject, maxObject);
    }
}

POTHOS_TEST_BLOCK("/gpu/tests", test_clamp)
{
    GPUTests::setupTestEnv();

    for(const auto& type: GPUTests::getAllDTypes())
    {
        testClampBlockForType(type);
    }
}
