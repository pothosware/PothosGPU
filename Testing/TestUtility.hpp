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
static std::vector<T> bufferChunkToStdVector(const Pothos::BufferChunk& bufferChunkIn)
{
    std::vector<T> ret(bufferChunkIn.elements());
    POTHOS_TEST_EQUAL((ret.size() * sizeof(T)), bufferChunkIn.length);
    std::memcpy(
        ret.data(),
        bufferChunkIn.as<const void*>(),
        bufferChunkIn.length);

    return ret;
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

    std::vector<T> xs(N);
    typename std::vector<T>::iterator x;
    T val;
    for (x = xs.begin(), val = a; x != xs.end(); ++x, val += h)
        *x = val;
    return xs;
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

void addMinMaxToAfArray(af::array& rAfArray);

//
// Getting random inputs
//

static constexpr size_t TestInputLength = 4096;

Pothos::BufferChunk getTestInputs(const std::string& type);

Pothos::Object getRandomValue(const Pothos::BufferChunk& bufferChunk);

Pothos::Object getSingleTestInput(const std::string& type);

//
// Only test against blocks that exist
//

inline bool doesBlockExist(const std::string& blockPath)
{
    return Pothos::PluginRegistry::exists("/blocks"+blockPath);
}

//
// For stats test and comparing BufferChunks
//

template <typename T>
static T median(const std::vector<T>& inputs)
{
    std::vector<T> sortedInputs(inputs);
    std::sort(sortedInputs.begin(), sortedInputs.end());
    auto sortedIndex = size_t(std::floor(inputs.size()/2));

    return sortedInputs[sortedIndex];
}

template <typename T>
static T medAbsDev(const std::vector<T>& inputs)
{
    const T med = median(inputs);
    std::vector<T> diffs;

    std::transform(
        inputs.begin(),
        inputs.end(),
        std::back_inserter(diffs),
        [&med](T input){return std::abs<T>(input-med);});

    return median(diffs);
}

//
// Misc
//

const std::vector<std::string>& getAllDTypeNames();

std::vector<Pothos::BufferChunk> convert2DAfArrayToBufferChunks(const af::array& afArray);

af::array convertBufferChunksTo2DAfArray(const std::vector<Pothos::BufferChunk>& bufferChunks);

}
