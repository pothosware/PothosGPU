// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>

#include <complex>
#include <cmath>
#include <functional>
#include <random>

namespace PothosArrayFireTests
{

using PortInfoVector = std::vector<Pothos::PortInfo>;

//
// Verification function types
//

template <typename In, typename Out>
using UnaryFunc = std::function<Out(const In&)>;

template <typename In, typename Out>
using BinaryFunc = std::function<Out(const In&, const In&)>;

template <typename T>
static UnaryFunc<T, T> binaryFuncToUnary(
    const BinaryFunc<T, T>& binaryFunc,
    const T& operand)
{
    UnaryFunc<T, T> ret = (nullptr == binaryFunc)
                        ? UnaryFunc<T, T>(nullptr)
                        : std::bind(binaryFunc, std::placeholders::_1, operand);
    return ret;
}

//
// Templated type calls
//

template <typename T>
void testOneToOneBlock(
    const std::string& blockRegistryPath,
    const UnaryFunc<T, T>& verificationFunc);

template <typename T>
void testOneToOneBlockF2C(
    const std::string& blockRegistryPath,
    const UnaryFunc<T, std::complex<T>>& verificationFunc);

template <typename T>
void testOneToOneBlockC2F(
    const std::string& blockRegistryPath,
    const UnaryFunc<std::complex<T>, T>& verificationFunc);

template <typename T>
void testTwoToOneBlock(
    const std::string& blockRegistryPath,
    const BinaryFunc<T, T>& verificationFunc,
    bool removeZerosInBuffer1);

template <typename T>
void testTwoToOneBlockF2C(
    const std::string& blockRegistryPath,
    const BinaryFunc<T, std::complex<T>>& verificationFunc,
    bool removeZerosInBuffer1);

template <typename T>
void testNToOneBlock(
    const std::string& blockRegistryPath,
    size_t numChannels,
    const BinaryFunc<T, T>& verificationFunc);

template <typename T>
void testScalarOpBlock(
    const std::string& blockRegistryPath,
    size_t numChannels,
    const BinaryFunc<T, T>& verificationFunc,
    bool allowZeroScalar);

void testCastBlockForType(const std::string& inputType);

void testClampBlockForType(const std::string& inputType);

void testComparatorBlockForType(const std::string& type);

void testFlatBlockForType(const std::string& type);

void testIsInfNaNBlockForType(const std::string& type);

void testSplitComplexBlockForType(const std::string& floatType);

//
// Getting random inputs
//

// Templateless for the case where templates would make the binary size
// explode
Pothos::BufferChunk getTestInputs(const std::string& type);

template <typename T>
static EnableIfInteger<T, std::vector<T>> getTestInputs(bool shuffle = true)
{
    static std::random_device rd;
    static std::mt19937 g(rd());

    static constexpr T minValue = std::is_same<T, std::int8_t>::value ? T(-5) : T(-25);
    static constexpr size_t numInputs = std::is_same<T, std::int8_t>::value ? 11 : 51;

    auto testParams = getIntTestParams<T>(minValue, T(1), numInputs);
    if(shuffle)
    {
        std::shuffle(testParams.begin(), testParams.end(), g);
    }

    return testParams;
}

template <typename T>
static EnableIfUnsignedInt<T, std::vector<T>> getTestInputs(bool shuffle = true)
{
    static std::random_device rd;
    static std::mt19937 g(rd());

    static constexpr T minValue = std::is_same<T, std::uint8_t>::value ? T(5) : T(25);
    static constexpr size_t numInputs = std::is_same<T, std::uint8_t>::value ? 9 : 76;

    auto testParams = getIntTestParams<T>(minValue, T(1), numInputs);
    if(shuffle)
    {
        std::shuffle(testParams.begin(), testParams.end(), g);
    }

    return testParams;
}

template <typename T>
static EnableIfFloat<T, std::vector<T>> getTestInputs(bool shuffle = true)
{
    static std::random_device rd;
    static std::mt19937 g(rd());

    // To not have nice even numbers
    static constexpr size_t numInputs = 123;

    auto testParams = linspace<T>(10.0f, 20.0f, numInputs);
    if(shuffle)
    {
        std::shuffle(testParams.begin(), testParams.end(), g);
    }

    return testParams;
}

template <typename T>
static EnableIfComplex<T, std::vector<T>> getTestInputs(bool shuffle = true)
{
    using Scalar = typename T::value_type;

    static std::random_device rd;
    static std::mt19937 g(rd());

    // To not have nice even numbers
    static constexpr size_t numInputs = 246;

    auto testParams = toComplexVector(linspace<Scalar>(10.0f, 20.0f, numInputs));
    if(shuffle)
    {
        std::shuffle(testParams.begin(), testParams.end(), g);
    }

    return testParams;
}

template <typename T>
static inline T getSingleTestInput()
{
    return getTestInputs<T>()[0];
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

}
