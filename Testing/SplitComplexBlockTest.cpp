// Copyright (c) 2019-2020 Nicholas Corgan
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
void testSplitComplexBlockForType(const std::string& type)
{
    static constexpr const char* blockRegistryPath = "/arrayfire/arith/split_complex";

    if(isDTypeFloat(Pothos::DType(type)))
    {
        std::cout << "Testing " << blockRegistryPath
                  << " (type: " << type << ")" << std::endl;

        const std::string complexType = "complex_"+type;

        auto block = Pothos::BlockRegistry::make(
                         blockRegistryPath,
                         "Auto",
                         type);
        auto realBlock = Pothos::BlockRegistry::make(
                             "/arrayfire/arith/real",
                             "Auto",
                             type);
        auto imagBlock = Pothos::BlockRegistry::make(
                             "/arrayfire/arith/imag",
                             "Auto",
                             type);

        auto testInputs = getTestInputs(complexType);

        auto feederSource = Pothos::BlockRegistry::make(
                                "/blocks/feeder_source",
                                complexType);
        feederSource.call("feedBuffer", testInputs);

        auto collectorSinkReal = Pothos::BlockRegistry::make(
                                     "/blocks/collector_sink",
                                     type);
        auto collectorSinkImag = Pothos::BlockRegistry::make(
                                     "/blocks/collector_sink",
                                     type);
        auto collectorSinkRealBlock = Pothos::BlockRegistry::make(
                                          "/blocks/collector_sink",
                                          type);
        auto collectorSinkImagBlock = Pothos::BlockRegistry::make(
                                          "/blocks/collector_sink",
                                          type);

        // Execute the topology.
        {
            Pothos::Topology topology;

            topology.connect(feederSource, 0, block, 0);
            topology.connect(feederSource, 0, realBlock, 0);
            topology.connect(feederSource, 0, imagBlock, 0);

            topology.connect(block, "re", collectorSinkReal, 0);
            topology.connect(block, "im", collectorSinkImag, 0);
            topology.connect(realBlock, 0, collectorSinkRealBlock, 0);
            topology.connect(imagBlock, 0, collectorSinkImagBlock, 0);

            topology.commit();
            POTHOS_TEST_TRUE(topology.waitInactive(0.05));
        }

        // Make sure the blocks output data.
        std::cout << " * Testing re..." << std::endl;
        auto realOutputs = collectorSinkReal.call<Pothos::BufferChunk>("getBuffer");
        auto realBlockOutputs = collectorSinkRealBlock.call<Pothos::BufferChunk>("getBuffer");
        PothosArrayFireTests::testBufferChunk(
            realBlockOutputs,
            realOutputs);

        std::cout << " * Testing im..." << std::endl;
        auto imagOutputs = collectorSinkImag.call<Pothos::BufferChunk>("getBuffer");
        auto imagBlockOutputs = collectorSinkImagBlock.call<Pothos::BufferChunk>("getBuffer");
        PothosArrayFireTests::testBufferChunk(
            imagBlockOutputs,
            imagOutputs);
    }
    else
    {
        POTHOS_TEST_THROWS(
            Pothos::BlockRegistry::make(
                blockRegistryPath,
                type),
        Pothos::ProxyExceptionMessage);
    }
}

}
