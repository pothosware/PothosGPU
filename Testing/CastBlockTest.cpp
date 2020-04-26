// Copyright (c) 2019-2020 Nicholas Corgan
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

static void addCornerCases(
    const std::string& type1,
    const std::string& type2,
    Pothos::BufferChunk& inputBuffer)
{
    POTHOS_TEST_GE(inputBuffer.elements(), 9);

    if((("float32" == type1) || ("complex_float32" == type1)) && Pothos::DType(type2).isFloat())
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
    else if(("float64" == type1) && ("complex_float64" == type2))
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
    const std::string& type1,
    const std::string& type2)
{
    static constexpr const char* afCastRegistryPath = "/arrayfire/array/cast";

    std::cout << "Testing " << afCastRegistryPath
              << " (types: " << type1 << " -> " << type2 << ")" << std::endl;

    Pothos::DType inputDType(type1);
    Pothos::DType outputDType(type2);

    if(isDTypeComplexFloat(inputDType) && !outputDType.isComplex())
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
                                   outputDType);

        auto testInputs = AFTests::getTestInputs(inputDType.name());
        addCornerCases(type1, type2, testInputs);

        auto feederSource = Pothos::BlockRegistry::make(
                                "/blocks/feeder_source",
                                inputDType);
        feederSource.call("feedBuffer", testInputs);

        auto afCollectorSink = Pothos::BlockRegistry::make(
                                   "/blocks/collector_sink",
                                   outputDType);
        auto pothosCollectorSink = Pothos::BlockRegistry::make(
                                       "/blocks/collector_sink",
                                       outputDType);

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
        AFTests::testBufferChunk(
            pothosOutput,
            afOutput);
    }
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_cast)
{
    AFTests::setupTestEnv();

    const auto& dtypeNames = AFTests::getAllDTypeNames();

    for(const auto& inputType: dtypeNames)
    {
        for(const auto& outputType: dtypeNames)
        {
            testCastBlock(inputType, outputType);
        }
    }
}
