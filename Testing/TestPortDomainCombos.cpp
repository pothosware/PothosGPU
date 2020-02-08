// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include <arrayfire.h>

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include "BlockExecutionTests.hpp"
#include "TestUtility.hpp"
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

// Currently, our only non-file source needs ArrayFire 3.4.0+.
#if AF_API_VERSION_CURRENT >= 34

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
                     "/arrayfire/arith/cos",
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

#endif

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

// TODO: chaining multiple backends
