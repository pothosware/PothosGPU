// Copyright (c) 2019 Nick Foster
//               2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <cstdint>
#include <cmath>
#include <iostream>

static constexpr size_t NUM_POINTS = 12;

//
// Utility code
//

// Make it easier to templatize
template <typename Type>
static double logNTmpl(Type input, Type base)
{
    return std::log(input) / std::log(base);
}

//
// Test implementations
//

template <typename Type>
using LogTmplFcn = double(*)(Type);

template <typename Type>
static void testLogNImpl(Type base)
{
    const auto blockPath = "/gpu/arith/log";
    const auto dtype = Pothos::DType(typeid(Type));
    std::cout << "Testing " << blockPath << " with type " << dtype.toString()
              << " and base " << Pothos::Object(base).toString() << std::endl;

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    auto log = Pothos::BlockRegistry::make(blockPath, "Auto", dtype, base);
    POTHOS_TEST_EQUAL(base, log.template call<Type>("base"));

    Pothos::BufferChunk abuffOut = collector.call("getBuffer");
    // Load the feeder
    auto buffIn = Pothos::BufferChunk(typeid(Type), NUM_POINTS);
    auto pIn = buffIn.as<Type *>();
    for (size_t i = 0; i < buffIn.elements(); i++)
    {
        pIn[i] = Type(10*(i+1));
    }
    feeder.call("feedBuffer", buffIn);

    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, log, 0);
        topology.connect(log   , 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    // Check the collector
    Pothos::BufferChunk buffOut = collector.call("getBuffer");
    POTHOS_TEST_EQUAL(buffOut.elements(), buffIn.elements());
    auto pOut = buffOut.as<const Type *>();
    for (size_t i = 0; i < buffOut.elements(); i++)
    {
        // Allow up to an error of 1 because of fixed point truncation rounding
        const auto expected = logNTmpl(pIn[i], base);
        POTHOS_TEST_CLOSE(pOut[i], expected, 1);
    }
}

POTHOS_TEST_BLOCK("/gpu/tests", test_log)
{
    for(size_t base = 2; base <= 10; ++base)
    {
        testLogNImpl<float>(float(base));
        testLogNImpl<double>(double(base));
    }
}
