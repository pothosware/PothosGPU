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

// Currently, our only non-file source needs ArrayFire 3.4.0+.
#if AF_API_VERSION_CURRENT >= 34

POTHOS_TEST_BLOCK("/arrayfire/tests", test_chaining_arrayfire_blocks)
{
    const std::string type = "float64";

    auto afRandomSource = Pothos::BlockRegistry::make(
                              "/arrayfire/random/source",
                              type,
                              "NORMAL",
                              4);

    auto afAbs = Pothos::BlockRegistry::make(
                     "/arrayfire/arith/abs",
                     type,
                     2);

    auto afCeil = Pothos::BlockRegistry::make(
                      "/arrayfire/arith/ceil",
                      type,
                      2);

    auto afCos = Pothos::BlockRegistry::make(
                     "/arrayfire/arith/cos",
                     type,
                     4);

    auto afHypot = Pothos::BlockRegistry::make(
                       "/arrayfire/arith/hypot",
                       type);

    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             type);

    // Execute the topology.
    {
        Pothos::Topology topology;

        topology.connect(afRandomSource, 0, afAbs, 0);
        topology.connect(afRandomSource, 1, afAbs, 1);
        topology.connect(afRandomSource, 2, afCeil, 0);
        topology.connect(afRandomSource, 3, afCeil, 1);

        topology.connect(afAbs, 0, afCos, 0);
        topology.connect(afAbs, 1, afCos, 1);
        topology.connect(afCeil, 0, afCos, 2);
        topology.connect(afCeil, 1, afCos, 3);

        topology.connect(afCos, 0, collectorSink, 0);
        topology.connect(afCos, 1, collectorSink, 0);

        topology.connect(afCos, 2, afHypot, 0);
        topology.connect(afCos, 3, afHypot, 1);

        topology.connect(afHypot, 0, collectorSink, 0);

        topology.commit();
        Poco::Thread::sleep(5);
    }

    POTHOS_TEST_TRUE(collectorSink.call("getBuffer").call<int>("elements") > 0);
}

#endif

POTHOS_TEST_BLOCK("/arrayfire/tests", test_inputs_from_different_domains)
{
    const std::string type = "float64";

    auto afRandomSource = Pothos::BlockRegistry::make(
                              "/arrayfire/random/source",
                              type,
                              "NORMAL",
                              2);

    auto infiniteSource = Pothos::BlockRegistry::make("/blocks/infinite_source");

    auto afSin = Pothos::BlockRegistry::make(
                     "/arrayfire/arith/sin",
                     type,
                     4);

    auto collectorSink = Pothos::BlockRegistry::make(
                             "/blocks/collector_sink",
                             type);

    // Execute the topology.
    {
        Pothos::Topology topology;

        topology.connect(afRandomSource, 0, afSin, 0);
        topology.connect(infiniteSource, 0, afSin, 1);
        topology.connect(afRandomSource, 1, afSin, 2);
        topology.connect(infiniteSource, 0, afSin, 3);

        topology.connect(afSin, 0, collectorSink, 0);
        topology.connect(afSin, 1, collectorSink, 0);
        topology.connect(afSin, 2, collectorSink, 0);
        topology.connect(afSin, 3, collectorSink, 0);

        topology.commit();
        Poco::Thread::sleep(5);
    }

    POTHOS_TEST_TRUE(collectorSink.call("getBuffer").call<int>("elements") > 0);
}

// TODO: chaining multiple backends
