// Copyright (c) 2020-2021 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "PothosBlocksReplaceImpl.hpp"
#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Testing.hpp>

#include <complex>
#include <iostream>
#include <limits>
#include <type_traits>
#include <vector>

// To avoid collisions
namespace
{

//
// Test code
//

template <typename T>
struct TestParams
{
    Pothos::BufferChunk inputs;
    Pothos::BufferChunk expectedOutputs;

    T findValue;
    T replaceValue;
    double epsilon;
};

static constexpr size_t BufferLen = 1024;

template <typename T>
static inline typename std::enable_if<!detail::IsComplex<T>::value, T>::type getRandomValue(int min, int max)
{
    return T(rand() % (max-min)) + min;
}

template <typename T>
static inline typename std::enable_if<detail::IsComplex<T>::value, T>::type getRandomValue(int min, int max)
{
    using U = typename T::value_type;

    return T{getRandomValue<U>(min,max), getRandomValue<U>(min,max)};
}

template <typename T>
static void testBufferChunksEqual(
    const Pothos::BufferChunk& expected,
    const Pothos::BufferChunk& actual,
    double epsilon)
{
    POTHOS_TEST_EQUAL(expected.dtype, actual.dtype);
    POTHOS_TEST_EQUAL(expected.elements(), actual.elements());

    for(size_t elem = 0; elem < expected.elements(); ++elem)
    {
        POTHOS_TEST_TRUE(detail::isEqual(
            expected.template as<const T*>()[elem],
            actual.template as<const T*>()[elem],
            epsilon));
    }
}

template <typename T>
static TestParams<T> getTestParams(
    const T& findValue,
    const T& replaceValue)
{
    static constexpr size_t NumOldValue = BufferLen / 20;

    TestParams<T> testParams;
    testParams.inputs = Pothos::BufferChunk(typeid(T), BufferLen);
    testParams.expectedOutputs = Pothos::BufferChunk(typeid(T), BufferLen);
    testParams.findValue = findValue;
    testParams.replaceValue = replaceValue;
    testParams.epsilon = 1e-6;

    for(size_t elem = 0; elem < BufferLen; ++elem)
    {
        testParams.inputs.template as<T*>()[elem] = getRandomValue<T>(0,100);
    }

    // Make sure we actually have instances of our old value.
    std::vector<size_t> findValueIndices;

    for(size_t i = 0; i < NumOldValue; ++i)
    {
        findValueIndices.emplace_back(rand() % BufferLen);
        testParams.inputs.template as<T*>()[findValueIndices.back()] = testParams.findValue;
    }

    replaceBuffer<T>(
        testParams.inputs,
        testParams.expectedOutputs,
        testParams.findValue,
        testParams.replaceValue,
        testParams.epsilon,
        BufferLen);

    // Make sure the values were actually replaced in our expected output.
    for(size_t elem: findValueIndices)
    {
        POTHOS_TEST_TRUE(detail::isEqual(
            testParams.replaceValue,
            testParams.expectedOutputs.template as<const T*>()[elem],
            testParams.epsilon));
    }

    return testParams;
}

template <typename T>
static void testReplace(
    const T& findValue,
    const T& replaceValue)
{
    const auto dtype = Pothos::DType(typeid(T));
    const auto params = getTestParams<T>(findValue, replaceValue);
    
    std::cout << " * Testing " << dtype.toString() << "..." << std::endl;
    std::cout << "   * " << findValue << " -> " << replaceValue << std::endl;

    auto source = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    source.call("feedBuffer", params.inputs);

    auto replace = Pothos::BlockRegistry::make(
                       "/gpu/data/replace",
                       "Auto",
                       dtype,
                       params.findValue,
                       params.replaceValue);

    POTHOS_TEST_TRUE(detail::isEqual(
        params.findValue,
        replace.template call<T>("findValue"),
        params.epsilon));
    POTHOS_TEST_TRUE(detail::isEqual(
        params.replaceValue,
        replace.template call<T>("replaceValue"),
        params.epsilon));

    auto sink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;
        topology.connect(source, 0, replace, 0);
        topology.connect(replace, 0, sink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    testBufferChunksEqual<T>(
        params.expectedOutputs,
        sink.call<Pothos::BufferChunk>("getBuffer"),
        params.epsilon);
}

// Since we can't do runtime-default parameters
template <typename T>
static void testReplace()
{
    testReplace<T>(
        getRandomValue<T>(0,50),
        getRandomValue<T>(51,100));
}

//
// Tests
//

POTHOS_TEST_BLOCK("/gpu/tests", test_replace)
{
    srand(0ULL);

    testReplace<char>();
    testReplace<short>();
    testReplace<int>();
    testReplace<long long>();
    testReplace<unsigned char>();
    testReplace<unsigned short>();
    testReplace<unsigned>();
    testReplace<unsigned long long>();
    testReplace<float>();
    testReplace<double>();
    testReplace<std::complex<float>>();
    testReplace<std::complex<double>>();
}

POTHOS_TEST_BLOCK("/gpu/tests", test_replace_infinity)
{
    testReplace<float>(
        std::numeric_limits<float>::infinity(),
        0.0f);
    testReplace<double>(
        std::numeric_limits<double>::infinity(),
        0.0f);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_replace_neg_infinity)
{
    testReplace<float>(
        -std::numeric_limits<float>::infinity(),
        0.0f);
    testReplace<double>(
        -std::numeric_limits<double>::infinity(),
        0.0f);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_replace_nan)
{
    testReplace<float>(
        std::numeric_limits<float>::quiet_NaN(),
        0.0f);
    testReplace<double>(
        std::numeric_limits<double>::quiet_NaN(),
        0.0f);
}

POTHOS_TEST_BLOCK("/gpu/tests", test_replace_neg_nan)
{
    testReplace<float>(
        -std::numeric_limits<float>::quiet_NaN(),
        0.0f);
    testReplace<double>(
        -std::numeric_limits<double>::quiet_NaN(),
        0.0f);
}

}
