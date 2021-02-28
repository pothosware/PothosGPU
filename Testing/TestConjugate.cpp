// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

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

template <typename Type>
static void getConjugateTestValues(
    Pothos::BufferChunk* pInputs,
    Pothos::BufferChunk* pExpectedOutputs)
{
    using ComplexType = std::complex<Type>;
    const auto dtype = Pothos::DType(typeid(ComplexType));

    (*pInputs) = GPUTests::getTestInputs(dtype.name());
    (*pExpectedOutputs) = Pothos::BufferChunk(typeid(ComplexType), pInputs->elements());

    for (size_t elem = 0; elem < pInputs->elements(); ++elem)
    {
        pExpectedOutputs->as<ComplexType*>()[elem] = std::conj(pInputs->as<ComplexType*>()[elem]);
    }
}

template <typename Type>
static void testConjugate()
{
    using ComplexType = std::complex<Type>;

    Pothos::BufferChunk inputs;
    Pothos::BufferChunk expectedOutputs;
    getConjugateTestValues<Type>(&inputs, &expectedOutputs);

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
