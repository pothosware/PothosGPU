// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Testing.hpp>

#include <arrayfire.h>

#include <string>
#include <typeinfo>

template <typename T>
static void testBufferConversions(
    const std::string& dtypeName,
    ::af_dtype afDType)
{
    static constexpr dim_t ArrDim = 128;

    auto afArray = af::randu(ArrDim, afDType);

    auto bufferChunk = Pothos::Object(afArray).convert<Pothos::BufferChunk>();
    POTHOS_TEST_EQUAL(ArrDim, bufferChunk.elements());
    POTHOS_TEST_EQUAL(dtypeName, bufferChunk.dtype.name());
    POTHOS_TEST_TRUE(Pothos::DType(typeid(T)) == bufferChunk.dtype);
    POTHOS_TEST_EQUALA(
        reinterpret_cast<const T*>(afArray.device<std::uint8_t>()),
        bufferChunk.as<const T*>(),
        ArrDim);

    auto afArray2 = Pothos::Object(bufferChunk).convert<af::array>();
    POTHOS_TEST_EQUAL(ArrDim, afArray2.elements());
    POTHOS_TEST_EQUAL(afDType, afArray2.type());
    POTHOS_TEST_EQUALA(
        reinterpret_cast<const T*>(afArray.device<std::uint8_t>()),
        reinterpret_cast<const T*>(afArray2.device<std::uint8_t>()),
        ArrDim);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_buffer_conversions)
{
    testBufferConversions<std::int16_t>("int16", ::s16);
    testBufferConversions<std::int32_t>("int32", ::s32);
    testBufferConversions<std::int64_t>("int64", ::s64);
    testBufferConversions<std::uint16_t>("uint16", ::u16);
    testBufferConversions<std::uint32_t>("uint32", ::u32);
    testBufferConversions<std::uint64_t>("uint64", ::u64);
    testBufferConversions<float>("float32", ::f32);
    testBufferConversions<double>("float64", ::f64);
    testBufferConversions<std::complex<float>>("complex_float32", ::c32);
    testBufferConversions<std::complex<double>>("complex_float64", ::c64);
}
