// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>

#include <complex>
#include <cmath>
#include <random>
#include <vector>

namespace PothosArrayFireTests
{

using PortInfoVector = std::vector<Pothos::PortInfo>;

//
// Templated type calls
//

template <typename T>
void testOneToOneBlock(const std::string& blockRegistryPath);

template <typename T>
void testOneToOneBlockF2C(const std::string& blockRegistryPath);

template <typename T>
void testOneToOneBlockC2F(const std::string& blockRegistryPath);

template <typename T>
void testTwoToOneBlock(
    const std::string& blockRegistryPath,
    bool removeZerosInBuffer1);

template <typename T>
void testTwoToOneBlockF2C(
    const std::string& blockRegistryPath,
    bool removeZerosInBuffer1);

template <typename T>
void testNToOneBlock(
    const std::string& blockRegistryPath,
    size_t numChannels);

template <typename T1, typename T2>
void testReducedBlock(
    const std::string& blockRegistryPath,
    size_t numChannels);

template <typename T>
void testScalarOpBlock(
    const std::string& blockRegistryPath,
    size_t numChannels,
    bool allowZeroScalar);

//
// Getting random inputs
//

// Templateless for the case where templates would make the binary size
// explode
Pothos::BufferChunk getTestInputs(const std::string& type);

Pothos::Object getSingleTestInput(const std::string& type);

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
// Misc
//

const std::vector<std::string>& getAllDTypeNames();

}
