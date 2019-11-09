// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <Poco/Thread.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <string>
#include <typeinfo>

static float mean(const std::vector<size_t>& inputVec)
{
    return static_cast<float>(std::accumulate(
               inputVec.begin(),
               inputVec.end(),
               0.0f))
           / static_cast<float>(inputVec.size());
}

static float collectorSinksMean(const std::vector<Pothos::Proxy>& collectorSinks)
{
    std::vector<size_t> outputSizes;
    std::transform(
        collectorSinks.begin(),
        collectorSinks.end(),
        std::back_inserter(outputSizes),
        [](const Pothos::Proxy& collectorSink)
        {
            return collectorSink.call<Pothos::BufferChunk>("getBuffer").elements();
        });

    return mean(outputSizes);
}

static void testOptimizedOneToOneBlock(
    size_t numChannels,
    const std::string& block0Path,
    const std::string& block1Path,
    const std::string& block2Path,
    bool isScalar)
{
    static const Pothos::DType dtype(typeid(float));

    Pothos::Proxy block00, block01, block1, block1Opt, block2, block2Opt;
    if(isScalar)
    {
        static const float scalar0 = 1.0;
        static const float scalar1 = 2.0;
        static const float scalar2 = 3.0;

        block00 = Pothos::BlockRegistry::make(
                      block0Path,
                      dtype,
                      scalar0,
                      numChannels);
        block01 = Pothos::BlockRegistry::make(
                      block0Path,
                      dtype,
                      scalar0,
                      numChannels);
        block1 = Pothos::BlockRegistry::make(
                      block1Path,
                      dtype,
                      scalar1,
                      numChannels);
        block1Opt = Pothos::BlockRegistry::make(
                      block1Path,
                      dtype,
                      scalar1,
                      numChannels);
        block2 = Pothos::BlockRegistry::make(
                      block2Path,
                      dtype,
                      scalar2,
                      numChannels);
        block2Opt = Pothos::BlockRegistry::make(
                      block2Path,
                      dtype,
                      scalar2,
                      numChannels);
    }
    else
    {
        block00 = Pothos::BlockRegistry::make(
                      block0Path,
                      dtype,
                      numChannels);
        block01 = Pothos::BlockRegistry::make(
                      block0Path,
                      dtype,
                      numChannels);
        block1 = Pothos::BlockRegistry::make(
                      block1Path,
                      dtype,
                      numChannels);
        block1Opt = Pothos::BlockRegistry::make(
                      block1Path,
                      dtype,
                      numChannels);
        block2 = Pothos::BlockRegistry::make(
                      block2Path,
                      dtype,
                      numChannels);
        block2Opt = Pothos::BlockRegistry::make(
                      block2Path,
                      dtype,
                      numChannels);
    }

    block1Opt.call("setBlockAssumesArrayFireInputs", true);
    POTHOS_TEST_TRUE(block1Opt.call<bool>("getBlockAssumesArrayFireInputs"));
    block2Opt.call("setBlockAssumesArrayFireInputs", true);
    POTHOS_TEST_TRUE(block2Opt.call<bool>("getBlockAssumesArrayFireInputs"));

    std::vector<Pothos::Proxy> noiseSources(numChannels);
    std::vector<Pothos::Proxy> collectorSinks(numChannels);
    std::vector<Pothos::Proxy> collectorSinksOpt(numChannels);
    for(size_t chan = 0; chan < numChannels; ++chan)
    {
        noiseSources[chan] = Pothos::BlockRegistry::make(
                                 "/comms/noise_source",
                                 dtype);
        collectorSinks[chan] = Pothos::BlockRegistry::make(
                                   "/blocks/collector_sink",
                                   dtype);
        collectorSinksOpt[chan] = Pothos::BlockRegistry::make(
                                      "/blocks/collector_sink",
                                      dtype);
    }

    std::cout << "Testing with " << numChannels << " channels..." << std::endl;

    size_t unoptimizedAverage = 0;
    size_t optimizedAverage = 0;
    do
    {
        // Execute the topology.
        {
            Pothos::Topology topology;
            for(size_t chan = 0; chan < numChannels; ++chan)
            {
                topology.connect(
                    noiseSources[chan],
                    0,
                    block00,
                    chan);
                topology.connect(
                    noiseSources[chan],
                    0,
                    block01,
                    chan);

                topology.connect(
                    block00,
                    chan,
                    block1,
                    chan);
                topology.connect(
                    block1,
                    chan,
                    block2,
                    chan);
                topology.connect(
                    block2,
                    chan,
                    collectorSinks[chan],
                    0);

                topology.connect(
                    block01,
                    chan,
                    block1Opt,
                    chan);
                topology.connect(
                    block1Opt,
                    chan,
                    block2Opt,
                    chan);
                topology.connect(
                    block2Opt,
                    chan,
                    collectorSinksOpt[chan],
                    0);
            }

            // When this block exits, the flowgraph will stop.
            topology.commit();

            Poco::Thread::sleep(1000);
        }

        unoptimizedAverage = collectorSinksMean(collectorSinks);
        optimizedAverage = collectorSinksMean(collectorSinksOpt);
    } while((0 == unoptimizedAverage) || (0 == optimizedAverage));

    std::cout << unoptimizedAverage << " vs " << optimizedAverage << std::endl;
}

//
// Actual tests
//

POTHOS_TEST_BLOCK("/arrayfire/tests", test_optimized_one_to_one_block)
{
    const std::string block0Path = "/arrayfire/arith/sin";
    const std::string block1Path = "/arrayfire/arith/abs";
    const std::string block2Path = "/arrayfire/arith/cos";

    testOptimizedOneToOneBlock(2, block0Path, block1Path, block2Path, false /*isScalar*/);
    testOptimizedOneToOneBlock(4, block0Path, block1Path, block2Path, false /*isScalar*/);
    testOptimizedOneToOneBlock(6, block0Path, block1Path, block2Path, false /*isScalar*/);
    testOptimizedOneToOneBlock(8, block0Path, block1Path, block2Path, false /*isScalar*/);
    testOptimizedOneToOneBlock(10, block0Path, block1Path, block2Path, false /*isScalar*/);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_optimized_scalar_op_block)
{
    const std::string block0Path = "/arrayfire/scalar/add";
    const std::string block1Path = "/arrayfire/scalar/subtract";
    const std::string block2Path = "/arrayfire/scalar/multiply";

    testOptimizedOneToOneBlock(2, block0Path, block1Path, block2Path, true /*isScalar*/);
    testOptimizedOneToOneBlock(4, block0Path, block1Path, block2Path, true /*isScalar*/);
    testOptimizedOneToOneBlock(6, block0Path, block1Path, block2Path, true /*isScalar*/);
    testOptimizedOneToOneBlock(8, block0Path, block1Path, block2Path, true /*isScalar*/);
    testOptimizedOneToOneBlock(10, block0Path, block1Path, block2Path, true /*isScalar*/);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_optimized_two_to_one_block)
{
    const Pothos::DType dtype(typeid(float));

    auto noiseSource = Pothos::BlockRegistry::make(
                           "/comms/noise_source",
                           dtype);

    auto arrayFireSin = Pothos::BlockRegistry::make(
                            "/arrayfire/arith/sin",
                            dtype,
                            2);
    auto arrayFireCos = Pothos::BlockRegistry::make(
                            "/arrayfire/arith/cos",
                            dtype,
                            2);

    auto arrayFireHypot = Pothos::BlockRegistry::make(
                              "/arrayfire/arith/hypot",
                              dtype);
    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             dtype);

    auto arrayFireHypotOpt = Pothos::BlockRegistry::make(
                                 "/arrayfire/arith/hypot",
                                 dtype);
    auto collectorSinkOpt = Pothos::BlockRegistry::make(
                                "/blocks/collector_sink",
                                dtype);

    arrayFireHypotOpt.call("setBlockAssumesArrayFireInputs", true);
    POTHOS_TEST_TRUE(arrayFireHypotOpt.call<bool>("getBlockAssumesArrayFireInputs"));

    size_t unoptimized = 0;
    size_t optimized = 0;
    do
    {
        // Execute the topology.
        {
            collectorSink.call("clear");
            collectorSinkOpt.call("clear");

            Pothos::Topology topology;

            for(size_t chan = 0; chan < 2; ++chan)
            {
                topology.connect(
                    noiseSource,
                    0,
                    arrayFireSin,
                    chan);
                topology.connect(
                    noiseSource,
                    0,
                    arrayFireCos,
                    chan);
            }

            topology.connect(
                arrayFireSin,
                0,
                arrayFireHypot,
                0);
            topology.connect(
                arrayFireCos,
                0,
                arrayFireHypot,
                1);
            topology.connect(
                arrayFireSin,
                1,
                arrayFireHypotOpt,
                0);
            topology.connect(
                arrayFireCos,
                1,
                arrayFireHypotOpt,
                1);

            topology.connect(
                arrayFireHypot,
                0,
                collectorSink,
                0);
            topology.connect(
                arrayFireHypotOpt,
                0,
                collectorSinkOpt,
                0);

            // When this block exits, the flowgraph will stop.
            topology.commit();

            Poco::Thread::sleep(2000);
        }

        unoptimized = collectorSink.call("getBuffer").call<size_t>("elements");
        optimized = collectorSinkOpt.call("getBuffer").call<size_t>("elements");
    } while ((0 == unoptimized) || (0 == optimized));

    std::cout << unoptimized << " vs " << optimized << std::endl;
}
