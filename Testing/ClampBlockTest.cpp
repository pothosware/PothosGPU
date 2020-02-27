// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <arrayfire.h>

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"
#include "Utility.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

static constexpr const char* blockRegistryPath = "/arrayfire/arith/clamp";
static constexpr const char* pluginPath = "/blocks/arrayfire/arith/clamp";

#define GET_MINMAX_OBJECTS(typeStr, cType) \
    if(type == typeStr) \
    { \
        const auto sortedInputs = PothosArrayFireTests::getTestInputs<cType>(false /*shuffle*/); \
        assert(sortedInputs.size() >= 6); \
 \
        (*pMinObjectOut) = Pothos::Object(sortedInputs[2]); \
        (*pMaxObjectOut) = Pothos::Object(sortedInputs[sortedInputs.size()-3]); \
    }

static void getMinMaxObjects(
    const std::string& type,
    Pothos::Object* pMinObjectOut,
    Pothos::Object* pMaxObjectOut)
{
    assert(nullptr != pMinObjectOut);
    assert(nullptr != pMaxObjectOut);

    // ArrayFire doesn't support int8
    GET_MINMAX_OBJECTS("int16", std::int16_t)
    GET_MINMAX_OBJECTS("int32", std::int32_t)
    GET_MINMAX_OBJECTS("int64", std::int64_t)
    GET_MINMAX_OBJECTS("uint8", std::uint8_t)
    GET_MINMAX_OBJECTS("uint16", std::uint16_t)
    GET_MINMAX_OBJECTS("uint32", std::uint32_t)
    GET_MINMAX_OBJECTS("uint64", std::uint64_t)
    GET_MINMAX_OBJECTS("float32", float)
    GET_MINMAX_OBJECTS("float64", double)
    // This block has no complex implementation.
}

void testClampBlockForType(const std::string& type)
{
    std::cout << "Testing " << blockRegistryPath
              << " (type: " << type << ")" << std::endl;

    Pothos::Object minObject, maxObject;
    getMinMaxObjects(type, &minObject, &maxObject);

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

        auto testInputs = PothosArrayFireTests::getTestInputs(type);

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

        // TODO: output verification
        auto output = collectorSink.call<Pothos::BufferChunk>("getBuffer");
        POTHOS_TEST_EQUAL(
            testInputs.elements(),
            output.elements());
    }
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_clamp)
{
    PothosArrayFireTests::setupTestEnv();

    for(const auto& type: PothosArrayFireTests::getAllDTypeNames())
    {
        testClampBlockForType(type);
    }
}
