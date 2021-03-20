// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>
#include <Pothos/Proxy.hpp>

#include <Poco/Random.h>

#include <cmath>
#include <iostream>
#include <random>

static Poco::Random rng;

template <typename Type>
static void getTestValues(
    Pothos::BufferChunk* pInputs,
    Pothos::BufferChunk* pExpectedIntegralOutputs,
    Pothos::BufferChunk* pExpectedFractionalOutputs)
{
    (*pInputs) = Pothos::BufferChunk(
        typeid(Type),
        GPUTests::TestInputLength);
    (*pExpectedIntegralOutputs) = Pothos::BufferChunk(
        typeid(Type),
        GPUTests::TestInputLength);
    (*pExpectedFractionalOutputs) = Pothos::BufferChunk(
        typeid(Type),
        GPUTests::TestInputLength);

    for (size_t elem = 0; elem < GPUTests::TestInputLength; ++elem)
    {
        // nextFloat() returns a value between 0-1
        pInputs->as<Type*>()[elem] = Type(rng.nextFloat() * rng.next(1000000));
        pExpectedFractionalOutputs->as<Type*>()[elem] = std::modf(
            pInputs->as<const Type*>()[elem],
            &pExpectedIntegralOutputs->as<Type*>()[elem]);
    }
}

template <typename Type>
static void testModF()
{
    Pothos::BufferChunk inputs;
    Pothos::BufferChunk expectedIntegralOutputs;
    Pothos::BufferChunk expectedFractionalOutputs;
    getTestValues<Type>(
        &inputs,
        &expectedIntegralOutputs,
        &expectedFractionalOutputs);

    const auto dtype = Pothos::DType(typeid(Type));
    std::cout << "Testing " << dtype.toString() << "..." << std::endl;

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto modf = Pothos::BlockRegistry::make("/gpu/arith/modf", "Auto", dtype);
    auto integralSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);
    auto fractionalSink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    source.call("feedBuffer", inputs);

    {
        Pothos::Topology topology;

        topology.connect(source, 0, modf, 0);
        topology.connect(modf, "int", integralSink, 0);
        topology.connect(modf, "frac", fractionalSink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    GPUTests::testBufferChunk(
        expectedIntegralOutputs,
        integralSink.call<Pothos::BufferChunk>("getBuffer"));
    GPUTests::testBufferChunk(
        expectedFractionalOutputs,
        fractionalSink.call<Pothos::BufferChunk>("getBuffer"));
}

POTHOS_TEST_BLOCK("/gpu/tests", test_modf)
{
    testModF<float>();
    testModF<double>();
}
