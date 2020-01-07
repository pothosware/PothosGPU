// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <iostream>
#include <string>
#include <vector>

static void testFlatBlock(
    const std::string& type,
    size_t numChannels)
{
    static constexpr const char* blockRegistryPath = "/arrayfire/data/flat";

    std::cout << "Testing " << blockRegistryPath
              << " (type: " << type
              << ", chans: " << numChannels << ")" << std::endl;

    auto block = Pothos::BlockRegistry::make(
                     blockRegistryPath,
                     type,
                     numChannels);

    std::vector<Pothos::BufferChunk> testInputs;
    std::vector<Pothos::Proxy> feederSources;

    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             type);

    for(size_t chan = 0; chan < numChannels; ++chan)
    {
        testInputs.emplace_back(getTestInputs(type));

        feederSources.emplace_back(
            Pothos::BlockRegistry::make(
                "/blocks/feeder_source",
                type));
        feederSources.back().call(
            "feedBuffer",
            testInputs[chan]);
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
        }

        topology.connect(
            block,
            0,
            collectorSink,
            0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.05));
    }

    // Make sure the blocks output data.
    // TODO: output verification
    auto output = collectorSink.call<Pothos::BufferChunk>("getBuffer");
    POTHOS_TEST_EQUAL(
        static_cast<int>(testInputs[0].elements() * testInputs.size()),
        output.elements());
}

void testFlatBlockForType(const std::string& type)
{
    testFlatBlock(type, 1);
    testFlatBlock(type, 3);
}
