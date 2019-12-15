// Copyright (c) 2019 Nicholas Corgan
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
    const std::string& type2,
    size_t numChannels)
{
    static constexpr const char* blockRegistryPath = "/arrayfire/array/cast";

    std::cout << "Testing " << blockRegistryPath
              << " (types: " << type1 << " -> " << type2
              << ", chans: " << numChannels << ")" << std::endl;

    Pothos::DType inputDType(type1);
    Pothos::DType outputDType(type2);

    if(isDTypeComplexFloat(inputDType) && !outputDType.isComplex())
    {
        POTHOS_TEST_THROWS(
            Pothos::BlockRegistry::make(
                blockRegistryPath,
                type1,
                type2,
                numChannels),
        Pothos::ProxyExceptionMessage);
    }
    else
    {
        auto block = Pothos::BlockRegistry::make(
                         blockRegistryPath,
                         type1,
                         type2,
                         numChannels);

        std::vector<Pothos::BufferChunk> testInputs;
        std::vector<Pothos::Proxy> feederSources;
        std::vector<Pothos::Proxy> collectorSinks;

        for(size_t chan = 0; chan < numChannels; ++chan)
        {
            testInputs.emplace_back(getTestInputs(inputDType.name()));

            feederSources.emplace_back(
                Pothos::BlockRegistry::make(
                    "/blocks/feeder_source",
                    inputDType));
            feederSources.back().call(
                "feedBuffer",
                testInputs[chan]);

            collectorSinks.emplace_back(
                Pothos::BlockRegistry::make(
                    "/blocks/collector_sink",
                    outputDType));
        }

        // Execute the topology.
        {
            Pothos::Topology topology;
            for(size_t chan = 0; chan < numChannels; ++chan)
            {
                topology.connect(
                    feederSources[chan],
                    0,
                    block,
                    chan);
                topology.connect(
                    block,
                    chan,
                    collectorSinks[chan],
                    0);
            }

            topology.commit();
            POTHOS_TEST_TRUE(topology.waitInactive(0.05));
        }

        // Make sure the blocks output data.
        // TODO: output verification
        for(size_t chan = 0; chan < numChannels; ++chan)
        {
            const auto& chanInputs = testInputs[chan];
            const size_t numInputs = chanInputs.elements();

            auto chanOutputs = collectorSinks[chan].call<Pothos::BufferChunk>("getBuffer");
            POTHOS_TEST_EQUAL(
                numInputs,
                chanOutputs.elements());
        }
    }
}

void testCastBlockForType(const std::string& inputType)
{
    static const std::vector<std::string> AllTypes =
    {
        // ArrayFire doesn't support int8
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
    for(const auto& outputType: AllTypes)
    {
        testCastBlock(inputType, outputType, 1);
        testCastBlock(inputType, outputType, 3);
    }
}
