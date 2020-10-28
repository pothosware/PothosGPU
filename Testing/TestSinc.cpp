// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "TestUtility.hpp"

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>

#include <iostream>
#include <vector>

static constexpr size_t numRepetitions = 100;

template <typename Type>
void testSincTmpl()
{
    /*
     * Note: NumPy's sinc is normalized, so we need to divide by Pi to get the
     * non-normalized output.
     *
     * >>> np.linspace(0,0.5,10)
     * array([ 0.        ,  0.05555556,  0.11111111,  0.16666667,  0.22222222,
     *         0.27777778,  0.33333333,  0.38888889,  0.44444444,  0.5       ])
     * >>> np.sinc(np.linspace(0,0.5,10) / np.pi)
     * array([ 1.        ,  0.99948568,  0.99794366,  0.9953768 ,  0.99178985,
     *         0.98718944,  0.98158409,  0.97498415,  0.96740182,  0.95885108])
     */
    const auto inputs = GPUTests::stdVectorToBufferChunk<Type>(
        std::vector<Type>
        {
             0.        ,  0.05555556,  0.11111111,  0.16666667,  0.22222222,
             0.27777778,  0.33333333,  0.38888889,  0.44444444,  0.5
        });
    const auto expectedOutputs = GPUTests::stdVectorToBufferChunk<Type>(
        std::vector<Type>
        {
            1.        ,  0.99948568,  0.99794366,  0.9953768 ,  0.99178985,
            0.98718944,  0.98158409,  0.97498415,  0.96740182,  0.95885108
        });

    const auto dtype = Pothos::DType(typeid(Type));
    std::cout << "Testing " << dtype.toString() << "..." << std::endl;

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto sinc = Pothos::BlockRegistry::make("/gpu/signal/sinc", "Auto", dtype);
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    feeder.call("feedBuffer", inputs);

    {
        Pothos::Topology topology;

        topology.connect(
            feeder, 0,
            sinc, 0);
        topology.connect(
            sinc, 0,
            collector, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    GPUTests::testBufferChunk(
        expectedOutputs,
        collector.call<Pothos::BufferChunk>("getBuffer"));
}

POTHOS_TEST_BLOCK("/gpu/tests", test_sinc)
{
    testSincTmpl<float>();
    testSincTmpl<double>();
}
