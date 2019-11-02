// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>

#include <complex>
#include <cmath>
#include <functional>
#include <random>

using InputPortVector = std::vector<Pothos::InputPort*>;
using OutputPortVector = std::vector<Pothos::OutputPort*>;
using PortInfoVector = std::vector<Pothos::PortInfo>;

template <typename In, typename Out>
using UnaryFunc = std::function<Out(const In&)>;

template <typename In, typename Out>
using BinaryFunc = std::function<Out(const In&, const In&)>;

//
// Templated type calls
//

template <typename T>
void testOneToOneBlock(
    const std::string& blockRegistryPath,
    size_t numChannels,
    const UnaryFunc<T, T>& verificationFunc);

template <typename In, typename Out>
void testOneToOneBlock(
    const std::string& blockRegistryPath,
    size_t numChannels,
    const UnaryFunc<In, Out>& verificationFunc);

template <typename T>
void testTwoToOneBlock(
    const std::string& blockRegistryPath,
    const BinaryFunc<T, T>& verificationFunc,
    bool removeZerosInBuffer1);

template <typename In, typename Out>
void testTwoToOneBlock(
    const std::string& blockRegistryPath,
    const BinaryFunc<In, Out>& verificationFunc,
    bool removeZerosInBuffer1);

//
// Getting random inputs
//

template <typename T>
static EnableIfInteger<T, std::vector<T>> getTestInputs()
{
    static std::random_device rd;
    static std::mt19937 g(rd());

    static constexpr T minValue = std::is_same<T, std::int8_t>::value ? T(-5) : T(-25);
    static constexpr size_t numInputs = std::is_same<T, std::int8_t>::value ? 11 : 51;

    auto testParams = getIntTestParams<T>(minValue, T(1), numInputs);
    std::shuffle(testParams.begin(), testParams.end(), g);

    return testParams;
}

template <typename T>
static EnableIfUnsignedInt<T, std::vector<T>> getTestInputs()
{
    static std::random_device rd;
    static std::mt19937 g(rd());

    static constexpr T minValue = std::is_same<T, std::uint8_t>::value ? T(5) : T(25);
    static constexpr size_t numInputs = std::is_same<T, std::uint8_t>::value ? 9 : 76;

    auto testParams = getIntTestParams<T>(minValue, T(1), numInputs);
    std::shuffle(testParams.begin(), testParams.end(), g);

    return testParams;
}

template <typename T>
static EnableIfFloat<T, std::vector<T>> getTestInputs()
{
    static std::random_device rd;
    static std::mt19937 g(rd());

    // To not have nice even numbers
    static constexpr size_t numInputs = 123;

    auto testParams = linspace<T>(10.0f, 20.0f, numInputs);
    std::shuffle(testParams.begin(), testParams.end(), g);

    return testParams;
}

template <typename T>
static EnableIfComplex<T, std::vector<T>> getTestInputs()
{
    using Scalar = typename T::value_type;

    static std::random_device rd;
    static std::mt19937 g(rd());

    // To not have nice even numbers
    static constexpr size_t numInputs = 246;

    auto testParams = toComplexVector(linspace<Scalar>(10.0f, 20.0f, numInputs));
    std::shuffle(testParams.begin(), testParams.end(), g);

    return testParams;
}

//
// Manual verification functions
//

template <typename T>
static inline EnableIfComplex<T, typename T::value_type> testReal(const T& val)
{
    return val.real();
}

template <typename T>
static inline EnableIfComplex<T, typename T::value_type> testImag(const T& val)
{
    return val.imag();
}

template <typename T>
static inline EnableIfFloat<T, T> testSigmoid(const T& val)
{
    return (T(1.0) / (1.0 + std::exp(val * T(-1))));
}

template <typename T>
static inline EnableIfFloat<T, T> testFactorial(const T& val)
{
    return std::tgamma(val + T(1.0));
}
