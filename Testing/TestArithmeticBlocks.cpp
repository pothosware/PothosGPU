// Copyright (c) 2014-2016 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "TestUtility.hpp"

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>

#include <algorithm>
#include <complex>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <vector>

//
// Common
//

constexpr size_t bufferLen = 4096;

//
// /gpu/array/arithmetic
//

struct ArithmeticTestValues
{
    std::vector<Pothos::BufferChunk> inputs;
    Pothos::BufferChunk expectedOutputs;

    template <typename T>
    void setup(size_t numInputs, size_t bufferLength)
    {
        static const Pothos::DType dtype(typeid(T));

        for (size_t input = 0; input < numInputs; ++input)
        {
            inputs.emplace_back(Pothos::BufferChunk(dtype, bufferLength));
        }

        expectedOutputs = Pothos::BufferChunk(dtype, bufferLength);
    }
};

template <typename T>
static GPUTests::EnableIfNotComplex<T, ArithmeticTestValues> getAddTestValues()
{
    constexpr auto numInputs = 3;

    static const Pothos::DType dtype(typeid(T));

    ArithmeticTestValues testValues;
    testValues.setup<T>(numInputs, bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        testValues.inputs[0].as<T*>()[elem] = static_cast<T>(elem);
        testValues.inputs[1].as<T*>()[elem] = static_cast<T>(elem/2);
        testValues.inputs[2].as<T*>()[elem] = static_cast<T>(elem/4);

        if (std::is_signed<T>::value)
        {
            testValues.inputs[1].as<T*>()[elem] *= -1;
            testValues.inputs[2].as<T*>()[elem] *= -1;

            testValues.expectedOutputs.as<T*>()[elem] = static_cast<T>(elem - (elem / 2) - (elem / 4));
        }
        else
        {
            testValues.expectedOutputs.as<T*>()[elem] = static_cast<T>(elem + (elem / 2) + (elem / 4));
        }
    }

    return testValues;
}

// Fully co-opt the scalar implementation since complex addition is just (real+real, imag+imag)
template <typename T>
static EnableIfComplex<T, ArithmeticTestValues> getAddTestValues()
{
    using ScalarType = typename T::value_type;
    static const Pothos::DType dtype(typeid(T));

    auto testValues = getAddTestValues<ScalarType>();
    for (auto& input : testValues.inputs) input.dtype = dtype;
    testValues.expectedOutputs.dtype = dtype;

    return testValues;
}

template <typename T>
static GPUTests::EnableIfNotComplex<T, ArithmeticTestValues> getSubTestValues()
{
    constexpr auto numInputs = 2;

    static const Pothos::DType dtype(typeid(T));

    ArithmeticTestValues testValues;
    testValues.setup<T>(numInputs, bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        testValues.inputs[0].as<T*>()[elem] = static_cast<T>(elem);
        testValues.inputs[1].as<T*>()[elem] = std::is_signed<T>::value ? static_cast<T>(elem * 2)
                                                                       : static_cast<T>(elem / 2);

        testValues.expectedOutputs.as<T*>()[elem] = testValues.inputs[0].as<T*>()[elem]
                                                  - testValues.inputs[1].as<T*>()[elem];
    }

    return testValues;
}

// Fully co-opt the scalar implementation since complex subtraction is just (real-real, imag-imag)
template <typename T>
static EnableIfComplex<T, ArithmeticTestValues> getSubTestValues()
{
    using ScalarType = typename T::value_type;
    static const Pothos::DType dtype(typeid(T));

    auto testValues = getSubTestValues<ScalarType>();
    for (auto& input : testValues.inputs) input.dtype = dtype;
    testValues.expectedOutputs.dtype = dtype;

    return testValues;
}

template <typename T>
static GPUTests::EnableIfNotComplex<T, ArithmeticTestValues> getMulTestValues()
{
    constexpr auto numInputs = 2;

    static const Pothos::DType dtype(typeid(T));

    ArithmeticTestValues testValues;
    testValues.setup<T>(numInputs, bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        testValues.inputs[0].as<T*>()[elem] = static_cast<T>(elem);
        testValues.inputs[1].as<T*>()[elem] = static_cast<T>((elem % 2) + 1);

        if (std::is_signed<T>::value) testValues.inputs[1].as<T*>()[elem] *= -1;

        testValues.expectedOutputs.as<T*>()[elem] = testValues.inputs[0].as<T*>()[elem]
                                                  * testValues.inputs[1].as<T*>()[elem];
    }

    return testValues;
}

// Out of laziness, get the scalar version's values and recalculate the outputs.
template <typename T>
static EnableIfComplex<T, ArithmeticTestValues> getMulTestValues()
{
    using ScalarType = typename T::value_type;
    static const Pothos::DType dtype(typeid(T));

    auto testValues = getMulTestValues<ScalarType>();
    POTHOS_TEST_EQUAL(2, testValues.inputs.size());

    for (auto& input : testValues.inputs) input.dtype = dtype;
    testValues.expectedOutputs.dtype = dtype;

    for (size_t elem = 0; elem < testValues.expectedOutputs.elements(); ++elem)
    {
        testValues.expectedOutputs.template as<T*>()[elem] =
            testValues.inputs[0].template as<const T*>()[elem] *
            testValues.inputs[1].template as<const T*>()[elem];
    }

    return testValues;
}

template <typename T>
static GPUTests::EnableIfNotComplex<T, ArithmeticTestValues> getDivTestValues()
{
    constexpr auto numInputs = 2;

    static const Pothos::DType dtype(typeid(T));

    ArithmeticTestValues testValues;
    testValues.setup<T>(numInputs, bufferLen);

    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        testValues.inputs[0].as<T*>()[elem] = static_cast<T>(elem);
        testValues.inputs[1].as<T*>()[elem] = static_cast<T>((elem % 2) + 1);

        if (std::is_signed<T>::value) testValues.inputs[1].as<T*>()[elem] *= -1;

        testValues.expectedOutputs.as<T*>()[elem] = testValues.inputs[0].as<T*>()[elem]
                                                  / testValues.inputs[1].as<T*>()[elem];
    }

    return testValues;
}

// Out of laziness, get the scalar version's values and recalculate the outputs.
template <typename T>
static EnableIfComplex<T, ArithmeticTestValues> getDivTestValues()
{
    using ScalarType = typename T::value_type;
    static const Pothos::DType dtype(typeid(T));

    auto testValues = getDivTestValues<ScalarType>();
    POTHOS_TEST_EQUAL(2, testValues.inputs.size());

    for (auto& input : testValues.inputs) input.dtype = dtype;
    testValues.expectedOutputs.dtype = dtype;

    for (size_t elem = 0; elem < testValues.expectedOutputs.elements(); ++elem)
    {
        testValues.expectedOutputs.template as<T*>()[elem] =
            testValues.inputs[0].template as<const T*>()[elem] /
            testValues.inputs[1].template as<const T*>()[elem];
    }

    return testValues;
}

template <typename T>
static void testArithmeticOp(
    const std::string& operation,
    const ArithmeticTestValues& testValues)
{
    const Pothos::DType dtype(typeid(T));

    std::cout << " * Testing " << operation << "..." << std::endl;

    const auto numInputs = testValues.inputs.size();

    auto arithmetic = Pothos::BlockRegistry::make(
                          "/gpu/array/arithmetic",
                          "Auto",
                          operation,
                          dtype,
                          numInputs);

    std::vector<Pothos::Proxy> feeders(numInputs);
    for (size_t input = 0; input < numInputs; ++input)
    {
        feeders[input] = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
        feeders[input].call("feedBuffer", testValues.inputs[input]);
    }

    auto sink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        for (size_t input = 0; input < numInputs; ++input)
        {
            topology.connect(feeders[input], 0, arithmetic, input);
        }
        topology.connect(arithmetic, 0, sink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    GPUTests::testBufferChunk(
        testValues.expectedOutputs,
        sink.call<Pothos::BufferChunk>("getBuffer"));
}

template <typename T>
static void testArithmetic()
{
    std::cout << "Testing " << Pothos::DType(typeid(T)).toString() << "..." << std::endl;

    testArithmeticOp<T>("Add", getAddTestValues<T>());
    testArithmeticOp<T>("Subtract", getSubTestValues<T>());
    testArithmeticOp<T>("Multiply", getMulTestValues<T>());
    testArithmeticOp<T>("Divide", getDivTestValues<T>());
    // TODO: modulus
}

POTHOS_TEST_BLOCK("/gpu/tests", test_array_arithmetic)
{
    testArithmetic<std::int16_t>();
    testArithmetic<std::int32_t>();
    testArithmetic<std::int64_t>();
    testArithmetic<std::uint8_t>();
    testArithmetic<std::uint16_t>();
    testArithmetic<std::uint32_t>();
    testArithmetic<std::uint64_t>();
    testArithmetic<float>();
    testArithmetic<double>();
    testArithmetic<std::complex<float>>();
    testArithmetic<std::complex<double>>();
}

//
// /gpu/scalar/arithmetic
//

struct ScalarArithmeticTestValues
{
    Pothos::BufferChunk inputs;
    Pothos::Object scalar;
    Pothos::BufferChunk expectedOutputs;

    template <typename T>
    void setup(const T& testScalar, size_t bufferLength)
    {
        static const Pothos::DType dtype(typeid(T));

        inputs = Pothos::BufferChunk(typeid(T), bufferLength);
        scalar = Pothos::Object(testScalar);
        expectedOutputs = Pothos::BufferChunk(dtype, bufferLength);
    }
};

template <typename T>
static inline GPUTests::EnableIfNotComplex<T, T> avoidZero(const T& value)
{
    return (T(0) == value) ? (value+1) : value;
}

template <typename T>
static inline EnableIfComplex<T, T> avoidZero(const T& value)
{
    T ret(value);

    if(T(0) == ret.real()) ret = T(ret.real()+1, ret.imag());
    if(T(0) == ret.imag()) ret = T(ret.real(), ret.imag()+1);

    return ret;
}

template <typename T>
static inline GPUTests::EnableIfNotComplex<T, T> getTestScalar()
{
    return T(2);
}

template <typename T>
static inline EnableIfComplex<T, T> getTestScalar()
{
    return T(3, 2);
}

template <typename T>
static void getTestValues(
    ScalarArithmeticTestValues* pAddTestValues,
    ScalarArithmeticTestValues* pSubtractTestValues,
    ScalarArithmeticTestValues* pMultiplyTestValues,
    ScalarArithmeticTestValues* pDivideTestValues)
{
    const auto testConstant = getTestScalar<T>();

    pAddTestValues->setup(testConstant, bufferLen);
    pSubtractTestValues->setup(testConstant, bufferLen);
    pMultiplyTestValues->setup(testConstant, bufferLen);
    pDivideTestValues->setup(testConstant, bufferLen);

    // Avoid setting input to 0 when unsigned
    for (size_t elem = 0; elem < bufferLen; ++elem)
    {
        auto value = static_cast<T>(elem + 2);
        if (std::is_signed<T>::value) value -= static_cast<T>(bufferLen / 2);
        value = avoidZero(value);

        pAddTestValues->inputs.as<T*>()[elem] = value;
        pSubtractTestValues->inputs.as<T*>()[elem] = value;
        pMultiplyTestValues->inputs.as<T*>()[elem] = value;
        pDivideTestValues->inputs.as<T*>()[elem] = value;

        pAddTestValues->expectedOutputs.as<T*>()[elem] = value + testConstant;
        pSubtractTestValues->expectedOutputs.as<T*>()[elem] = value - testConstant;
        pMultiplyTestValues->expectedOutputs.as<T*>()[elem] = value * testConstant;
        pDivideTestValues->expectedOutputs.as<T*>()[elem] = value / testConstant;
    }
}

template <typename T>
static void testScalarArithmeticOp(
    const std::string& operation,
    const ScalarArithmeticTestValues& testValues)
{
    const Pothos::DType dtype(typeid(T));

    std::cout << " * Testing " << operation << "..." << std::endl;

    auto scalarArithmetic = Pothos::BlockRegistry::make(
                                "/gpu/scalar/arithmetic",
                                "Auto",
                                operation,
                                dtype,
                                testValues.scalar);
    POTHOS_TEST_EQUAL(
        testValues.scalar.extract<T>(),
        scalarArithmetic.call<T>("scalar"));

    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    feeder.call("feedBuffer", testValues.inputs);

    auto sink = Pothos::BlockRegistry::make("/blocks/collector_sink", dtype);

    {
        Pothos::Topology topology;

        topology.connect(feeder, 0, scalarArithmetic, 0);
        topology.connect(scalarArithmetic, 0, sink, 0);

        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    GPUTests::testBufferChunk(
        testValues.expectedOutputs,
        sink.call<Pothos::BufferChunk>("getBuffer"));
}

template <typename T>
static void testScalarArithmetic()
{
    std::cout << "Testing " << Pothos::DType(typeid(T)).toString() << "..." << std::endl;

    ScalarArithmeticTestValues addTestValues;
    ScalarArithmeticTestValues subtractTestValues;
    ScalarArithmeticTestValues multiplyTestValues;
    ScalarArithmeticTestValues divideTestValues;

    getTestValues<T>(
        &addTestValues,
        &subtractTestValues,
        &multiplyTestValues,
        &divideTestValues);

    testScalarArithmeticOp<T>("Add", addTestValues);
    testScalarArithmeticOp<T>("Subtract", subtractTestValues);
    testScalarArithmeticOp<T>("Multiply", multiplyTestValues);
    testScalarArithmeticOp<T>("Divide", divideTestValues);
    // TODO: modulus
}

POTHOS_TEST_BLOCK("/gpu/tests", test_scalar_arithmetic)
{
    testScalarArithmetic<std::int16_t>();
    testScalarArithmetic<std::int32_t>();
    testScalarArithmetic<std::int64_t>();
    testScalarArithmetic<std::uint8_t>();
    testScalarArithmetic<std::uint16_t>();
    testScalarArithmetic<std::uint32_t>();
    testScalarArithmetic<std::uint64_t>();
    testScalarArithmetic<float>();
    testScalarArithmetic<double>();
    testScalarArithmetic<std::complex<float>>();
    testScalarArithmetic<std::complex<double>>();
}
