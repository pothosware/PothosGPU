// Copyright (c) 2019 Nicholas Corgan
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

namespace PothosArrayFireTests
{

// Test this block by comparing the outputs to blocks that individually output
// the real and imaginary components.
static void testSplitComplexBlock(
    const std::string& type,
    size_t numChannels)
{
    static constexpr const char* blockRegistryPath = "/arrayfire/arith/split_complex";

    if(isDTypeFloat(Pothos::DType(type)))
    {
        std::cout << "Testing " << blockRegistryPath
                  << " (type: " << type
                  << ", chans: " << numChannels << ")" << std::endl;

        const std::string complexType = "complex_"+type;

        auto block = Pothos::BlockRegistry::make(
                         blockRegistryPath,
                         "Auto",
                         type,
                         numChannels);
        auto realBlock = Pothos::BlockRegistry::make(
                             "/arrayfire/arith/real",
                             "Auto",
                             type,
                             numChannels);
        auto imagBlock = Pothos::BlockRegistry::make(
                             "/arrayfire/arith/imag",
                             "Auto",
                             type,
                             numChannels);

        std::vector<Pothos::BufferChunk> testInputs;
        std::vector<Pothos::Proxy> feederSources;
        std::vector<Pothos::Proxy> collectorSinksReal;
        std::vector<Pothos::Proxy> collectorSinksImag;
        std::vector<Pothos::Proxy> collectorSinksRealBlock;
        std::vector<Pothos::Proxy> collectorSinksImagBlock;

        for(size_t chan = 0; chan < numChannels; ++chan)
        {
            POTHOS_TEST_EQUAL(
                complexType,
                block.call("input", chan).call("dtype").call<std::string>("name"));
            POTHOS_TEST_EQUAL(
                type,
                block.call("output", "re"+std::to_string(chan)).call("dtype").call<std::string>("name"));
            POTHOS_TEST_EQUAL(
                type,
                block.call("output", "im"+std::to_string(chan)).call("dtype").call<std::string>("name"));

            testInputs.emplace_back(getTestInputs(complexType));

            feederSources.emplace_back(
                Pothos::BlockRegistry::make(
                    "/blocks/feeder_source",
                    type));
            feederSources.back().call(
                "feedBuffer",
                testInputs.back());

            collectorSinksReal.emplace_back(
                Pothos::BlockRegistry::make(
                    "/blocks/collector_sink",
                    type));
            collectorSinksImag.emplace_back(
                Pothos::BlockRegistry::make(
                    "/blocks/collector_sink",
                    type));
            collectorSinksRealBlock.emplace_back(
                Pothos::BlockRegistry::make(
                    "/blocks/collector_sink",
                    type));
            collectorSinksImagBlock.emplace_back(
                Pothos::BlockRegistry::make(
                    "/blocks/collector_sink",
                    type));
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
                    feederSources[chan],
                    0,
                    realBlock,
                    chan);
                topology.connect(
                    feederSources[chan],
                    0,
                    imagBlock,
                    chan);

                topology.connect(
                    block,
                    "re"+std::to_string(chan),
                    collectorSinksReal[chan],
                    0);
                topology.connect(
                    block,
                    "im"+std::to_string(chan),
                    collectorSinksImag[chan],
                    0);

                topology.connect(
                    realBlock,
                    chan,
                    collectorSinksRealBlock[chan],
                    0);
                topology.connect(
                    imagBlock,
                    chan,
                    collectorSinksImagBlock[chan],
                    0);
            }

            topology.commit();
            POTHOS_TEST_TRUE(topology.waitInactive(0.05));
        }

        // Make sure the blocks output data.
        for(size_t chan = 0; chan < numChannels; ++chan)
        {
            std::cout << " * Testing re" << chan << "..." << std::endl;
            auto realOutputs = collectorSinksReal[chan].call<Pothos::BufferChunk>("getBuffer");
            auto realBlockOutputs = collectorSinksRealBlock[chan].call<Pothos::BufferChunk>("getBuffer");
            PothosArrayFireTests::testBufferChunk(
                realBlockOutputs,
                realOutputs);

            std::cout << " * Testing im" << chan << "..." << std::endl;
            auto imagOutputs = collectorSinksImag[chan].call<Pothos::BufferChunk>("getBuffer");
            auto imagBlockOutputs = collectorSinksImagBlock[chan].call<Pothos::BufferChunk>("getBuffer");
            PothosArrayFireTests::testBufferChunk(
                imagBlockOutputs,
                imagOutputs);
        }
    }
    else
    {
        POTHOS_TEST_THROWS(
            Pothos::BlockRegistry::make(
                blockRegistryPath,
                type,
                numChannels),
        Pothos::ProxyExceptionMessage);
    }
}

void testSplitComplexBlockForType(const std::string& type)
{
    testSplitComplexBlock(type, 1);
    testSplitComplexBlock(type, 3);
}

}
