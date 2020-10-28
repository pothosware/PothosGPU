// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <vector>

//
// Test code
//

template <typename Type>
static std::vector<Type> linspace(Type begin, Type end, Type step)
{
    POTHOS_TEST_LE(begin, end);

    std::vector<Type> output;
    output.emplace_back(begin);

    size_t multiplier = 0;
    while (output.back() <= end)
    {
        output.emplace_back(begin + (Type(++multiplier) * step));
    }
    if (output.back() > end) output.pop_back();

    return output;
}

template <typename Type>
struct TestValues
{
    Pothos::BufferChunk inputs;
    Pothos::BufferChunk expectedOutputs;
    Type exponent;

    TestValues(const std::vector<Type>& vals, Type exp):
        inputs(GPUTests::stdVectorToBufferChunk(vals)),
        expectedOutputs(GPUTests::stdVectorToBufferChunk(vals)),
        exponent(exp)
    {
        // When the root can resolve to either a negative or positive
        // number, the block will return positive.
        if (std::is_signed<Type>::value && (std::fmod(exp,2) == 0))
        {
            for (size_t elem = 0; elem < expectedOutputs.elements(); ++elem)
            {
                if (inputs.as<const Type*>()[elem] < 0)
                {
                    expectedOutputs.as<Type*>()[elem] *= Type(-1);
                }
            }
        }
    }
};

template <typename Type>
static std::vector<TestValues<Type>> getTestValues()
{
    const auto inputs = linspace<Type>(-10, 10, Type(0.1));
    POTHOS_TEST_FALSE(inputs.empty());

    return std::vector<TestValues<Type>>
    {
        { inputs, 1 },
        { inputs, 2 },
        { inputs, 3 },
        { inputs, 4 },
        { inputs, 5 },
        { inputs, 6 },
        { inputs, 7 },
        { inputs, 8 },
        { inputs, 9 },
        { inputs, 10 },
    };
}

//
// Test implementation
//

template <typename Type>
static void testPowRoot(const TestValues<Type>& testValues)
{
    static const auto dtype = Pothos::DType(typeid(Type));

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    feeder.call("feedBuffer", testValues.inputs);

    auto pow = Pothos::BlockRegistry::make(
                   "/gpu/arith/pow",
                   "Auto",
                   dtype,
                   testValues.exponent);
    POTHOS_TEST_EQUAL(
        testValues.exponent,
        pow.template call<Type>("power"));

    auto root = Pothos::BlockRegistry::make(
                    "/gpu/arith/root",
                    "Auto",
                    dtype,
                    testValues.exponent);
    POTHOS_TEST_EQUAL(
        testValues.exponent,
        root.template call<Type>("root"));

    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        topology.connect(feeder, 0, pow, 0);
        topology.connect(pow, 0, root, 0);
        topology.connect(root, 0, collector, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    GPUTests::testBufferChunk(
        testValues.expectedOutputs,
        collector.call<Pothos::BufferChunk>("getBuffer"));
}

template <typename Type>
static void testPowRoot()
{
    static const auto dtype = Pothos::DType(typeid(Type));

    std::cout << "Testing " << dtype.toString() << "..." << std::endl;

    auto allTestValues = getTestValues<Type>();

    for (const auto& testValues : allTestValues)
    {
        std::cout << " * Exponent: " << testValues.exponent << std::endl;
        testPowRoot<Type>(testValues);
    }
}

POTHOS_TEST_BLOCK("/gpu/tests", test_pow_root)
{
    testPowRoot<float>();
    testPowRoot<double>();
}
