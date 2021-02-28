// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "DeviceCache.hpp"
#include "TestUtility.hpp"

#include <Poco/Random.h>
#include <Poco/Timestamp.h>

#include <Pothos/Framework.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

#include <algorithm>

namespace GPUTests
{

void setupTestEnv()
{
    af::setBackend(PothosGPU::getAvailableBackends()[0]);
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

    IfTypeThenCompare("int16", short)
    IfTypeThenCompare("int32", int)
    IfTypeThenCompare("int64", long long)
    IfTypeThenCompare("uint8", unsigned char)
    IfTypeThenCompare("uint16", unsigned short)
    IfTypeThenCompare("uint32", unsigned)
    IfTypeThenCompare("uint64", unsigned long long)
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
        rAfArray(0) = typename PothosGPU::PothosToAF<T>::type(
                                   std::numeric_limits<Scalar>::min(),
                                   std::numeric_limits<Scalar>::max());
    }
    else
    {
        rAfArray(0,0) = typename PothosGPU::PothosToAF<T>::type(
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

Pothos::Object getRandomValue(const Pothos::BufferChunk& bufferChunk)
{
    #define GET_RANDOM_VALUE_OF_TYPE(typeStr, cType) \
        if(bufferChunk.dtype.name() == typeStr) \
        { \
            return Pothos::Object(bufferChunk.as<const cType*>()[Poco::Random().next(Poco::UInt32(bufferChunk.elements()))]); \
        }

    GET_RANDOM_VALUE_OF_TYPE("int8",            char)
    GET_RANDOM_VALUE_OF_TYPE("int16",           short)
    GET_RANDOM_VALUE_OF_TYPE("int32",           int)
    GET_RANDOM_VALUE_OF_TYPE("int64",           long long)
    GET_RANDOM_VALUE_OF_TYPE("uint8",           unsigned char)
    GET_RANDOM_VALUE_OF_TYPE("uint16",          unsigned short)
    GET_RANDOM_VALUE_OF_TYPE("uint32",          unsigned)
    GET_RANDOM_VALUE_OF_TYPE("uint64",          unsigned long long)
    GET_RANDOM_VALUE_OF_TYPE("float32",         float)
    GET_RANDOM_VALUE_OF_TYPE("float64",         double)
    GET_RANDOM_VALUE_OF_TYPE("complex_float32", std::complex<float>)
    GET_RANDOM_VALUE_OF_TYPE("complex_float64", std::complex<double>)

    // Should never happen
    return Pothos::Object();
}

#define RETURN_OBJECT(typeStr) \
    if(type == typeStr) return getRandomValue(getTestInputs(typeStr));

Pothos::Object getSingleTestInput(const std::string& type)
{
    RETURN_OBJECT("int8")
    RETURN_OBJECT("int16")
    RETURN_OBJECT("int32")
    RETURN_OBJECT("int64")
    RETURN_OBJECT("uint8")
    RETURN_OBJECT("uint16")
    RETURN_OBJECT("uint32")
    RETURN_OBJECT("uint64")
    RETURN_OBJECT("float32")
    RETURN_OBJECT("float64")
    // ArrayFire doesn't support any integral complex type
    RETURN_OBJECT("complex_float32")
    RETURN_OBJECT("complex_float64")

    // Should never happen
    return Pothos::Object();
}

const std::vector<Pothos::DType>& getAllDTypes()
{
    static const std::vector<Pothos::DType> AllDTypes =
    {
        typeid(char),
        typeid(signed char),
        typeid(short),
        typeid(int),
        typeid(long),
        typeid(long long),
        typeid(unsigned char),
        typeid(unsigned short),
        typeid(unsigned int),
        typeid(unsigned long),
        typeid(unsigned long long),
        typeid(float),
        typeid(double),
        typeid(std::complex<float>),
        typeid(std::complex<double>),
    };
    return AllDTypes;
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

// TODO: reseed for each test, this doesn't work for some reason
pothos_static_block(pothosGPUSeedRandom)
{
    af::setSeed(Poco::Timestamp().utcTime());
}

}
