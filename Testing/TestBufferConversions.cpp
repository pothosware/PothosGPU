// Copyright (c) 2019 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "BufferConversions.hpp"
#include "DeviceCache.hpp"
#include "Utility.hpp"
#include "TestUtility.hpp"
#include "BlockExecutionTests.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Testing.hpp>

#include <arrayfire.h>

#include <iostream>
#include <string>
#include <typeinfo>

namespace PothosArrayFireTests
{

template <typename T>
static void test1DArrayConversion(
    const std::string& dtypeName,
    af::dtype afDType)
{
    static constexpr dim_t ArrDim = 128;

    std::cout << " * Testing " << dtypeName << "..." << std::endl;

    auto afArray = af::randu(ArrDim, afDType);

    auto bufferChunk = Pothos::Object(afArray).convert<Pothos::BufferChunk>();
    POTHOS_TEST_EQUAL(ArrDim, bufferChunk.elements());
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

    auto bufferChunk2 = moveAfArrayToBufferChunk(std::move(afArray2));
    PothosArrayFireTests::testBufferChunk(
        bufferChunk,
        bufferChunk2);
}

template <typename T>
static void test2DArrayConversion(
    const std::string& dtypeName,
    af::dtype afDType)
{
    static constexpr dim_t ArrDim = 32;

    std::cout << " * Testing " << dtypeName << "..." << std::endl;

    auto afArray = af::randu(ArrDim, ArrDim, afDType);
    for(dim_t row = 0; row < ArrDim; ++row)
    {
        const af::array::array_proxy afArrayRow = afArray.row(row);

        auto bufferChunk = Pothos::Object(afArrayRow)
                               .convert<Pothos::BufferChunk>();
        POTHOS_TEST_EQUAL(ArrDim, bufferChunk.elements());
        POTHOS_TEST_TRUE(Pothos::DType(typeid(T)) == bufferChunk.dtype);
        POTHOS_TEST_EQUALA(
            reinterpret_cast<const T*>(afArrayRow.device<std::uint8_t>()),
            bufferChunk.as<const T*>(),
            ArrDim);
    }
}

template <typename T>
static void testStdVectorToAfArrayConversion(af::dtype expectedAfDType)
{
    std::cout << " * Testing " << Pothos::DType(typeid(T)).name() << "..." << std::endl;

    const std::vector<T> stdVector = getTestInputs<T>();

    const auto afArray = Pothos::Object(stdVector).convert<af::array>();
    POTHOS_TEST_EQUAL(1, afArray.numdims());
    POTHOS_TEST_TRUE(expectedAfDType == afArray.type());
    POTHOS_TEST_EQUAL(
        stdVector.size(),
        static_cast<size_t>(afArray.elements()));

    const auto stdVector2 = Pothos::Object(afArray).convert<std::vector<T>>();
    POTHOS_TEST_EQUALV(
        stdVector,
        stdVector2);
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_array_conversion)
{
    using namespace PothosArrayFireTests;

    for(const auto& backend: getAvailableBackends())
    {
        af::setBackend(backend);
        std::cout << "Backend: " << Pothos::Object(backend).convert<std::string>() << std::endl;

        test1DArrayConversion<std::int8_t>("int8", ::b8);
        test1DArrayConversion<std::int16_t>("int16", ::s16);
        test1DArrayConversion<std::int32_t>("int32", ::s32);
        test1DArrayConversion<std::int64_t>("int64", ::s64);
        test1DArrayConversion<std::uint16_t>("uint16", ::u16);
        test1DArrayConversion<std::uint32_t>("uint32", ::u32);
        test1DArrayConversion<std::uint64_t>("uint64", ::u64);
        test1DArrayConversion<float>("float32", ::f32);
        test1DArrayConversion<double>("float64", ::f64);
        test1DArrayConversion<std::complex<float>>("complex_float32", ::c32);
        test1DArrayConversion<std::complex<double>>("complex_float64", ::c64);
    }

#if !IS_AF_CONFIG_PER_THREAD
    // Restore global backend and device to fastest options.
    af::setBackend(getAvailableBackends()[0]);
    af::setDevice(0);
#endif
}

}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_af_arrayproxy_conversion)
{
    using namespace PothosArrayFireTests;

    for(const auto& backend: getAvailableBackends())
    {
        af::setBackend(backend);
        std::cout << "Backend: " << Pothos::Object(backend).convert<std::string>() << std::endl;

        test2DArrayConversion<std::int8_t>("int8", ::b8);
        test2DArrayConversion<std::int16_t>("int16", ::s16);
        test2DArrayConversion<std::int32_t>("int32", ::s32);
        test2DArrayConversion<std::int64_t>("int64", ::s64);
        test2DArrayConversion<std::uint16_t>("uint16", ::u16);
        test2DArrayConversion<std::uint32_t>("uint32", ::u32);
        test2DArrayConversion<std::uint64_t>("uint64", ::u64);
        test2DArrayConversion<float>("float32", ::f32);
        test2DArrayConversion<double>("float64", ::f64);
        test2DArrayConversion<std::complex<float>>("complex_float32", ::c32);
        test2DArrayConversion<std::complex<double>>("complex_float64", ::c64);
    }

#if !IS_AF_CONFIG_PER_THREAD
    // Restore global backend and device to fastest options.
    af::setBackend(getAvailableBackends()[0]);
    af::setDevice(0);
#endif
}

POTHOS_TEST_BLOCK("/arrayfire/tests", test_std_vector_conversion)
{
    using namespace PothosArrayFireTests;

    for(const auto& backend: getAvailableBackends())
    {
        af::setBackend(backend);
        std::cout << "Backend: " << Pothos::Object(backend).convert<std::string>() << std::endl;

        testStdVectorToAfArrayConversion<float>(::f32);
        testStdVectorToAfArrayConversion<double>(::f64);

        testStdVectorToAfArrayConversion<std::complex<float>>(::c32);
        testStdVectorToAfArrayConversion<std::complex<double>>(::c64);
    }

#if !IS_AF_CONFIG_PER_THREAD
    // Restore global backend and device to fastest options.
    af::setBackend(getAvailableBackends()[0]);
    af::setDevice(0);
#endif
}
