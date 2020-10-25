// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <algorithm>
#include <complex>
#include <cstring>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace GPUTests
{

//
// Useful typedefs
//

using InputPortVector = std::vector<Pothos::InputPort*>;
using OutputPortVector = std::vector<Pothos::OutputPort*>;

template <typename T, typename U>
using EnableIfInteger = typename std::enable_if<!IsComplex<T>::value && !std::is_floating_point<T>::value && !std::is_unsigned<T>::value, U>::type;

template <typename T, typename U>
using EnableIfUnsignedInt = typename std::enable_if<std::is_unsigned<T>::value, U>::type;

template <typename T, typename U>
using EnableIfAnyInt = typename std::enable_if<!IsComplex<T>::value && !std::is_floating_point<T>::value, U>::type;

template <typename T, typename U>
using EnableIfFloat = typename std::enable_if<!IsComplex<T>::value && std::is_floating_point<T>::value, U>::type;

template <typename T, typename U>
using EnableIfComplex = typename std::enable_if<IsComplex<T>::value, U>::type;

template <typename T, typename U, typename V>
using EnableIfBothComplex = typename std::enable_if<IsComplex<T>::value && IsComplex<U>::value, V>::type;

template <typename T, typename U>
using EnableIfNotComplex = typename std::enable_if<!IsComplex<T>::value, U>::type;

template <typename T, typename U, typename V>
using EnableIfSecondComplex = typename std::enable_if<!IsComplex<T>::value && IsComplex<U>::value, V>::type;

template <typename T, typename U, typename V>
using EnableIfNeitherComplex = typename std::enable_if<!IsComplex<T>::value && !IsComplex<U>::value, V>::type;

template <typename T, typename U, typename Ret>
using EnableIfTypeMatches = typename std::enable_if<std::is_same<T, U>::value, Ret>::type;

template <typename T, typename U, typename Ret>
using EnableIfTypeDoesNotMatch = typename std::enable_if<!std::is_same<T, U>::value, Ret>::type;

template <typename T, typename Ret, size_t size>
using EnableIfTypeSizeIsGE = typename std::enable_if<sizeof(T) >= size, Ret>::type;

template <typename T, typename Ret, size_t size>
using EnableIfTypeSizeIsLT = typename std::enable_if<sizeof(T) < size, Ret>::type;

//
// Should be called at the beginning of each test
//

void setupTestEnv();

//
// Utility functions
//

template <typename T>
static inline EnableIfAnyInt<T, void> testEqual(T x, T y)
{
    POTHOS_TEST_EQUAL(x, y);
}

template <typename T>
static inline EnableIfFloat<T, void> testEqual(T x, T y)
{
    POTHOS_TEST_CLOSE(x, y, 1e-6);
}

template <typename T>
static inline EnableIfComplex<T, void> testEqual(T x, T y)
{
    testEqual(x.real(), y.real());
    testEqual(x.imag(), y.imag());
}

template <typename T>
static Pothos::BufferChunk stdVectorToBufferChunk(const std::vector<T>& vectorIn)
{
    static Pothos::DType dtype(typeid(T));

    auto ret = Pothos::BufferChunk(dtype, vectorIn.size());
    auto buf = ret.as<T*>();
    std::memcpy(
        buf,
        vectorIn.data(),
        ret.length);

    return ret;
}

template <typename In, typename Out>
static std::vector<Out> staticCastVector(const std::vector<In>& vectorIn)
{
    std::vector<Out> vectorOut;
    vectorOut.reserve(vectorIn.size());
    std::transform(
        vectorIn.begin(),
        vectorIn.end(),
        std::back_inserter(vectorOut),
        [](In val){return static_cast<Out>(val);});

    return vectorOut;
}

template <typename In, typename Out>
static std::vector<Out> reinterpretCastVector(const std::vector<In>& vectorIn)
{
    static_assert(sizeof(In) == sizeof(Out), "sizeof(In) != sizeof(Out)");

    std::vector<Out> vectorOut(vectorIn.size());
    std::memcpy(
        vectorOut.data(),
        vectorIn.data(),
        vectorIn.size() * sizeof(In));

    return vectorOut;
}

// Assumption: vectorIn is an even size
template <typename T>
static std::vector<std::complex<T>> toComplexVector(const std::vector<T>& vectorIn)
{
    std::vector<std::complex<T>> vectorOut(vectorIn.size() / 2);
    std::memcpy(
        (void*)vectorOut.data(),
        (const void*)vectorIn.data(),
        vectorIn.size() * sizeof(T));

    return vectorOut;
}

// https://gist.github.com/lorenzoriano/5414671
template <typename T>
static std::vector<T> linspace(T a, T b, size_t N)
{
    T h = (b - a) / static_cast<T>(N-1);
    std::cout << a << " " << b << " " << N << " " << h << std::endl;

    std::vector<T> xs(N);
    typename std::vector<T>::iterator x;
    T val;
    for (x = xs.begin(), val = a; x != xs.end(); ++x, val += h)
        *x = val;
    return xs;
}

template <typename T>
static std::vector<T> getIntTestParams(T a, T step, size_t N)
{
    std::vector<T> ret;
    ret.reserve(N);

    for(size_t i = 0; i < N; ++i)
    {
        // Skip 0 to avoid crashes
        if((a + (T(i)*step)) >= 0) ret.emplace_back(a + (T(i+1)*step));
        else                       ret.emplace_back(a + (T(i)*step));
    }

    return ret;
}

template <typename T>
static EnableIfFloat<T, void> testBufferChunk(
    const Pothos::BufferChunk& bufferChunk,
    const std::vector<T>& expectedOutputs,
    T epsilon = 1e-6)
{
    POTHOS_TEST_GT(bufferChunk.elements(), 0);
    auto pOut = bufferChunk.as<const T*>();
    for (size_t i = 0; i < bufferChunk.elements(); i++)
    {
        POTHOS_TEST_CLOSE(
            expectedOutputs[i],
            pOut[i],
            epsilon);
    }
}

template <typename T>
static EnableIfAnyInt<T, void> testBufferChunk(
    const Pothos::BufferChunk& bufferChunk,
    const std::vector<T>& expectedOutputs,
    T epsilon = T(0))
{
    (void)epsilon;

    POTHOS_TEST_GT(bufferChunk.elements(), 0);
    auto pOut = bufferChunk.as<const T*>();
    for (size_t i = 0; i < bufferChunk.elements(); i++)
    {
        POTHOS_TEST_EQUAL(
            expectedOutputs[i],
            pOut[i]);
    }
}

// Pass in a "complex" epsilon so the template works
template <typename T>
static EnableIfComplex<T, void> testBufferChunk(
    const Pothos::BufferChunk& bufferChunk,
    const std::vector<T>& expectedOutputs,
    T epsilon = T{1e-6,1e-6})
{
    auto pOut = bufferChunk.as<const T*>();
    for (size_t i = 0; i < bufferChunk.elements(); i++)
    {
        POTHOS_TEST_CLOSE(
            expectedOutputs[i].real(),
            pOut[i].real(),
            epsilon.real());
        POTHOS_TEST_CLOSE(
            expectedOutputs[i].imag(),
            pOut[i].imag(),
            epsilon.real());
    }
}

template <typename ReturnType, typename... ArgsType>
ReturnType getAndCallPlugin(
    const std::string& proxyPath,
    ArgsType&&... args)
{
    auto plugin = Pothos::PluginRegistry::get(proxyPath);
    auto getter = plugin.getObject().extract<Pothos::Callable>();

    return getter.call<ReturnType>(args...);
}

void testBufferChunk(
    const Pothos::BufferChunk& expectedBufferChunk,
    const Pothos::BufferChunk& actualBufferChunk);

template <typename AfArrayType>
static void compareAfArrayToBufferChunk(
    const AfArrayType& afArray,
    const Pothos::BufferChunk& bufferChunk)
{
    POTHOS_TEST_EQUAL(
        afArray.bytes(),
        bufferChunk.length);

    testBufferChunk(
        Pothos::Object(afArray).convert<Pothos::BufferChunk>(),
        bufferChunk);
}

template <typename T>
static EnableIfNotComplex<T, void> addMinMaxToAfArray(af::array& rAfArray)
{
    if(1 == rAfArray.numdims())
    {
        rAfArray(0) = std::numeric_limits<T>::min();
        rAfArray(1) = std::numeric_limits<T>::max();
    }
    else
    {
        rAfArray(0,0) = std::numeric_limits<T>::min();
        rAfArray(0,1) = std::numeric_limits<T>::max();
    }
}

template <typename T>
static EnableIfComplex<T, void> addMinMaxToAfArray(af::array& rAfArray)
{
    using Scalar = typename T::value_type;

    if(1 == rAfArray.numdims())
    {
        rAfArray(0) = typename PothosToAF<T>::type(
                                   std::numeric_limits<Scalar>::min(),
                                   std::numeric_limits<Scalar>::max());
    }
    else
    {
        rAfArray(0,0) = typename PothosToAF<T>::type(
                                     std::numeric_limits<Scalar>::min(),
                                     std::numeric_limits<Scalar>::max());
    }
}

// To enable doing this in non-templated functions.
void addMinMaxToAfArray(af::array& rAfArray, const std::string& type);

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

//
// For debugging purposes
//

template <typename T>
std::string bufferChunkToString(const Pothos::BufferChunk& bufferChunk)
{
    std::ostringstream ostream;

    const T* buff = bufferChunk.as<const T*>();
    for(size_t i = 0; i < bufferChunk.elements(); ++i)
    {
        if(0 != i)
        {
            ostream << " ";
        }
        ostream << (ssize_t)buff[i];
    }

    return ostream.str();
}
