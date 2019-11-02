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

static void testOptimizedOneToOneBlock(size_t numChannels)
{
    static const Pothos::DType dtype(typeid(float));

    auto arrayFireSin0 = Pothos::BlockRegistry::make(
                             "/arrayfire/arith/sin",
                             dtype,
                             numChannels);
    auto arrayFireSin1 = Pothos::BlockRegistry::make(
                             "/arrayfire/arith/sin",
                             dtype,
                             numChannels);
    auto arrayFireAbs = Pothos::BlockRegistry::make(
                            "/arrayfire/arith/abs",
                            dtype,
                            numChannels);
    auto arrayFireAbsOpt = Pothos::BlockRegistry::make(
                               "/arrayfire/arith/abs",
                               dtype,
                               numChannels);
    auto arrayFireCos = Pothos::BlockRegistry::make(
                            "/arrayfire/arith/cos",
                            dtype,
                            numChannels);
    auto arrayFireCosOpt = Pothos::BlockRegistry::make(
                               "/arrayfire/arith/cos",
                               dtype,
                               numChannels);

    arrayFireAbsOpt.call("setBlockAssumesArrayFireInputs", true);
    POTHOS_TEST_TRUE(arrayFireAbsOpt.call<bool>("getBlockAssumesArrayFireInputs"));
    arrayFireCosOpt.call("setBlockAssumesArrayFireInputs", true);
    POTHOS_TEST_TRUE(arrayFireCosOpt.call<bool>("getBlockAssumesArrayFireInputs"));

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
                    arrayFireSin0,
                    chan);
                topology.connect(
                    noiseSources[chan],
                    0,
                    arrayFireSin1,
                    chan);

                topology.connect(
                    arrayFireSin0,
                    chan,
                    arrayFireAbs,
                    chan);
                topology.connect(
                    arrayFireAbs,
                    chan,
                    arrayFireCos,
                    chan);
                topology.connect(
                    arrayFireCos,
                    chan,
                    collectorSinks[chan],
                    0);

                topology.connect(
                    arrayFireSin1,
                    chan,
                    arrayFireAbsOpt,
                    chan);
                topology.connect(
                    arrayFireAbsOpt,
                    chan,
                    arrayFireCosOpt,
                    chan);
                topology.connect(
                    arrayFireCosOpt,
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
    testOptimizedOneToOneBlock(2);
    testOptimizedOneToOneBlock(4);
    testOptimizedOneToOneBlock(6);
    testOptimizedOneToOneBlock(8);
    testOptimizedOneToOneBlock(10);
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
