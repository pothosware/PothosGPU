// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "BufferConversions.hpp"
#include "DeviceCache.hpp"
#include "Utility.hpp"
#include "TestUtility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>
#include <Pothos/Testing.hpp>

#include <Poco/Types.h>

#include <arrayfire.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <typeinfo>

namespace GPUTests
{

static void test1DArrayConversion(const Pothos::DType& dtype)
{
    constexpr dim_t ArrDim = 128;
    const auto afDType = Pothos::Object(dtype).convert<af::dtype>();

    std::cout << " * Testing " << dtype.name() << "..." << std::endl;

    auto afArray = af::randu(ArrDim, afDType);
    addMinMaxToAfArray(afArray);

    auto convertedBufferChunk = Pothos::Object(afArray).convert<Pothos::BufferChunk>();
    compareAfArrayToBufferChunk(
        afArray,
        convertedBufferChunk);

    auto convertedAfArray = Pothos::Object(convertedBufferChunk).convert<af::array>();
    compareAfArrayToBufferChunk(
        convertedAfArray,
        convertedBufferChunk);
}

static void test2DArrayConversion(const Pothos::DType& dtype)
{
    constexpr dim_t ArrDim1 = 16;
    constexpr dim_t ArrDim2 = 32;
    const auto afDType = Pothos::Object(dtype).convert<af::dtype>();

    std::cout << " * Testing " << dtype.name() << "..." << std::endl;

    auto afArray = af::randu(ArrDim1, ArrDim2, afDType);
    addMinMaxToAfArray(afArray);

    for(dim_t row = 0; row < ArrDim1; ++row)
    {
        const af::array::array_proxy afArrayRow = afArray.row(row);

        auto convertedBufferChunk = Pothos::Object(afArrayRow)
                                        .convert<Pothos::BufferChunk>();
        compareAfArrayToBufferChunk(
            afArrayRow,
            convertedBufferChunk);

        auto convertedAfArray = Pothos::Object(convertedBufferChunk).convert<af::array>();
        compareAfArrayToBufferChunk(
            convertedAfArray,
            convertedBufferChunk);
    }
}

template <typename T>
static void testStdVectorToAfArrayConversion(af::dtype expectedAfDType)
{
    Pothos::DType dtype(typeid(T));

    std::cout << " * Testing " << dtype.name() << "..." << std::endl;

    const std::vector<T> stdVector = bufferChunkToStdVector<T>(getTestInputs(dtype.name()));

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

}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_array_conversion)
{
    using namespace GPUTests;

    for(const auto& backend: PothosGPU::getAvailableBackends())
    {
        af::setBackend(backend);
        std::cout << "Backend: " << Pothos::Object(backend).convert<std::string>() << std::endl;

        for(const auto& dtype: getAllDTypes())
        {
            test1DArrayConversion(dtype);
        }
    }
}

POTHOS_TEST_BLOCK("/gpu/tests", test_af_arrayproxy_conversion)
{
    using namespace GPUTests;

    for(const auto& backend: PothosGPU::getAvailableBackends())
    {
        af::setBackend(backend);
        std::cout << "Backend: " << Pothos::Object(backend).convert<std::string>() << std::endl;

        for(const auto& dtype: getAllDTypes())
        {
            test2DArrayConversion(dtype);
        }
    }
}

POTHOS_TEST_BLOCK("/gpu/tests", test_std_vector_conversion)
{
    using namespace GPUTests;

    for(const auto& backend: PothosGPU::getAvailableBackends())
    {
        af::setBackend(backend);
        std::cout << "Backend: " << Pothos::Object(backend).convert<std::string>() << std::endl;

        testStdVectorToAfArrayConversion<float>(::f32);
        testStdVectorToAfArrayConversion<double>(::f64);

        testStdVectorToAfArrayConversion<std::complex<float>>(::c32);
        testStdVectorToAfArrayConversion<std::complex<double>>(::c64);
    }
}
