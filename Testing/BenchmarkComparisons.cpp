// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "DeviceCache.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <arrayfire.h>

#include <Poco/Thread.h>

#include <iostream>
#include <vector>

static constexpr size_t SleepTime = 1000;

static void benchmarkBlock(
    const std::string& commsBlockPath,
    const std::string& afBlockPath,
    const std::string& dtype)
{
    auto noiseSource = Pothos::BlockRegistry::make(
                           "/comms/noise_source",
                           dtype);
    auto rateMonitor = Pothos::BlockRegistry::make("/blocks/rate_monitor");

    double commsRate = 0.0;
    double afRate = 0.0;

    //
    // /comms
    //

    auto commsAbs = Pothos::BlockRegistry::make(
                        commsBlockPath,
                        dtype);

    {
        Pothos::Topology topology;
        topology.connect(noiseSource, 0, commsAbs, 0);
        topology.connect(commsAbs, 0, rateMonitor, 0);

        topology.commit();
        Poco::Thread::sleep(SleepTime);

        commsRate = rateMonitor.call<double>("rate");
    }
    std::cout << commsBlockPath << ": " << commsRate << std::endl;

    //
    // /arrayfire
    //

    std::cout << afBlockPath << ":" << std::endl;

    const auto& deviceCache = getDeviceCache();
    for(const auto& device: deviceCache)
    {
        auto afAbs = Pothos::BlockRegistry::make(
                         afBlockPath,
                         device.name,
                         dtype,
                         1);

        {
            Pothos::Topology topology;
            topology.connect(noiseSource, 0, afAbs, 0);
            topology.connect(afAbs, 0, rateMonitor, 0);

            topology.commit();
            Poco::Thread::sleep(SleepTime);

            afRate = rateMonitor.call<double>("rate");
        }

        std::cout << " * " << device.name << ": " << afRate << std::endl;
    }
}

static void benchmarkArithmeticBlock(
    const std::string& commsOperation,
    const std::string& afOperation,
    const std::string& dtype)
{
    static const std::string commsBlockPath = "/comms/arithmetic";
    static const std::string afBlockPath = "/arrayfire/array/arithmetic";
    static constexpr size_t nchans = 16;

    std::vector<Pothos::Proxy> noiseSources;
    for(size_t i = 0; i < nchans; ++i)
    {
        noiseSources.emplace_back(Pothos::BlockRegistry::make(
                                      "/comms/noise_source",
                                      dtype));
    }
    auto rateMonitor = Pothos::BlockRegistry::make("/blocks/rate_monitor");

    double commsRate = 0.0;
    double afRate = 0.0;

    //
    // /comms
    //

    auto commsArithmetic = Pothos::BlockRegistry::make(
                               commsBlockPath,
                               dtype,
                               commsOperation);
    commsArithmetic.call("setNumInputs", nchans);

    {
        Pothos::Topology topology;
        
        for(size_t i = 0; i < nchans; ++i)
        {
            topology.connect(noiseSources[i], 0, commsArithmetic, i);
        }

        topology.connect(commsArithmetic, 0, rateMonitor, 0);

        topology.commit();
        Poco::Thread::sleep(SleepTime);

        commsRate = rateMonitor.call<double>("rate");
    }
    std::cout << commsBlockPath << " (" << commsOperation <<  "): " << commsRate << std::endl;

    //
    // /arrayfire
    //

    std::cout << afBlockPath << " (" << afOperation << "):" << std::endl;

    const auto& deviceCache = getDeviceCache();
    for(const auto& device: deviceCache)
    {
        auto afArithmetic = Pothos::BlockRegistry::make(
                                afBlockPath,
                                device.name,
                                afOperation,
                                dtype,
                                nchans);

        {
            Pothos::Topology topology;

            for(size_t i = 0; i < nchans; ++i)
            {
                topology.connect(noiseSources[i], 0, afArithmetic, i);
            }

            topology.connect(afArithmetic, 0, rateMonitor, 0);

            topology.commit();
            Poco::Thread::sleep(SleepTime);

            afRate = rateMonitor.call<double>("rate");
        }

        std::cout << " * " << device.name << ": " << afRate << std::endl;
    }
}

POTHOS_TEST_BLOCK("/arrayfire/tests", benchmark_abs)
{
    benchmarkBlock(
        "/comms/abs",
        "/arrayfire/arith/abs",
        "float64");
}

POTHOS_TEST_BLOCK("/arrayfire/tests", benchmark_conjg)
{
    benchmarkBlock(
        "/comms/conjugate",
        "/arrayfire/arith/conjg",
        "complex_float64");
}

POTHOS_TEST_BLOCK("/arrayfire/tests", benchmark_arg)
{
    benchmarkBlock(
        "/comms/angle",
        "/arrayfire/arith/arg",
        "complex_float64");
}

POTHOS_TEST_BLOCK("/arrayfire/tests", benchmark_add)
{
    benchmarkArithmeticBlock(
        "ADD",
        "+",
        "float64");
}

POTHOS_TEST_BLOCK("/arrayfire/tests", benchmark_sub)
{
    benchmarkArithmeticBlock(
        "SUB",
        "-",
        "float64");
}

POTHOS_TEST_BLOCK("/arrayfire/tests", benchmark_mul)
{
    benchmarkArithmeticBlock(
        "MUL",
        "*",
        "float64");
}

POTHOS_TEST_BLOCK("/arrayfire/tests", benchmark_div)
{
    benchmarkArithmeticBlock(
        "DIV",
        "/",
        "float64");
}
