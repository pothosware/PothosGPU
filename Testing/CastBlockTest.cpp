// Copyright (c) 2019-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <typeinfo>
#include <vector>

// To avoid collisions
namespace
{

static void addCornerCases(
    const Pothos::DType& type1,
    const Pothos::DType& type2,
    Pothos::BufferChunk& inputBuffer)
{
    POTHOS_TEST_GE(inputBuffer.elements(), 9);

    if((("float32" == type1.name()) || ("complex_float32" == type1.name())) && type2.isFloat())
    {
        inputBuffer.as<float*>()[0] = std::numeric_limits<float>::min();
        inputBuffer.as<float*>()[1] = std::numeric_limits<float>::max();
        inputBuffer.as<float*>()[2] = std::numeric_limits<float>::infinity();
        inputBuffer.as<float*>()[3] = -std::numeric_limits<float>::infinity();
        inputBuffer.as<float*>()[4] = std::numeric_limits<float>::lowest();
        inputBuffer.as<float*>()[5] = std::numeric_limits<float>::epsilon();
        inputBuffer.as<float*>()[6] = std::nexttoward(std::numeric_limits<float>::min(), 0.0f);
        inputBuffer.as<float*>()[7] = std::nexttoward(std::numeric_limits<float>::max(), 0.0f);
        inputBuffer.as<float*>()[8] = std::numeric_limits<float>::denorm_min();
    }
    else if(("float64" == type1.name()) && ("complex_float64" == type2.name()))
    {
        inputBuffer.as<double*>()[0] = std::numeric_limits<double>::min();
        inputBuffer.as<double*>()[1] = std::numeric_limits<double>::max();
        inputBuffer.as<double*>()[2] = std::numeric_limits<double>::infinity();
        inputBuffer.as<double*>()[3] = -std::numeric_limits<double>::infinity();
        inputBuffer.as<double*>()[4] = std::numeric_limits<double>::lowest();
        inputBuffer.as<double*>()[5] = std::numeric_limits<double>::epsilon();
        inputBuffer.as<double*>()[6] = std::nexttoward(std::numeric_limits<double>::min(), 0.0f);
        inputBuffer.as<double*>()[7] = std::nexttoward(std::numeric_limits<double>::max(), 0.0f);
        inputBuffer.as<double*>()[8] = std::numeric_limits<double>::denorm_min();
    }
}

static void testCastBlock(
    const Pothos::DType& type1,
    const Pothos::DType& type2)
{
    static constexpr const char* afCastRegistryPath = "/gpu/array/cast";

    std::cout << "Testing " << afCastRegistryPath
              << " (types: " << type1.name() << " -> " << type2.name() << ")" << std::endl;

    if(PothosGPU::isDTypeComplexFloat(type1) && !type2.isComplex())
    {
        POTHOS_TEST_THROWS(
            Pothos::BlockRegistry::make(
                afCastRegistryPath,
                "Auto",
                type1,
                type2),
        Pothos::ProxyExceptionMessage);
    }
    else
    {
        auto afCast = Pothos::BlockRegistry::make(
                          afCastRegistryPath,
                          "Auto",
                          type1,
                          type2);

        auto pothosConverter = Pothos::BlockRegistry::make(
                                   "/blocks/converter",
                                   type2);

        auto testInputs = GPUTests::getTestInputs(type1.name());
        addCornerCases(type1, type2, testInputs);

        auto feederSource = Pothos::BlockRegistry::make(
                                "/blocks/feeder_source",
                                type1);
        feederSource.call("feedBuffer", testInputs);

        auto afCollectorSink = Pothos::BlockRegistry::make(
                                   "/blocks/collector_sink",
                                   type2);
        auto pothosCollectorSink = Pothos::BlockRegistry::make(
                                       "/blocks/collector_sink",
                                       type2);

        // Execute the topology.
        {
            Pothos::Topology topology;
            topology.connect(feederSource, 0, afCast, 0);
            topology.connect(afCast, 0, afCollectorSink, 0);

            topology.connect(feederSource, 0, pothosConverter, 0);
            topology.connect(pothosConverter, 0, pothosCollectorSink, 0);

            topology.commit();
            POTHOS_TEST_TRUE(topology.waitInactive(0.05));
        }

        // This block is meant to be a faster version of Pothos's converter
        // block, so we need to make sure the outputs match.
        auto afOutput = afCollectorSink.call<Pothos::BufferChunk>("getBuffer");
        auto pothosOutput = pothosCollectorSink.call<Pothos::BufferChunk>("getBuffer");
        POTHOS_TEST_EQUAL(
            testInputs.elements(),
            afOutput.elements());
        GPUTests::testBufferChunk(
            pothosOutput,
            afOutput);
    }
}

POTHOS_TEST_BLOCK("/gpu/tests", test_cast)
{
    GPUTests::setupTestEnv();

    const auto& dtypes = GPUTests::getAllDTypes();

    for(const auto& inputType: dtypes)
    {
        for(const auto& outputType: dtypes)
        {
            testCastBlock(inputType, outputType);
        }
    }
}

}
