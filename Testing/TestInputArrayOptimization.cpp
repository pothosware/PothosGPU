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

static double mean(const std::vector<double>& inputVec)
{
    return std::accumulate(
               inputVec.begin(),
               inputVec.end(),
               0.0)
           / inputVec.size();
}

static double rateMonitorsMean(const std::vector<Pothos::Proxy>& rateMonitors)
{
    std::vector<double> rates;
    std::transform(
        rateMonitors.begin(),
        rateMonitors.end(),
        std::back_inserter(rates),
        [](const Pothos::Proxy& rateMonitor)
        {
            return rateMonitor.call<double>("rate");
        });

    return mean(rates);
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
    std::vector<Pothos::Proxy> rateMonitors(numChannels);
    std::vector<Pothos::Proxy> rateMonitorsOpt(numChannels);
    for(size_t chan = 0; chan < numChannels; ++chan)
    {
        noiseSources[chan] = Pothos::BlockRegistry::make(
                                 "/comms/noise_source",
                                 dtype);
        rateMonitors[chan] = Pothos::BlockRegistry::make("/blocks/rate_monitor");
        rateMonitorsOpt[chan] = Pothos::BlockRegistry::make("/blocks/rate_monitor");
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
                    rateMonitors[chan],
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
                    rateMonitorsOpt[chan],
                    0);
            }

            // When this block exits, the flowgraph will stop.
            topology.commit();

            Poco::Thread::sleep(1000);
        }

        unoptimizedAverage = static_cast<size_t>(rateMonitorsMean(rateMonitors));
        optimizedAverage = static_cast<size_t>(rateMonitorsMean(rateMonitorsOpt));
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
    auto rateMonitor = Pothos::BlockRegistry::make("/blocks/rate_monitor");

    auto arrayFireHypotOpt = Pothos::BlockRegistry::make(
                                 "/arrayfire/arith/hypot",
                                 dtype);
    auto rateMonitorOpt = Pothos::BlockRegistry::make("/blocks/rate_monitor");

    arrayFireHypotOpt.call("setBlockAssumesArrayFireInputs", true);
    POTHOS_TEST_TRUE(arrayFireHypotOpt.call<bool>("getBlockAssumesArrayFireInputs"));

    size_t unoptimized = 0;
    size_t optimized = 0;
    do
    {
        // Execute the topology.
        {
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
                rateMonitor,
                0);
            topology.connect(
                arrayFireHypotOpt,
                0,
                rateMonitorOpt,
                0);

            // When this block exits, the flowgraph will stop.
            topology.commit();

            Poco::Thread::sleep(2000);
        }

        unoptimized = rateMonitor.call<size_t>("rate");
        optimized = rateMonitorOpt.call<size_t>("rate");
    } while ((0 == unoptimized) || (0 == optimized));

    std::cout << unoptimized << " vs " << optimized << std::endl;
}
