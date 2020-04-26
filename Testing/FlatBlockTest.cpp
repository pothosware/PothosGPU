// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

static Pothos::BufferChunk concatBufferChunks(
    const std::vector<Pothos::BufferChunk>& bufferChunks)
{
    Pothos::BufferChunk ret;
    std::for_each(
        bufferChunks.begin(),
        bufferChunks.end(),
        [&ret](const Pothos::BufferChunk& bufferChunk)
        {
            ret.append(bufferChunk);
        });

    return ret;
}

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
                     "Auto",
                     type,
                     numChannels);

    std::vector<Pothos::BufferChunk> testInputs;
    std::vector<Pothos::Proxy> feederSources;

    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             type);

    for(size_t chan = 0; chan < numChannels; ++chan)
    {
        testInputs.emplace_back(AFTests::getTestInputs(type));

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

    AFTests::testBufferChunk(
        concatBufferChunks(testInputs),
        collectorSink.call<Pothos::BufferChunk>("getBuffer"));
}

static void testFlatBlockForType(const std::string& type)
{
    testFlatBlock(type, 1);
    testFlatBlock(type, 3);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_flat)
{
    AFTests::setupTestEnv();

    for(const auto& type: AFTests::getAllDTypeNames())
    {
        testFlatBlockForType(type);
    }
}
