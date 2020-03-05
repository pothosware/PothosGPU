// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <arrayfire.h>

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"
#include "Utility.hpp"

#include <iostream>
#include <string>
#include <vector>

//
// Test implementations
//

static void testArrayOpBlock(
    const std::string& blockRegistryPath,
    const std::string& inputDType,
    const std::string& outputDType,
    const std::string& operation,
    size_t nchans)
{
    if(inputDType == outputDType)
    {
        std::cout << "Testing " << operation
                  << " (" << inputDType << ", "
                  << nchans << " chans)..." << std::endl;
    }
    else
    {
        std::cout << "Testing " << operation
                  << " (" << inputDType <<  " -> " << outputDType << ", "
                  << nchans << " chans)..." << std::endl;

    }

    auto arrayOpBlock = Pothos::BlockRegistry::make(
                               blockRegistryPath,
                               "Auto",
                               operation,
                               inputDType,
                               nchans);

    std::vector<Pothos::BufferChunk> allTestInputs;
    std::vector<Pothos::Proxy> feederSources;
    std::vector<Pothos::Proxy> collectorSinks;
    for(size_t chan = 0; chan < nchans; ++chan)
    {
        allTestInputs.emplace_back(PothosArrayFireTests::getTestInputs(inputDType));

        feederSources.emplace_back(Pothos::BlockRegistry::make(
                                       "/blocks/feeder_source",
                                       inputDType));
        feederSources.back().call("feedBuffer", allTestInputs.back());

        collectorSinks.emplace_back(Pothos::BlockRegistry::make(
                                        "/blocks/collector_sink",
                                        outputDType));
    }

    {
        Pothos::Topology topology;

        for(size_t chan = 0; chan < nchans; ++chan)
        {
            topology.connect(
                feederSources[chan], 0,
                arrayOpBlock, chan);
            topology.connect(
                arrayOpBlock, chan,
                collectorSinks[chan], 0);
        }

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }
}

//
// Registered tests
//

POTHOS_TEST_BLOCK("/arrayfire/tests", test_array_arithmetic)
{
    const auto& allDTypeNames = PothosArrayFireTests::getAllDTypeNames();
    const std::vector<std::string> allOperations = {"Add", "Subtract", "Multiply", "Divide"};

    for(const auto& dtype: allDTypeNames)
    {
        for(const auto& operation: allOperations)
        {
            testArrayOpBlock(
                "/arrayfire/array/arithmetic",
                dtype,
                dtype,
                operation,
                3);
        }
    }
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_array_bitwise)
{
    const std::vector<std::string> validDTypeNames = {"int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64"};
    const std::vector<std::string> allOperations = {"And", "Or", "XOr", "Left Shift", "Right Shift"};

    for(const auto& dtype: validDTypeNames)
    {
        for(const auto& operation: allOperations)
        {
            testArrayOpBlock(
                "/arrayfire/array/bitwise",
                dtype,
                dtype,
                operation,
                (operation.find("Shift") == std::string::npos) ? 3 : 2);
        }
    }
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_array_logical)
{
    const std::vector<std::string> validDTypeNames = {"int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64"};
    const std::vector<std::string> allOperations = {"And", "Or"};

    for(const auto& dtype: validDTypeNames)
    {
        for(const auto& operation: allOperations)
        {
            testArrayOpBlock(
                "/arrayfire/array/logical",
                dtype,
                "int8",
                operation,
                3);
        }
    }
}
