// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "BlockExecutionTests.hpp"
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
    static constexpr const char* blockRegistryPath = "/arrayfire/array/cast";

    std::cout << "Testing " << blockRegistryPath
              << " (types: " << type1 << " -> " << type2 << ")" << std::endl;

    Pothos::DType inputDType(type1);
    Pothos::DType outputDType(type2);

    if(isDTypeComplexFloat(inputDType) && !outputDType.isComplex())
    {
        POTHOS_TEST_THROWS(
            Pothos::BlockRegistry::make(
                blockRegistryPath,
                "Auto",
                type1,
                type2),
        Pothos::ProxyExceptionMessage);
    }
    else
    {
        auto block = Pothos::BlockRegistry::make(
                         blockRegistryPath,
                         "Auto",
                         type1,
                         type2);

        auto testInputs = AFTests::getTestInputs(inputDType.name());

        auto feederSource = Pothos::BlockRegistry::make(
                                "/blocks/feeder_source",
                                inputDType);
        feederSource.call("feedBuffer", testInputs);

        auto collectorSink = Pothos::BlockRegistry::make(
                                 "/blocks/collector_sink",
                                 outputDType);

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
