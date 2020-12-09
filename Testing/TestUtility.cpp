// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "DeviceCache.hpp"
#include "TestUtility.hpp"

#include <Poco/Random.h>

#include <Pothos/Framework.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

namespace GPUTests
{

void setupTestEnv()
{
    af::setBackend(getAvailableBackends()[0]);
}

template <typename T>
static inline EnableIfNotComplex<T, T> epsilon()
{
    return T(1e-6);
}

template <typename T>
static inline EnableIfComplex<T, T> epsilon()
{
    using ScalarType = typename T::value_type;

    return T(epsilon<ScalarType>(), epsilon<ScalarType>());
}

// TODO: consolidate all these testBufferChunk calls
template <typename T>
static EnableIfAnyInt<T, void> compareBufferChunks(
    const Pothos::BufferChunk& expectedBufferChunk,
    const Pothos::BufferChunk& actualBufferChunk)
{
    POTHOS_TEST_EQUALA(
        expectedBufferChunk.as<const T*>(),
        actualBufferChunk.as<const T*>(),
        expectedBufferChunk.elements());
}

template <typename T>
static EnableIfFloat<T, void> compareBufferChunks(
    const Pothos::BufferChunk& expectedBufferChunk,
    const Pothos::BufferChunk& actualBufferChunk)
{
    POTHOS_TEST_CLOSEA(
        expectedBufferChunk.as<const T*>(),
        actualBufferChunk.as<const T*>(),
        expectedBufferChunk.elements(),
        T(1e-6));
}

template <typename T>
static EnableIfComplex<T, void> compareBufferChunks(
    const Pothos::BufferChunk& expectedBufferChunk,
    const Pothos::BufferChunk& actualBufferChunk)
{
    using ScalarType = typename T::value_type;

    auto scalarExpected = expectedBufferChunk;
    scalarExpected.dtype = typeid(ScalarType);

    auto scalarActual = actualBufferChunk;
    scalarActual.dtype = typeid(ScalarType);

    compareBufferChunks<ScalarType>(
        scalarExpected,
        scalarActual);
}

void testBufferChunk(
    const Pothos::BufferChunk& expectedBufferChunk,
    const Pothos::BufferChunk& actualBufferChunk)
{
    POTHOS_TEST_EQUAL(
        expectedBufferChunk.dtype,
        actualBufferChunk.dtype);
    POTHOS_TEST_EQUAL(
        expectedBufferChunk.elements(),
        actualBufferChunk.elements());

    #define IfTypeThenCompare(typeStr, cType) \
        if(expectedBufferChunk.dtype.name() == typeStr) \
        { \
            compareBufferChunks<cType>( \
                expectedBufferChunk, \
                actualBufferChunk); \
            return; \
        }

    IfTypeThenCompare("int16", std::int16_t)
    IfTypeThenCompare("int32", std::int32_t)
    IfTypeThenCompare("int64", std::int64_t)
    IfTypeThenCompare("uint8", std::uint8_t)
    IfTypeThenCompare("uint16", std::uint16_t)
    IfTypeThenCompare("uint32", std::uint32_t)
    IfTypeThenCompare("uint64", std::uint64_t)
    IfTypeThenCompare("float32", float)
    IfTypeThenCompare("float64", double)
    IfTypeThenCompare("complex_float32", std::complex<float>)
    IfTypeThenCompare("complex_float64", std::complex<double>)
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

void addMinMaxToAfArray(af::array& rAfArray)
{
    #define IfTypeThenAdd(typeEnum, cType) \
        if(rAfArray.type() == typeEnum) \
        { \
            addMinMaxToAfArray<cType>(rAfArray); \
            return; \
        }

    IfTypeThenAdd(::b8, std::int8_t)
    IfTypeThenAdd(::s16, std::int16_t)
    IfTypeThenAdd(::s32, std::int32_t)
    IfTypeThenAdd(::s64, std::int64_t)
    IfTypeThenAdd(::u8, std::uint8_t)
    IfTypeThenAdd(::u16, std::uint16_t)
    IfTypeThenAdd(::u32, std::uint32_t)
    IfTypeThenAdd(::u64, std::uint64_t)
    IfTypeThenAdd(::f32, float)
    IfTypeThenAdd(::f64, double)
    IfTypeThenAdd(::c32, std::complex<float>)
    IfTypeThenAdd(::c64, std::complex<double>)
}

Pothos::BufferChunk getTestInputs(const std::string& type)
{
    const auto afDType = Pothos::Object(Pothos::DType(type)).convert<af::dtype>();

    return Pothos::Object(af::randu(TestInputLength, afDType)).convert<Pothos::BufferChunk>();
}

#define RETURN_OBJECT(typeStr, cType) \
    if(type == typeStr) \
        return Pothos::Object(getSingleTestInput<cType>());

Pothos::Object getSingleTestInput(const std::string& type)
{
    // ArrayFire doesn't support int8
    RETURN_OBJECT("int16", std::int16_t)
    RETURN_OBJECT("int32", std::int32_t)
    RETURN_OBJECT("int64", std::int64_t)
    RETURN_OBJECT("uint8", std::uint8_t)
    RETURN_OBJECT("uint16", std::uint16_t)
    RETURN_OBJECT("uint32", std::uint32_t)
    RETURN_OBJECT("uint64", std::uint64_t)
    RETURN_OBJECT("float32", float)
    RETURN_OBJECT("float64", double)
    // ArrayFire doesn't support any integral complex type
    RETURN_OBJECT("complex_float32", std::complex<float>)
    RETURN_OBJECT("complex_float64", std::complex<double>)

    // Should never happen
    return Pothos::Object();
}

const std::vector<std::string>& getAllDTypeNames()
{
    static const std::vector<std::string> AllTypes =
    {
        // ArrayFire doesn't support int8
        "int16",
        "int32",
        "int64",
        "uint8",
        "uint16",
        "uint32",
        "uint64",
        "float32",
        "float64",
        // ArrayFire doesn't support complex integral types
        "complex_float32",
        "complex_float64"
    };
    return AllTypes;
}

std::vector<Pothos::BufferChunk> convert2DAfArrayToBufferChunks(const af::array& afArray)
{
    POTHOS_TEST_EQUAL(2, afArray.numdims());
    const auto numRows = afArray.dims(0);

    std::vector<Pothos::BufferChunk> bufferChunks;
    for(dim_t i = 0; i < numRows; ++i)
    {
        bufferChunks.emplace_back(Pothos::Object(afArray.row(i)).convert<Pothos::BufferChunk>());
    }

    return bufferChunks;
}

// Assumption: all BufferChunks are of same type and size
af::array convertBufferChunksTo2DAfArray(const std::vector<Pothos::BufferChunk>& bufferChunks)
{
    POTHOS_TEST_FALSE(bufferChunks.empty());

    const auto afDType = Pothos::Object(bufferChunks[0].dtype).convert<af::dtype>();
    af::dim4 dims(dim_t(bufferChunks.size()), dim_t(bufferChunks[0].elements()));

    af::array afArray(dims, afDType);
    for(dim_t row = 0; row < dim_t(bufferChunks.size()); ++row)
    {
        afArray.row(row) = Pothos::Object(bufferChunks[row]).convert<af::array>();
    }

    return afArray;
}

pothos_static_block(pothosGPUSeedRandom)
{
    af::setSeed(Poco::Random().next());
}

}
