// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "DeviceCache.hpp"
#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>

namespace AFTests
{

void setupTestEnv()
{
    af::setBackend(getAvailableBackends()[0]);
}

void testBufferChunk(
    const Pothos::BufferChunk& expectedBufferChunk,
    const Pothos::BufferChunk& actualBufferChunk)
{
    POTHOS_TEST_EQUAL(
        expectedBufferChunk.dtype.name(),
        actualBufferChunk.dtype.name());
    POTHOS_TEST_EQUAL(
        expectedBufferChunk.elements(),
        actualBufferChunk.elements());

    #define IfTypeThenCompare(typeStr, cType) \
        if(expectedBufferChunk.dtype.name() == typeStr) \
        { \
            POTHOS_TEST_EQUALA( \
                expectedBufferChunk.as<const cType*>(), \
                actualBufferChunk.as<const cType*>(), \
                expectedBufferChunk.elements()); \
            return; \
        }

    IfTypeThenCompare("int8", std::int8_t)
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

void addMinMaxToAfArray(af::array& rAfArray, const std::string& type)
{
    #define IfTypeThenAdd(typeStr, ctype) \
        if(typeStr == type) \
        { \
            addMinMaxToAfArray<ctype>(rAfArray); \
            return; \
        }

    IfTypeThenAdd("int8", std::int8_t)
    IfTypeThenAdd("int16", std::int16_t)
    IfTypeThenAdd("int32", std::int32_t)
    IfTypeThenAdd("int64", std::int64_t)
    IfTypeThenAdd("uint8", std::uint8_t)
    IfTypeThenAdd("uint16", std::uint16_t)
    IfTypeThenAdd("uint32", std::uint32_t)
    IfTypeThenAdd("uint64", std::uint64_t)
    IfTypeThenAdd("float32", float)
    IfTypeThenAdd("float64", double)
    IfTypeThenAdd("complex_float32", std::complex<float>)
    IfTypeThenAdd("complex_float64", std::complex<double>)
}

}
