// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <arrayfire.h>

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"
#include "Utility.hpp"

#include <iostream>
#include <string>
#include <vector>

static const std::string blockRegistryPath = "/arrayfire/data/replace";

static void testReplaceBlockForType(const std::string& type)
{
    std::cout << "Testing " << blockRegistryPath
              << " (type: " << type << ")..." << std::endl;
    
    const bool isValidDType = (std::string::npos == type.find("int64"));

    if(isValidDType)
    {
        auto replace = Pothos::BlockRegistry::make(
                           blockRegistryPath,
                           "Auto",
                           type,
                           0,
                           0);

        // TODO: output validation
        auto feederSource = Pothos::BlockRegistry::make("/blocks/feeder_source", type);
        feederSource.call("feedBuffer", PothosArrayFireTests::getTestInputs(type));
        
        auto collectorSink = Pothos::BlockRegistry::make("/blocks/collector_sink", type);

        {
            Pothos::Topology topology;

            topology.connect(
                feederSource, 0,
                replace, 0);
            topology.connect(
                replace, 0,
                collectorSink, 0);

            topology.commit();
            POTHOS_TEST_TRUE(topology.waitInactive());
        }
    }
    else
    {
        POTHOS_TEST_THROWS(
            Pothos::BlockRegistry::make(
                blockRegistryPath,
                "Auto",
                type,
                0,
                0),
            Pothos::ProxyExceptionMessage);
    }
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_replace)
{
    PothosArrayFireTests::setupTestEnv();

    for(const auto& type: PothosArrayFireTests::getAllDTypeNames())
    {
        testReplaceBlockForType(type);
    }
}
