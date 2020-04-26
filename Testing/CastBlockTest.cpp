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
#include <string>
#include <typeinfo>
#include <vector>

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
