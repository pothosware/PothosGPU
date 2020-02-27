// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <arrayfire.h>

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"

#include "DeviceCache.hpp"
#include "Utility.hpp"

#include <Poco/Thread.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

using namespace PothosArrayFireTests;

static constexpr long SleepTimeMs = 500;

POTHOS_TEST_BLOCK("/arrayfire/tests", test_chaining_arrayfire_blocks)
{
    const std::string type = "float64";

    auto afRandomSource = Pothos::BlockRegistry::make(
                              "/arrayfire/random/source",
                              "Auto",
                              type,
                              "NORMAL");

    auto afAbs = Pothos::BlockRegistry::make(
                     "/arrayfire/arith/abs",
                     "Auto",
                     type);

    auto afCeil = Pothos::BlockRegistry::make(
                      "/arrayfire/arith/ceil",
                      "Auto",
                      type);

    auto afCos = Pothos::BlockRegistry::make(
                     "/arrayfire/trig/cos",
                     "Auto",
                     type);

    auto afHypot = Pothos::BlockRegistry::make(
                       "/arrayfire/arith/hypot",
                       "Auto",
                       type);

    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             type);

    // Execute the topology.
    {
        Pothos::Topology topology;

        topology.connect(afRandomSource, 0, afAbs, 0);
        topology.connect(afRandomSource, 0, afCeil, 0);

        topology.connect(afAbs, 0, afCos, 0);
        topology.connect(afCeil, 0, afCos, 1);

        topology.connect(afCos, 0, collectorSink, 0);
        topology.connect(afCos, 1, collectorSink, 0);

        topology.connect(afCos, 0, afHypot, 0);
        topology.connect(afCos, 1, afHypot, 1);

        topology.connect(afHypot, 0, collectorSink, 0);

        topology.commit();
        Poco::Thread::sleep(SleepTimeMs);
    }

    POTHOS_TEST_TRUE(collectorSink.call("getBuffer").call<int>("elements") > 0);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_inputs_from_different_domains)
{
    const std::string type = "float64";

    auto afRandomSource = Pothos::BlockRegistry::make(
                              "/arrayfire/random/source",
                              "Auto",
                              type,
                              "NORMAL");

    auto infiniteSource = Pothos::BlockRegistry::make("/blocks/infinite_source");
    infiniteSource.call("enableBuffers", true);

    auto afHypot = Pothos::BlockRegistry::make(
                       "/arrayfire/arith/hypot",
                       "Auto",
                       type);

    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             type);

    // Execute the topology.
    {
        Pothos::Topology topology;

        topology.connect(afRandomSource, 0, afHypot, 0);
        topology.connect(infiniteSource, 0, afHypot, 1);

        topology.connect(afHypot, 0, collectorSink, 0);
        topology.connect(afHypot, 1, collectorSink, 0);

        topology.commit();
        Poco::Thread::sleep(SleepTimeMs);
    }

    POTHOS_TEST_TRUE(collectorSink.call("getBuffer").call<int>("elements") > 0);
}

static std::vector<std::string> getSingleDevicePerBackend()
{
    std::vector<std::string> devices;
    
    const auto& availableBackends = getAvailableBackends();
    const auto& deviceCache = getDeviceCache();
    
    for(auto backend: availableBackends)
    {
        auto deviceIter = std::find_if(
                              deviceCache.begin(),
                              deviceCache.end(),
                              [&backend](const DeviceCacheEntry& entry)
                              {
                                  return (entry.afBackendEnum == backend);
                              });
        if(deviceIter != deviceCache.end())
        {
            devices.emplace_back(deviceIter->name);
        }
    }

    return devices;
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_multiple_backends_into_one_sink)
{
    const auto devices = getSingleDevicePerBackend();
    
    if(devices.size() > 1)
    {
        const double constant = 5.0;

        std::vector<Pothos::Proxy> afBlocks;
        for(const auto& device: devices) 
        {
            std::cout << "Adding " << device << " to topology..." << std::endl;

            afBlocks.emplace_back(Pothos::BlockRegistry::make(
                                      "/arrayfire/data/constant",
                                      device,
                                      "float64",
                                      constant));
        }

        auto collectorSink = Pothos::BlockRegistry::make(
                                 "/blocks/collector_sink",
                                 "float64");

        {
            Pothos::Topology topology;
            for(auto block: afBlocks)
            {
                topology.connect(block, 0, collectorSink, 0);
            }

            topology.commit();
            Poco::Thread::sleep(SleepTimeMs);
        }

        // No matter the backend, the value should be the same.
        auto buffOut = collectorSink.call<Pothos::BufferChunk>("getBuffer");
        POTHOS_TEST_TRUE(buffOut.elements() > 0);

        const double* begin = buffOut;
        const double* end = begin + buffOut.elements();
        POTHOS_TEST_TRUE(end == std::find_if(begin, end, [&constant](double val){return val != constant;}));
    }
    else
    {
        std::cout << "Skipping test. Only one ArrayFire device available." << std::endl;
    }
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_chaining_multiple_backends)
{
    const auto devices = getSingleDevicePerBackend();
    
    if(devices.size() > 1)
    {
        constexpr double constant = 5.0;
        constexpr double multiplier = 2.0;

        std::vector<Pothos::Proxy> afBlocks;
        for(const auto& device: devices) 
        {
            std::cout << "Adding " << device << " to topology..." << std::endl;

            if(&device == &devices[0])
            {
                // Make the first block a source.
                afBlocks.emplace_back(Pothos::BlockRegistry::make(
                                          "/arrayfire/data/constant",
                                          device,
                                          "float64",
                                          constant));
            }
            else
            {
                afBlocks.emplace_back(Pothos::BlockRegistry::make(
                                          "/arrayfire/scalar/arithmetic",
                                          device,
                                          "*",
                                          "float64",
                                          multiplier));
            }
        }

        auto collectorSink = Pothos::BlockRegistry::make(
                                 "/blocks/collector_sink",
                                 "float64");

        {
            Pothos::Topology topology;
            for(size_t i = 0; i < (afBlocks.size()-1); ++i)
            {
                topology.connect(afBlocks[i], 0, afBlocks[i+1], 0);
            }
            topology.connect(afBlocks.back(), 0, collectorSink, 0);

            topology.commit();
            Poco::Thread::sleep(SleepTimeMs);
        }

        // No matter the backend, the value should be the same.
        auto buffOut = collectorSink.call<Pothos::BufferChunk>("getBuffer");
        POTHOS_TEST_TRUE(buffOut.elements() > 0);

        const double* begin = buffOut;
        const double* end = begin + buffOut.elements();
        const double expectedValue = constant * (multiplier * (afBlocks.size()-1));
        
        POTHOS_TEST_TRUE(end == std::find_if(begin, end, [&expectedValue](double val){return val != expectedValue;}));
    }
    else
    {
        std::cout << "Skipping test. Only one ArrayFire device available." << std::endl;
    }
}
