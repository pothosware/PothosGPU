// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>
#include <Pothos/Proxy.hpp>

#include <complex>
#include <iostream>
#include <random>

//
// Common
//

constexpr size_t bufferLen = 4096;

template <typename Type>
static void getTestValues(
    Pothos::BufferChunk* pInputs,
    Pothos::BufferChunk* pExpectedOutputs)
{
    using ComplexType = std::complex<Type>;

    (*pInputs) = Pothos::BufferChunk(typeid(ComplexType), bufferLen);
    (*pExpectedOutputs) = Pothos::BufferChunk(typeid(ComplexType), bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        pInputs->as<ComplexType*>()[elem] = ComplexType(Type(rand() % 100), Type(rand() % 100));
        pExpectedOutputs->as<ComplexType*>()[elem] = std::conj(pInputs->as<ComplexType*>()[elem]);
    }
}

template <typename Type>
static void testConjugate()
{
    using ComplexType = std::complex<Type>;

    Pothos::BufferChunk inputs;
    Pothos::BufferChunk expectedOutputs;
    getTestValues<Type>(&inputs, &expectedOutputs);

    const auto dtype = Pothos::DType(typeid(ComplexType));

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto conj = Pothos::BlockRegistry::make("/gpu/arith/conjg", "Auto", dtype);
    auto sink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    source.call("feedBuffer", inputs);

    {
        Pothos::Topology topology;

        topology.connect(source, 0, conj, 0);
        topology.connect(conj, 0, sink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    GPUTests::testBufferChunk(
        expectedOutputs,
        sink.call<Pothos::BufferChunk>("getBuffer"));
}

POTHOS_TEST_BLOCK("/gpu/tests", test_conjugate)
{
    testConjugate<float>();
    testConjugate<double>();
}
